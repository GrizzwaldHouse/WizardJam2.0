// ============================================================================
// BTTask_FlyToStagingZone.h
// Developer: Marcus Daley
// Date: January 23, 2026 (Updated January 28, 2026)
// Project: END2507
// ============================================================================
// Purpose:
// BT Task that flies the agent to their staging zone location.
// PRIMARY: Reads staging zone location from blackboard (written by BTService_FindStagingZone)
// FALLBACK: Queries GameMode directly if blackboard location is not set
//
// Perception-Based Architecture (January 28, 2026):
// Staging zones now broadcast via AI Perception, agents perceive them while flying.
// BTService_FindStagingZone finds the zone and writes to StagingZoneLocationKey.
// This task reads that location instead of relying on GameMode timing.
//
// Gas Station Pattern:
// This task navigates the agent to their "starting line" position.
// When the agent arrives (detected by staging zone overlap), the zone
// notifies GameMode - this task just needs to reach the location.
//
// Navigation Solutions Implemented:
// 1. FlightSteeringComponent integration for obstacle avoidance
// 2. Configurable ArrivalRadius with extended default
// 3. Velocity-based arrival detection (catches high-speed overshoots)
// 4. Stuck detection with position history
// 5. Timeout fallback system
// 6. Perception-based target acquisition (via BTService_FindStagingZone)
//
// Usage:
// 1. Add BTService_FindStagingZone to write staging zone to blackboard
// 2. Add this task after broom mounting
// 3. Configure StagingZoneLocationKey to read from service's output
// 4. Task ticks flight until agent reaches ArrivalRadius
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_FlyToStagingZone.generated.h"

class UAC_FlightSteeringComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogBTTask_FlyToStagingZone, Log, All);

UCLASS(meta = (DisplayName = "Fly To Staging Zone"))
class END2507_API UBTTask_FlyToStagingZone : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_FlyToStagingZone();

    // ========================================================================
    // BTTaskNode Interface
    // ========================================================================

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
    virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

    virtual FString GetStaticDescription() const override;

    // ========================================================================
    // MANDATORY: FBlackboardKeySelector Initialization
    // ========================================================================

    virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

