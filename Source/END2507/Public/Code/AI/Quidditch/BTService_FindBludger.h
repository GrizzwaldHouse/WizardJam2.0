// ============================================================================
// BTService_FindBludger.h
// Developer: Marcus Daley
// Date: January 24, 2026
// Project: END2507
// ============================================================================
// Purpose:
// BT Service that locates Bludgers and tracks their targets for Beater AI.
// Also identifies threatening Bludgers heading toward teammates.
//
// Algorithm Influence: MsPacManAgent.java INVERTED safety scoring
// - Instead of finding safest path FROM threats, finds threats TO intercept
// - Prioritizes Bludgers closest to teammates for defensive play
//
// Usage:
// 1. Attach to Beater behavior branch
// 2. Outputs nearest Bludger and highest-threat Bludger
// 3. Also outputs nearest enemy for offensive targeting
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTService_FindBludger.generated.h"

// Forward declarations
class AAIController;
class AActor;
class APawn;

UCLASS(meta = (DisplayName = "Find Bludger"))
class END2507_API UBTService_FindBludger : public UBTService
{
    GENERATED_BODY()

public:
    UBTService_FindBludger();

    // ========================================================================
    // BTService Interface
    // ========================================================================

    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

    virtual FString GetStaticDescription() const override;

    // ========================================================================
    // MANDATORY: FBlackboardKeySelector Initialization
    // ========================================================================

    virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

protected:
    // ========================================================================
    // OUTPUT BLACKBOARD KEYS
    // ========================================================================

    // Nearest Bludger actor (Object)
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector NearestBludgerKey;

    // Bludger location (Vector)
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector BludgerLocationKey;

    // Bludger velocity for interception (Vector)
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector BludgerVelocityKey;

    // Most threatening Bludger - closest to a teammate (Object)
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector ThreateningBludgerKey;

    // The teammate being threatened (Object)
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector ThreatenedTeammateKey;

    // Best enemy target for offensive Bludger hit (Object)
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector BestEnemyTargetKey;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    // Class of Bludger actor to search for
    UPROPERTY(EditAnywhere, Category = "Perception")
    TSubclassOf<AActor> BludgerClass;

    // Maximum range to perceive Bludgers
    UPROPERTY(EditAnywhere, Category = "Perception")
    float MaxBludgerRange;

    // Distance threshold to consider Bludger a threat to teammate
    UPROPERTY(EditAnywhere, Category = "Threat")
    float ThreatRadius;

    // Prefer hitting Bludger at enemies close to our goal
    UPROPERTY(EditAnywhere, Category = "Targeting")
    float EnemyPriorityRadius;

private:
    // Search perception for all Bludgers
    TArray<AActor*> FindBludgersInPerception(AAIController* AIController) const;

    // Fallback world search
    TArray<AActor*> FindBludgersInWorld(UWorld* World) const;

    // Find Bludger most threatening to teammates (MsPacMan INVERTED)
    AActor* FindMostThreateningBludger(const TArray<AActor*>& Bludgers,
        APawn* OwnerPawn, UWorld* World, APawn*& OutThreatenedTeammate) const;

    // Find best enemy target for offensive play
    APawn* FindBestEnemyTarget(APawn* OwnerPawn, UWorld* World) const;
};

