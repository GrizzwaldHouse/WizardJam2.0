// ============================================================================
// BTTask_ControlFlight.cpp
// Implementation of continuous flight control for AI agents
//
// Developer: Marcus Daley
// Date: January 19, 2026
// Project: Game Architecture (END2507)
//
// This implementation shows how simple AI flight control becomes when using
// the Core API approach. The AI just calls the same functions the player uses.
//
// FLIGHT CONTROL LOGIC:
// 1. Read target from Blackboard
// 2. Calculate altitude difference
// 3. Call SetVerticalInput() with normalized value (-1 to +1)
// 4. Calculate horizontal distance
// 5. Call SetBoostEnabled() if far away
// 6. Add movement input toward target
// 7. Check if arrived, finish task if so
// ============================================================================

#include "Code/AI/BTTask_ControlFlight.h"
#include "Code/Flight/AC_BroomComponent.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Code/Utilities/AC_StaminaComponent.h"

// ============================================================================
// CONSTRUCTOR
// ============================================================================

UBTTask_ControlFlight::UBTTask_ControlFlight()
    : ArrivalTolerance(100.0f)
    , AltitudeTolerance(50.0f)
    , bUseBoostWhenFar(true)
    , BoostDistanceThreshold(500.0f)
    , VerticalInputMultiplier(1.0f)
    , bDirectFlight(true)
    , BoostStaminaThreshold(0.4f)
    , ThrottleStaminaThreshold(0.25f)
    , LowStaminaMovementScale(0.5f)
    , bLandWhenStaminaCritical(true)
    , CriticalStaminaThreshold(0.15f)
{
    NodeName = "Control Flight";

    // This task ticks every frame while active
    bNotifyTick = true;

    // Key filters allow both Object (Actor) and Vector types in BT editor dropdown
    TargetKey.AddObjectFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTTask_ControlFlight, TargetKey),
        AActor::StaticClass());
    TargetKey.AddVectorFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTTask_ControlFlight, TargetKey));
}

// ============================================================================
// EXECUTE TASK (Called once when task starts)
// ============================================================================

EBTNodeResult::Type UBTTask_ControlFlight::ExecuteTask(
    UBehaviorTreeComponent& OwnerComp, 
    uint8* NodeMemory)
{
    // Validate we have an AI controller and pawn
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController || !AIController->GetPawn())
    {
        UE_LOG(LogTemp, Warning, 
            TEXT("[BTTask_ControlFlight] No AI Controller or Pawn"));
        return EBTNodeResult::Failed;
    }

    // Verify pawn has broom component and is flying
    UAC_BroomComponent* BroomComp = 
        AIController->GetPawn()->FindComponentByClass<UAC_BroomComponent>();
    
    if (!BroomComp)
    {
        UE_LOG(LogTemp, Warning, 
            TEXT("[BTTask_ControlFlight] Pawn has no AC_BroomComponent"));
        return EBTNodeResult::Failed;
    }

    if (!BroomComp->IsFlying())
    {
        UE_LOG(LogTemp, Warning, 
            TEXT("[BTTask_ControlFlight] Pawn is not flying - run BTTask_MountBroom first"));
        return EBTNodeResult::Failed;
    }

    // Validate target key is set
    FVector TargetLocation;
    if (!GetTargetLocation(OwnerComp, TargetLocation))
    {
        UE_LOG(LogTemp, Warning, 
            TEXT("[BTTask_ControlFlight] Could not get target location from Blackboard"));
        return EBTNodeResult::Failed;
    }

    UE_LOG(LogTemp, Display, 
        TEXT("[BTTask_ControlFlight] Started flying toward %s"),
        *TargetLocation.ToString());

    // Task runs continuously until we reach target
    return EBTNodeResult::InProgress;
}

// ============================================================================
// TICK TASK (Called every frame while task is active)
// ============================================================================

