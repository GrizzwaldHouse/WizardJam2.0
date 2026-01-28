// ============================================================================
// BTTask_ReturnToHome.cpp
// Developer: Marcus Daley
// Date: January 26, 2026
// Project: END2507
// ============================================================================

#include "Code/AI/BTTask_ReturnToHome.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Code/Flight/AC_BroomComponent.h"
#include "Code/Utilities/AC_StaminaComponent.h"
// NOTE: GameMode include removed - now reads HomeLocation from Blackboard instead

DEFINE_LOG_CATEGORY(LogBTTask_ReturnToHome);

UBTTask_ReturnToHome::UBTTask_ReturnToHome()
	: ArrivalRadius(200.0f)
	, FlightSpeedMultiplier(1.0f)
	, bUseBoostWhenFar(true)
	, BoostDistanceThreshold(500.0f)
	, BoostDisableThreshold(300.0f)   // Hysteresis: boost turns OFF at 300, turns ON at 500
	, MinStaminaForBoost(0.3f)        // Don't boost below 30% stamina
	, AltitudeScale(200.0f)
	, SlotName(NAME_None)
	, bEnableTimeout(true)     // Default: timeout enabled
	, TimeoutDuration(45.0f)   // Default: 45 seconds to reach home (longer than staging)
	, CachedHomeLocation(FVector::ZeroVector)
	, bLocationSet(false)
	, ElapsedFlightTime(0.0f)
	, bIsFlying(false)
	, bCurrentlyBoosting(false)
{
	NodeName = TEXT("Return To Home (Fallback)");
	bNotifyTick = true;

	// MANDATORY: Add filter for Vector key type
	HomeLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ReturnToHome, HomeLocationKey));
}

void UBTTask_ReturnToHome::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	// MANDATORY: Resolve blackboard key
	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		HomeLocationKey.ResolveSelectedKey(*BBAsset);
	}
}

EBTNodeResult::Type UBTTask_ReturnToHome::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		UE_LOG(LogBTTask_ReturnToHome, Warning, TEXT("No AIController"));
		return EBTNodeResult::Failed;
	}

	APawn* Pawn = AIController->GetPawn();
	if (!Pawn)
	{
		UE_LOG(LogBTTask_ReturnToHome, Warning, TEXT("No Pawn"));
		return EBTNodeResult::Failed;
	}

	// ========================================================================
	// Check if agent is flying - this determines navigation mode
	// If agent dismounted mid-flight (stamina ran out), we need ground navigation
	// ========================================================================
	UAC_BroomComponent* BroomComp = Pawn->FindComponentByClass<UAC_BroomComponent>();
	bIsFlying = (BroomComp && BroomComp->IsFlying());

	UE_LOG(LogBTTask_ReturnToHome, Display,
		TEXT("[%s] ReturnToHome starting | IsFlying=%s | BroomComp=%s"),
		*Pawn->GetName(),
		bIsFlying ? TEXT("YES") : TEXT("NO"),
		BroomComp ? TEXT("Valid") : TEXT("NULL"));

	// ========================================================================
	// SIMPLIFIED: Read HomeLocation from Blackboard (set to spawn position)
	// This avoids the staging zone lookup that fails when zones aren't placed
	// HomeLocation is initialized in AIC_QuidditchController::SetupBlackboard()
	// ========================================================================
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB)
	{
		UE_LOG(LogBTTask_ReturnToHome, Warning, TEXT("No Blackboard"));
		return EBTNodeResult::Failed;
	}

	// Read HomeLocation from blackboard (initialized to spawn position)
	if (HomeLocationKey.IsSet())
	{
		CachedHomeLocation = BB->GetValueAsVector(HomeLocationKey.SelectedKeyName);
	}
	else
	{
		// Fallback: use hardcoded key name if selector not configured
		CachedHomeLocation = BB->GetValueAsVector(FName("HomeLocation"));
		UE_LOG(LogBTTask_ReturnToHome, Warning,
			TEXT("[%s] HomeLocationKey not set - using hardcoded 'HomeLocation' key"), *Pawn->GetName());
	}

	if (CachedHomeLocation.IsZero())
	{
		UE_LOG(LogBTTask_ReturnToHome, Warning,
			TEXT("[%s] HomeLocation is zero - agent may have spawned at origin or BB key not initialized!"),
			*Pawn->GetName());
		return EBTNodeResult::Failed;
	}

	// If agent is walking (not flying), use ground-level destination
	// This handles the case where agent ran out of stamina and dismounted
	if (!bIsFlying)
	{
		// Use only XY position, keep current Z (ground level)
		CachedHomeLocation.Z = Pawn->GetActorLocation().Z;
		UE_LOG(LogBTTask_ReturnToHome, Display,
			TEXT("[%s] Agent is WALKING - using ground-level home at %s"),
			*Pawn->GetName(), *CachedHomeLocation.ToString());
	}

	bLocationSet = true;
	ElapsedFlightTime = 0.0f;  // Reset timer when task starts

	UE_LOG(LogBTTask_ReturnToHome, Display,
		TEXT("[%s] Returning home to %s | Mode=%s | Timeout=%s (%.0fs)"),
		*Pawn->GetName(),
		*CachedHomeLocation.ToString(),
		bIsFlying ? TEXT("FLYING") : TEXT("WALKING"),
		bEnableTimeout ? TEXT("ON") : TEXT("OFF"),
		TimeoutDuration);

	// Check if already at home (use 2D distance for walking, 3D for flying)
	float Distance;
	if (bIsFlying)
	{
		Distance = FVector::Dist(Pawn->GetActorLocation(), CachedHomeLocation);
	}
	else
	{
		Distance = FVector::Dist2D(Pawn->GetActorLocation(), CachedHomeLocation);
	}

	if (Distance <= ArrivalRadius)
	{
		UE_LOG(LogBTTask_ReturnToHome, Display,
			TEXT("[%s] Already at home!"), *Pawn->GetName());
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::InProgress;
}

