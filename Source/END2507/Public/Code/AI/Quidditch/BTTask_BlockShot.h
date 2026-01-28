// ============================================================================
// BTTask_BlockShot.h
// Developer: Marcus Daley
// Date: January 24, 2026
// Project: END2507
// ============================================================================
// Purpose:
// BT Task for Keeper to intercept incoming Quaffle shots.
// Uses PathSearch intercept algorithm adapted from BTTask_PredictIntercept.
//
// The Keeper calculates intercept point for incoming Quaffle and moves to block.
//
// Usage:
// 1. Attach after BTTask_PositionInGoal detects incoming shot
// 2. Uses Quaffle velocity to predict intercept point
// 3. Keeper moves to intercept position
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_BlockShot.generated.h"

UCLASS(meta = (DisplayName = "Block Shot"))
class END2507_API UBTTask_BlockShot : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_BlockShot();

    // ========================================================================
    // BTTaskNode Interface
    // ========================================================================

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

    virtual FString GetStaticDescription() const override;

    // ========================================================================
    // MANDATORY: FBlackboardKeySelector Initialization
    // ========================================================================

    virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

protected:
    // ========================================================================
    // INPUT BLACKBOARD KEYS
    // ========================================================================

    // Incoming Quaffle to block (Object)
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector QuaffleKey;

    // Quaffle velocity for prediction (Vector)
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector QuaffleVelocityKey;

    // Goal center we're defending (Vector)
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector GoalCenterKey;

    // ========================================================================
    // OUTPUT BLACKBOARD KEYS
    // ========================================================================

    // Calculated intercept position (Vector)
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector InterceptPositionKey;

    // True if shot is blockable (Bool)
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector CanBlockKey;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    // Maximum speed Keeper can move to intercept
    UPROPERTY(EditAnywhere, Category = "Block")
    float KeeperMaxSpeed;

    // Minimum Quaffle speed to consider it a "shot"
    UPROPERTY(EditAnywhere, Category = "Block")
    float MinShotSpeed;

    // Maximum time to predict ahead
    UPROPERTY(EditAnywhere, Category = "Block")
    float MaxPredictionTime;

    // Distance considered a successful block
    UPROPERTY(EditAnywhere, Category = "Block")
    float BlockRadius;

private:
    // Calculate if shot can be intercepted and where
    bool CalculateIntercept(const FVector& KeeperPos, const FVector& QuafflePos,
        const FVector& QuaffleVel, FVector& OutInterceptPoint, float& OutTimeToIntercept) const;

    // Check if Quaffle is heading toward goal
    bool IsHeadingTowardGoal(const FVector& QuafflePos, const FVector& QuaffleVel,
        const FVector& GoalCenter) const;
};

