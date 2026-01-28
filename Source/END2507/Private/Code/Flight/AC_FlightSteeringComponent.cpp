// ============================================================================
// AC_FlightSteeringComponent.cpp
// Developer: Marcus Daley
// Date: January 27, 2026
// Project: END2507
// ============================================================================

#include "Code/Flight/AC_FlightSteeringComponent.h"
#include "DrawDebugHelpers.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY(LogFlightSteering);

UAC_FlightSteeringComponent::UAC_FlightSteeringComponent()
	: ObstacleDetectionRange(800.0f)
	, ObstacleDetectionRadius(100.0f)
	, WhiskerCount(5)
	, WhiskerAngle(30.0f)
	, ObstacleTraceChannel(ECC_Visibility)
	, AvoidanceStrength(1.5f)
	, AvoidanceResponseCurve(nullptr)
	, MinAltitude(200.0f)
	, MaxAltitude(2000.0f)
	, AltitudeCheckDistance(500.0f)
	, bEnforceAltitudeBounds(true)
	, ArrivalRadius(200.0f)
	, SlowdownRadius(500.0f)
	, bSlowOnArrival(true)
	, SteeringSmoothing(5.0f)
	, bUseSmoothSteering(true)
	, bDrawDebugTraces(false)
	, bDrawDebugSteering(false)
	, DebugDrawDuration(0.0f)
	, LastSteeringOutput(FVector::ZeroVector)
	, CurrentAltitude(0.0f)
	, CachedDeltaTime(0.016f)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UAC_FlightSteeringComponent::BeginPlay()
{
	Super::BeginPlay();

	CachedOwner = GetOwner();

	if (CachedOwner.IsValid())
	{
		UE_LOG(LogFlightSteering, Display,
			TEXT("[%s] FlightSteeringComponent initialized | DetectionRange=%.0f | AvoidanceStrength=%.1f | ArrivalRadius=%.0f"),
			*CachedOwner->GetName(), ObstacleDetectionRange, AvoidanceStrength, ArrivalRadius);
	}
}

void UAC_FlightSteeringComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	CachedDeltaTime = DeltaTime;
}

// ============================================================================
// PRIMARY API
// ============================================================================

FVector UAC_FlightSteeringComponent::CalculateSteeringToward(const FVector& TargetLocation) const
{
	if (!CachedOwner.IsValid())
	{
		return FVector::ZeroVector;
	}

	AActor* Owner = CachedOwner.Get();
	FVector OwnerLocation = Owner->GetActorLocation();
	FVector OwnerForward = Owner->GetActorForwardVector();

	// Step 1: Calculate seek vector toward target
	FVector SeekDirection = CalculateSeekVector(TargetLocation);

	// Step 2: Perform whisker traces for obstacle detection
	TArray<FHitResult> ObstacleHits = PerformObstacleDetection(SeekDirection);

	// Step 3: Calculate avoidance vector if obstacles detected
	FVector AvoidanceDirection = CalculateAvoidanceVector(ObstacleHits);

	// Step 4: Calculate altitude correction
	float AltitudeCorrection = bEnforceAltitudeBounds ? CalculateAltitudeCorrection() : 0.0f;

	// Step 5: Calculate arrival throttle
	float DistanceToTarget = FVector::Dist(OwnerLocation, TargetLocation);
	float Throttle = CalculateArrivalThrottle(DistanceToTarget);

	// Step 6: Combine all steering influences
	FVector CombinedSteering = CombineSteeringVectors(SeekDirection, AvoidanceDirection, AltitudeCorrection, Throttle);

	// Step 7: Convert to local input space (pitch/yaw/thrust)
	FVector LocalInput = WorldDirectionToLocalInput(CombinedSteering);

	// Step 8: Apply smoothing if enabled
	if (bUseSmoothSteering)
	{
		LocalInput = const_cast<UAC_FlightSteeringComponent*>(this)->SmoothSteering(LocalInput);
	}

	// Step 9: Clamp to valid range
	LocalInput.X = FMath::Clamp(LocalInput.X, -1.0f, 1.0f);
	LocalInput.Y = FMath::Clamp(LocalInput.Y, -1.0f, 1.0f);
	LocalInput.Z = FMath::Clamp(LocalInput.Z * Throttle, -1.0f, 1.0f);

	// Debug visualization
	if (bDrawDebugSteering)
	{
		DrawDebugLine(GetWorld(), OwnerLocation, OwnerLocation + SeekDirection * 300.0f,
			FColor::Green, false, DebugDrawDuration, 0, 3.0f);
		DrawDebugLine(GetWorld(), OwnerLocation, OwnerLocation + AvoidanceDirection * 300.0f,
			FColor::Red, false, DebugDrawDuration, 0, 3.0f);
		DrawDebugLine(GetWorld(), OwnerLocation, OwnerLocation + CombinedSteering * 300.0f,
			FColor::Yellow, false, DebugDrawDuration, 0, 5.0f);
	}

	LastSteeringOutput = LocalInput;
	return LocalInput;
}

