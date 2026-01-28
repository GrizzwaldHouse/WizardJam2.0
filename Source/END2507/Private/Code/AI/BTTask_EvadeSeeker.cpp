// ============================================================================
// BTTask_EvadeSeeker.cpp
// Developer: Marcus Daley
// Date: January 26, 2026
// Project: END2507 / WizardJam
// ============================================================================
// KEY FIXES APPLIED:
// 1. Added AddObjectFilter() and AddBoolFilter() in constructor
// 2. Added InitializeFromAsset() override with ResolveSelectedKey() calls
// Without these, blackboard key IsSet() returns false even when configured!
// ============================================================================

#include "Code/AI/BTTask_EvadeSeeker.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

DEFINE_LOG_CATEGORY(LogEvadeSeeker);

UBTTask_EvadeSeeker::UBTTask_EvadeSeeker()
	: SafeDistance(1500.0f)
	, EvasionSpeedMultiplier(1.5f)
	, MaxEvasionTime(5.0f)
	, bIncludeVerticalEvasion(true)
	, DirectionRandomization(15.0f)
	, CurrentEvasionTime(0.0f)
	, CachedFleeDirection(FVector::ZeroVector)
{
	NodeName = TEXT("Evade Seeker");

	// This task ticks every frame while active
	bNotifyTick = true;
	bNotifyTaskFinished = true;

	// CRITICAL FIX #1: Register blackboard key filters for editor dropdown
	ThreatActorKey.AddObjectFilter(
		this,
		GET_MEMBER_NAME_CHECKED(UBTTask_EvadeSeeker, ThreatActorKey),
		AActor::StaticClass());

	IsEvadingKey.AddBoolFilter(
		this,
		GET_MEMBER_NAME_CHECKED(UBTTask_EvadeSeeker, IsEvadingKey));
}

void UBTTask_EvadeSeeker::InitializeFromAsset(UBehaviorTree& Asset)
{
	// Always call parent first
	Super::InitializeFromAsset(Asset);

	// CRITICAL FIX #2: Resolve key selectors against the blackboard asset
	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		ThreatActorKey.ResolveSelectedKey(*BBAsset);
		IsEvadingKey.ResolveSelectedKey(*BBAsset);

		UE_LOG(LogEvadeSeeker, Log,
			TEXT("[EvadeSeeker] Resolved keys - ThreatActor: '%s', IsEvading: '%s'"),
			*ThreatActorKey.SelectedKeyName.ToString(),
			IsEvadingKey.IsSet() ? *IsEvadingKey.SelectedKeyName.ToString() : TEXT("(not set)"));
	}
}

EBTNodeResult::Type UBTTask_EvadeSeeker::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC)
	{
		UE_LOG(LogEvadeSeeker, Warning, TEXT("[EvadeSeeker] No AIController!"));
		return EBTNodeResult::Failed;
	}

	APawn* OwnerPawn = AIC->GetPawn();
	if (!OwnerPawn)
	{
		UE_LOG(LogEvadeSeeker, Warning, TEXT("[EvadeSeeker] No Pawn!"));
		return EBTNodeResult::Failed;
	}

	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	if (!BBComp)
	{
		UE_LOG(LogEvadeSeeker, Warning, TEXT("[EvadeSeeker] No BlackboardComponent!"));
		return EBTNodeResult::Failed;
	}

	// Validate threat key is configured
	if (!ThreatActorKey.IsSet())
	{
		UE_LOG(LogEvadeSeeker, Error,
			TEXT("[EvadeSeeker] ThreatActorKey not set! Full rebuild required after code changes."));
		return EBTNodeResult::Failed;
	}

	// Get the threat actor from blackboard
	AActor* ThreatActor = Cast<AActor>(BBComp->GetValueAsObject(ThreatActorKey.SelectedKeyName));
	if (!ThreatActor)
	{
		// No threat to evade - succeed immediately
		UE_LOG(LogEvadeSeeker, Display,
			TEXT("[EvadeSeeker] %s -> No threat in blackboard, succeeding"),
			*OwnerPawn->GetName());
		return EBTNodeResult::Succeeded;
	}

	// Check if already at safe distance
	const float CurrentDist = FVector::Dist(OwnerPawn->GetActorLocation(), ThreatActor->GetActorLocation());
	if (CurrentDist >= SafeDistance)
	{
		UE_LOG(LogEvadeSeeker, Display,
			TEXT("[EvadeSeeker] %s -> Already at safe distance (%.0f >= %.0f), succeeding"),
			*OwnerPawn->GetName(), CurrentDist, SafeDistance);
		return EBTNodeResult::Succeeded;
	}

	// Initialize evasion state
	CurrentEvasionTime = 0.0f;
	CachedFleeDirection = CalculateFleeDirection(
		OwnerPawn->GetActorLocation(),
		ThreatActor->GetActorLocation());

	// Set evading flag in blackboard if key is configured
	if (IsEvadingKey.IsSet())
	{
		BBComp->SetValueAsBool(IsEvadingKey.SelectedKeyName, true);
	}

	UE_LOG(LogEvadeSeeker, Display,
		TEXT("[EvadeSeeker] %s -> Started evading %s (distance: %.0f, safe: %.0f)"),
		*OwnerPawn->GetName(),
		*ThreatActor->GetName(),
		CurrentDist,
		SafeDistance);

	// Task runs continuously until safe distance reached or timeout
	return EBTNodeResult::InProgress;
}

