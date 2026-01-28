// ============================================================================
// BTTask_FlyToStagingZone.cpp
// Developer: Marcus Daley
// Date: January 23, 2026
// Project: END2507
// ============================================================================

#include "Code/AI/BTTask_FlyToStagingZone.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Code/GameModes/QuidditchGameMode.h"
#include "Code/Flight/AC_BroomComponent.h"
#include "Code/Flight/AC_FlightSteeringComponent.h"
#include "Code/Utilities/AC_StaminaComponent.h"

DEFINE_LOG_CATEGORY(LogBTTask_FlyToStagingZone);

UBTTask_FlyToStagingZone::UBTTask_FlyToStagingZone()
    : ArrivalRadius(400.0f)           // Solution 2: Increased from 200 to 400
    , ExtendedArrivalRadius(800.0f)   // Solution 3: Extended radius for overshoot detection
    , FlightSpeedMultiplier(1.0f)
    , bUseBoostWhenFar(true)
    , BoostDistanceThreshold(500.0f)
    , BoostDisableThreshold(300.0f)   // Hysteresis: boost turns OFF at 300, turns ON at 500
    , MinStaminaForBoost(0.3f)        // Don't boost below 30% stamina to preserve flight ability
    , AltitudeScale(200.0f)
    , bUseFlightSteering(false)       // DISABLED: FlightSteering returns local input space, causes navigation issues
    , bUsePredictiveSteering(false)   // Staging zone is static, no prediction needed
    , PredictionTime(0.5f)
    , bEnableVelocityArrival(true)    // Solution 3: Enabled by default
    , OvershootSpeedThreshold(-100.0f)
    , bEnableStuckDetection(true)     // Solution 4: Enabled by default
    , StuckCheckInterval(2.0f)
    , StuckSampleCount(5)
    , StuckDistanceThreshold(100.0f)
    , SlotName(NAME_None)
    , bEnableTimeout(true)            // Solution 5: Already implemented
    , TimeoutDuration(30.0f)
    , CachedStagingLocation(FVector::ZeroVector)
    , bLocationSet(false)
    , ElapsedFlightTime(0.0f)
    , TimeSinceLastSample(0.0f)
    , bCurrentlyBoosting(false)
{
    NodeName = TEXT("Fly To Staging Zone");
    bNotifyTick = true;

    // MANDATORY: Add filter for Vector key types
    StagingZoneLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FlyToStagingZone, StagingZoneLocationKey));
    TargetLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FlyToStagingZone, TargetLocationKey));
}

void UBTTask_FlyToStagingZone::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);

    // MANDATORY: Resolve blackboard keys
    if (UBlackboardData* BBAsset = GetBlackboardAsset())
    {
        StagingZoneLocationKey.ResolveSelectedKey(*BBAsset);
        TargetLocationKey.ResolveSelectedKey(*BBAsset);
    }
}

