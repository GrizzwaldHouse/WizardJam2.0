// ============================================================================
// BTService_FindSnitch.h
// Developer: Marcus Daley
// Date: January 24, 2026
// Project: END2507
// ============================================================================
// Purpose:
// BT Service that continuously scans for the Golden Snitch using AI perception.
// Used by Seeker role to track the Snitch and write to Blackboard.
//
// Algorithm: Perception-based target acquisition
// The Snitch is a fast-moving target, so this service runs at high frequency
// to keep the Blackboard updated with the latest Snitch position.
//
// Usage:
// 1. Attach to Seeker behavior branch
// 2. Configure SnitchActorKey to write perceived Snitch
// 3. Configure SnitchLocationKey for predicted position
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_FindSnitch.generated.h"

UCLASS()
class END2507_API UBTService_FindSnitch : public UBTService
{
    GENERATED_BODY()

public:
    UBTService_FindSnitch();

    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
    virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
    virtual FString GetStaticDescription() const override;

protected:
    // Blackboard key for the Snitch actor reference
    UPROPERTY(EditDefaultsOnly, Category = "Blackboard")
    FBlackboardKeySelector SnitchActorKey;

    // Blackboard key for the Snitch's current location
    UPROPERTY(EditDefaultsOnly, Category = "Blackboard")
    FBlackboardKeySelector SnitchLocationKey;

    // Blackboard key for the Snitch's velocity (for prediction)
    UPROPERTY(EditDefaultsOnly, Category = "Blackboard")
    FBlackboardKeySelector SnitchVelocityKey;

    // Class filter for Snitch detection
    UPROPERTY(EditDefaultsOnly, Category = "Perception")
    TSubclassOf<AActor> SnitchClass;

    // Maximum perception range for Snitch
    UPROPERTY(EditDefaultsOnly, Category = "Perception")
    float MaxSnitchRange;

private:
    // Find Snitch using AI perception
    AActor* FindSnitchInPerception(AAIController* AIController) const;

    // Fallback: Find Snitch by class in world
    AActor* FindSnitchInWorld(UWorld* World) const;
};
