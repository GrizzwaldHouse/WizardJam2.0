// ============================================================================
// BTTask_ControlFlight.h
// Behavior Tree task for AI continuous flight control toward a target
//
// Developer: Marcus Daley
// Date: January 19, 2026
// Project: Game Architecture (END2507)
//
// PURPOSE:
// Handles continuous flight control for AI agents that are already mounted
// on brooms. This task ticks every frame, adjusting vertical input and
// boost to reach a target location.
//
// FLIGHT CONTROL FLOW:
// 1. AI is already flying (BTTask_MountBroom succeeded first)
// 2. This task reads target location from Blackboard
// 3. Each tick: Calculate direction to target
// 4. Call SetVerticalInput() to ascend/descend as needed
// 5. Call SetBoostEnabled() when far from target
// 6. Use standard movement input for horizontal movement
// 7. Task succeeds when AI reaches target (within tolerance)
//
// COMPARISON TO PLAYER:
// Player: Holds E key → HandleAscendInput() → SetVerticalInput(1.0)
// AI:     This task   → SetVerticalInput(calculated_value)
//                        ↑
//                   SAME CORE API FUNCTION!
//
// USAGE IN BEHAVIOR TREE:
// Sequence:
//   1. BTTask_MountBroom (bMountBroom = true)
//   2. BTTask_ControlFlight (this task)
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_ControlFlight.generated.h"

UCLASS(meta = (DisplayName = "Control Flight"))
class END2507_API UBTTask_ControlFlight : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_ControlFlight();

    // Called once when task starts
    virtual EBTNodeResult::Type ExecuteTask(
        UBehaviorTreeComponent& OwnerComp, 
        uint8* NodeMemory) override;
    
    // Called every frame while task is active
    virtual void TickTask(
        UBehaviorTreeComponent& OwnerComp, 
        uint8* NodeMemory, 
        float DeltaSeconds) override;

    // Description shown in Behavior Tree editor
    virtual FString GetStaticDescription() const override;

    // CRITICAL: Resolves blackboard key at runtime
    // Without this, FBlackboardKeySelector won't work correctly in BT editor
    virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

protected:
    // ========================================================================
    // BLACKBOARD CONFIGURATION
    // ========================================================================

    // Blackboard key for target location (Vector) or target actor (Object)
    // If Object: Uses actor's location as target
    // If Vector: Uses vector directly
    UPROPERTY(EditAnywhere, Category = "Flight|Target")
    FBlackboardKeySelector TargetKey;

    // Optional: Blackboard key for flight state (Bool)
    // If set, task reads IsFlying from Blackboard (observer pattern)
    // If not set, task queries BroomComponent directly (legacy)
    // RECOMMENDED: Set this and use with a parent decorator for proper abort behavior
    UPROPERTY(EditAnywhere, Category = "Flight|State")
    FBlackboardKeySelector IsFlyingKey;

    // ========================================================================
    // FLIGHT PARAMETERS
    // ========================================================================

    // How close to target before considering "arrived" (in units)
    UPROPERTY(EditAnywhere, Category = "Flight|Parameters", 
        meta = (ClampMin = "10.0", ClampMax = "1000.0"))
    float ArrivalTolerance;

    // How close in altitude before stopping vertical movement
    UPROPERTY(EditAnywhere, Category = "Flight|Parameters",
        meta = (ClampMin = "10.0", ClampMax = "500.0"))
    float AltitudeTolerance;

    // ========================================================================
    // BOOST CONFIGURATION
    // ========================================================================

    // If true, enable boost when far from target
    UPROPERTY(EditAnywhere, Category = "Flight|Boost")
    bool bUseBoostWhenFar;

    // Distance threshold to enable boost (if bUseBoostWhenFar is true)
    UPROPERTY(EditAnywhere, Category = "Flight|Boost",
        meta = (EditCondition = "bUseBoostWhenFar", ClampMin = "100.0"))
    float BoostDistanceThreshold;

    // ========================================================================
    // MOVEMENT PARAMETERS
    // ========================================================================

    // Multiplier for vertical input (1.0 = full speed, 0.5 = half speed)
    UPROPERTY(EditAnywhere, Category = "Flight|Movement",
        meta = (ClampMin = "0.1", ClampMax = "1.0"))
    float VerticalInputMultiplier;

    // If true, move directly toward target (3D movement)
    // If false, only adjust altitude, let navigation handle horizontal
    UPROPERTY(EditAnywhere, Category = "Flight|Movement")
    bool bDirectFlight;

private:
    // Helper to get target location from blackboard
    bool GetTargetLocation(UBehaviorTreeComponent& OwnerComp, FVector& OutLocation) const;
};