EBTNodeResult::Type UBTTask_FlyToStagingZone::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        UE_LOG(LogBTTask_FlyToStagingZone, Warning, TEXT("No AIController"));
        return EBTNodeResult::Failed;
    }

    APawn* Pawn = AIController->GetPawn();
    if (!Pawn)
    {
        UE_LOG(LogBTTask_FlyToStagingZone, Warning, TEXT("No Pawn"));
        return EBTNodeResult::Failed;
    }

    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB)
    {
        UE_LOG(LogBTTask_FlyToStagingZone, Warning, TEXT("No BlackboardComponent"));
        return EBTNodeResult::Failed;
    }

    // Verify agent is actually flying before attempting flight navigation
    UAC_BroomComponent* BroomComp = Pawn->FindComponentByClass<UAC_BroomComponent>();
    if (!BroomComp || !BroomComp->IsFlying())
    {
        UE_LOG(LogBTTask_FlyToStagingZone, Warning,
            TEXT("[%s] Cannot fly to staging - agent is not flying! BroomComp=%s, IsFlying=%s"),
            *Pawn->GetName(),
            BroomComp ? TEXT("Valid") : TEXT("NULL"),
            (BroomComp && BroomComp->IsFlying()) ? TEXT("YES") : TEXT("NO"));
        return EBTNodeResult::Failed;
    }

    // Solution 1: Cache FlightSteeringComponent if using obstacle avoidance
    if (bUseFlightSteering)
    {
        UAC_FlightSteeringComponent* SteeringComp = Pawn->FindComponentByClass<UAC_FlightSteeringComponent>();
        if (SteeringComp)
        {
            CachedSteeringComponent = SteeringComp;
            UE_LOG(LogBTTask_FlyToStagingZone, Display,
                TEXT("[%s] FlightSteeringComponent found - obstacle avoidance enabled"),
                *Pawn->GetName());
        }
        else
        {
            UE_LOG(LogBTTask_FlyToStagingZone, Warning,
                TEXT("[%s] FlightSteeringComponent not found - using direct flight"),
                *Pawn->GetName());
        }
    }

    // ========================================================================
    // PERCEPTION-BASED TARGET ACQUISITION (January 28, 2026)
    // PRIMARY: Read staging zone location from blackboard (written by BTService_FindStagingZone)
    // FALLBACK 1: Query GameMode directly
    // FALLBACK 2: Use HomeLocation
    // ========================================================================

    FString LocationSource = TEXT("Unknown");

    // PRIMARY: Read from StagingZoneLocationKey (written by BTService_FindStagingZone)
    if (StagingZoneLocationKey.IsSet())
    {
        CachedStagingLocation = BB->GetValueAsVector(StagingZoneLocationKey.SelectedKeyName);
        if (!CachedStagingLocation.IsZero())
        {
            LocationSource = TEXT("Perception (BTService)");
            UE_LOG(LogBTTask_FlyToStagingZone, Display,
                TEXT("[%s] Using PERCEPTION-BASED staging location from blackboard: %s"),
                *Pawn->GetName(), *CachedStagingLocation.ToString());
        }
    }

    // FALLBACK 1: Query GameMode directly (legacy method)
    if (CachedStagingLocation.IsZero())
    {
        AQuidditchGameMode* GameMode = Cast<AQuidditchGameMode>(Pawn->GetWorld()->GetAuthGameMode());
        if (GameMode)
        {
            EQuidditchTeam Team = GameMode->GetAgentTeam(Pawn);
            EQuidditchRole Role = GameMode->GetAgentRole(Pawn);

            if (Team != EQuidditchTeam::None && Role != EQuidditchRole::None)
            {
                // Query staging zone location using configured slot name
                FName EffectiveSlotName = SlotName;
                if (SlotName.IsNone())
                {
                    FString RoleStr = UEnum::GetValueAsString(Role);
                    int32 ColonIndex;
                    if (RoleStr.FindLastChar(TEXT(':'), ColonIndex))
                    {
                        RoleStr = RoleStr.Mid(ColonIndex + 1);
                    }
                    EffectiveSlotName = FName(*RoleStr);
                }
                CachedStagingLocation = GameMode->GetStagingZoneLocation(Team, Role, EffectiveSlotName);

                if (!CachedStagingLocation.IsZero())
                {
                    LocationSource = TEXT("GameMode (fallback)");
                    UE_LOG(LogBTTask_FlyToStagingZone, Display,
                        TEXT("[%s] Using GAMEMODE staging location (fallback): %s"),
                        *Pawn->GetName(), *CachedStagingLocation.ToString());
                }
            }
            else
            {
                UE_LOG(LogBTTask_FlyToStagingZone, Warning,
                    TEXT("[%s] Agent not registered with GameMode (Team=%s, Role=%s)"),
                    *Pawn->GetName(),
                    *UEnum::GetValueAsString(Team),
                    *UEnum::GetValueAsString(Role));
            }
        }
        else
        {
            UE_LOG(LogBTTask_FlyToStagingZone, Warning,
                TEXT("[%s] No QuidditchGameMode - cannot query staging zone"),
                *Pawn->GetName());
        }
    }

    // FALLBACK 2: Use HomeLocation from blackboard
    if (CachedStagingLocation.IsZero())
    {
        CachedStagingLocation = BB->GetValueAsVector(FName("HomeLocation"));
        if (!CachedStagingLocation.IsZero())
        {
            LocationSource = TEXT("HomeLocation (fallback)");
            UE_LOG(LogBTTask_FlyToStagingZone, Warning,
                TEXT("[%s] Using HOMELOCATION as staging zone fallback: %s"),
                *Pawn->GetName(), *CachedStagingLocation.ToString());
        }
    }

    // Final check - we need a valid location
    if (CachedStagingLocation.IsZero())
    {
        UE_LOG(LogBTTask_FlyToStagingZone, Error,
            TEXT("[%s] FAILED - No staging zone location found via perception, GameMode, or HomeLocation!"),
            *Pawn->GetName());
        return EBTNodeResult::Failed;
    }

    // Write to blackboard for other systems
    if (TargetLocationKey.IsSet())
    {
        BB->SetValueAsVector(TargetLocationKey.SelectedKeyName, CachedStagingLocation);
    }
    // Also write to StageLocation key for external systems to read
    BB->SetValueAsVector(FName("StageLocation"), CachedStagingLocation);

    // Reset all state
    bLocationSet = true;
    ElapsedFlightTime = 0.0f;
    TimeSinceLastSample = 0.0f;
    PositionHistory.Empty();

    UE_LOG(LogBTTask_FlyToStagingZone, Display,
        TEXT("[%s] Flying to staging zone at %s | Source=%s | ArrivalRadius=%.0f | Timeout=%s (%.0fs) | Steering=%s"),
        *Pawn->GetName(),
        *CachedStagingLocation.ToString(),
        *LocationSource,
        ArrivalRadius,
        bEnableTimeout ? TEXT("ON") : TEXT("OFF"),
        TimeoutDuration,
        bUseFlightSteering ? TEXT("ON") : TEXT("OFF"));

    // Check if already at destination
    float Distance = FVector::Dist(Pawn->GetActorLocation(), CachedStagingLocation);
    if (Distance <= ArrivalRadius)
    {
        UE_LOG(LogBTTask_FlyToStagingZone, Display,
            TEXT("[%s] Already at staging zone!"), *Pawn->GetName());
        return EBTNodeResult::Succeeded;
    }

    return EBTNodeResult::InProgress;
}

