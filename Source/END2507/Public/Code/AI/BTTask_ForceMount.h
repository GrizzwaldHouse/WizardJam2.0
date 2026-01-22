// ============================================================================
// BTTask_ForceMount.h
// Testing utility - Force AI to mount broom immediately
//
// Developer: Marcus Daley
// Date: January 22, 2026
// Project: Game Architecture (END2507)
//
// PURPOSE:
// Debug/test task that bypasses normal broom acquisition flow.
// Spawns a broom at AI location and mounts immediately.
// USE FOR TESTING ONLY - Remove from production behavior trees.
//
// USAGE:
// 1. Add to BT as first task to immediately test flight
// 2. AI will spawn broom and mount without walking to pickup
// 3. Useful for testing BTTask_ControlFlight in isolation
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ForceMount.generated.h"

class ABroomActor;

UCLASS(meta = (DisplayName = "Force Mount (DEBUG)"))
class END2507_API UBTTask_ForceMount : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_ForceMount();

    virtual EBTNodeResult::Type ExecuteTask(
        UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory) override;

    virtual FString GetStaticDescription() const override;

protected:
    // Blueprint class for broom to spawn (set in BT defaults)
    UPROPERTY(EditAnywhere, Category = "Debug")
    TSubclassOf<ABroomActor> BroomClass;

    // Offset from AI location to spawn broom
    UPROPERTY(EditAnywhere, Category = "Debug")
    FVector SpawnOffset;
};
