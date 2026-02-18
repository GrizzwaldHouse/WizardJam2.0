# MRC-MATH-S003: Collision Avoidance (Obstacle Avoidance)

**Developer:** Marcus Daley
**Project:** WizardJam (END2507)
**Status:** Ready
**Date Created:** February 16, 2026
**Skill Level:** Intermediate

---

## System Information

| Field | Value |
|-------|-------|
| MRC Number | MRC-MATH-S003 |
| System | 3D Math for AI |
| Subsystem | Obstacle Detection and Avoidance |
| Prerequisites | Line traces, Dot product, Vector projection, Cross product |
| Existing Code | `AC_FlightSteeringComponent.h` (lines 97-230) |

---

## Concept Overview

Flying agents need to avoid obstacles (arena walls, towers, other players, Bludgers) while pursuing their primary goal. The avoidance system works in three stages:

1. **Detect** -- Cast rays (whiskers) forward to find obstacles
2. **Evaluate** -- Calculate how much of the path is blocked
3. **Steer** -- Compute an avoidance vector and blend it with the desired path

### Why Not Just Use NavMesh?

UE5 NavMesh is a **2D ground plane**. It cannot path to elevated 3D positions. Flying agents must use raycast-based avoidance for true 3D obstacle detection. This is documented in `MRC-TEMPLATE_New_AI_Role.md` (line 721-725).

---

## The Math

### Stage 1: Whisker Raycasting

Cast N rays forward in a cone pattern. The center ray points in the movement direction. Additional whisker rays spread at angles.

```
CenterRay = ForwardVector * DetectionRange
WhiskerRay[i] = RotateVector(ForwardVector, Angle[i]) * DetectionRange
```

For 5 whiskers at 30-degree spread:
```
Angles = [-30, -15, 0, +15, +30]
```

UE5: Use `SphereTraceSingleByChannel` for thickness (prevents slipping through gaps).

### Stage 2: Avoidance Vector Calculation

For each hit, compute a repulsion vector weighted by closeness:

```
// How close is the obstacle (0 = at detection range, 1 = touching us)
Urgency = 1.0 - (HitDistance / DetectionRange)

// Direction to push away from obstacle
AvoidDirection = Normalize(MyPosition - HitLocation)

// Scale by urgency (closer = stronger push)
AvoidForce += AvoidDirection * Urgency * AvoidanceStrength
```

### Stage 3: Blending with Primary Steering

The avoidance vector and seek vector are blended:

```
// Simple priority blend
if (ObstacleDetected):
    FinalSteering = Seek * (1 - Urgency) + Avoidance * Urgency
else:
    FinalSteering = Seek
```

Better approach -- **weighted sum with normalization:**

```
FinalSteering = Normalize(Seek * SeekWeight + Avoidance * AvoidWeight)
```

### Cross Product for Avoidance Direction

When an obstacle is directly ahead, the dot product alone cannot tell us which direction to swerve. The cross product gives us a perpendicular escape direction:

```
EscapeDirection = Cross(ForwardVector, ObstacleNormal)
```

If the escape direction has magnitude near zero (hitting head-on), use the world up vector as a fallback:

```
if |EscapeDirection| < threshold:
    EscapeDirection = Cross(ForwardVector, UpVector)
```

### Projection for Wall Sliding

When steering parallel to a wall, project the desired velocity onto the wall plane:

```
WallNormal = HitResult.Normal
ProjectedVelocity = DesiredVelocity - WallNormal * Dot(DesiredVelocity, WallNormal)
```

This creates "wall sliding" behavior where the agent follows the wall contour instead of stopping dead.

---

## C++ Code Templates (UE5 Compilable)