void UBTTask_ReturnToHome::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
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

	// Check if flight state changed mid-task (agent dismounted due to stamina)
	UAC_BroomComponent* BroomComp = Pawn->FindComponentByClass<UAC_BroomComponent>();
	bool bCurrentlyFlying = (BroomComp && BroomComp->IsFlying());

	if (bIsFlying && !bCurrentlyFlying)
	{
		// Agent just dismounted! Adjust to ground navigation
		bIsFlying = false;
		CachedHomeLocation.Z = Pawn->GetActorLocation().Z;
		UE_LOG(LogBTTask_ReturnToHome, Warning,
			TEXT("[%s] Agent DISMOUNTED mid-flight! Switching to ground navigation at %s"),
			*Pawn->GetName(), *CachedHomeLocation.ToString());
	}

	// Update elapsed time and check timeout
	ElapsedFlightTime += DeltaSeconds;
	if (bEnableTimeout && ElapsedFlightTime >= TimeoutDuration)
	{
		UE_LOG(LogBTTask_ReturnToHome, Warning,
			TEXT("[%s] TIMEOUT after %.1fs - failed to reach home! (Mode=%s)"),
			*Pawn->GetName(), ElapsedFlightTime, bIsFlying ? TEXT("FLYING") : TEXT("WALKING"));

		// Stop boost before failing
		if (BroomComp)
		{
			BroomComp->SetBoostEnabled(false);
			BroomComp->SetVerticalInput(0.0f);
		}

		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// Calculate distance and direction
	FVector CurrentLocation = Pawn->GetActorLocation();
	FVector ToHome = CachedHomeLocation - CurrentLocation;

	// Use 2D distance for walking, 3D for flying
	float Distance;
	if (bIsFlying)
	{
		Distance = ToHome.Size();
	}
	else
	{
		Distance = FVector::Dist2D(CurrentLocation, CachedHomeLocation);
		// For walking, flatten the direction vector
		ToHome.Z = 0.0f;
	}

	// Check arrival
	if (Distance <= ArrivalRadius)
	{
		UE_LOG(LogBTTask_ReturnToHome, Display,
			TEXT("[%s] Arrived home (%.0f units) | Mode=%s"),
			*Pawn->GetName(), Distance, bIsFlying ? TEXT("FLYING") : TEXT("WALKING"));

		// Stop boost if flying
		if (BroomComp)
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

		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// Flight-specific controls
	if (bIsFlying && BroomComp)
	{
		// Get stamina for stamina-aware boost decision
		UAC_StaminaComponent* StaminaComp = Pawn->FindComponentByClass<UAC_StaminaComponent>();
		float CurrentStaminaPercent = StaminaComp ? StaminaComp->GetStaminaPercent() : 1.0f;

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

		// Calculate altitude difference
		float AltitudeDiff = CachedHomeLocation.Z - CurrentLocation.Z;
		float VerticalInput = FMath::Clamp(AltitudeDiff / AltitudeScale, -1.0f, 1.0f);
		BroomComp->SetVerticalInput(VerticalInput);
	}

	// Add movement input toward home (works for both walking and flying)
	// For flying, use only horizontal direction - vertical is handled by SetVerticalInput
	FVector Direction;
	if (bIsFlying)
	{
		// Horizontal direction only - SetVerticalInput handles altitude
		Direction = FVector(ToHome.X, ToHome.Y, 0.0f).GetSafeNormal();

		// Direct velocity control for AI pawns - AddMovementInput requires PlayerController processing
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

				// Log BEFORE assignment to compare
				FVector VelBefore = MoveComp->Velocity;

				// Direct velocity assignment for AI - instant direction change
				MoveComp->Velocity = DesiredVelocity;

				// Log AFTER assignment to verify it took effect
				UE_LOG(LogBTTask_ReturnToHome, Verbose,
					TEXT("[%s] VEL SET: Before=%s | Desired=%s | After=%s | Dir=%s"),
					*Pawn->GetName(),
					*VelBefore.ToString(),
					*DesiredVelocity.ToString(),
					*MoveComp->Velocity.ToString(),
					*Direction.ToString());
			}
		}
	}
	else
	{
		Direction = ToHome.GetSafeNormal();
		// Ground movement still uses AddMovementInput (works for walking characters)
		Pawn->AddMovementInput(Direction, FlightSpeedMultiplier);
	}

	// Debug log every 2 seconds - include velocity to diagnose circular flight
	static float DebugTimer = 0.0f;
	DebugTimer += GetWorld()->GetDeltaSeconds();
	if (DebugTimer >= 2.0f)
	{
		DebugTimer = 0.0f;
		FVector ActualVelocity = FVector::ZeroVector;
		if (ACharacter* Char = Cast<ACharacter>(Pawn))
		{
			if (UCharacterMovementComponent* MC = Char->GetCharacterMovement())
			{
				ActualVelocity = MC->Velocity;
			}
		}
		UE_LOG(LogBTTask_ReturnToHome, Display,
			TEXT("[%s] ReturnHome TICK: Dist=%.0f | Home=%s | Current=%s | Dir=%s | Vel=%s | Flying=%s"),
			*Pawn->GetName(), Distance,
			*CachedHomeLocation.ToString(),
			*CurrentLocation.ToString(),
			*Direction.ToString(),
			*ActualVelocity.ToString(),
			bIsFlying ? TEXT("YES") : TEXT("NO"));
	}

	// Rotate pawn toward home (only yaw for walking)
	if (!Direction.IsNearlyZero())
	{
		FRotator TargetRotation = Direction.Rotation();
		if (!bIsFlying)
		{
			// For walking, only rotate yaw (keep pitch/roll at 0)
			TargetRotation.Pitch = 0.0f;
			TargetRotation.Roll = 0.0f;
		}
		FRotator CurrentRotation = Pawn->GetActorRotation();
		FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaSeconds, 5.0f);
		Pawn->SetActorRotation(NewRotation);
	}
}

void UBTTask_ReturnToHome::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	// Clean up state
	bLocationSet = false;
	CachedHomeLocation = FVector::ZeroVector;
	ElapsedFlightTime = 0.0f;
	bIsFlying = false;
	bCurrentlyBoosting = false;

	// Stop boost
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

FString UBTTask_ReturnToHome::GetStaticDescription() const
{
	FString SlotDesc = SlotName.IsNone() ? TEXT("(Role Name)") : SlotName.ToString();
	return FString::Printf(TEXT("Return home\nArrival: %.0f | Slot: %s"), ArrivalRadius, *SlotDesc);
}