void UBTTask_FlyToStagingZone::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    if (!bLocationSet)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    APawn* Pawn = AIController->GetPawn();
    if (!Pawn)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    // Update elapsed time and check timeout (Solution 5)
    ElapsedFlightTime += DeltaSeconds;
    if (bEnableTimeout && ElapsedFlightTime >= TimeoutDuration)
    {
        UE_LOG(LogBTTask_FlyToStagingZone, Warning,
            TEXT("[%s] TIMEOUT after %.1fs - failed to reach staging zone! Triggering fallback."),
            *Pawn->GetName(), ElapsedFlightTime);

        if (UAC_BroomComponent* BroomComp = Pawn->FindComponentByClass<UAC_BroomComponent>())
        {
            BroomComp->SetBoostEnabled(false);
            BroomComp->SetVerticalInput(0.0f);
        }

        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    // Calculate distance and direction
    FVector CurrentLocation = Pawn->GetActorLocation();
    FVector ToTarget = CachedStagingLocation - CurrentLocation;
    float Distance = ToTarget.Size();

    // Update position history for stuck detection (Solution 4)
    if (bEnableStuckDetection)
    {
        UpdatePositionHistory(CurrentLocation, DeltaSeconds);

        // Check if stuck (only after we have enough samples)
        if (PositionHistory.Num() >= StuckSampleCount && IsAgentStuck())
        {
            UE_LOG(LogBTTask_FlyToStagingZone, Warning,
                TEXT("[%s] STUCK DETECTED - agent not making progress, failing to trigger fallback"),
                *Pawn->GetName());

            if (UAC_BroomComponent* BroomComp = Pawn->FindComponentByClass<UAC_BroomComponent>())
            {
                BroomComp->SetBoostEnabled(false);
                BroomComp->SetVerticalInput(0.0f);
            }

            FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
            return;
        }
    }

    // Solution 2: Standard arrival check with increased radius
    if (Distance <= ArrivalRadius)
    {
        UE_LOG(LogBTTask_FlyToStagingZone, Display,
            TEXT("[%s] Arrived at staging zone (%.0f units <= %.0f radius)"),
            *Pawn->GetName(), Distance, ArrivalRadius);

        if (UAC_BroomComponent* BroomComp = Pawn->FindComponentByClass<UAC_BroomComponent>())
        {
            BroomComp->SetBoostEnabled(false);
            BroomComp->SetVerticalInput(0.0f);
        }

        // Actively stop horizontal movement to prevent drift
        ACharacter* Character = Cast<ACharacter>(Pawn);
        if (Character)
        {
            UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
            if (MoveComp)
            {
                FVector Velocity = MoveComp->Velocity;
                Velocity.X = 0.0f;
                Velocity.Y = 0.0f;
                MoveComp->Velocity = Velocity;
            }
        }

        // Set ReachedStagingZone flag for downstream BT nodes
        UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
        if (BB)
        {
            BB->SetValueAsBool(FName("ReachedStagingZone"), true);
        }

        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        return;
    }

    // Solution 3: Velocity-based overshoot detection
    if (bEnableVelocityArrival && HasOvershotTarget(Pawn, ToTarget, Distance))
    {
        UE_LOG(LogBTTask_FlyToStagingZone, Display,
            TEXT("[%s] OVERSHOOT DETECTED at %.0f units - close enough, marking arrived"),
            *Pawn->GetName(), Distance);

        if (UAC_BroomComponent* BroomComp = Pawn->FindComponentByClass<UAC_BroomComponent>())
        {
            BroomComp->SetBoostEnabled(false);
            BroomComp->SetVerticalInput(0.0f);
        }

        // Set ReachedStagingZone flag for downstream BT nodes
        UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
        if (BB)
        {
            BB->SetValueAsBool(FName("ReachedStagingZone"), true);
        }

        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        return;
    }

    // Get broom component for flight control
    UAC_BroomComponent* BroomComp = Pawn->FindComponentByClass<UAC_BroomComponent>();

    // Get stamina for stamina-aware boost decision
    UAC_StaminaComponent* StaminaComp = Pawn->FindComponentByClass<UAC_StaminaComponent>();
    float CurrentStaminaPercent = StaminaComp ? StaminaComp->GetStaminaPercent() : 1.0f;

    // Solution 1: Use FlightSteeringComponent if available
    if (bUseFlightSteering && CachedSteeringComponent.IsValid())
    {
        FVector SteeringInput = CachedSteeringComponent->CalculateSteeringToward(CachedStagingLocation);

        if (BroomComp)
        {
            // Boost with hysteresis to prevent oscillation:
            // - Turn ON when distance > BoostDistanceThreshold (500) AND stamina >= 30%
            // - Turn OFF when distance < BoostDisableThreshold (300) OR stamina < 30%
            bool bShouldBoost = bCurrentlyBoosting;
            if (!bCurrentlyBoosting)
            {
                // Currently not boosting - check if we should turn ON
                bShouldBoost = bUseBoostWhenFar
                    && (Distance > BoostDistanceThreshold)
                    && (CurrentStaminaPercent >= MinStaminaForBoost);
            }
            else
            {
                // Currently boosting - check if we should turn OFF
                bShouldBoost = bUseBoostWhenFar
                    && (Distance > BoostDisableThreshold)
                    && (CurrentStaminaPercent >= MinStaminaForBoost);
            }
            bCurrentlyBoosting = bShouldBoost;
            BroomComp->SetBoostEnabled(bShouldBoost);

            // Apply steering output (Pitch, Yaw, Thrust mapped from steering component)
            BroomComp->SetVerticalInput(SteeringInput.X);  // Pitch controls altitude

            // Use steering output for movement direction
            // Direct velocity control for AI pawns - AddMovementInput requires PlayerController processing
            FVector SteeringDirection = Pawn->GetActorForwardVector() * SteeringInput.Z;
            SteeringDirection += Pawn->GetActorRightVector() * SteeringInput.Y * 0.5f;
            SteeringDirection = SteeringDirection.GetSafeNormal();

            ACharacter* Character = Cast<ACharacter>(Pawn);
            if (Character)
            {
                UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
                if (MoveComp && MoveComp->MovementMode == MOVE_Flying)
                {
                    float TargetSpeed = MoveComp->MaxFlySpeed * FlightSpeedMultiplier;

                    // Slow down as we approach the target to prevent overshooting
                    float SlowdownRadius = ArrivalRadius * 2.0f;
                    if (Distance < SlowdownRadius)
                    {
                        float SlowdownFactor = Distance / SlowdownRadius;
                        TargetSpeed *= FMath::Max(SlowdownFactor, 0.2f);
                    }

                    // Set velocity directly in target direction - no interpolation
                    // VInterpTo causes circular orbits when direction changes rapidly at high speed
                    FVector DesiredVelocity = SteeringDirection * TargetSpeed;

                    // Preserve vertical velocity managed by BroomComponent
                    DesiredVelocity.Z = MoveComp->Velocity.Z;

                    // Direct velocity assignment for AI - instant direction change
                    MoveComp->Velocity = DesiredVelocity;
                }
            }

            // Rotate toward movement direction
            FVector MoveDir = (CachedStagingLocation - CurrentLocation).GetSafeNormal();
            if (!MoveDir.IsNearlyZero())
            {
                FRotator TargetRotation = MoveDir.Rotation();
                FRotator CurrentRotation = Pawn->GetActorRotation();
                FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaSeconds, 5.0f);
                Pawn->SetActorRotation(NewRotation);
            }
        }
    }
    else
    {
        // Fallback: Direct flight without obstacle avoidance
        if (BroomComp)
        {
            // Boost with hysteresis to prevent oscillation:
            // - Turn ON when distance > BoostDistanceThreshold (500) AND stamina >= 30%
            // - Turn OFF when distance < BoostDisableThreshold (300) OR stamina < 30%
            bool bShouldBoost = bCurrentlyBoosting;
            if (!bCurrentlyBoosting)
            {
                // Currently not boosting - check if we should turn ON
                bShouldBoost = bUseBoostWhenFar
                    && (Distance > BoostDistanceThreshold)
                    && (CurrentStaminaPercent >= MinStaminaForBoost);
            }
            else
            {
                // Currently boosting - check if we should turn OFF
                bShouldBoost = bUseBoostWhenFar
                    && (Distance > BoostDisableThreshold)
                    && (CurrentStaminaPercent >= MinStaminaForBoost);
            }
            bCurrentlyBoosting = bShouldBoost;
            BroomComp->SetBoostEnabled(bShouldBoost);

            float AltitudeDiff = CachedStagingLocation.Z - CurrentLocation.Z;
            float VerticalInput = FMath::Clamp(AltitudeDiff / AltitudeScale, -1.0f, 1.0f);
            BroomComp->SetVerticalInput(VerticalInput);
        }

        // Use horizontal direction only - SetVerticalInput handles altitude separately
        // Direct velocity control for AI pawns - AddMovementInput requires PlayerController processing
        FVector Direction = FVector(ToTarget.X, ToTarget.Y, 0.0f).GetSafeNormal();

        ACharacter* Character = Cast<ACharacter>(Pawn);
        if (Character)
        {
            UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
            if (MoveComp && MoveComp->MovementMode == MOVE_Flying)
            {
                float TargetSpeed = MoveComp->MaxFlySpeed * FlightSpeedMultiplier;

                // Slow down as we approach the target to prevent overshooting
                float SlowdownRadius = ArrivalRadius * 2.0f;
                if (Distance < SlowdownRadius)
                {
                    float SlowdownFactor = Distance / SlowdownRadius;
                    TargetSpeed *= FMath::Max(SlowdownFactor, 0.2f);
                }

                // Set velocity directly in target direction - no interpolation
                // VInterpTo causes circular orbits when direction changes rapidly at high speed
                FVector DesiredVelocity = Direction * TargetSpeed;

                // Preserve vertical velocity managed by BroomComponent
                DesiredVelocity.Z = MoveComp->Velocity.Z;

                // Direct velocity assignment for AI - instant direction change
                MoveComp->Velocity = DesiredVelocity;
            }
        }

        if (!Direction.IsNearlyZero())
        {
            FRotator TargetRotation = Direction.Rotation();
            FRotator CurrentRotation = Pawn->GetActorRotation();
            FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaSeconds, 5.0f);
            Pawn->SetActorRotation(NewRotation);
        }
    }
}

