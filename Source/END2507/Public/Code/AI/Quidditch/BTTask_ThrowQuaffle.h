// ============================================================================
// BTTask_ThrowQuaffle.h
// Developer: Marcus Daley
// Date: January 24, 2026
// Project: END2507
// ============================================================================
// Purpose:
// BT Task that throws the Quaffle toward a goal or teammate.
// Uses prediction for moving targets.
//
// Usage:
// 1. Attach to Chaser sequence after picking up Quaffle
// 2. Configure ThrowTarget (Goal actor or teammate)
// 3. Task calculates throw velocity and releases Quaffle
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_ThrowQuaffle.generated.h"

UENUM(BlueprintType)
enum class EQuaffleThrowType : uint8
{
    ToGoal,
    ToTeammate,
    ToLocation
};

UCLASS(meta = (DisplayName = "Throw Quaffle"))
class END2507_API UBTTask_ThrowQuaffle : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_ThrowQuaffle();

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

    // Quaffle actor currently held (Object)
    UPROPERTY(EditDefaultsOnly, Category = "Blackboard")
    FBlackboardKeySelector HeldQuaffleKey;

    // Target to throw at - goal or teammate (Object)
    UPROPERTY(EditDefaultsOnly, Category = "Blackboard")
    FBlackboardKeySelector ThrowTargetKey;

    // Alternative: throw to specific location (Vector)
    UPROPERTY(EditDefaultsOnly, Category = "Blackboard")
    FBlackboardKeySelector ThrowLocationKey;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    // Type of throw to execute
    UPROPERTY(EditDefaultsOnly, Category = "Throw")
    EQuaffleThrowType ThrowType;

    // Speed of thrown Quaffle
    UPROPERTY(EditDefaultsOnly, Category = "Throw")
    float ThrowSpeed;

    // Lead factor for moving target prediction (1.0 = perfect lead)
    UPROPERTY(EditDefaultsOnly, Category = "Throw")
    float LeadFactor;

    // Arc height for lob passes (0 = straight throw)
    UPROPERTY(EditDefaultsOnly, Category = "Throw")
    float ArcHeight;

    // Minimum range to attempt throw
    UPROPERTY(EditDefaultsOnly, Category = "Throw")
    float MinThrowRange;

    // Maximum range for accurate throw
    UPROPERTY(EditDefaultsOnly, Category = "Throw")
    float MaxThrowRange;

private:
    // Calculate throw direction with optional prediction
    FVector CalculateThrowDirection(const FVector& FromPos, const FVector& TargetPos,
        const FVector& TargetVelocity) const;

    // Execute the throw by detaching and launching Quaffle
    bool ExecuteThrow(AActor* Quaffle, APawn* Thrower, const FVector& ThrowVelocity);
};

