// ============================================================================
// BTTask_ChaseSnitch.h
// Behavior Tree task for Seeker AI to chase the Golden Snitch
//
// Developer: Marcus Daley
// Date: January 25, 2026
// Project: WizardJam (END2507)
//
// PURPOSE:
// Handles continuous chase behavior for Seeker role AI agents pursuing
// the Golden Snitch. This task ticks every frame, reading the Snitch location
// from Blackboard and controlling flight to intercept.
//
// CHASE CONTROL FLOW:
// 1. Read Snitch location from Blackboard (updated by BTService_FindSnitch)
// 2. Calculate direction vector toward Snitch
// 3. Call SetVerticalInput() to ascend/descend as needed
// 4. Enable boost when Snitch is far away (burst speed)
// 5. Use standard movement input for horizontal movement
// 6. Task succeeds when within catch radius of Snitch
//
// USAGE IN BEHAVIOR TREE:
// Sequence: SeekerBehavior (BTDecorator_IsSeeker must pass)
//   1. BTService_FindSnitch (continuously writes Snitch location to BB)
//   2. BTTask_ChaseSnitch (this task - loops until catch)
//
// COMPARISON TO BTTask_ControlFlight:
// ControlFlight: One-time navigation to staging zone (succeeds on arrival)
// ChaseSnitch: Continuous pursuit of moving target (loops until catch)
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_ChaseSnitch.generated.h"

UCLASS(meta = (DisplayName = "Chase Snitch"))
class END2507_API UBTTask_ChaseSnitch : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_ChaseSnitch();

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
    // MANDATORY per Day 17 lesson learned
    virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

protected:
    // ========================================================================
    // BLACKBOARD CONFIGURATION
    // ========================================================================

    // Blackboard key for Snitch location (Vector)
    // Updated continuously by BTService_FindSnitch
    UPROPERTY(EditDefaultsOnly, Category = "Snitch|Blackboard")
    FBlackboardKeySelector SnitchLocationKey;

    // ========================================================================
    // CHASE PARAMETERS
    // ========================================================================

    // Distance from Snitch to consider "caught" (in cm)
    // Default: 200cm (2 meters) - Snitch overlap detection handles actual catch
    UPROPERTY(EditDefaultsOnly, Category = "Snitch|Parameters", 
        meta = (ClampMin = "50.0", ClampMax = "500.0"))
    float CatchRadius;

    // How close in altitude before stopping vertical movement
    UPROPERTY(EditDefaultsOnly, Category = "Snitch|Parameters",
        meta = (ClampMin = "10.0", ClampMax = "200.0"))
    float AltitudeTolerance;

    // ========================================================================
    // BOOST CONFIGURATION
    // ========================================================================

    // If true, enable boost when Snitch is far away
    UPROPERTY(EditDefaultsOnly, Category = "Snitch|Boost")
    bool bUseBoostForPursuit;

    // Distance threshold to enable boost (if bUseBoostForPursuit is true)
    // Default: 1000cm (10 meters)
    UPROPERTY(EditDefaultsOnly, Category = "Snitch|Boost",
        meta = (EditCondition = "bUseBoostForPursuit", ClampMin = "100.0"))
    float BoostDistanceThreshold;

    // ========================================================================
    // MOVEMENT PARAMETERS
    // ========================================================================

    // Multiplier for vertical input (1.0 = full speed, 0.5 = half speed)
    UPROPERTY(EditDefaultsOnly, Category = "Snitch|Movement",
        meta = (ClampMin = "0.1", ClampMax = "1.0"))
    float VerticalInputMultiplier;

    // Maximum vertical input change per second (smoothing)
    // Prevents jerky altitude changes when Snitch changes height quickly
    UPROPERTY(EditDefaultsOnly, Category = "Snitch|Movement",
        meta = (ClampMin = "0.5", ClampMax = "10.0"))
    float MaxVerticalInputChangeRate;

private:
    // Helper to get Snitch location from blackboard
    bool GetSnitchLocation(UBehaviorTreeComponent& OwnerComp, FVector& OutLocation) const;
};
