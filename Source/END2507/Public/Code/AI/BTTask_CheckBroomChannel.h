// ============================================================================
// BTTask_CheckBroomChannel.h
// Checks if pawn has "Broom" channel and sets HasBroom blackboard key
//
// Developer: Marcus Daley
// Date: January 22, 2026
// Project: END2507
//
// PURPOSE:
// Bridge between SpellCollectionComponent's channel system and the Blackboard.
// After agent collects BroomCollectible, it gains "Broom" channel but the
// HasBroom blackboard key isn't automatically set. This task does that sync.
//
// USAGE:
// 1. Add this task after Move To (collectible pickup) in AcquisitionPath
// 2. Set HasBroomKey to the "HasBroom" blackboard key (Bool type)
// 3. Task succeeds if channel found and key set, fails otherwise
//
// WHY THIS EXISTS:
// The BT decorator "Has Channel: Broom" checks if pawn has the channel,
// but it can't SET a blackboard key. Other parts of the BT need HasBroom
// as a boolean key for their decorators. This task bridges that gap.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_CheckBroomChannel.generated.h"

UCLASS(meta = (DisplayName = "Check Broom Channel"))
class END2507_API UBTTask_CheckBroomChannel : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_CheckBroomChannel();

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual FString GetStaticDescription() const override;
    virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

protected:
    // The channel name to check for (default: "Broom")
    UPROPERTY(EditAnywhere, Category = "Channel")
    FName ChannelToCheck;

    // Blackboard key to set when channel is found (Bool type)
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector HasBroomKey;
};
