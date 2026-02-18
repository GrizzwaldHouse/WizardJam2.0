# MRC-MATH-S001: Vector Intercept (Lead Target Prediction)

**Developer:** Marcus Daley
**Project:** WizardJam (END2507)
**Status:** Ready
**Date Created:** February 16, 2026
**Skill Level:** Intermediate

---

## System Information

| Field | Value |
|-------|-------|
| MRC Number | MRC-MATH-S001 |
| System | 3D Math for AI |
| Subsystem | Pursuit / Intercept Prediction |
| Prerequisites | Vectors, Velocity, Relative Motion, Quadratic Formula |
| Existing Code | `BTTask_PredictIntercept.cpp`, `AC_FlightSteeringComponent.h` |

---

## Concept Overview

**Problem:** A Seeker flying at speed `S` wants to catch a Snitch at position `T` moving with velocity `V`. Where should the Seeker aim?

**Naive approach (direct pursuit):** Fly toward the target's current position. This creates a "dog curve" -- the pursuer always chases where the target *was*, resulting in longer flight paths and wasted stamina.

**Intercept approach (lead prediction):** Calculate where the target *will be* at time `t`, and fly there directly.

### The Math

Given:
- Pursuer position: `P`
- Pursuer max speed: `S`
- Target position: `T`
- Target velocity: `V`

We need to find time `t` such that:
```
|T + V*t - P| = S*t
```

Squaring both sides:
```
|V|^2 * t^2 + 2*(T-P) dot V * t + |T-P|^2 = S^2 * t^2
```

Rearranging into standard quadratic form `at^2 + bt + c = 0`:
```
a = |V|^2 - S^2
b = 2 * (T-P) dot V
c = |T-P|^2
```

Solve with quadratic formula: `t = (-b +/- sqrt(b^2 - 4ac)) / 2a`

Once we have `t`, the intercept point is:
```
InterceptPoint = T + V * t
```

### Edge Cases

| Condition | Meaning | Handling |
|-----------|---------|----------|
| `discriminant < 0` | Target too fast to intercept | Fall back to max prediction time |
| `a = 0` (speeds equal) | Linear equation | Solve `t = -c / b` |
| `both t negative` | Target already behind us | Fall back to direct pursuit |
| `target stationary` | `V = 0` | Skip quadratic, use `t = dist / speed` |

---

## C++ Code Template (UE5 Compilable)

```cpp
// ============================================================================
// Standalone utility function - reusable across any project
// Drop into any utility header/cpp pair
// ============================================================================

// Predicts where a moving target will be when a pursuer can reach it
// Returns the intercept point in world space
// OutTimeToIntercept receives the calculated time (clamped to MaxTime)
FORCEINLINE FVector PredictIntercept(
    const FVector& PursuerPos,
    float PursuerSpeed,
    const FVector& TargetPos,
    const FVector& TargetVel,
    float MaxPredictionTime,
    float& OutTimeToIntercept)
{
    // Stationary target -- go straight there
    if (TargetVel.IsNearlyZero())
    {
        OutTimeToIntercept = FMath::Min(
            FVector::Dist(PursuerPos, TargetPos) / FMath::Max(PursuerSpeed, KINDA_SMALL_NUMBER),
            MaxPredictionTime);
        return TargetPos;
    }

    // Build quadratic: a*t^2 + b*t + c = 0
    FVector RelPos = TargetPos - PursuerPos;
    float a = TargetVel.SizeSquared() - PursuerSpeed * PursuerSpeed;
    float b = 2.0f * FVector::DotProduct(RelPos, TargetVel);
    float c = RelPos.SizeSquared();

    float Time = MaxPredictionTime; // Default fallback

    if (FMath::IsNearlyZero(a))
    {
        // Linear case: same speed
        if (!FMath::IsNearlyZero(b))
        {
            float t = -c / b;
            if (t > 0.0f) Time = t;
        }
    }
    else
    {
        float Disc = b * b - 4.0f * a * c;
        if (Disc >= 0.0f)
        {
            float SqrtDisc = FMath::Sqrt(Disc);
            float t1 = (-b + SqrtDisc) / (2.0f * a);
            float t2 = (-b - SqrtDisc) / (2.0f * a);

            // Smallest positive root
            if (t1 > 0.0f && t2 > 0.0f) Time = FMath::Min(t1, t2);
            else if (t1 > 0.0f)          Time = t1;
            else if (t2 > 0.0f)          Time = t2;
        }
    }

    Time = FMath::Clamp(Time, 0.0f, MaxPredictionTime);
    OutTimeToIntercept = Time;
    return TargetPos + TargetVel * Time;
}
```

