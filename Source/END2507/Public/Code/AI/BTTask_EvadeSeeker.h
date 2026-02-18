// ============================================================================
// BTTask_EvadeSeeker.h
// Developer: Marcus Daley
// Date: January 26, 2026
// Project: END2507 / WizardJam
// ============================================================================
// Purpose:
// BT Task that executes evasive movement away from a threat actor.
// Reads threat location from blackboard, calculates flee vector, and applies
// movement input until safe distance is reached or timeout occurs.
// Returns InProgress while evading, Succeeded when safe distance reached.
// Works with any pawn that responds to AddMovementInput.
//
// USAGE:
// 1. Pair with BTService_TrackNearestSeeker to populate threat key
// 2. Set ThreatActorKey to the blackboard key containing the threat actor
// 3. Optionally set IsEvadingKey to track evasion state
// 4. Configure SafeDistance, EvasionSpeedMultiplier, and other parameters
//
// CRITICAL INITIALIZATION:
// - FBlackboardKeySelector requires AddObjectFilter()/AddBoolFilter() in constructor
// - FBlackboardKeySelector requires InitializeFromAsset() override
// - Without BOTH, IsSet() returns false even when configured in editor!
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_EvadeSeeker.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogEvadeSeeker, Log, All);

UCLASS(meta = (DisplayName = "Evade Seeker"))
class END2507_API UBTTask_EvadeSeeker : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_EvadeSeeker();

	// Called when task starts execution
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;

	// Called each tick while task is InProgress
	virtual void TickTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory,
		float DeltaSeconds) override;

	// Called when task is aborted
	virtual EBTNodeResult::Type AbortTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;

	// Description shown in Behavior Tree editor
	virtual FString GetStaticDescription() const override;

	// CRITICAL: Resolves blackboard key selectors at runtime
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

protected:
	// ========================================================================
	// BLACKBOARD CONFIGURATION
	// ========================================================================

	// Blackboard key containing the threat actor to evade (Object type)
	UPROPERTY(EditDefaultsOnly, Category = "Blackboard")
	FBlackboardKeySelector ThreatActorKey;

	// Blackboard key to write evading state (Bool type, optional)
	UPROPERTY(EditDefaultsOnly, Category = "Blackboard")
	FBlackboardKeySelector IsEvadingKey;

	// ========================================================================
	// EVASION PARAMETERS
	// ========================================================================

	// Distance to maintain from threat before task succeeds
	UPROPERTY(EditDefaultsOnly, Category = "Evasion", meta = (ClampMin = "100.0"))
	float SafeDistance;

	// Speed multiplier during evasion (1.0 = normal speed)
	UPROPERTY(EditDefaultsOnly, Category = "Evasion", meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float EvasionSpeedMultiplier;

	// Maximum time to spend evading before giving up (0 = no limit)
	UPROPERTY(EditDefaultsOnly, Category = "Evasion", meta = (ClampMin = "0.0"))
	float MaxEvasionTime;

	// Include vertical component in evasion (fly up/down as well as away)
	UPROPERTY(EditDefaultsOnly, Category = "Evasion")
	bool bIncludeVerticalEvasion;

	// Randomization angle applied to flee direction (degrees)
	// Makes movement less predictable
	UPROPERTY(EditDefaultsOnly, Category = "Evasion", meta = (ClampMin = "0.0", ClampMax = "45.0"))
	float DirectionRandomization;

private:
	// Calculate flee direction away from threat with optional randomization
	FVector CalculateFleeDirection(
		const FVector& OwnerLocation,
		const FVector& ThreatLocation) const;

	// Apply movement input to the pawn
	void ApplyEvasionMovement(
		APawn* Pawn,
		const FVector& FleeDirection,
		float DeltaSeconds) const;

	// Clear the evading flag in blackboard
	void ClearEvadingFlag(UBlackboardComponent* BBComp);

	// Tracks elapsed evasion time for timeout
	float CurrentEvasionTime;

	// Cached flee direction to maintain consistent movement
	FVector CachedFleeDirection;
};
