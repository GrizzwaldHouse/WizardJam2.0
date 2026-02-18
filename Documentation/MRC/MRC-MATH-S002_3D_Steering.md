# MRC-MATH-S002: 3D Steering -- Seek, Arrive, Flee

**Developer:** Marcus Daley
**Project:** WizardJam (END2507)
**Status:** Ready
**Date Created:** February 16, 2026
**Skill Level:** Foundational

---

## System Information

| Field | Value |
|-------|-------|
| MRC Number | MRC-MATH-S002 |
| System | 3D Math for AI |
| Subsystem | Steering Behaviors (Craig Reynolds) |
| Prerequisites | Vector normalization, Dot product, Clamp |
| Existing Code | `AC_FlightSteeringComponent.h`, `FlyingSteeringComponent.h` |

---

## Concept Overview

Steering behaviors translate high-level AI goals ("go there", "run away") into low-level movement vectors. Every flying agent in WizardJam uses some combination of these three primitives.

### Seek

Drive toward a target at maximum speed.

```
DesiredVelocity = Normalize(Target - Position) * MaxSpeed
Steering = DesiredVelocity - CurrentVelocity
```

**Problem:** Seek never slows down. The agent overshoots and oscillates around the target.

### Arrive

Seek with a deceleration zone. When within `SlowdownRadius`, scale speed proportionally to distance.

```
Distance = |Target - Position|
if Distance < SlowdownRadius:
    Speed = MaxSpeed * (Distance / SlowdownRadius)
else:
    Speed = MaxSpeed

DesiredVelocity = Normalize(Target - Position) * Speed
Steering = DesiredVelocity - CurrentVelocity
```

**Key insight:** The ratio `Distance / SlowdownRadius` maps the deceleration curve. At the edge of the zone, speed is 100%. At the center, speed is 0%.

### Flee

Inverse of Seek. Drive directly away from a threat.

```
DesiredVelocity = Normalize(Position - Threat) * MaxSpeed
Steering = DesiredVelocity - CurrentVelocity
```

**Design note:** For the Snitch AI, flee is weighted by distance -- the closer a Seeker gets, the stronger the flee force.

---

## The Math in Detail

### Vector Normalization (refresher)

```
Normalize(V) = V / |V|
|V| = sqrt(V.X^2 + V.Y^2 + V.Z^2)
```

UE5: `FVector::GetSafeNormal()` (returns zero vector if length < threshold, prevents division by zero)

### Dot Product (steering quality check)

```
A dot B = |A| * |B| * cos(theta)
```

If normalized: `A dot B = cos(theta)`

| Value | Meaning |
|-------|---------|
| `1.0` | Facing directly at target |
| `0.0` | Perpendicular to target |
| `-1.0` | Facing directly away |

Use this to check if steering is working:
```cpp
float Facing = FVector::DotProduct(ForwardVector, DirectionToTarget);
// Facing > 0.7 = roughly aimed at target (within ~45 degrees)
```

### Velocity Clamping

Steering forces accumulate. Without clamping, agents accelerate infinitely.

```
Acceleration = Clamp(Steering / Mass, -MaxAccel, MaxAccel)
NewVelocity = Clamp(Velocity + Acceleration * DeltaTime, 0, MaxSpeed)
```

UE5: `FVector::GetClampedToMaxSize(MaxLength)` handles magnitude clamping in 3D.

---

## C++ Code Templates (UE5 Compilable)

### Seek
```cpp
// Returns world-space steering vector toward TargetLocation
FVector CalculateSeek(
    const FVector& CurrentLocation,
    const FVector& CurrentVelocity,
    const FVector& TargetLocation,
    float MaxSpeed)
{
    FVector DesiredVelocity = (TargetLocation - CurrentLocation).GetSafeNormal() * MaxSpeed;
    return DesiredVelocity - CurrentVelocity;
}
```

### Arrive
```cpp
// Returns world-space steering vector with deceleration near target
FVector CalculateArrive(
    const FVector& CurrentLocation,
    const FVector& CurrentVelocity,
    const FVector& TargetLocation,
    float MaxSpeed,
    float SlowdownRadius)
{
    FVector ToTarget = TargetLocation - CurrentLocation;
    float Distance = ToTarget.Size();

    if (Distance < KINDA_SMALL_NUMBER)
    {
        // Already there -- zero out velocity
        return -CurrentVelocity;
    }

    float Speed = MaxSpeed;
    if (Distance < SlowdownRadius)
    {
        Speed = MaxSpeed * (Distance / SlowdownRadius);
    }

    FVector DesiredVelocity = ToTarget.GetSafeNormal() * Speed;
    return DesiredVelocity - CurrentVelocity;
}
```

### Flee
```cpp
// Returns world-space steering vector away from ThreatLocation
FVector CalculateFlee(
    const FVector& CurrentLocation,
    const FVector& CurrentVelocity,
    const FVector& ThreatLocation,
    float MaxSpeed)
{
    FVector DesiredVelocity = (CurrentLocation - ThreatLocation).GetSafeNormal() * MaxSpeed;
    return DesiredVelocity - CurrentVelocity;
}
```

### Applying Steering to UE5 Character Movement
```cpp
// Inside a BTTask::TickTask or ActorComponent::TickComponent:
void ApplySteering(ACharacter* Character, const FVector& SteeringForce, float DeltaTime)
{
    if (!Character) return;

    UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
    if (!MoveComp) return;

    // Clamp steering to max acceleration
    FVector ClampedSteering = SteeringForce.GetClampedToMaxSize(MoveComp->GetMaxAcceleration());

    // Integrate acceleration into velocity
    FVector NewVelocity = MoveComp->Velocity + ClampedSteering * DeltaTime;

    // Clamp to max fly speed
    float MaxFlySpeed = MoveComp->MaxFlySpeed;
    NewVelocity = NewVelocity.GetClampedToMaxSize(MaxFlySpeed);

    // Direct velocity assignment (no VInterpTo -- causes circular orbits)
    MoveComp->Velocity = NewVelocity;
}
```