FVector UAC_FlightSteeringComponent::CalculateSteeringTowardActor(const AActor* TargetActor) const
{
	if (!TargetActor)
	{
		UE_LOG(LogFlightSteering, Warning, TEXT("CalculateSteeringTowardActor called with null actor"));
		return FVector::ZeroVector;
	}

	return CalculateSteeringToward(TargetActor->GetActorLocation());
}

FVector UAC_FlightSteeringComponent::CalculateFleeingFrom(const FVector& ThreatLocation) const
{
	if (!CachedOwner.IsValid())
	{
		return FVector::ZeroVector;
	}

	FVector OwnerLocation = CachedOwner->GetActorLocation();

	// Flee direction is opposite of seek direction
	FVector FleeDirection = (OwnerLocation - ThreatLocation).GetSafeNormal();

	// Still need obstacle avoidance while fleeing
	TArray<FHitResult> ObstacleHits = PerformObstacleDetection(FleeDirection);
	FVector AvoidanceDirection = CalculateAvoidanceVector(ObstacleHits);

	// Combine flee with avoidance
	FVector CombinedDirection = (FleeDirection + AvoidanceDirection * AvoidanceStrength).GetSafeNormal();

	// Apply altitude correction
	float AltitudeCorrection = bEnforceAltitudeBounds ? CalculateAltitudeCorrection() : 0.0f;

	FVector LocalInput = WorldDirectionToLocalInput(CombinedDirection);
	LocalInput.X += AltitudeCorrection * 0.5f;

	// Full throttle when fleeing
	LocalInput.Z = 1.0f;

	return LocalInput;
}

FVector UAC_FlightSteeringComponent::CalculateSteeringTowardWithPrediction(const AActor* TargetActor, float PredictionTime) const
{
	if (!TargetActor)
	{
		return FVector::ZeroVector;
	}

	FVector TargetLocation = TargetActor->GetActorLocation();
	FVector TargetVelocity = TargetActor->GetVelocity();

	// Predict future position
	FVector PredictedLocation = TargetLocation + (TargetVelocity * PredictionTime);

	UE_LOG(LogFlightSteering, Verbose,
		TEXT("Predicting target at %s (%.1fs ahead) | Velocity=%s"),
		*PredictedLocation.ToString(), PredictionTime, *TargetVelocity.ToString());

	return CalculateSteeringToward(PredictedLocation);
}

// ============================================================================
// QUERY FUNCTIONS
// ============================================================================

bool UAC_FlightSteeringComponent::IsObstacleAhead(float CheckDistance) const
{
	if (!CachedOwner.IsValid())
	{
		return false;
	}

	float Range = (CheckDistance > 0.0f) ? CheckDistance : ObstacleDetectionRange;
	FVector Forward = CachedOwner->GetActorForwardVector();

	TArray<FHitResult> Hits = PerformObstacleDetection(Forward);
	return Hits.Num() > 0;
}

float UAC_FlightSteeringComponent::GetDistanceToTarget(const FVector& TargetLocation) const
{
	if (!CachedOwner.IsValid())
	{
		return MAX_FLT;
	}

	return FVector::Dist(CachedOwner->GetActorLocation(), TargetLocation);
}

bool UAC_FlightSteeringComponent::IsWithinArrivalRadius(const FVector& TargetLocation, float CustomRadius) const
{
	float Radius = (CustomRadius > 0.0f) ? CustomRadius : ArrivalRadius;
	return GetDistanceToTarget(TargetLocation) <= Radius;
}

FHitResult UAC_FlightSteeringComponent::GetLastObstacleHit() const
{
	return LastObstacleHit;
}

// ============================================================================
// RUNTIME CONFIGURATION
// ============================================================================

void UAC_FlightSteeringComponent::SetObstacleDetectionRange(float NewRange)
{
	ObstacleDetectionRange = FMath::Clamp(NewRange, 100.0f, 2000.0f);
	UE_LOG(LogFlightSteering, Display, TEXT("ObstacleDetectionRange set to %.0f"), ObstacleDetectionRange);
}

void UAC_FlightSteeringComponent::SetAvoidanceStrength(float NewStrength)
{
	AvoidanceStrength = FMath::Clamp(NewStrength, 0.5f, 3.0f);
	UE_LOG(LogFlightSteering, Display, TEXT("AvoidanceStrength set to %.1f"), AvoidanceStrength);
}

