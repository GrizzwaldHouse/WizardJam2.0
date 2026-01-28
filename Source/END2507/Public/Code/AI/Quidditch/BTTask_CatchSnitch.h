// ============================================================================
// BTTask_CatchSnitch.h
// Developer: Marcus Daley
// Date: January 24, 2026
// Project: END2507
// ============================================================================
// Purpose:
// BT Task executed when Seeker is within catch range of the Snitch.
// Triggers match end via GameMode delegate, awards 150 points.
//
// Usage:
// 1. Place after BTTask_PredictIntercept in Seeker sequence
// 2. Guard with BTDecorator_IsInRange checking CatchRadius
// 3. On success, broadcasts OnSnitchCaught via GameMode
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_CatchSnitch.generated.h"

UCLASS(meta = (DisplayName = "Catch Snitch"))
class END2507_API UBTTask_CatchSnitch : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_CatchSnitch();

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
    // BLACKBOARD KEYS
    // ========================================================================

    // Target Snitch actor to catch (Object)
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector SnitchActorKey;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    // Distance within which catch attempt succeeds
    UPROPERTY(EditAnywhere, Category = "Catch")
    float CatchRadius;

    // Points awarded for catching Snitch (standard: 150)
    UPROPERTY(EditAnywhere, Category = "Catch")
    int32 SnitchPointValue;

    // Should destroy Snitch actor on catch
    UPROPERTY(EditAnywhere, Category = "Catch")
    bool bDestroySnitchOnCatch;

private:
    // Attempt to catch the Snitch and notify GameMode
    bool TryCatchSnitch(AActor* Snitch, APawn* Seeker);
};

