// ============================================================================
// AC_FlightSteeringComponent.h
// Developer: Marcus Daley
// Date: January 27, 2026
// Project: END2507
// ============================================================================
// Purpose:
// Provides obstacle-aware steering calculations for any flying actor.
// Returns normalized input vectors that can be passed to any flight system.
//
// Modularity:
// This component is REUSABLE - it calculates steering but does NOT apply it.
// Works with any flight system (BroomComponent, vehicles, etc.)
// No dependencies on game-specific classes.
//
// Usage:
// 1. Add component to any flying actor
// 2. Call CalculateSteeringToward(TargetLocation) each frame
// 3. Pass returned FVector to your flight input system
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AC_FlightSteeringComponent.generated.h"

class UCurveFloat;

DECLARE_LOG_CATEGORY_EXTERN(LogFlightSteering, Log, All);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent, DisplayName="Flight Steering Component"))
class END2507_API UAC_FlightSteeringComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAC_FlightSteeringComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ========================================================================
	// PRIMARY API - Steering Calculations
	// ========================================================================

	// Main function - calculates obstacle-aware steering toward a world location
	// Returns FVector with X=Pitch(-1 to 1), Y=Yaw(-1 to 1), Z=Thrust(-1 to 1)
	UFUNCTION(BlueprintCallable, Category = "Flight Steering")
	FVector CalculateSteeringToward(const FVector& TargetLocation) const;

	// Convenience wrapper that extracts location from actor
	UFUNCTION(BlueprintCallable, Category = "Flight Steering")
	FVector CalculateSteeringTowardActor(const AActor* TargetActor) const;

	// Inverse of seek - steers AWAY from threat while avoiding obstacles
	UFUNCTION(BlueprintCallable, Category = "Flight Steering")
	FVector CalculateFleeingFrom(const FVector& ThreatLocation) const;

	// Leads the target based on velocity - better for catching moving targets
	UFUNCTION(BlueprintCallable, Category = "Flight Steering")
	FVector CalculateSteeringTowardWithPrediction(const AActor* TargetActor, float PredictionTime) const;

	// ========================================================================
	// QUERY FUNCTIONS
	// ========================================================================

	// Quick check without full steering calculation
	UFUNCTION(BlueprintCallable, Category = "Flight Steering|Query")
	bool IsObstacleAhead(float CheckDistance = 0.0f) const;

	// Simple distance check for arrival detection
	UFUNCTION(BlueprintCallable, Category = "Flight Steering|Query")
	float GetDistanceToTarget(const FVector& TargetLocation) const;

	// Check if close enough to consider 'arrived'
	UFUNCTION(BlueprintCallable, Category = "Flight Steering|Query")
	bool IsWithinArrivalRadius(const FVector& TargetLocation, float CustomRadius = 0.0f) const;

	// Access to what we're avoiding
	UFUNCTION(BlueprintCallable, Category = "Flight Steering|Query")
	FHitResult GetLastObstacleHit() const;

	// ========================================================================
	// RUNTIME CONFIGURATION
	// ========================================================================

	UFUNCTION(BlueprintCallable, Category = "Flight Steering|Config")
	void SetObstacleDetectionRange(float NewRange);

	UFUNCTION(BlueprintCallable, Category = "Flight Steering|Config")
	void SetAvoidanceStrength(float NewStrength);

	UFUNCTION(BlueprintCallable, Category = "Flight Steering|Config")
	void SetAltitudeBounds(float NewMin, float NewMax);