### Existing Implementation

See `BTTask_PredictIntercept.cpp` (lines 154-259) for the full production version integrated with the Behavior Tree system, including:
- Blackboard key I/O with safety checks
- Direct pursuit fallback when distance < `DirectPursuitDistance`
- `LeadFactor` multiplier for designer tuning

---

## Blueprint-Safe Equivalent

Create a **Blueprint Function Library** with this node:

```cpp
// In a UBlueprintFunctionLibrary subclass header:
UFUNCTION(BlueprintPure, Category = "Math|Intercept",
    meta = (DisplayName = "Predict Intercept Point"))
static FVector BP_PredictIntercept(
    FVector PursuerLocation,
    float PursuerSpeed,
    FVector TargetLocation,
    FVector TargetVelocity,
    float MaxPredictionSeconds,
    float& OutTimeToIntercept);
```

**Blueprint usage:** Wire `GetActorLocation` and `GetVelocity` into the target inputs. Feed the output `InterceptPoint` into a `Move To` or `Set Blackboard Value as Vector` node.

---

## Unit Test Cases

### Test 1: Stationary Target
```
PursuerPos = (0, 0, 500)
PursuerSpeed = 800
TargetPos = (1600, 0, 500)
TargetVel = (0, 0, 0)

Expected: InterceptPoint = (1600, 0, 500)
Expected: TimeToIntercept = 2.0 seconds (1600 / 800)
```

### Test 2: Target Moving Perpendicular
```
PursuerPos = (0, 0, 500)
PursuerSpeed = 800
TargetPos = (1000, 0, 500)
TargetVel = (0, 400, 0)  // Moving right

Expected: InterceptPoint.Y > 0 (leads the target)
Expected: TimeToIntercept < 2.0 (shorter than direct pursuit)
```

### Test 3: Target Moving Away (Same Speed)
```
PursuerPos = (0, 0, 500)
PursuerSpeed = 800
TargetPos = (500, 0, 500)
TargetVel = (800, 0, 0)  // Same speed, moving away

Expected: Falls back to MaxPredictionTime (can't close gap)
```

### Test 4: Target Faster Than Pursuer
```
PursuerPos = (0, 0, 500)
PursuerSpeed = 600
TargetPos = (200, 0, 500)
TargetVel = (1200, 0, 0)  // Snitch is faster

Expected: discriminant < 0 -> fallback
```

---

## Gamified AI Task

**Scenario:** "Snitch Sprint"

Place the AI Seeker at one end of the pitch. The Snitch flies in a sinusoidal path (varies velocity each second). The Seeker must use intercept prediction to catch the Snitch.

**Scoring:**
- Catch in < 10 seconds = Gold
- Catch in < 20 seconds = Silver
- Catch at all = Bronze
- Compare direct pursuit time vs. intercept time to demonstrate improvement

**Setup in editor:**
1. Place `BP_QuidditchAgent` with Seeker role
2. Place `BP_GoldenSnitch` with `bUseSinusoidalPath = true`
3. Verify `BTTask_PredictIntercept` is in the Seeker BT branch
4. Log `TimeToIntercept` from Blackboard to compare approaches

---

## Related MRC Cards

| MRC Number | Title | Relationship |
|------------|-------|--------------|
| MRC-MATH-S002 | 3D Steering: Seek, Arrive, Flee | Steering uses intercept output as target |
| MRC-MATH-S003 | Collision Avoidance | Avoidance blends with intercept path |
| MRC-MATH-S005 | Flocking | Chasers use intercept for team passing lanes |
| MRC-004 | Test Seeker Pathing | Production Seeker uses this algorithm |

---

## Key Takeaways

1. **Intercept > Direct Pursuit** -- Leading the target saves 20-40% flight time
2. **Quadratic formula is the core** -- Master it and you can solve any constant-speed intercept
3. **Always handle edge cases** -- Negative discriminant, zero coefficients, stationary targets
4. **Clamp prediction time** -- Unbounded prediction leads to absurd flight paths
5. **Direct pursuit fallback** -- When close enough, skip math and just go there

---

**END OF MRC-MATH-S001**