void UAC_FlightSteeringComponent::SetAltitudeBounds(float NewMin, float NewMax)
{
	MinAltitude = FMath::Max(0.0f, NewMin);
	MaxAltitude = FMath::Max(MinAltitude + 100.0f, NewMax);
	UE_LOG(LogFlightSteering, Display, TEXT("Altitude bounds set to [%.0f, %.0f]"), MinAltitude, MaxAltitude);
}

// ============================================================================
// INTERNAL FUNCTIONS
// ============================================================================

TArray<FHitResult> UAC_FlightSteeringComponent::PerformObstacleDetection(const FVector& LookDirection) const
{
	TArray<FHitResult> Results;

	if (!CachedOwner.IsValid())
	{
		return Results;
	}

	AActor* Owner = CachedOwner.Get();
	FVector Start = Owner->GetActorLocation();
	FVector Forward = LookDirection.GetSafeNormal();

	// Get perpendicular vectors for whisker spread
	FVector Right = FVector::CrossProduct(Forward, FVector::UpVector).GetSafeNormal();
	FVector Up = FVector::CrossProduct(Right, Forward).GetSafeNormal();

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);

	// Center ray
	FVector CenterEnd = Start + Forward * ObstacleDetectionRange;
	FHitResult CenterHit;
	if (GetWorld()->SweepSingleByChannel(CenterHit, Start, CenterEnd, FQuat::Identity,
		ObstacleTraceChannel, FCollisionShape::MakeSphere(ObstacleDetectionRadius), QueryParams))
	{
		Results.Add(CenterHit);
		LastObstacleHit = CenterHit;
	}

	if (bDrawDebugTraces)
	{
		DrawDebugLine(GetWorld(), Start, CenterEnd, CenterHit.bBlockingHit ? FColor::Red : FColor::Green,
			false, DebugDrawDuration, 0, 2.0f);
	}

	// Whisker rays at angles
	float AngleStep = WhiskerAngle / ((WhiskerCount - 1) / 2.0f);
	for (int32 i = 1; i < WhiskerCount; i++)
	{
		float Angle = ((i + 1) / 2) * AngleStep * ((i % 2 == 0) ? 1.0f : -1.0f);
		float AngleRad = FMath::DegreesToRadians(Angle);

		// Horizontal whiskers
		FVector WhiskerDir = (Forward * FMath::Cos(AngleRad) + Right * FMath::Sin(AngleRad)).GetSafeNormal();
		FVector WhiskerEnd = Start + WhiskerDir * ObstacleDetectionRange;

		FHitResult WhiskerHit;
		if (GetWorld()->SweepSingleByChannel(WhiskerHit, Start, WhiskerEnd, FQuat::Identity,
			ObstacleTraceChannel, FCollisionShape::MakeSphere(ObstacleDetectionRadius), QueryParams))
		{
			Results.Add(WhiskerHit);
		}

		if (bDrawDebugTraces)
		{
			DrawDebugLine(GetWorld(), Start, WhiskerEnd, WhiskerHit.bBlockingHit ? FColor::Red : FColor::Cyan,
				false, DebugDrawDuration, 0, 1.0f);
		}

		// Vertical whiskers (above and below)
		FVector VertWhiskerDir = (Forward * FMath::Cos(AngleRad) + Up * FMath::Sin(AngleRad)).GetSafeNormal();
		FVector VertWhiskerEnd = Start + VertWhiskerDir * ObstacleDetectionRange;

		FHitResult VertHit;
		if (GetWorld()->SweepSingleByChannel(VertHit, Start, VertWhiskerEnd, FQuat::Identity,
			ObstacleTraceChannel, FCollisionShape::MakeSphere(ObstacleDetectionRadius), QueryParams))
		{
			Results.Add(VertHit);
		}

		if (bDrawDebugTraces)
		{
			DrawDebugLine(GetWorld(), Start, VertWhiskerEnd, VertHit.bBlockingHit ? FColor::Red : FColor::Magenta,
				false, DebugDrawDuration, 0, 1.0f);
		}
	}

	return Results;
}

FVector UAC_FlightSteeringComponent::CalculateAvoidanceVector(const TArray<FHitResult>& Hits) const
{
	if (Hits.Num() == 0 || !CachedOwner.IsValid())
	{
		return FVector::ZeroVector;
	}

	FVector OwnerLocation = CachedOwner->GetActorLocation();
	FVector Avoidance = FVector::ZeroVector;

	for (const FHitResult& Hit : Hits)
	{
		// Push direction is away from hit point
		FVector PushDirection = (OwnerLocation - Hit.ImpactPoint).GetSafeNormal();

		// Weight by distance (closer = stronger push)
		float DistanceRatio = 1.0f - (Hit.Distance / ObstacleDetectionRange);

		// Use curve if available
		if (AvoidanceResponseCurve)
		{
			DistanceRatio = AvoidanceResponseCurve->GetFloatValue(DistanceRatio);
		}

		Avoidance += PushDirection * DistanceRatio;
	}

	// Normalize if we have any avoidance
	if (!Avoidance.IsNearlyZero())
	{
		Avoidance = Avoidance.GetSafeNormal();
	}

	return Avoidance;
}