protected:
	// ========================================================================
	// OBSTACLE DETECTION CONFIG
	// ========================================================================

	// How far ahead to check for obstacles
	UPROPERTY(EditDefaultsOnly, Category = "Obstacle Detection", meta = (ClampMin = "100.0", ClampMax = "2000.0"))
	float ObstacleDetectionRange;

	// Sphere trace radius for obstacle detection
	UPROPERTY(EditDefaultsOnly, Category = "Obstacle Detection", meta = (ClampMin = "25.0", ClampMax = "500.0"))
	float ObstacleDetectionRadius;

	// Number of detection rays (center + angled whiskers)
	UPROPERTY(EditDefaultsOnly, Category = "Obstacle Detection", meta = (ClampMin = "3", ClampMax = "9"))
	int32 WhiskerCount;

	// Angle spread for whisker rays in degrees
	UPROPERTY(EditDefaultsOnly, Category = "Obstacle Detection", meta = (ClampMin = "10.0", ClampMax = "60.0"))
	float WhiskerAngle;

	// Collision channel for obstacle traces
	UPROPERTY(EditDefaultsOnly, Category = "Obstacle Detection")
	TEnumAsByte<ECollisionChannel> ObstacleTraceChannel;

	// ========================================================================
	// AVOIDANCE CONFIG
	// ========================================================================

	// How aggressively to avoid obstacles (multiplier on avoidance vector)
	UPROPERTY(EditDefaultsOnly, Category = "Avoidance", meta = (ClampMin = "0.5", ClampMax = "3.0"))
	float AvoidanceStrength;

	// Optional curve for distance-based avoidance strength (closer = stronger)
	UPROPERTY(EditDefaultsOnly, Category = "Avoidance")
	UCurveFloat* AvoidanceResponseCurve;

	// ========================================================================
	// ALTITUDE CONFIG
	// ========================================================================

	// Minimum height above ground to maintain
	UPROPERTY(EditDefaultsOnly, Category = "Altitude", meta = (ClampMin = "0.0"))
	float MinAltitude;

	// Maximum flight ceiling
	UPROPERTY(EditDefaultsOnly, Category = "Altitude", meta = (ClampMin = "100.0"))
	float MaxAltitude;

	// How far down to trace for ground detection
	UPROPERTY(EditDefaultsOnly, Category = "Altitude")
	float AltitudeCheckDistance;

	// Whether to automatically adjust steering for altitude limits
	UPROPERTY(EditDefaultsOnly, Category = "Altitude")
	bool bEnforceAltitudeBounds;

	// ========================================================================
	// ARRIVAL CONFIG
	// ========================================================================

	// Distance at which we consider ourselves 'arrived' at target
	UPROPERTY(EditDefaultsOnly, Category = "Arrival", meta = (ClampMin = "50.0"))
	float ArrivalRadius;

	// Distance at which to begin slowing down
	UPROPERTY(EditDefaultsOnly, Category = "Arrival", meta = (ClampMin = "100.0"))
	float SlowdownRadius;

	// Whether to reduce thrust when approaching target
	UPROPERTY(EditDefaultsOnly, Category = "Arrival")
	bool bSlowOnArrival;

	// ========================================================================
	// SMOOTHING CONFIG
	// ========================================================================

	// How smoothly steering changes (higher = snappier, lower = smoother)
	UPROPERTY(EditDefaultsOnly, Category = "Smoothing", meta = (ClampMin = "1.0", ClampMax = "20.0"))
	float SteeringSmoothing;

	// Whether to interpolate steering for smoother flight
	UPROPERTY(EditDefaultsOnly, Category = "Smoothing")
	bool bUseSmoothSteering;

	// ========================================================================
	// DEBUG CONFIG
	// ========================================================================

	// Draw obstacle detection rays in game
	UPROPERTY(EditDefaultsOnly, Category = "Debug")
	bool bDrawDebugTraces;

	// Draw steering vectors in game
	UPROPERTY(EditDefaultsOnly, Category = "Debug")
	bool bDrawDebugSteering;

	// How long debug draws persist (0 = one frame)
	UPROPERTY(EditDefaultsOnly, Category = "Debug")
	float DebugDrawDuration;

private:
	// ========================================================================
	// INTERNAL FUNCTIONS
	// ========================================================================

	// Executes all whisker traces and returns hits
	TArray<FHitResult> PerformObstacleDetection(const FVector& LookDirection) const;

	// Combines all obstacle hits into single avoidance direction
	FVector CalculateAvoidanceVector(const TArray<FHitResult>& Hits) const;

	// Basic direction toward target
	FVector CalculateSeekVector(const FVector& TargetLocation) const;

	// Returns pitch adjustment needed to maintain altitude bounds
	float CalculateAltitudeCorrection() const;

	// Returns thrust multiplier based on distance (1.0 = full, 0.0 = stop)
	float CalculateArrivalThrottle(float DistanceToTarget) const;

	// Blends all steering influences into final output
	FVector CombineSteeringVectors(const FVector& Seek, const FVector& Avoidance, float AltitudeCorrection, float Throttle) const;

	// Converts world-space direction to pitch/yaw/thrust input
	FVector WorldDirectionToLocalInput(const FVector& WorldDirection) const;

	// Interpolates toward target steering for smooth flight
	FVector SmoothSteering(const FVector& TargetSteering);

	// ========================================================================
	// CACHED STATE
	// ========================================================================

	// Cached owner actor pointer
	TWeakObjectPtr<AActor> CachedOwner;

	// Previous frame's steering for smoothing
	mutable FVector LastSteeringOutput;

	// Most recent obstacle detection result
	mutable FHitResult LastObstacleHit;

	// Cached ground distance from last check
	mutable float CurrentAltitude;

	// Delta time for smoothing (updated each tick)
	float CachedDeltaTime;
};
