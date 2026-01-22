// ============================================================================
// BTService_FindCollectible.h
// Behavior Tree service that scans AI Perception for collectible actors
//
// Developer: Marcus Daley
// Date: January 21, 2026
// Project: END2507
//
// PURPOSE:
// Periodically scans the AI's perception for collectible actors (brooms,
// spells, pickups) and writes the nearest valid one to a blackboard key.
// Used to locate broom collectibles when AI doesn't have flight capability.
//
// USAGE:
// 1. Add as service to a BT composite node
// 2. Set CollectibleClass to filter by type (or leave null for any)
// 3. Set OutputKey to blackboard key for storing found actor (MUST be Object type)
// 4. Optionally set RequiredGrantChannel to filter by what the collectible grants
// 5. Optionally set MaxSearchDistance to limit range (0 = unlimited)
//
// REQUIREMENTS:
// - AI Controller must have UAIPerceptionComponent
// - Collectibles must be sensed (Sight, Hearing, etc.)
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
#include "BTService_FindCollectible.generated.h"

UCLASS(meta = (DisplayName = "Find Collectible"))
class END2507_API UBTService_FindCollectible : public UBTService
{
    GENERATED_BODY()

public:
    UBTService_FindCollectible();

    // Called periodically while service is active
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

    // Description shown in Behavior Tree editor
    virtual FString GetStaticDescription() const override;

    // CRITICAL: Resolves the blackboard key selector at runtime
    // Without this override, OutputKey.IsSet() returns false!
    virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

protected:
    // Filter to only find actors of this class (null = any actor)
    UPROPERTY(EditAnywhere, Category = "Collectible")
    TSubclassOf<AActor> CollectibleClass;

    // Blackboard key to store the found collectible (Object type)
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector OutputKey;

    // Optional: Only find collectibles that grant this specific channel
    UPROPERTY(EditAnywhere, Category = "Collectible")
    FName RequiredGrantChannel;

    // Maximum distance to search (0 = unlimited)
    UPROPERTY(EditAnywhere, Category = "Collectible", meta = (ClampMin = "0.0"))
    float MaxSearchDistance;
};
