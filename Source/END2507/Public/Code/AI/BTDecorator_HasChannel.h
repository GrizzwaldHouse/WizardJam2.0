// ============================================================================
// BTDecorator_HasChannel.h
// Behavior Tree decorator that checks channel state via Blackboard
//
// Developer: Marcus Daley
// Date: January 20, 2026 (Refactored January 23, 2026)
// Project: END2507
//
// PURPOSE:
// Gates behavior tree branches based on whether a channel Blackboard key is true.
// The AI Controller updates these keys when channels are added/removed via
// AC_SpellCollectionComponent delegates (observer pattern).
//
// ARCHITECTURE (January 23, 2026 Refactor):
// OLD (Anti-Pattern): Decorator -> FindComponent -> SpellComp->HasChannel()
// NEW (Observer Pattern): Decorator -> Blackboard->GetValueAsBool(ChannelKey)
//
// The Controller binds to AC_SpellCollectionComponent::OnChannelAdded and updates
// Blackboard keys like HasBroomChannel. This decorator reads those keys.
//
// USAGE:
// 1. Add as decorator to a BT node
// 2. Set ChannelKey to the Blackboard key (e.g., "HasBroomChannel")
// 3. Set bInvertResult = true to pass when channel is MISSING
// 4. Configure Flow Abort Mode for reactive behavior (Self or Both)
//
// BLACKBOARD KEYS (managed by AIC_QuidditchController):
// - HasBroomChannel (Bool): Set true when "Broom" channel acquired
// - HasFireChannel (Bool): Set true when "Fire" channel acquired
// - HasTeleportChannel (Bool): Set true when "Teleport" channel acquired
//
// OBSERVER ABORTS:
// When ChannelKey changes, decorator can abort current branch via:
// - FlowAbortMode = Self: Abort this branch only
// - FlowAbortMode = Both: Abort this branch and lower priority branches
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTDecorator_HasChannel.generated.h"

class UBehaviorTree;

UCLASS(meta = (DisplayName = "Has Channel (Blackboard)"))
class END2507_API UBTDecorator_HasChannel : public UBTDecorator
{
    GENERATED_BODY()

public:
    UBTDecorator_HasChannel();

    // Check if the condition is met by reading Blackboard
    virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

    // Description shown in Behavior Tree editor
    virtual FString GetStaticDescription() const override;

    // CRITICAL: Resolves blackboard key at runtime
    virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

protected:
    // Blackboard key for channel state (e.g., "HasBroomChannel")
    // This key is updated by the AI Controller when channels are added/removed
    UPROPERTY(EditAnywhere, Category = "Channel")
    FBlackboardKeySelector ChannelKey;

    // If true, decorator passes when channel is MISSING (key is false)
    // If false, decorator passes when channel is PRESENT (key is true)
    UPROPERTY(EditAnywhere, Category = "Channel")
    bool bInvertResult;
};
