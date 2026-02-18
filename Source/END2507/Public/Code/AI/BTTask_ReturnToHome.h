// ============================================================================
// BTTask_ReturnToHome.h
// Developer: Marcus Daley
// Date: January 26, 2026
// Project: END2507
// ============================================================================
// Purpose:
// BT Task that returns agent to their original staging zone when they have
// no active target. Prevents agents from wandering or hovering aimlessly.
//
// Fallback Behavior Pattern:
// Used in BT as lowest-priority node when all other behaviors fail.
// Ensures agents always have a "home base" to return to.
//
// Usage:
// 1. Add to BT as final fallback node (lowest priority in Selector)
// 2. Configure HomeLocationKey (Vector blackboard key)
// 3. Task queries GameMode for agent's staging zone
// 4. Flies agent back to staging zone
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_ReturnToHome.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBTTask_ReturnToHome, Log, All);

UCLASS(meta = (DisplayName = "Return To Home (Fallback)"))
class END2507_API UBTTask_ReturnToHome : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ReturnToHome();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	virtual FString GetStaticDescription() const override;

	// MANDATORY: FBlackboardKeySelector initialization
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

protected:
	// Blackboard key to write home location (Vector)
	UPROPERTY(EditDefaultsOnly, Category = "Blackboard")
	FBlackboardKeySelector HomeLocationKey;

	// Arrival radius (default: 200 units)
	UPROPERTY(EditDefaultsOnly, Category = "Navigation", meta = (ClampMin = "50.0", ClampMax = "500.0"))
	float ArrivalRadius;

	// Flight speed multiplier (default: 1.0 = normal speed)
	UPROPERTY(EditDefaultsOnly, Category = "Navigation", meta = (ClampMin = "0.5", ClampMax = "2.0"))
	float FlightSpeedMultiplier;

	// Enable boost when far from home
	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	bool bUseBoostWhenFar;

	// Distance to enable boost (default: 500 units)
	UPROPERTY(EditDefaultsOnly, Category = "Navigation", meta = (ClampMin = "100.0", EditCondition = "bUseBoostWhenFar"))
	float BoostDistanceThreshold;

	// Hysteresis threshold - boost turns OFF when distance drops below this value
	// Set lower than BoostDistanceThreshold to prevent oscillation
	UPROPERTY(EditDefaultsOnly, Category = "Navigation", meta = (ClampMin = "50.0", EditCondition = "bUseBoostWhenFar"))
	float BoostDisableThreshold;

	// Minimum stamina percent (0-1) required to enable boost - prevents depleting all stamina
	UPROPERTY(EditDefaultsOnly, Category = "Navigation", meta = (ClampMin = "0.1", ClampMax = "0.9", EditCondition = "bUseBoostWhenFar"))
	float MinStaminaForBoost;

	// Altitude adjustment scale (lower = steeper climbs)
	UPROPERTY(EditDefaultsOnly, Category = "Navigation", meta = (ClampMin = "50.0", ClampMax = "500.0"))
	float AltitudeScale;

	// Slot name for home staging zone lookup - must match StagingSlotName on the zone actor
	// Examples: "Seeker", "Chaser_Left", "Chaser_Center", "Chaser_Right", "Beater_A", "Beater_B", "Keeper"
	// If empty/None, defaults to the role name (e.g., "Seeker" for a Seeker agent)
	UPROPERTY(EditDefaultsOnly, Category = "Staging")
	FName SlotName;

	// ========================================================================
	// TIMEOUT CONFIGURATION
	// ========================================================================

	// Enable timeout - if agent can't reach home in time, task fails
	UPROPERTY(EditDefaultsOnly, Category = "Timeout")
	bool bEnableTimeout;

	// Maximum time (seconds) to reach home before failing
	// Only used if bEnableTimeout is true
	UPROPERTY(EditDefaultsOnly, Category = "Timeout", meta = (ClampMin = "5.0", ClampMax = "120.0", EditCondition = "bEnableTimeout"))
	float TimeoutDuration;

private:
	// Cached home location (set once in ExecuteTask)
	FVector CachedHomeLocation;

	// Track if location was set
	bool bLocationSet;

	// Track elapsed time for timeout
	float ElapsedFlightTime;

	// Track if agent is flying (determines navigation mode)
	// If agent dismounts mid-task, switches to ground navigation
	bool bIsFlying;

	// Track current boost state for hysteresis
	bool bCurrentlyBoosting;
};