void UBTTask_EvadeSeeker::TickTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	float DeltaSeconds)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	APawn* OwnerPawn = AIC->GetPawn();
	if (!OwnerPawn)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	if (!BBComp)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// Check for timeout
	CurrentEvasionTime += DeltaSeconds;
	if (MaxEvasionTime > 0.0f && CurrentEvasionTime >= MaxEvasionTime)
	{
		UE_LOG(LogEvadeSeeker, Display,
			TEXT("[EvadeSeeker] %s -> Evasion timeout (%.1fs), succeeding"),
			*OwnerPawn->GetName(), MaxEvasionTime);

		ClearEvadingFlag(BBComp);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// Get current threat location
	AActor* ThreatActor = Cast<AActor>(BBComp->GetValueAsObject(ThreatActorKey.SelectedKeyName));
	if (!ThreatActor)
	{
		// Threat lost - evasion complete
		UE_LOG(LogEvadeSeeker, Display,
			TEXT("[EvadeSeeker] %s -> Threat lost, evasion complete"),
			*OwnerPawn->GetName());

		ClearEvadingFlag(BBComp);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// Check if safe distance reached
	const float CurrentDist = FVector::Dist(OwnerPawn->GetActorLocation(), ThreatActor->GetActorLocation());
	if (CurrentDist >= SafeDistance)
	{
		UE_LOG(LogEvadeSeeker, Display,
			TEXT("[EvadeSeeker] %s -> Safe distance reached (%.0f >= %.0f)"),
			*OwnerPawn->GetName(), CurrentDist, SafeDistance);

		ClearEvadingFlag(BBComp);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// Recalculate flee direction periodically for dynamic evasion
	// Update every 0.5 seconds to balance responsiveness and consistency
	if (FMath::Fmod(CurrentEvasionTime, 0.5f) < DeltaSeconds)
	{
		CachedFleeDirection = CalculateFleeDirection(
			OwnerPawn->GetActorLocation(),
			ThreatActor->GetActorLocation());
	}

	// Apply evasion movement
	ApplyEvasionMovement(OwnerPawn, CachedFleeDirection, DeltaSeconds);
}

EBTNodeResult::Type UBTTask_EvadeSeeker::AbortTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	// Clear evading flag on abort
	if (UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent())
	{
		ClearEvadingFlag(BBComp);
	}

	UE_LOG(LogEvadeSeeker, Display, TEXT("[EvadeSeeker] Task aborted"));

	return EBTNodeResult::Aborted;
}

FVector UBTTask_EvadeSeeker::CalculateFleeDirection(
	const FVector& OwnerLocation,
	const FVector& ThreatLocation) const
{
	// Base flee direction is directly away from threat
	FVector FleeDir = (OwnerLocation - ThreatLocation).GetSafeNormal();

	// Handle edge case where owner and threat are at same location
	if (FleeDir.IsNearlyZero())
	{
		FleeDir = FVector(1.0f, 0.0f, 0.0f);
	}

	// Apply randomization to make movement less predictable
	if (DirectionRandomization > 0.0f)
	{
		const float RandomYaw = FMath::RandRange(-DirectionRandomization, DirectionRandomization);
		const FRotator Randomization(0.0f, RandomYaw, 0.0f);
		FleeDir = Randomization.RotateVector(FleeDir);
	}

	// Optionally flatten to horizontal only
	if (!bIncludeVerticalEvasion)
	{
		FleeDir.Z = 0.0f;
		FleeDir = FleeDir.GetSafeNormal();

		// Handle edge case where direction was purely vertical
		if (FleeDir.IsNearlyZero())
		{
			FleeDir = FVector(1.0f, 0.0f, 0.0f);
		}
	}

	return FleeDir;
}

void UBTTask_EvadeSeeker::ApplyEvasionMovement(
	APawn* Pawn,
	const FVector& FleeDirection,
	float DeltaSeconds) const
{
	if (!Pawn || FleeDirection.IsNearlyZero())
	{
		return;
	}

	// Direct velocity control for AI pawns - AddMovementInput requires PlayerController processing
	ACharacter* Character = Cast<ACharacter>(Pawn);
	if (Character)
	{
		UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
		if (MoveComp && MoveComp->MovementMode == MOVE_Flying)
		{
			float TargetSpeed = MoveComp->MaxFlySpeed * EvasionSpeedMultiplier;
			FVector DesiredVelocity = FleeDirection * TargetSpeed;

			// Preserve vertical velocity managed by BroomComponent
			DesiredVelocity.Z = MoveComp->Velocity.Z;

			// Direct velocity assignment for AI - instant direction change
			// VInterpTo causes circular orbits when direction changes rapidly at high speed
			MoveComp->Velocity = DesiredVelocity;
			return;
		}
	}

	// Fallback to AddMovementInput for non-flying/non-character pawns
	Pawn->AddMovementInput(FleeDirection, EvasionSpeedMultiplier);
}

void UBTTask_EvadeSeeker::ClearEvadingFlag(UBlackboardComponent* BBComp)
{
	if (BBComp && IsEvadingKey.IsSet())
	{
		BBComp->SetValueAsBool(IsEvadingKey.SelectedKeyName, false);
	}
}

FString UBTTask_EvadeSeeker::GetStaticDescription() const
{
	FString ThreatKey = ThreatActorKey.IsSet()
		? ThreatActorKey.SelectedKeyName.ToString()
		: TEXT("NOT SET!");

	FString Description = FString::Printf(
		TEXT("Evade: %s\nSafe Distance: %.0f\nSpeed: %.1fx"),
		*ThreatKey,
		SafeDistance,
		EvasionSpeedMultiplier);

	if (MaxEvasionTime > 0.0f)
	{
		Description += FString::Printf(TEXT("\nTimeout: %.1fs"), MaxEvasionTime);
	}

	if (bIncludeVerticalEvasion)
	{
		Description += TEXT("\n3D Evasion");
	}

	return Description;
}
