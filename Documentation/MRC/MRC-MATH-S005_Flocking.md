# MRC-MATH-S005: Flocking (Alignment / Separation / Cohesion)

**Developer:** Marcus Daley
**Project:** WizardJam (END2507)
**Status:** Ready
**Date Created:** February 16, 2026
**Skill Level:** Intermediate

---

## System Information

| Field | Value |
|-------|-------|
| MRC Number | MRC-MATH-S005 |
| System | 3D Math for AI |
| Subsystem | Reynolds Boids Flocking |
| Prerequisites | Vector normalization, aggregation, dot product |
| Existing Code | `FlyingSteeringComponent.h` (Flock.cs port), `BTTask_FlockWithTeam.h` |
| Algorithm Source | `FullSailAFI.SteeringBehaviors.StudentAI.Flock` (Flock.cs) |

---

## Concept Overview

Flocking produces emergent group behavior from three simple rules applied to each agent independently. No central coordinator is needed -- each agent makes local decisions based on its neighbors.

### The Three Rules (Craig Reynolds, 1986)

| Rule | Description | Visual |
|------|-------------|--------|
| **Separation** | Steer away from nearby neighbors to avoid collision | Agents push apart |
| **Alignment** | Steer toward the average heading of neighbors | Agents face same direction |
| **Cohesion** | Steer toward the center of mass of neighbors | Agents pull together |

### Combined Velocity

```
FlockSteering = W1 * Separation + W2 * Alignment + W3 * Cohesion
```

Where `W1`, `W2`, `W3` are tunable weights that define the flock personality.

### Quidditch Role Profiles

| Role | Separation | Alignment | Cohesion | Why |
|------|-----------|-----------|----------|-----|
| Seeker | Low (3) | Off | High (15) | Solo pursuit, target-focused |
| Chaser | Medium (5) | Medium (5) | Medium (5) | Team formation, balanced |
| Beater | Off | Low (2) | High (8) | Pursuit-focused, no formation |
| Keeper | Low (2) | Off | Max (20) | Position-hold at goal center |

These profiles match `FlyingSteeringComponent.h` line 26-30.

---

## The Math in Detail

### Neighbor Detection

Each agent considers only nearby agents within `FlockRadius`. This keeps computation O(n*k) where k is average neighbor count, not O(n^2).

```
For each agent A in world:
    if A is on my team AND Distance(me, A) < FlockRadius AND A != me:
        add A to Neighbors list
```

UE5: Use `FlockTag` (FName) to filter by team/role. See `FlyingSteeringComponent.h` line 199.

### Separation

For each neighbor closer than `SafeRadius`, compute a push-away vector weighted by closeness:

```
SeparationForce = ZeroVector
for each Neighbor in Neighbors:
    Distance = |MyPos - NeighborPos|
    if Distance < SafeRadius:
        AwayDirection = Normalize(MyPos - NeighborPos)
        Weight = 1.0 - (Distance / SafeRadius)  // Closer = stronger
        SeparationForce += AwayDirection * Weight

SeparationForce *= SeparationStrength
```

**Port reference:** Flock.cs Lines 105-140

```cpp
FVector UFlyingSteeringComponent::CalculateSeparationForce()
{
    TArray<AActor*> Neighbors = GetNearbyFlockMembers();
    if (Neighbors.Num() == 0) return FVector::ZeroVector;

    FVector SeparationForce = FVector::ZeroVector;
    FVector MyPosition = GetOwner()->GetActorLocation();

    for (AActor* Neighbor : Neighbors)
    {
        FVector ToMe = MyPosition - Neighbor->GetActorLocation();
        float Distance = ToMe.Size();

        if (Distance < SafeRadius && Distance > KINDA_SMALL_NUMBER)
        {
            // Closer neighbors produce stronger push
            float Weight = 1.0f - (Distance / SafeRadius);
            SeparationForce += ToMe.GetSafeNormal() * Weight;
        }
    }

    return SeparationForce * SeparationStrength;
}
```

### Alignment

Compute average velocity of all neighbors, then steer to match:

```
AverageVelocity = ZeroVector
for each Neighbor in Neighbors:
    AverageVelocity += Neighbor.Velocity
AverageVelocity /= Neighbors.Count

AlignmentForce = Normalize(AverageVelocity) - Normalize(MyVelocity)
AlignmentForce *= AlignmentStrength
```