### Whisker Raycast System
```cpp
// Returns array of hit results from whisker traces
TArray<FHitResult> PerformWhiskerTraces(
    const AActor* Owner,
    int32 WhiskerCount,
    float WhiskerAngle,
    float DetectionRange,
    float TraceRadius,
    ECollisionChannel TraceChannel)
{
    TArray<FHitResult> Results;
    if (!Owner || !Owner->GetWorld()) return Results;

    FVector Start = Owner->GetActorLocation();
    FVector Forward = Owner->GetActorForwardVector();
    FVector Right = Owner->GetActorRightVector();
    FVector Up = Owner->GetActorUpVector();

    // Generate whisker angles spread evenly across the cone
    for (int32 i = 0; i < WhiskerCount; i++)
    {
        float Fraction = (WhiskerCount > 1)
            ? (static_cast<float>(i) / (WhiskerCount - 1)) - 0.5f  // -0.5 to +0.5
            : 0.0f;  // Single whisker = center

        float AngleDeg = Fraction * 2.0f * WhiskerAngle;

        // Rotate forward vector around up axis (yaw spread)
        FVector WhiskerDir = Forward.RotateAngleAxis(AngleDeg, Up);
        FVector End = Start + WhiskerDir * DetectionRange;

        FHitResult Hit;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(Owner);

        bool bHit = Owner->GetWorld()->SweepSingleByChannel(
            Hit, Start, End, FQuat::Identity,
            TraceChannel,
            FCollisionShape::MakeSphere(TraceRadius),
            Params);

        if (bHit)
        {
            Results.Add(Hit);
        }
    }

    return Results;
}
```

### Avoidance Vector Calculation
```cpp
// Combines all obstacle hits into a single avoidance direction
FVector CalculateAvoidanceVector(
    const FVector& CurrentLocation,
    const TArray<FHitResult>& Hits,
    float DetectionRange,
    float AvoidanceStrength)
{
    if (Hits.Num() == 0) return FVector::ZeroVector;

    FVector TotalAvoidance = FVector::ZeroVector;

    for (const FHitResult& Hit : Hits)
    {
        float HitDistance = (Hit.ImpactPoint - CurrentLocation).Size();
        float Urgency = 1.0f - FMath::Clamp(HitDistance / DetectionRange, 0.0f, 1.0f);

        // Push away from the obstacle surface
        FVector AwayFromHit = (CurrentLocation - Hit.ImpactPoint).GetSafeNormal();

        // Use surface normal if available (better for walls)
        if (!Hit.ImpactNormal.IsNearlyZero())
        {
            AwayFromHit = Hit.ImpactNormal;
        }

        TotalAvoidance += AwayFromHit * Urgency * AvoidanceStrength;
    }

    return TotalAvoidance;
}
```

### Combined Steering + Avoidance
```cpp
// Final steering output blending seek with avoidance
FVector CalculateObstacleAwareSteering(
    const FVector& CurrentLocation,
    const FVector& CurrentVelocity,
    const FVector& TargetLocation,
    float MaxSpeed,
    const TArray<FHitResult>& ObstacleHits,
    float DetectionRange,
    float AvoidanceStrength)
{
    // Primary goal: seek target
    FVector SeekSteering = (TargetLocation - CurrentLocation).GetSafeNormal() * MaxSpeed
                           - CurrentVelocity;

    // Obstacle avoidance
    FVector Avoidance = CalculateAvoidanceVector(
        CurrentLocation, ObstacleHits, DetectionRange, AvoidanceStrength);

    if (Avoidance.IsNearlyZero())
    {
        return SeekSteering;
    }

    // Blend: avoidance priority increases with urgency
    float AvoidanceMagnitude = Avoidance.Size();
    float BlendFactor = FMath::Clamp(AvoidanceMagnitude / AvoidanceStrength, 0.0f, 1.0f);

    FVector Blended = FMath::Lerp(SeekSteering, Avoidance, BlendFactor);
    return Blended.GetClampedToMaxSize(MaxSpeed);
}
```

### Existing Production Implementation

See `AC_FlightSteeringComponent.h` for the full production version:
- `PerformObstacleDetection()` (line 204) -- Whisker traces
- `CalculateAvoidanceVector()` (line 207) -- Hit processing
- `CalculateSeekVector()` (line 210) -- Primary seek
- `CalculateAltitudeCorrection()` (line 213) -- Ground/ceiling bounds
- `CombineSteeringVectors()` (line 219) -- Final blend with all influences

