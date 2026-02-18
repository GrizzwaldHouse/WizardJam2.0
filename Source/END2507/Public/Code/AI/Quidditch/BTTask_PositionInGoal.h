// ============================================================================
// BTTask_PositionInGoal.h
// Developer: Marcus Daley
// Date: January 24, 2026
// Project: END2507
// ============================================================================
// Algorithm Source: Flock.cs Cohesion (ADAPTED)
//
// Original Cohesion: Move toward center of mass of neighbors
// Keeper Adaptation: Move toward optimal position between goal center and threat
//
// Position = Lerp(GoalCenter, ThreatDirection, ThreatFactor)
// Where ThreatFactor = 1 / Distance(Quaffle, Goal)
//
// This keeps Keeper near goal but shifts toward incoming threats.
//
// Usage:
// 1. Attach to Keeper behavior branch
// 2. Configure goal hoops and threat tracking
// 3. Outputs optimal position for Keeper to defend
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_PositionInGoal.generated.h"

UCLASS(meta = (DisplayName = "Position In Goal"))
class END2507_API UBTTask_PositionInGoal : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_PositionInGoal();

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

    // Goal center position to defend (Vector) - typically assigned at setup
    UPROPERTY(EditDefaultsOnly, Category = "Blackboard")
    FBlackboardKeySelector GoalCenterKey;

    // Current threat - Quaffle or enemy with Quaffle (Object)
    UPROPERTY(EditDefaultsOnly, Category = "Blackboard")
    FBlackboardKeySelector ThreatActorKey;

    // ========================================================================
    // OUTPUT BLACKBOARD KEYS
    // ========================================================================

    // Calculated optimal defense position (Vector)
    UPROPERTY(EditDefaultsOnly, Category = "Blackboard")
    FBlackboardKeySelector DefensePositionKey;

    // ========================================================================
    // CONFIGURATION (Cohesion-inspired)
    // ========================================================================

    // How far Keeper can move from goal center
    UPROPERTY(EditDefaultsOnly, Category = "Positioning")
    float MaxDefenseRadius;

    // Minimum distance to stay from goal center (don't enter goal)
    UPROPERTY(EditDefaultsOnly, Category = "Positioning")
    float MinDefenseRadius;

    // Height offset from goal center
    UPROPERTY(EditDefaultsOnly, Category = "Positioning")
    float DefenseHeight;

    // How aggressively to move toward threat (0-1)
    UPROPERTY(EditDefaultsOnly, Category = "Positioning", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ThreatResponseFactor;

    // Distance at which threat response is maximum
    UPROPERTY(EditDefaultsOnly, Category = "Positioning")
    float MaxThreatDistance;

private:
    // Calculate optimal position using Cohesion-like algorithm
    FVector CalculateDefensePosition(const FVector& GoalCenter, AActor* Threat) const;
};