**Port reference:** Flock.cs Lines 65-81

```cpp
FVector UFlyingSteeringComponent::CalculateAlignmentForce()
{
    if (AverageVelocity.IsNearlyZero()) return FVector::ZeroVector;

    FVector MyVelocity = GetActorVelocity(GetOwner());
    if (MyVelocity.IsNearlyZero()) return FVector::ZeroVector;

    // Normalized difference between average heading and our heading
    FVector DesiredHeading = AverageVelocity.GetSafeNormal();
    FVector CurrentHeading = MyVelocity.GetSafeNormal();

    FVector AlignmentForce = (DesiredHeading - CurrentHeading);
    return AlignmentForce * AlignmentStrength;
}
```

### Cohesion

Steer toward the center of mass of neighbors (or toward a specific target if override is set):

```
CenterOfMass = ZeroVector
for each Neighbor in Neighbors:
    CenterOfMass += Neighbor.Position
CenterOfMass /= Neighbors.Count

CohesionForce = Normalize(CenterOfMass - MyPosition) * (Distance / FlockRadius)
CohesionForce *= CohesionStrength
```

**Port reference:** Flock.cs Lines 83-103

```cpp
FVector UFlyingSteeringComponent::CalculateCohesionForce()
{
    FVector TargetPos;

    if (bHasTarget)
    {
        // Override: steer toward explicit target (Snitch, goal, etc.)
        TargetPos = TargetLocation;
    }
    else
    {
        // Default: steer toward flock center
        TargetPos = AveragePosition;
    }

    FVector MyPosition = GetOwner()->GetActorLocation();
    FVector ToTarget = TargetPos - MyPosition;
    float Distance = ToTarget.Size();

    if (Distance < KINDA_SMALL_NUMBER) return FVector::ZeroVector;

    // Strength scales with distance (farther = stronger pull)
    float DistanceFactor = FMath::Clamp(Distance / FlockRadius, 0.0f, 1.0f);
    FVector CohesionForce = ToTarget.GetSafeNormal() * DistanceFactor;

    return CohesionForce * CohesionStrength;
}
```

### Combining All Three

```cpp
FVector UFlyingSteeringComponent::CalculateSteeringForce(float DeltaTime)
{
    // Update neighbor averages
    CalculateFlockAverages();

    // Update target location if tracking an actor
    if (TargetActor.IsValid())
    {
        TargetLocation = TargetActor->GetActorLocation();
    }

    // Calculate individual forces
    FVector TotalForce = FVector::ZeroVector;

    if (bEnableSeparation)
    {
        TotalForce += CalculateSeparationForce();
    }

    if (bEnableAlignment)
    {
        TotalForce += CalculateAlignmentForce();
    }

    if (bEnableCohesion)
    {
        TotalForce += CalculateCohesionForce();
    }

    // Clamp to max speed
    return TotalForce.GetClampedToMaxSize(MaxSpeed);
}
```

---

## C++ Code Template: Standalone Flock Calculator

