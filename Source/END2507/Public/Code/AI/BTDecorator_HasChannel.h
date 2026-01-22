// ============================================================================
// BTDecorator_HasChannel.h
// Behavior Tree decorator that checks if AI pawn has a specific channel
//
// Developer: Marcus Daley
// Date: January 20, 2026
// Project: END2507
//
// PURPOSE:
// Gates behavior tree branches based on whether the AI pawn has a specific
// channel unlocked in its AC_SpellCollectionComponent. Used to check for
// broom access, spell schools, progression gates, etc.
//
// USAGE:
// 1. Add as decorator to a BT node
// 2. Set ChannelName to the channel to check (e.g., "Broom", "Fire", "Teleport")
// 3. Set bInverseCondition = true to pass when channel is MISSING
//
// EXAMPLE:
// - Check if AI has broom: ChannelName = "Broom", bInverseCondition = false
// - Check if AI needs broom: ChannelName = "Broom", bInverseCondition = true
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_HasChannel.generated.h"

class UAC_SpellCollectionComponent;

UCLASS(meta = (DisplayName = "Has Channel"))
class END2507_API UBTDecorator_HasChannel : public UBTDecorator
{
    GENERATED_BODY()

public:
    UBTDecorator_HasChannel();

    // Check if the condition is met
    virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

    // Description shown in Behavior Tree editor
    virtual FString GetStaticDescription() const override;

protected:
    // The channel name to check for (e.g., "Broom", "Fire", "Teleport")
    UPROPERTY(EditAnywhere, Category = "Channel")
    FName ChannelName;

    // If true, decorator passes when channel is MISSING
    // If false, decorator passes when channel is PRESENT
    UPROPERTY(EditAnywhere, Category = "Channel")
    bool bInvertResult;
};
