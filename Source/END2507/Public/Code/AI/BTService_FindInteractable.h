// ============================================================================
// BTService_FindInteractable.h
// Behavior Tree service that scans AI Perception for interactable actors
//
// Developer: Marcus Daley
// Date: January 22, 2026
// Project: END2507
//
// PURPOSE:
// Periodically scans the AI's perception for actors that implement IInteractable
// interface (brooms, doors, levers, NPCs) and writes the nearest valid one to a
// blackboard key. Used to locate interactable objects like BP_Broom_C that are
// NOT collectibles but still need to be found by AI.
//
// KEY DIFFERENCE FROM BTService_FindCollectible:
// - FindCollectible filters by ACollectiblePickup class hierarchy
// - FindInteractable filters by IInteractable interface implementation
// - BroomActor inherits from AActor (not ACollectiblePickup) but implements IInteractable
// - This service is REQUIRED for AI to find mounted broom actors
//
// USAGE:
// 1. Add as service to a BT composite node
// 2. Set InteractableClass to filter by actor type (e.g., ABroomActor)
// 3. Set OutputKey to blackboard key for storing found actor (MUST be Object type)
// 4. Optionally set MaxSearchDistance to limit range (0 = unlimited)
// 5. Optionally enable bRequireCanInteract to only find usable interactables
//
// REQUIREMENTS:
// - AI Controller must have UAIPerceptionComponent
// - Interactables must be sensed (Sight, Hearing, etc.)
// - Interactables must implement IInteractable interface
// - Blackboard must have an Object key for OutputKey
//
// CRITICAL INITIALIZATION:
// - FBlackboardKeySelector requires AddObjectFilter() in constructor
// - FBlackboardKeySelector requires InitializeFromAsset() override
// - Without BOTH, IsSet() returns false even when configured in editor!
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTService_FindInteractable.generated.h"

UCLASS(meta = (DisplayName = "Find Interactable"))
class END2507_API UBTService_FindInteractable : public UBTService
{
    GENERATED_BODY()

public:
    UBTService_FindInteractable();

    // Called periodically while service is active
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

    // Description shown in Behavior Tree editor
    virtual FString GetStaticDescription() const override;

    // CRITICAL: Resolves the blackboard key selector at runtime
    // Without this override, OutputKey.IsSet() returns false!
    virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

protected:
    // Filter to only find actors of this class (null = any IInteractable actor)
    // Set to ABroomActor::StaticClass() to only find brooms
    UPROPERTY(EditAnywhere, Category = "Interactable")
    TSubclassOf<AActor> InteractableClass;

    // Blackboard key to store the found interactable (Object type)
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector OutputKey;

    // Maximum distance to search (0 = unlimited)
    UPROPERTY(EditAnywhere, Category = "Interactable", meta = (ClampMin = "0.0"))
    float MaxSearchDistance;

    // If true, only return interactables where CanInteract() returns true
    // Useful for filtering out brooms that are already being ridden
    UPROPERTY(EditAnywhere, Category = "Interactable")
    bool bRequireCanInteract;
};