### Existing Production Implementation

See `AC_FlightSteeringComponent.h` (lines 43-78) for the full obstacle-aware version:
- `CalculateSteeringToward()` -- Seek + obstacle avoidance
- `CalculateFleeingFrom()` -- Flee + obstacle avoidance
- `CalculateSteeringTowardWithPrediction()` -- Seek with velocity lead
- `IsWithinArrivalRadius()` -- Arrive detection

---

## Blueprint-Safe Equivalents

### Pure Function Nodes
```cpp
UFUNCTION(BlueprintPure, Category = "Math|Steering",
    meta = (DisplayName = "Calculate Seek Steering"))
static FVector BP_CalculateSeek(
    FVector CurrentLocation,
    FVector CurrentVelocity,
    FVector TargetLocation,
    float MaxSpeed);

UFUNCTION(BlueprintPure, Category = "Math|Steering",
    meta = (DisplayName = "Calculate Arrive Steering"))
static FVector BP_CalculateArrive(
    FVector CurrentLocation,
    FVector CurrentVelocity,
    FVector TargetLocation,
    float MaxSpeed,
    float SlowdownRadius);

UFUNCTION(BlueprintPure, Category = "Math|Steering",
    meta = (DisplayName = "Calculate Flee Steering"))
static FVector BP_CalculateFlee(
    FVector CurrentLocation,
    FVector CurrentVelocity,
    FVector ThreatLocation,
    float MaxSpeed);
```

**Blueprint wiring:**
1. `Get Actor Location` + `Get Velocity` for self inputs
2. Target from Blackboard or actor reference
3. Output steering vector into `Add Movement Input` (player) or direct velocity assignment (AI)

---

## Unit Test Cases

### Test 1: Seek Direction
```
CurrentLocation = (0, 0, 500)
CurrentVelocity = (0, 0, 0)  // Stationary
TargetLocation = (1000, 0, 500)
MaxSpeed = 600

Expected: Steering.X > 0 (points toward target)
Expected: |Steering| = 600 (full speed from standstill)
```

### Test 2: Arrive Deceleration
```
CurrentLocation = (900, 0, 500)
CurrentVelocity = (600, 0, 0)
TargetLocation = (1000, 0, 500)
MaxSpeed = 600
SlowdownRadius = 300

Expected: |DesiredVelocity| = 600 * (100 / 300) = 200
Expected: Steering = (200, 0, 0) - (600, 0, 0) = (-400, 0, 0) -- braking
```

### Test 3: Flee Direction
```
CurrentLocation = (100, 0, 500)
CurrentVelocity = (0, 0, 0)
ThreatLocation = (0, 0, 500)  // Threat behind us
MaxSpeed = 800

Expected: Steering.X > 0 (moves away from threat)
Expected: |Steering| = 800
```

### Test 4: Seek Overshoot Detection
```
Run Seek for 100 frames, log distance to target each frame.
Without Arrive: Distance oscillates around 0 (overshoot)
With Arrive: Distance converges smoothly to 0
```

---

## Gamified AI Task

**Scenario:** "Keeper Training Drill"

Place a Keeper AI at the goal center. Launch Quaffles from random directions at varying speeds.

**Drill 1 -- Seek:** Keeper uses pure Seek toward incoming Quaffle. Observe overshoot and oscillation.

**Drill 2 -- Arrive:** Switch to Arrive. Keeper smoothly intercepts and stops at the Quaffle.

**Drill 3 -- Flee:** Spawn a Bludger near the Keeper. Keeper flees while maintaining goal proximity. Test the tension between flee (from Bludger) and seek (toward goal center).

**Scoring:**
- Blocks within 1 second = Gold
- No overshoot oscillation = Silver
- Successfully balances flee + seek = Bronze

---

## Visual Diagram: Steering Vector Composition

```
                Target
                  *
                 /|
                / |  DesiredVelocity = Norm(Target - Pos) * MaxSpeed
               /  |
              /   |
   Position  *--->|  CurrentVelocity
              \   |
               \  |
                \ |
                 \|
                  *  Steering = Desired - Current
                     (this is the force we apply)
```

---

## Related MRC Cards

| MRC Number | Title | Relationship |
|------------|-------|--------------|
| MRC-MATH-S001 | Vector Intercept | Intercept provides the target point for Seek |
| MRC-MATH-S003 | Collision Avoidance | Avoidance blends with steering output |
| MRC-MATH-S005 | Flocking | Flocking combines multiple steering behaviors |
| MRC-002 | Bind Overlap Events | Arrive radius triggers overlap detection |

---

## Key Takeaways

1. **Seek = go full speed toward target** -- Simple but causes overshoot
2. **Arrive = Seek + deceleration zone** -- Smooth approach, use for positioning
3. **Flee = inverse Seek** -- Use for evasion (Snitch AI, Bludger dodge)
4. **Steering = Desired - Current** -- This subtraction is the core of all behaviors
5. **Always use `GetSafeNormal()`** -- Prevents NaN from zero-length vectors
6. **Direct velocity assignment, never VInterpTo** -- VInterpTo causes circular orbits in AI flight

---

**END OF MRC-MATH-S002**
