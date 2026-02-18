// ============================================================================
// BTTask_FlockWithTeam.h
// Developer: Marcus Daley
// Date: January 24, 2026
// Project: END2507
// ============================================================================
// Algorithm Source: Flock.cs flocking behaviors
//
// Classic Boids flocking with three components:
// 1. SEPARATION: Avoid crowding neighbors (steer away from nearby agents)
// 2. ALIGNMENT: Steer toward average heading of neighbors
// 3. COHESION: Steer toward center of mass of neighbors
//
// Combined velocity = w1*Separation + w2*Alignment + w3*Cohesion
//
// Usage:
// - Attach to Chaser branch for team coordination
// - Weights are designer-configurable for different behaviors
// - Output is blended movement direction written to Blackboard
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_FlockWithTeam.generated.h"

UCLASS(meta = (DisplayName = "Flock With Team"))
class END2507_API UBTTask_FlockWithTeam : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_FlockWithTeam();

    // ========================================================================
    // BTTaskNode Interface
    // ========================================================================

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

    virtual FString GetStaticDescription() const override;

    // ========================================================================
    // MANDATORY: FBlackboardKeySelector Initialization
    // ========================================================================

    virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

protected:
    // ========================================================================
    // OUTPUT BLACKBOARD KEYS
    // ========================================================================

    // Calculated flock movement direction (Vector)
    UPROPERTY(EditDefaultsOnly, Category = "Blackboard")
    FBlackboardKeySelector FlockDirectionKey;

    // Calculated flock target position (Vector)
    UPROPERTY(EditDefaultsOnly, Category = "Blackboard")
    FBlackboardKeySelector FlockTargetKey;

    // ========================================================================
    // FLOCKING WEIGHTS (from Flock.cs)
    // ========================================================================

    // Separation weight - avoid crowding
    UPROPERTY(EditDefaultsOnly, Category = "Flocking|Weights", meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float SeparationWeight;

    // Alignment weight - match neighbor heading
    UPROPERTY(EditDefaultsOnly, Category = "Flocking|Weights", meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float AlignmentWeight;

    // Cohesion weight - move toward group center
    UPROPERTY(EditDefaultsOnly, Category = "Flocking|Weights", meta = (ClampMin = "0.0", ClampMax = "5.0"))
    float CohesionWeight;

    // ========================================================================
    // FLOCKING PARAMETERS
    // ========================================================================

    // Radius within which agents are considered neighbors
    UPROPERTY(EditDefaultsOnly, Category = "Flocking|Range")
    float NeighborRadius;

    // Minimum separation distance before avoidance kicks in
    UPROPERTY(EditDefaultsOnly, Category = "Flocking|Range")
    float SeparationRadius;

    // Maximum speed for flock movement
    UPROPERTY(EditDefaultsOnly, Category = "Flocking|Movement")
    float MaxFlockSpeed;

    // Distance ahead to place flock target point
    UPROPERTY(EditDefaultsOnly, Category = "Flocking|Movement")
    float FlockTargetDistance;

private:
    // ========================================================================
    // FLOCKING CALCULATIONS (from Flock.cs)
    // ========================================================================

    // Get all teammates within neighbor radius
    TArray<APawn*> GetNeighbors(APawn* OwnerPawn, UWorld* World) const;

    // Calculate separation vector (steer away from close neighbors)
    FVector CalculateSeparation(APawn* OwnerPawn, const TArray<APawn*>& Neighbors) const;

    // Calculate alignment vector (match average velocity)
    FVector CalculateAlignment(APawn* OwnerPawn, const TArray<APawn*>& Neighbors) const;

    // Calculate cohesion vector (steer toward center of mass)
    FVector CalculateCohesion(APawn* OwnerPawn, const TArray<APawn*>& Neighbors) const;
};