protected:
    // ========================================================================
    // BLACKBOARD CONFIGURATION
    // ========================================================================

    // Blackboard key to READ staging zone location from BTService_FindStagingZone (Vector)
    // PRIMARY source - if set and valid, uses this location
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector StagingZoneLocationKey;

    // Blackboard key to write staging zone location for other systems (Vector)
    // FALLBACK: If StagingZoneLocationKey is not set, queries GameMode and writes here
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector TargetLocationKey;

    // ========================================================================
    // NAVIGATION CONFIGURATION
    // ========================================================================

    // How close to staging zone before task succeeds (Solution 2: increased default)
    UPROPERTY(EditAnywhere, Category = "Navigation", meta = (ClampMin = "50.0", ClampMax = "1000.0"))
    float ArrivalRadius;

    // Extended arrival radius for velocity-based detection (Solution 3)
    // If moving away from target while within this radius, consider arrived
    UPROPERTY(EditAnywhere, Category = "Navigation", meta = (ClampMin = "100.0", ClampMax = "2000.0"))
    float ExtendedArrivalRadius;

    // Flight speed multiplier (1.0 = normal)
    UPROPERTY(EditAnywhere, Category = "Navigation", meta = (ClampMin = "0.5", ClampMax = "2.0"))
    float FlightSpeedMultiplier;

    // Enable boost when far from target
    UPROPERTY(EditAnywhere, Category = "Navigation")
    bool bUseBoostWhenFar;

    // Distance threshold to enable boost (boost turns ON above this distance)
    UPROPERTY(EditAnywhere, Category = "Navigation", meta = (ClampMin = "100.0", EditCondition = "bUseBoostWhenFar"))
    float BoostDistanceThreshold;

    // Hysteresis threshold - boost turns OFF when distance drops below this value
    // Set lower than BoostDistanceThreshold to prevent oscillation
    UPROPERTY(EditAnywhere, Category = "Navigation", meta = (ClampMin = "50.0", EditCondition = "bUseBoostWhenFar"))
    float BoostDisableThreshold;

    // Minimum stamina percent (0-1) required to enable boost - prevents depleting all stamina
    UPROPERTY(EditAnywhere, Category = "Navigation", meta = (ClampMin = "0.1", ClampMax = "0.9", EditCondition = "bUseBoostWhenFar"))
    float MinStaminaForBoost;

    // Altitude calculation scale (divides altitude difference to get vertical input)
    // Lower values = steeper climbs, higher values = gentler climbs
    UPROPERTY(EditAnywhere, Category = "Navigation", meta = (ClampMin = "50.0", ClampMax = "500.0"))
    float AltitudeScale;

    // ========================================================================
    // FLIGHT STEERING CONFIGURATION (Solution 1)
    // ========================================================================

    // Use FlightSteeringComponent for obstacle-aware navigation
    UPROPERTY(EditAnywhere, Category = "Flight Steering")
    bool bUseFlightSteering;

    // Use predictive steering to lead moving targets
    UPROPERTY(EditAnywhere, Category = "Flight Steering", meta = (EditCondition = "bUseFlightSteering"))
    bool bUsePredictiveSteering;

    // Prediction time for leading targets (seconds)
    UPROPERTY(EditAnywhere, Category = "Flight Steering", meta = (ClampMin = "0.0", ClampMax = "2.0", EditCondition = "bUsePredictiveSteering"))
    float PredictionTime;

    // ========================================================================
    // VELOCITY-BASED ARRIVAL (Solution 3)
    // ========================================================================

    // Enable velocity-based overshoot detection
    UPROPERTY(EditAnywhere, Category = "Arrival Detection")
    bool bEnableVelocityArrival;

    // Minimum speed (negative = moving away) to trigger overshoot detection
    UPROPERTY(EditAnywhere, Category = "Arrival Detection", meta = (EditCondition = "bEnableVelocityArrival"))
    float OvershootSpeedThreshold;

    // ========================================================================
    // STUCK DETECTION (Solution 4)
    // ========================================================================

    // Enable stuck detection based on position history
    UPROPERTY(EditAnywhere, Category = "Stuck Detection")
    bool bEnableStuckDetection;

    // How often to sample position (seconds)
    UPROPERTY(EditAnywhere, Category = "Stuck Detection", meta = (ClampMin = "0.5", ClampMax = "5.0", EditCondition = "bEnableStuckDetection"))
    float StuckCheckInterval;

    // Number of position samples to keep
    UPROPERTY(EditAnywhere, Category = "Stuck Detection", meta = (ClampMin = "3", ClampMax = "10", EditCondition = "bEnableStuckDetection"))
    int32 StuckSampleCount;

    // If agent moves less than this over all samples, consider stuck (units)
    UPROPERTY(EditAnywhere, Category = "Stuck Detection", meta = (ClampMin = "50.0", ClampMax = "500.0", EditCondition = "bEnableStuckDetection"))
    float StuckDistanceThreshold;

    // ========================================================================
    // STAGING ZONE CONFIGURATION
    // ========================================================================

    // Slot name for staging zone lookup - must match the StagingSlotName on the zone actor
    // Examples: "Seeker", "Chaser_Left", "Chaser_Center", "Chaser_Right", "Beater_A", "Beater_B", "Keeper"
    // If empty/None, defaults to the role name (e.g., "Seeker" for a Seeker agent)
    UPROPERTY(EditAnywhere, Category = "Staging")
    FName SlotName;

    // ========================================================================
    // TIMEOUT CONFIGURATION (Solution 5 - already implemented)
    // ========================================================================

    // Enable timeout - if agent can't reach staging zone in time, task fails (triggers fallback)
    UPROPERTY(EditAnywhere, Category = "Timeout")
    bool bEnableTimeout;

    // Maximum time (seconds) to reach staging zone before failing and triggering fallback
    // Only used if bEnableTimeout is true
    UPROPERTY(EditAnywhere, Category = "Timeout", meta = (ClampMin = "5.0", ClampMax = "120.0", EditCondition = "bEnableTimeout"))
    float TimeoutDuration;

private:
    // ========================================================================
    // RUNTIME STATE
    // ========================================================================

    // Cached staging zone location (set once in ExecuteTask)
    FVector CachedStagingLocation;

    // Track if we've set the location
    bool bLocationSet;

    // Track elapsed time for timeout
    float ElapsedFlightTime;

    // Position history for stuck detection (Solution 4)
    TArray<FVector> PositionHistory;

    // Time since last position sample
    float TimeSinceLastSample;

    // Track current boost state for hysteresis
    bool bCurrentlyBoosting;

    // Cached FlightSteeringComponent reference
    TWeakObjectPtr<UAC_FlightSteeringComponent> CachedSteeringComponent;

    // ========================================================================
    // HELPER FUNCTIONS
    // ========================================================================

    // Check if agent has overshot target based on velocity (Solution 3)
    bool HasOvershotTarget(APawn* Pawn, const FVector& ToTarget, float Distance) const;

    // Check if agent is stuck based on position history (Solution 4)
    bool IsAgentStuck() const;

    // Update position history for stuck detection
    void UpdatePositionHistory(const FVector& CurrentLocation, float DeltaSeconds);

    // Notify GameMode directly when agent arrives at staging zone
    // Bypasses physics overlap dependency (dual detection fix - Feb 17, 2026)
    void NotifyGameModeArrival(APawn* Pawn);
};