```cpp
// ============================================================================
// Reusable flock calculation -- no UE5 component dependency
// Can be used in any context (BT Task, Controller, etc.)
// ============================================================================

struct FFlockParams
{
    float SeparationWeight = 5.0f;
    float AlignmentWeight = 5.0f;
    float CohesionWeight = 5.0f;
    float NeighborRadius = 500.0f;
    float SeparationRadius = 150.0f;
    float MaxSpeed = 800.0f;
};

struct FFlockMember
{
    FVector Position;
    FVector Velocity;
};

FVector CalculateFlockSteering(
    const FFlockMember& Self,
    const TArray<FFlockMember>& Neighbors,
    const FFlockParams& Params,
    const FVector* OptionalTarget = nullptr)
{
    if (Neighbors.Num() == 0)
    {
        // No neighbors -- if target exists, seek it
        if (OptionalTarget)
        {
            return (*OptionalTarget - Self.Position).GetSafeNormal() * Params.MaxSpeed;
        }
        return FVector::ZeroVector;
    }

    // ================================================================
    // SEPARATION
    // ================================================================
    FVector SeparationForce = FVector::ZeroVector;
    for (const FFlockMember& N : Neighbors)
    {
        FVector ToSelf = Self.Position - N.Position;
        float Dist = ToSelf.Size();
        if (Dist < Params.SeparationRadius && Dist > KINDA_SMALL_NUMBER)
        {
            SeparationForce += ToSelf.GetSafeNormal() * (1.0f - Dist / Params.SeparationRadius);
        }
    }

    // ================================================================
    // ALIGNMENT
    // ================================================================
    FVector AvgVelocity = FVector::ZeroVector;
    for (const FFlockMember& N : Neighbors)
    {
        AvgVelocity += N.Velocity;
    }
    AvgVelocity /= Neighbors.Num();

    FVector AlignmentForce = FVector::ZeroVector;
    if (!AvgVelocity.IsNearlyZero() && !Self.Velocity.IsNearlyZero())
    {
        AlignmentForce = AvgVelocity.GetSafeNormal() - Self.Velocity.GetSafeNormal();
    }

    // ================================================================
    // COHESION
    // ================================================================
    FVector CohesionTarget;
    if (OptionalTarget)
    {
        CohesionTarget = *OptionalTarget;
    }
    else
    {
        CohesionTarget = FVector::ZeroVector;
        for (const FFlockMember& N : Neighbors)
        {
            CohesionTarget += N.Position;
        }
        CohesionTarget /= Neighbors.Num();
    }

    FVector ToCohesion = CohesionTarget - Self.Position;
    float CohesionDist = ToCohesion.Size();
    FVector CohesionForce = FVector::ZeroVector;
    if (CohesionDist > KINDA_SMALL_NUMBER)
    {
        float Factor = FMath::Clamp(CohesionDist / Params.NeighborRadius, 0.0f, 1.0f);
        CohesionForce = ToCohesion.GetSafeNormal() * Factor;
    }

    // ================================================================
    // COMBINE
    // ================================================================
    FVector Total = SeparationForce * Params.SeparationWeight
                  + AlignmentForce * Params.AlignmentWeight
                  + CohesionForce * Params.CohesionWeight;

    return Total.GetClampedToMaxSize(Params.MaxSpeed);
}
```

---

## Blueprint-Safe Equivalents

```cpp
// Blueprint Function Library
UFUNCTION(BlueprintPure, Category = "Math|Flocking",
    meta = (DisplayName = "Calculate Separation Force"))
static FVector BP_CalculateSeparation(
    FVector SelfLocation,
    const TArray<FVector>& NeighborLocations,
    float SeparationRadius,
    float Strength);

UFUNCTION(BlueprintPure, Category = "Math|Flocking",
    meta = (DisplayName = "Calculate Alignment Force"))
static FVector BP_CalculateAlignment(
    FVector SelfVelocity,
    const TArray<FVector>& NeighborVelocities,
    float Strength);

UFUNCTION(BlueprintPure, Category = "Math|Flocking",
    meta = (DisplayName = "Calculate Cohesion Force"))
static FVector BP_CalculateCohesion(
    FVector SelfLocation,
    const TArray<FVector>& NeighborLocations,
    float FlockRadius,
    float Strength);
```

**Blueprint usage:**
1. Get nearby team agents via `Get All Actors With Tag`
2. Extract positions and velocities into arrays
3. Feed into the three function nodes
4. Add weighted outputs together
5. Apply as movement input

---

## Unit Test Cases

### Test 1: Separation Only (Two Agents Too Close)
```
Agent A: Position (0, 0, 500), Velocity (100, 0, 0)
Agent B: Position (50, 0, 500), Velocity (100, 0, 0)
SeparationRadius = 150

Expected: Agent A gets force in -X direction (pushed away from B)
Expected: Agent B gets force in +X direction (pushed away from A)
Expected: Force magnitude proportional to closeness
```

### Test 2: Alignment Only (Two Agents Different Headings)
```
Agent A: Velocity (100, 0, 0)  // Flying east
Agent B: Velocity (0, 100, 0)  // Flying north

Expected: Alignment steers both toward northeast (average heading)
Expected: Force = Normalize(50, 50, 0) - Normalize(CurrentVelocity)
```

