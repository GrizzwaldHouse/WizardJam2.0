// ============================================================================
// BTTask_PredictIntercept.h
// Developer: Marcus Daley
// Date: January 24, 2026
// Project: END2507
// ============================================================================
// Purpose:
// BT Task that calculates the optimal intercept point for a moving target.
// Used by Seeker to predict where the Snitch will be and fly to intercept.
//
// Algorithm Source: PathSearch.cpp intercept prediction
// This implements the classic pursuit/intercept algorithm:
// 1. Get target position and velocity
// 2. Get self position and max speed
// 3. Calculate time to intercept
// 4. Calculate intercept point = TargetPos + TargetVel * TimeToIntercept
//
// The algorithm solves: |InterceptPoint - SelfPos| / SelfSpeed = Time
// Where: InterceptPoint = TargetPos + TargetVel * Time
//
// Usage:
// 1. Requires target actor and velocity from Blackboard
// 2. Writes calculated intercept point to Blackboard
// 3. Succeeds when intercept point is calculated
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_PredictIntercept.generated.h"

UCLASS()
class END2507_API UBTTask_PredictIntercept : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_PredictIntercept();

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
    virtual FString GetStaticDescription() const override;

protected:
    // ========================================================================
    // INPUT KEYS
    // ========================================================================

    // Target actor to intercept (e.g., Snitch)
    UPROPERTY(EditDefaultsOnly, Category = "Blackboard|Input")
    FBlackboardKeySelector TargetActorKey;

    // Target's current velocity (optional - will calculate if not provided)
    UPROPERTY(EditDefaultsOnly, Category = "Blackboard|Input")
    FBlackboardKeySelector TargetVelocityKey;

    // ========================================================================
    // OUTPUT KEYS
    // ========================================================================

    // Calculated intercept point
    UPROPERTY(EditDefaultsOnly, Category = "Blackboard|Output")
    FBlackboardKeySelector InterceptPointKey;

    // Estimated time to intercept
    UPROPERTY(EditDefaultsOnly, Category = "Blackboard|Output")
    FBlackboardKeySelector TimeToInterceptKey;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    // Pursuer's maximum speed (units per second)
    UPROPERTY(EditDefaultsOnly, Category = "Intercept", meta = (ClampMin = "100.0"))
    float PursuerMaxSpeed;

    // Maximum prediction time (seconds) - prevents chasing forever
    UPROPERTY(EditDefaultsOnly, Category = "Intercept", meta = (ClampMin = "0.5", ClampMax = "10.0"))
    float MaxPredictionTime;

    // Minimum distance to switch from intercept to direct pursuit
    UPROPERTY(EditDefaultsOnly, Category = "Intercept")
    float DirectPursuitDistance;

    // How far ahead to lead the target (multiplier)
    UPROPERTY(EditDefaultsOnly, Category = "Intercept", meta = (ClampMin = "0.5", ClampMax = "2.0"))
    float LeadFactor;

private:
    // PathSearch.cpp algorithm implementation
    FVector CalculateInterceptPoint(
        const FVector& PursuerPos,
        float PursuerSpeed,
        const FVector& TargetPos,
        const FVector& TargetVel,
        float& OutTimeToIntercept) const;

    // Quadratic solver for intercept time
    bool SolveInterceptTime(
        const FVector& RelativePos,
        const FVector& RelativeVel,
        float PursuerSpeed,
        float& OutTime) const;
};