void UBTTask_ControlFlight::TickTask(
    UBehaviorTreeComponent& OwnerComp, 
    uint8* NodeMemory, 
    float DeltaSeconds)
{
    // Get references
    AAIController* AIController = OwnerComp.GetAIOwner();
    APawn* AIPawn = AIController ? AIController->GetPawn() : nullptr;
    
    if (!AIPawn)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    UAC_BroomComponent* BroomComp = AIPawn->FindComponentByClass<UAC_BroomComponent>();
    if (!BroomComp || !BroomComp->IsFlying())
    {
        // Lost flight capability - task fails
        UE_LOG(LogTemp, Warning, 
            TEXT("[BTTask_ControlFlight] Lost flight during task"));
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    // Get target location from blackboard
    FVector TargetLocation;
    if (!GetTargetLocation(OwnerComp, TargetLocation))
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    // Get current location
    FVector CurrentLocation = AIPawn->GetActorLocation();

    // ========================================================================
    // VERTICAL CONTROL - Using the Core API!
    // Same function the player's HandleAscendInput calls
    // ========================================================================

    float AltitudeDiff = TargetLocation.Z - CurrentLocation.Z;
    
    if (FMath::Abs(AltitudeDiff) > AltitudeTolerance)
    {
        // Need to ascend or descend
        // Normalize to -1.0 to 1.0 range, scale by multiplier
        float VerticalInput = FMath::Clamp(AltitudeDiff / 200.0f, -1.0f, 1.0f);
        VerticalInput *= VerticalInputMultiplier;
        
        // Call Core API - same function player input uses!
        BroomComp->SetVerticalInput(VerticalInput);
    }
    else
    {
        // At correct altitude - hover
        BroomComp->SetVerticalInput(0.0f);
    }

    // ========================================================================
    // STAMINA CHECK - Critical threshold detection
    // ========================================================================

    float StaminaPercent = GetStaminaPercentage(AIPawn);

    // Handle missing stamina component (error case)
    if (StaminaPercent < 0.0f)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[BTTask_ControlFlight] Cannot determine stamina - aborting flight"));
        BroomComp->SetBoostEnabled(false);
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    // Check for critical stamina - abort flight and land
    if (bLandWhenStaminaCritical && StaminaPercent < CriticalStaminaThreshold)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[BTTask_ControlFlight] CRITICAL STAMINA (%.0f%%) - Aborting flight!"),
            StaminaPercent * 100.0f);

        // Disable boost immediately
        BroomComp->SetBoostEnabled(false);

        // Disable flight (this will trigger landing)
        BroomComp->SetFlightEnabled(false);

        // Task fails - AI needs to recover stamina
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    // ========================================================================
    // BOOST CONTROL - With stamina awareness
    // Same function the player's HandleBoostInput calls
    // ========================================================================

    float HorizontalDist = FVector::Dist2D(CurrentLocation, TargetLocation);

    if (bUseBoostWhenFar)
    {
        // Boost requires BOTH distance threshold AND sufficient stamina
        bool bDistanceWantsBoost = HorizontalDist > BoostDistanceThreshold;
        bool bStaminaAllowsBoost = StaminaPercent >= BoostStaminaThreshold;

        bool bShouldBoost = bDistanceWantsBoost && bStaminaAllowsBoost;
        BroomComp->SetBoostEnabled(bShouldBoost);

        // Log when boost is denied due to stamina
        if (bDistanceWantsBoost && !bStaminaAllowsBoost)
        {
            UE_LOG(LogTemp, Verbose,
                TEXT("[BTTask_ControlFlight] Boost denied - stamina %.0f%% < threshold %.0f%%"),
                StaminaPercent * 100.0f, BoostStaminaThreshold * 100.0f);
        }
    }
    else
    {
        // Boost disabled by config - ensure it's off
        BroomComp->SetBoostEnabled(false);
    }

    // ========================================================================
    // MOVEMENT SPEED CALCULATION - Throttle based on stamina
    // ========================================================================

    float MovementScale = 1.0f;

    // Apply throttling when stamina is low (but not critical)
    if (StaminaPercent < ThrottleStaminaThreshold)
    {
        // Interpolate between LowStaminaMovementScale and 1.0 based on stamina
        // At ThrottleStaminaThreshold: full speed
        // At CriticalStaminaThreshold: LowStaminaMovementScale
        float ThrottleRange = ThrottleStaminaThreshold - CriticalStaminaThreshold;
        if (ThrottleRange > KINDA_SMALL_NUMBER)
        {
            float StaminaInRange = StaminaPercent - CriticalStaminaThreshold;
            float ThrottleAlpha = FMath::Clamp(StaminaInRange / ThrottleRange, 0.0f, 1.0f);
            MovementScale = FMath::Lerp(LowStaminaMovementScale, 1.0f, ThrottleAlpha);
        }
        else
        {
            MovementScale = LowStaminaMovementScale;
        }

        UE_LOG(LogTemp, Verbose,
            TEXT("[BTTask_ControlFlight] Throttling movement to %.0f%% (stamina: %.0f%%)"),
            MovementScale * 100.0f, StaminaPercent * 100.0f);
    }

    // ========================================================================
    // HORIZONTAL MOVEMENT - Direct velocity control for AI pawns
    // AddMovementInput requires PlayerController processing, so we directly
    // set velocity for AI-controlled flight
    // ========================================================================

    FVector DirectionToTarget;
    if (bDirectFlight)
    {
        // Move directly toward target in 3D with stamina-scaled speed
        DirectionToTarget = (TargetLocation - CurrentLocation).GetSafeNormal();
    }
    else
    {
        // Only horizontal movement - vertical handled by SetVerticalInput above
        DirectionToTarget = (TargetLocation - CurrentLocation);
        DirectionToTarget.Z = 0.0f;
        DirectionToTarget.Normalize();
    }

    // Direct velocity control for AI pawns
    ACharacter* Character = Cast<ACharacter>(AIPawn);
    if (Character)
    {
        UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
        if (MoveComp && MoveComp->MovementMode == MOVE_Flying)
        {
            float TargetSpeed = MoveComp->MaxFlySpeed * MovementScale;
            FVector DesiredVelocity = DirectionToTarget * TargetSpeed;

            // For bDirectFlight, include Z component; otherwise preserve BroomComponent's Z
            if (!bDirectFlight)
            {
                DesiredVelocity.Z = MoveComp->Velocity.Z;
            }

            // Direct velocity assignment for AI - instant direction change
            // VInterpTo causes circular orbits when direction changes rapidly at high speed
            MoveComp->Velocity = DesiredVelocity;
        }
    }

    // ========================================================================
    // ROTATION CONTROL - Prevents circular/spiral movement
    // Without this, pawn faces wrong direction while AddMovementInput pushes
    // toward target, causing spiral paths instead of direct flight
    // ========================================================================
    FVector RotationDirection = (TargetLocation - CurrentLocation);
    RotationDirection.Z = 0.0f;  // Keep rotation on horizontal plane only

    if (!RotationDirection.IsNearlyZero())
    {
        RotationDirection.Normalize();
        FRotator TargetRotation = RotationDirection.Rotation();
        FRotator CurrentRotation = AIPawn->GetActorRotation();
        FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaSeconds, 5.0f);
        AIPawn->SetActorRotation(NewRotation);
    }

    // ========================================================================
    // CHECK ARRIVAL
    // ========================================================================

    float TotalDist = FVector::Dist(CurrentLocation, TargetLocation);
    
    if (TotalDist < ArrivalTolerance)
    {
        // Arrived at target!
        BroomComp->SetVerticalInput(0.0f);
        BroomComp->SetBoostEnabled(false);
        
        UE_LOG(LogTemp, Display, 
            TEXT("[BTTask_ControlFlight] Arrived at target!"));
        
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
    }
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