### Test 3: Cohesion Only (Scattered Agents)
```
Agent A: Position (0, 0, 500)
Agent B: Position (400, 0, 500)
Agent C: Position (200, 400, 500)
FlockRadius = 500

Center of Mass: (200, 133, 500)
Expected: All three agents steer toward (200, 133, 500)
Expected: Farther agents have stronger cohesion
```

### Test 4: Combined with Target Override
```
3 Chasers near Quaffle at (1000, 0, 500)
Weights: Sep=5, Align=5, Coh=5
Target override: QuaffleLocation

Expected: Chasers move toward Quaffle (cohesion to target)
Expected: Chasers maintain spacing (separation)
Expected: Chasers align heading toward Quaffle (alignment)
Expected: Formation converges as they approach
```

### Test 5: Edge Case -- Solo Agent
```
Single agent, no neighbors.
Expected: Separation = 0, Alignment = 0, Cohesion = 0
Expected: If target set, cohesion steers toward target
Expected: If no target, returns ZeroVector
```

---

## Gamified AI Task

**Scenario:** "Chaser Formation Drill"

Place 3 Chaser agents on one team. No ball, no opponents. Observe emergent behavior:

**Phase 1 -- Separation Only:**
- Set AlignmentStrength = 0, CohesionStrength = 0
- Agents should spread apart and drift aimlessly

**Phase 2 -- Alignment Only:**
- Set SeparationStrength = 0, CohesionStrength = 0
- Agents should converge to same heading, may collide

**Phase 3 -- Cohesion Only:**
- Set SeparationStrength = 0, AlignmentStrength = 0
- Agents should collapse to a single point (pile up)

**Phase 4 -- All Three Balanced:**
- Set all weights to 5.0
- Agents should fly in formation with stable spacing

**Phase 5 -- With Target:**
- Add a Quaffle target
- Formation should move cohesively toward the Quaffle

**Scoring:**
- Can predict each phase's behavior before running = Gold
- Identifies instability in Phase 2 and 3 = Silver
- Successfully tunes weights for stable formation = Bronze

---

## Weight Tuning Guide

| Symptom | Cause | Fix |
|---------|-------|-----|
| Agents pile on top of each other | Separation too low | Increase SeparationStrength or SeparationRadius |
| Agents scatter and don't group | Cohesion too low | Increase CohesionStrength |
| Formation is wobbly/jittery | Alignment too low | Increase AlignmentStrength for smoother heading |
| Agents ignore the target | CohesionStrength too low relative to others | Increase or use target override |
| Formation too rigid/unresponsive | All weights too high | Scale all weights down proportionally |
| Agents spiral around target | Alignment + Cohesion competing | Reduce Alignment near target, or use Arrive behavior |

---

## Visual Diagram: Flock Force Composition

```
       Separation                    Alignment                   Cohesion

    B ←── A ──→ C                A → → →                     A
   (push away from               B → → →                    ↗
    close neighbors)              C → → →                   B → → Center
                               (match heading)                ↘
                                                               C
                                                          (pull toward
                                                           group center)

Combined:
    A maintains spacing, matches team heading, pulls toward formation center
    = Stable flock formation
```

---

## Related MRC Cards

| MRC Number | Title | Relationship |
|------------|-------|--------------|
| MRC-MATH-S002 | 3D Steering | Flocking builds on Seek/Flee primitives |
| MRC-MATH-S003 | Collision Avoidance | Avoidance supplements Separation for hard obstacles |
| MRC-MATH-S001 | Vector Intercept | Chasers use intercept to lead Quaffle passes |
| MRC-MATH-S004 | Discrete Logic | Switch dispatches role-specific flock configs |
| MRC-TEMPLATE | New AI Role | Flocking integration for Chaser/Beater roles |

---

## Key Takeaways

1. **Three rules, emergent behavior** -- Simple local rules produce complex group movement
2. **Weights are everything** -- Same algorithm, different weights = completely different behavior
3. **Target override is critical** -- Without it, cohesion just makes a ball of agents
4. **Separation prevents pileup** -- Always have some separation unless intentionally disabled
5. **O(n*k) not O(n^2)** -- Neighbor radius bounds keep computation manageable
6. **Tag-based filtering** -- Use `FlockTag` to separate teams/roles into independent flocks
7. **Port from Flock.cs** -- All math traces back to the Full Sail reference implementation

---

**END OF MRC-MATH-S005**
