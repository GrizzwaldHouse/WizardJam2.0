// ============================================================================
// BTService_FindStagingZone.h
// Developer: Marcus Daley
// Date: January 28, 2026
// Project: END2507
// ============================================================================
// Purpose:
// BT Service that finds staging zones using AI perception instead of GameMode lookup.
// This eliminates timing issues where GameMode lookup returns zero coordinates
// before staging zones have registered.
//
// Algorithm: Perception-based target acquisition
// Flying agents perceive staging zones that have registered themselves with
// UAIPerceptionStimuliSourceComponent. The service filters for zones matching
// the agent's team and role, then writes to Blackboard.
//
// Usage:
// 1. Attach to FlyToStagingZone behavior branch
// 2. Configure StagingZoneActorKey to write perceived zone
// 3. Configure StagingZoneLocationKey for flight destination
// 4. Service filters by agent's QuidditchTeam and QuidditchRole
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_FindStagingZone.generated.h"

// Forward declarations
class AQuidditchStagingZone;
class AQuidditchGameMode;

UCLASS()
class END2507_API UBTService_FindStagingZone : public UBTService
{
    GENERATED_BODY()

public:
    UBTService_FindStagingZone();

    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
    virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
    virtual FString GetStaticDescription() const override;

protected:
    // Blackboard key for the StagingZone actor reference
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector StagingZoneActorKey;

    // Blackboard key for the StagingZone's location (flight destination)
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector StagingZoneLocationKey;

    // Maximum perception range for staging zones
    UPROPERTY(EditAnywhere, Category = "Perception")
    float MaxStagingZoneRange;

private:
    // Find staging zone matching agent's team/role using AI perception
    AQuidditchStagingZone* FindStagingZoneInPerception(AAIController* AIController, APawn* OwnerPawn) const;

    // Fallback: Find staging zone by tag in world (if perception fails)
    AQuidditchStagingZone* FindStagingZoneInWorld(UWorld* World, APawn* OwnerPawn) const;

    // Get agent's team and role from GameMode
    bool GetAgentTeamAndRole(APawn* Pawn, int32& OutTeam, int32& OutRole) const;

    // Cached GameMode (set on first tick)
    mutable TWeakObjectPtr<AQuidditchGameMode> CachedGameMode;
};