---

## Blueprint-Safe Equivalents

```cpp
UFUNCTION(BlueprintCallable, Category = "Math|Avoidance",
    meta = (DisplayName = "Check Obstacles Ahead"))
static bool BP_IsObstacleAhead(
    AActor* Owner,
    float DetectionRange,
    float TraceRadius,
    FHitResult& OutHit);

UFUNCTION(BlueprintPure, Category = "Math|Avoidance",
    meta = (DisplayName = "Calculate Avoidance Steering"))
static FVector BP_CalculateAvoidanceSteering(
    FVector CurrentLocation,
    FVector DesiredDirection,
    FVector ObstacleNormal,
    float ObstacleDistance,
    float DetectionRange,
    float AvoidanceStrength);
```

---

## Unit Test Cases

### Test 1: No Obstacles
```
Place agent in open field, target 2000 units ahead.
No obstacles between agent and target.

Expected: Avoidance = ZeroVector
Expected: FinalSteering = pure Seek
```

### Test 2: Wall Directly Ahead
```
Place agent flying toward a wall at 90 degrees.
Wall at 500 units, DetectionRange = 800.

Expected: Urgency = 1 - (500/800) = 0.375
Expected: Avoidance pushes away from wall normal
Expected: Agent curves around wall
```

### Test 3: Narrow Gap
```
Place agent between two walls, 400 units apart.
Whiskers detect both walls.

Expected: Left wall avoidance + Right wall avoidance = net zero lateral
Expected: Agent flies straight through the gap center
```

### Test 4: Head-On Collision
```
Place agent flying directly at a column (spherical obstacle).
Obstacle at 300 units, centered on flight path.

Expected: Cross product escape direction kicks in
Expected: Agent swerves left or right (deterministic based on cross product)
```

---

## Gamified AI Task

**Scenario:** "Obstacle Course Run"

Build a linear obstacle course with:
1. Wide walls requiring lateral avoidance
2. Columns requiring tight maneuvering
3. Narrow gaps requiring center-line precision
4. A zigzag corridor requiring continuous steering

**Test procedure:**
1. Run agent through with avoidance disabled -- count collisions
2. Run agent through with avoidance enabled -- count collisions
3. Compare completion times and collision counts

**Scoring:**
- 0 collisions, < 15 seconds = Gold
- < 3 collisions, < 25 seconds = Silver
- Completes course = Bronze

---

## Debug Visualization

The `AC_FlightSteeringComponent` supports these debug draws (set `bDrawDebugTraces = true` and `bDrawDebugSteering = true` in the Blueprint defaults):

| Color | Meaning |
|-------|---------|
| Green line | Whisker ray, no hit |
| Red line | Whisker ray, hit detected |
| Yellow arrow | Avoidance vector direction |
| Blue arrow | Final blended steering direction |
| Cyan sphere | Arrival radius |

---

## Related MRC Cards

| MRC Number | Title | Relationship |
|------------|-------|--------------|
| MRC-MATH-S001 | Vector Intercept | Intercept point feeds into seek target |
| MRC-MATH-S002 | 3D Steering | Avoidance blends with seek/arrive/flee |
| MRC-MATH-S005 | Flocking | Separation is a soft form of avoidance |

---

## Key Takeaways

1. **Whisker rays > single ray** -- Single ray misses offset obstacles
2. **Urgency scaling** -- Closer obstacles get exponentially more avoidance weight
3. **Surface normal > direction from hit** -- Normals give cleaner wall sliding
4. **Cross product for head-on** -- When dot product fails, cross product provides escape
5. **Projection for wall sliding** -- Don't stop at walls, slide along them
6. **Never use NavMesh for 3D flight** -- It's a 2D ground plane

---

**END OF MRC-MATH-S003**