FVector UAC_FlightSteeringComponent::CalculateSeekVector(const FVector& TargetLocation) const
{
	if (!CachedOwner.IsValid())
	{
		return FVector::ZeroVector;
	}

	FVector OwnerLocation = CachedOwner->GetActorLocation();
	return (TargetLocation - OwnerLocation).GetSafeNormal();
}

float UAC_FlightSteeringComponent::CalculateAltitudeCorrection() const
{
	if (!CachedOwner.IsValid())
	{
		return 0.0f;
	}

	FVector OwnerLocation = CachedOwner->GetActorLocation();

	// Trace down to find ground
	FHitResult GroundHit;
	FVector TraceStart = OwnerLocation;
	FVector TraceEnd = OwnerLocation - FVector(0.0f, 0.0f, AltitudeCheckDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(CachedOwner.Get());

	if (GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		CurrentAltitude = GroundHit.Distance;
	}
	else
	{
		// No ground found within range - assume we're high enough
		CurrentAltitude = AltitudeCheckDistance;
	}

	// Calculate correction
	float Correction = 0.0f;

	if (CurrentAltitude < MinAltitude)
	{
		// Too low - pitch up
		Correction = FMath::GetMappedRangeValueClamped(
			FVector2D(0.0f, MinAltitude),
			FVector2D(1.0f, 0.0f),
			CurrentAltitude);
	}
	else if (CurrentAltitude > MaxAltitude)
	{
		// Too high - pitch down
		Correction = FMath::GetMappedRangeValueClamped(
			FVector2D(MaxAltitude, MaxAltitude + 500.0f),
			FVector2D(0.0f, -1.0f),
			CurrentAltitude);
	}

	return Correction;
}

float UAC_FlightSteeringComponent::CalculateArrivalThrottle(float DistanceToTarget) const
{
	if (!bSlowOnArrival)
	{
		return 1.0f;
	}

	if (DistanceToTarget <= ArrivalRadius)
	{
		return 0.0f;
	}

	if (DistanceToTarget >= SlowdownRadius)
	{
		return 1.0f;
	}

	// Smoothly reduce throttle as we approach
	return FMath::GetMappedRangeValueClamped(
		FVector2D(ArrivalRadius, SlowdownRadius),
		FVector2D(0.2f, 1.0f),  // Never go below 0.2 while still moving
		DistanceToTarget);
}

FVector UAC_FlightSteeringComponent::CombineSteeringVectors(const FVector& Seek, const FVector& Avoidance, float AltitudeCorrection, float Throttle) const
{
	// Blend seek with avoidance
	FVector Combined;

	if (Avoidance.IsNearlyZero())
	{
		Combined = Seek;
	}
	else
	{
		// Weight avoidance based on strength setting
		Combined = (Seek + Avoidance * AvoidanceStrength).GetSafeNormal();
	}

	return Combined;
}

FVector UAC_FlightSteeringComponent::WorldDirectionToLocalInput(const FVector& WorldDirection) const
{
	if (!CachedOwner.IsValid())
	{
		return FVector::ZeroVector;
	}

	AActor* Owner = CachedOwner.Get();
	FVector Forward = Owner->GetActorForwardVector();
	FVector Right = Owner->GetActorRightVector();
	FVector Up = Owner->GetActorUpVector();

	// Calculate yaw (turn left/right)
	FVector FlatDirection = FVector(WorldDirection.X, WorldDirection.Y, 0.0f).GetSafeNormal();
	FVector FlatForward = FVector(Forward.X, Forward.Y, 0.0f).GetSafeNormal();

	float YawDot = FVector::DotProduct(FlatDirection, FlatForward);
	float YawCross = FVector::CrossProduct(FlatForward, FlatDirection).Z;

	// Yaw: positive = turn right, negative = turn left
	float Yaw = FMath::Atan2(YawCross, YawDot) / PI;  // Normalize to -1..1

	// Calculate pitch (up/down)
	float Pitch = WorldDirection.Z;  // Already roughly -1..1 for normalized vector

	// Thrust is based on forward component
	float Thrust = FMath::Max(0.0f, FVector::DotProduct(WorldDirection, Forward));

	return FVector(Pitch, Yaw, Thrust);
}

FVector UAC_FlightSteeringComponent::SmoothSteering(const FVector& TargetSteering)
{
	float Alpha = FMath::Clamp(CachedDeltaTime * SteeringSmoothing, 0.0f, 1.0f);
	LastSteeringOutput = FMath::Lerp(LastSteeringOutput, TargetSteering, Alpha);
	return LastSteeringOutput;
}