void UBTTask_FlyToStagingZone::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
    // Clean up state
    bLocationSet = false;
    CachedStagingLocation = FVector::ZeroVector;
    ElapsedFlightTime = 0.0f;
    TimeSinceLastSample = 0.0f;
    PositionHistory.Empty();
    CachedSteeringComponent.Reset();
    bCurrentlyBoosting = false;

    // Stop boost if we have pawn access
    if (AAIController* AIController = OwnerComp.GetAIOwner())
    {
        if (APawn* Pawn = AIController->GetPawn())
        {
            if (UAC_BroomComponent* BroomComp = Pawn->FindComponentByClass<UAC_BroomComponent>())
            {
                BroomComp->SetBoostEnabled(false);
            }
        }
    }
}

FString UBTTask_FlyToStagingZone::GetStaticDescription() const
{
    FString SlotDesc = SlotName.IsNone() ? TEXT("(Role Name)") : SlotName.ToString();
    FString Features;

    if (bUseFlightSteering) Features += TEXT("Steering ");
    if (bEnableVelocityArrival) Features += TEXT("Velocity ");
    if (bEnableStuckDetection) Features += TEXT("Stuck ");
    if (bEnableTimeout) Features += TEXT("Timeout ");

    return FString::Printf(TEXT("Fly to staging zone\nArrival: %.0f | Slot: %s\nFeatures: %s"),
        ArrivalRadius, *SlotDesc, *Features);
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

bool UBTTask_FlyToStagingZone::HasOvershotTarget(APawn* Pawn, const FVector& ToTarget, float Distance) const
{
    // Only check overshoot within extended radius
    if (Distance > ExtendedArrivalRadius)
    {
        return false;
    }

    FVector Velocity = Pawn->GetVelocity();
    if (Velocity.IsNearlyZero())
    {
        return false;
    }

    // Calculate approach speed (positive = moving toward, negative = moving away)
    FVector ToTargetNorm = ToTarget.GetSafeNormal();
    float ApproachSpeed = FVector::DotProduct(Velocity, ToTargetNorm);

    // If moving away from target (negative approach) and within extended radius
    if (ApproachSpeed < OvershootSpeedThreshold)
    {
        UE_LOG(LogBTTask_FlyToStagingZone, Verbose,
            TEXT("Overshoot check: Distance=%.0f, ApproachSpeed=%.0f (threshold=%.0f)"),
            Distance, ApproachSpeed, OvershootSpeedThreshold);
        return true;
    }

    return false;
}

bool UBTTask_FlyToStagingZone::IsAgentStuck() const
{
    if (PositionHistory.Num() < StuckSampleCount)
    {
        return false;
    }

    // Calculate total movement over all samples
    float TotalMovement = 0.0f;
    for (int32 i = 1; i < PositionHistory.Num(); i++)
    {
        TotalMovement += FVector::Dist(PositionHistory[i], PositionHistory[i - 1]);
    }

    // If total movement is below threshold, agent is stuck
    bool bStuck = TotalMovement < StuckDistanceThreshold;

    if (bStuck)
    {
        UE_LOG(LogBTTask_FlyToStagingZone, Verbose,
            TEXT("Stuck check: TotalMovement=%.0f over %d samples (threshold=%.0f) = %s"),
            TotalMovement, PositionHistory.Num(), StuckDistanceThreshold,
            bStuck ? TEXT("STUCK") : TEXT("OK"));
    }

    return bStuck;
}

void UBTTask_FlyToStagingZone::UpdatePositionHistory(const FVector& CurrentLocation, float DeltaSeconds)
{
    TimeSinceLastSample += DeltaSeconds;

    if (TimeSinceLastSample >= StuckCheckInterval)
    {
        TimeSinceLastSample = 0.0f;

        // Add current position
        PositionHistory.Add(CurrentLocation);

        // Keep only the most recent samples
        while (PositionHistory.Num() > StuckSampleCount)
        {
            PositionHistory.RemoveAt(0);
        }
    }
}
