// ============================================================================
// BTTask_HitBludger.h
// Developer: Marcus Daley
// Date: January 24, 2026
// Project: END2507
// ============================================================================
// Purpose:
// BT Task that hits a Bludger toward an enemy target.
// Beaters use bats to redirect Bludgers at opponents.
//
// Usage:
// 1. Attach after Beater intercepts a Bludger
// 2. Configure target enemy from BTService_FindBludger output
// 3. Task calculates hit direction with prediction
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_HitBludger.generated.h"

UCLASS(meta = (DisplayName = "Hit Bludger"))
class END2507_API UBTTask_HitBludger : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_HitBludger();

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

    // Bludger to hit (Object)
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector BludgerKey;

    // Target enemy to hit Bludger at (Object)
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector TargetEnemyKey;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    // Maximum distance to attempt hitting Bludger
    UPROPERTY(EditAnywhere, Category = "Hit")
    float MaxHitRange;

    // Speed imparted to Bludger on hit
    UPROPERTY(EditAnywhere, Category = "Hit")
    float HitForce;

    // Lead factor for target prediction (1.0 = perfect lead)
    UPROPERTY(EditAnywhere, Category = "Hit")
    float LeadFactor;

    // Cooldown before Beater can hit again
    UPROPERTY(EditAnywhere, Category = "Hit")
    float HitCooldown;

private:
    // Track last hit time for cooldown
    float LastHitTime;

    // Calculate hit direction toward predicted enemy position
    FVector CalculateHitDirection(APawn* Beater, AActor* Bludger, APawn* Enemy) const;

    // Apply force to redirect Bludger
    bool ApplyHitForce(AActor* Bludger, const FVector& HitDirection);
};