bool UBTTask_ControlFlight::GetTargetLocation(
    UBehaviorTreeComponent& OwnerComp, 
    FVector& OutLocation) const
{
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    if (!Blackboard || !TargetKey.IsSet())
    {
        return false;
    }

    // Check if key is an Object (actor) or Vector
    if (TargetKey.SelectedKeyType == UBlackboardKeyType_Object::StaticClass())
    {
        // Target is an actor - get its location
        UObject* TargetObject = Blackboard->GetValueAsObject(TargetKey.SelectedKeyName);
        AActor* TargetActor = Cast<AActor>(TargetObject);
        
        if (TargetActor)
        {
            OutLocation = TargetActor->GetActorLocation();
            return true;
        }
    }
    else if (TargetKey.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
    {
        // Target is a vector location
        OutLocation = Blackboard->GetValueAsVector(TargetKey.SelectedKeyName);

        // Validate vector is not zero (indicates uninitialized or failed lookup)
        if (OutLocation.IsNearlyZero())
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[BTTask_ControlFlight] TargetLocation is zero/unset - check staging zone registration"));
            return false;
        }

        return true;
    }

    return false;
}

// ============================================================================
// DESCRIPTION
// ============================================================================

FString UBTTask_ControlFlight::GetStaticDescription() const
{
    FString Description = FString::Printf(
        TEXT("Fly toward: %s\nArrival tolerance: %.0f"),
        TargetKey.IsSet() ? *TargetKey.SelectedKeyName.ToString() : TEXT("(not set)"),
        ArrivalTolerance
    );

    if (bUseBoostWhenFar)
    {
        Description += FString::Printf(
            TEXT("\nBoost when >%.0f away (need %.0f%% stamina)"),
            BoostDistanceThreshold,
            BoostStaminaThreshold * 100.0f);
    }

    if (bDirectFlight)
    {
        Description += TEXT("\nDirect 3D flight");
    }

    // Add stamina threshold info
    Description += FString::Printf(
        TEXT("\nThrottle below %.0f%% stamina"),
        ThrottleStaminaThreshold * 100.0f);

    if (bLandWhenStaminaCritical)
    {
        Description += FString::Printf(
            TEXT("\nAuto-land below %.0f%% stamina"),
            CriticalStaminaThreshold * 100.0f);
    }

    return Description;
}

// ============================================================================
// INITIALIZE FROM ASSET (CRITICAL for Blackboard key resolution)
// ============================================================================

void UBTTask_ControlFlight::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);

    // Resolve blackboard key against the behavior tree's blackboard asset
    // Without this, TargetKey.SelectedKeyType and SelectedKeyName won't be set
    if (UBlackboardData* BBAsset = GetBlackboardAsset())
    {
        TargetKey.ResolveSelectedKey(*BBAsset);
    }
}

// ============================================================================
// STAMINA HELPER
// ============================================================================

float UBTTask_ControlFlight::GetStaminaPercentage(APawn* Pawn) const
{
    if (!Pawn)
    {
        return -1.0f;
    }

    // Try to find AC_StaminaComponent on the pawn
    UAC_StaminaComponent* StaminaComp =
        Pawn->FindComponentByClass<UAC_StaminaComponent>();

    if (!StaminaComp)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[BTTask_ControlFlight] Pawn %s has no AC_StaminaComponent!"),
            *Pawn->GetName());
        return -1.0f;
    }

    return StaminaComp->GetStaminaPercent();
}
