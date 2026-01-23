// ============================================================================
// BTTask_MountBroom.h
// Behavior Tree task for AI to mount and dismount brooms
//
// Developer: Marcus Daley
// Date: January 19, 2026
// Project: Game Architecture (END2507)
//
// PURPOSE:
// Allows AI agents to mount and dismount brooms using the SAME
// AC_BroomComponent that players use. This demonstrates the power
// of the Core API design - AI calls SetFlightEnabled() directly,
// achieving identical results to player pressing the toggle key.
//
// WHY THIS WORKS:
// The AC_BroomComponent has a Core API that is input-agnostic:
//   - SetFlightEnabled(bool) - mount/dismount
//   - SetVerticalInput(float) - ascend/descend
//   - SetBoostEnabled(bool) - speed boost
//
// Players call these through input handlers. AI calls them directly.
// The flight mechanics are IDENTICAL regardless of who called the function.
//
// USAGE IN BEHAVIOR TREE:
// 1. Precondition: AI has heard flight signal or acquired broom
// 2. This task: Calls SetFlightEnabled(true) on the AI's BroomComponent
// 3. Result: AI is now flying, using the same component as the player
//
// BLUEPRINT SETUP:
// 1. Add to Behavior Tree task list
// 2. Set bMountBroom = true to mount, false to dismount
// 3. AI pawn must have AC_BroomComponent attached
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_MountBroom.generated.h"

class UBehaviorTree;

UCLASS(meta = (DisplayName = "Mount/Dismount Broom"))
class END2507_API UBTTask_MountBroom : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_MountBroom();

    // Execute the task - mounts or dismounts the broom
    virtual EBTNodeResult::Type ExecuteTask(
        UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory) override;

    // Description shown in Behavior Tree editor
    virtual FString GetStaticDescription() const override;

    // CRITICAL: Resolves blackboard key at runtime
    // Without this, FlightStateKey.IsSet() returns false even when configured!
    virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

protected:
    // If true, mount the broom. If false, dismount.
    UPROPERTY(EditAnywhere, Category = "Broom")
    bool bMountBroom;

    // Optional: Blackboard key to store flight state result
    // Leave empty to not update blackboard
    UPROPERTY(EditAnywhere, Category = "Broom")
    FBlackboardKeySelector FlightStateKey;
};
