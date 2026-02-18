// ============================================================================
// BTTask_WaitForMatchStart.h
// Developer: Marcus Daley
// Date: January 23, 2026
// Project: END2507
// ============================================================================
// Purpose:
// BT Task that waits for match to start by checking Blackboard flag.
// Uses Observer Pattern - NO POLLING GameMode directly.
//
// Gas Station Pattern:
// This is equivalent to gunCondition.wait() in the gas station simulation.
// The agent waits at the staging zone until the match starts.
// The controller's HandleMatchStarted() delegate handler sets BB.bMatchStarted.
//
// Usage:
// 1. Add to BT after BTTask_FlyToStagingZone
// 2. Configure MatchStartedKey (Bool)
// 3. Task ticks and checks Blackboard (NOT GameMode)
// 4. When BB.bMatchStarted = true, task succeeds
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_WaitForMatchStart.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBTTask_WaitForMatchStart, Log, All);

UCLASS(meta = (DisplayName = "Wait For Match Start"))
class END2507_API UBTTask_WaitForMatchStart : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_WaitForMatchStart();

    // ========================================================================
    // BTTaskNode Interface
    // ========================================================================

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

    virtual FString GetStaticDescription() const override;

    // ========================================================================
    // MANDATORY: FBlackboardKeySelector Initialization
    // ========================================================================

    virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

protected:
    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    // Blackboard key to check for match started (Bool)
    // Set by controller's HandleMatchStarted() delegate handler
    UPROPERTY(EditDefaultsOnly, Category = "Blackboard")
    FBlackboardKeySelector MatchStartedKey;

    // Optional: Play hover animation while waiting
    UPROPERTY(EditDefaultsOnly, Category = "Animation")
    bool bPlayHoverAnimation;

    // Tick interval for checking (not polling GameMode - just checking BB)
    UPROPERTY(EditDefaultsOnly, Category = "Performance", meta = (ClampMin = "0.1", ClampMax = "1.0"))
    float CheckInterval;

private:
    // Time accumulator for check interval
    float TimeSinceLastCheck;
};
