// ============================================================================
// BTTask_Interact.h
// Modular Behavior Tree task for AI interaction with IInteractable targets
//
// Developer: Marcus Daley
// Date: January 22, 2026
// Project: Game Architecture (END2507)
//
// PURPOSE:
// Generic interaction task that works with any actor implementing IInteractable.
// Use this for mounting brooms, opening doors, activating switches, etc.
//
// USAGE:
// 1. Set TargetKey to Blackboard key containing target actor
// 2. Set InteractionRange for how close AI must be
// 3. Optionally set RequiredChannel if interaction requires specific channel
// 4. Task moves to target, then calls Interact()
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_Interact.generated.h"

class UBehaviorTree;

UCLASS(meta = (DisplayName = "Interact With Target"))
class END2507_API UBTTask_Interact : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_Interact();

    virtual EBTNodeResult::Type ExecuteTask(
        UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory) override;

    virtual void TickTask(
        UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory,
        float DeltaSeconds) override;

    virtual FString GetStaticDescription() const override;

    // CRITICAL: Resolves blackboard keys at runtime
    virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

protected:
    // Blackboard key containing target actor to interact with
    UPROPERTY(EditAnywhere, Category = "Interaction")
    FBlackboardKeySelector TargetKey;

    // Maximum distance to initiate interaction
    UPROPERTY(EditAnywhere, Category = "Interaction",
        meta = (ClampMin = "50.0", ClampMax = "500.0"))
    float InteractionRange;

    // LEGACY: If set, AI must have this channel to interact (e.g., "Broom")
    // Uses direct component query - prefer RequiredChannelKey for observer pattern
    UPROPERTY(EditAnywhere, Category = "Interaction|Legacy")
    FName RequiredChannel;

    // PREFERRED: Blackboard key for channel requirement (Bool)
    // If set, task reads this key (maintained by Controller via OnChannelAdded delegate)
    // If not set, falls back to RequiredChannel FName with direct component query
    UPROPERTY(EditAnywhere, Category = "Interaction")
    FBlackboardKeySelector RequiredChannelKey;

    // Clear target from blackboard after successful interaction
    UPROPERTY(EditAnywhere, Category = "Interaction")
    bool bClearTargetOnSuccess;

    // Blackboard key to set true on successful interaction (optional)
    UPROPERTY(EditAnywhere, Category = "Interaction")
    FBlackboardKeySelector SuccessStateKey;

private:
    bool TryInteract(UBehaviorTreeComponent& OwnerComp, AActor* Target);

    // Checks if required channel is met
    // If RequiredChannelKey is set, reads from Blackboard (observer pattern)
    // Otherwise falls back to direct component query with RequiredChannel FName
    bool HasRequiredChannel(APawn* Pawn, UBlackboardComponent* Blackboard) const;
};
