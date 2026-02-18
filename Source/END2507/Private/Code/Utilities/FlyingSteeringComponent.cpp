// ============================================================================
// FlyingSteeringComponent.cpp
// Implementation of Flock.cs port to UE5
//
// Developer: Marcus Daley
// Date: January 20, 2026
// Project: WizardJam / GAR (END2507)
//
// Each function documents which Flock.cs lines it ports from.
// ============================================================================

#include "Code/Utilities/FlyingSteeringComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/MovementComponent.h"
#include "EngineUtils.h"

DEFINE_LOG_CATEGORY(LogFlockSteering);

// ============================================================================
// CONSTRUCTOR - Port of Flock.cs constructor (Lines 20-32)
// ============================================================================

UFlyingSteeringComponent::UFlyingSteeringComponent()
    // Initialize with Flock.cs default values
    : AlignmentStrength(5.0f)       // Flock.cs Line 23
    , CohesionStrength(5.0f)        // Flock.cs Line 24
    , SeparationStrength(5.0f)      // Flock.cs Line 25
    , FlockRadius(500.0f)           // Flock.cs Line 26 (50.0f * 10 for UE scale)
    , SafeRadius(150.0f)            // Combined safe radius check
    , FlockTag(NAME_None)
    , MaxSpeed(600.0f)
    , AveragePosition(FVector::ZeroVector)   // Flock.cs Line 29
    , AverageVelocity(FVector::ZeroVector)   // Flock.cs Line 30
    , TargetLocation(FVector::ZeroVector)
    , bHasTarget(false)
    , bEnableAlignment(true)
    , bEnableCohesion(true)
    , bEnableSeparation(true)
{
    PrimaryComponentTick.bCanEverTick = true;
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void UFlyingSteeringComponent::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogFlockSteering, Display,
        TEXT("[%s] FlyingSteeringComponent initialized | FlockTag: %s | Radius: %.0f"),
        *GetOwner()->GetName(),
        *FlockTag.ToString(),
        FlockRadius);
}

void UFlyingSteeringComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Update target location if tracking an actor
    if (TargetActor.IsValid())
    {
        TargetLocation = TargetActor->GetActorLocation();
    }

    // Recalculate flock averages each frame
    // Port of: Called at start of Flock.cs Update() (Line 151)
    CalculateFlockAverages();
}

// ============================================================================
// MAIN STEERING CALCULATION
// Port of: Flock.cs Update() method (Lines 146-179)
// ============================================================================

FVector UFlyingSteeringComponent::CalculateSteeringForce(float DeltaTime)
{
    // Port of Flock.cs Update() Lines 162-165:
    // Vector3 accel = Vector3.Empty;
    // accel = CalculateAlignmentAcceleration(boid);
    // accel += CalculateCohesionAcceleration(boid);
    // accel += CalculateSeparationAcceleration(boid);

    FVector TotalForce = FVector::ZeroVector;

    if (bEnableAlignment)
    {
        TotalForce += CalculateAlignmentForce();
    }

    if (bEnableCohesion)
    {
        TotalForce += CalculateCohesionForce();
    }

    if (bEnableSeparation)
    {
        TotalForce += CalculateSeparationForce();
    }

    // Port of Flock.cs Lines 167-175:
    // accel *= boid.MaxSpeed * deltaTime;
    // Clamp velocity to max speed
    TotalForce *= DeltaTime;

    if (TotalForce.Size() > MaxSpeed)
    {
        TotalForce = TotalForce.GetSafeNormal() * MaxSpeed;
    }

    return TotalForce;
}

// ============================================================================
// ALIGNMENT - Port of Flock.cs CalculateAlignmentAcceleration() (Lines 65-81)
// ============================================================================

FVector UFlyingSteeringComponent::CalculateAlignmentForce()
{
    // Port of Lines 67-69:
    // if (boid == null || boid.MaxSpeed <= 0) return Vector3.Empty;
    if (MaxSpeed <= 0.0f)
    {
        return FVector::ZeroVector;
    }

    // Port of Line 72:
    // Vector3 alignmentInfluence = AverageForward / boid.MaxSpeed;
    FVector AlignmentInfluence = AverageVelocity / MaxSpeed;

    // Port of Lines 75-78:
    // if (alignmentInfluence.Length > 1.0f)
    //     alignmentInfluence = alignmentInfluence / alignmentInfluence.Length;
    if (AlignmentInfluence.Size() > 1.0f)
    {
        AlignmentInfluence = AlignmentInfluence.GetSafeNormal();
    }

    // Port of Line 80:
    // return alignmentInfluence * AlignmentStrength;
    return AlignmentInfluence * AlignmentStrength;
}

// ============================================================================
// COHESION - Port of Flock.cs CalculateCohesionAcceleration() (Lines 83-103)
// ============================================================================

FVector UFlyingSteeringComponent::CalculateCohesionForce()
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return FVector::ZeroVector;
    }

    FVector MyPosition = Owner->GetActorLocation();

    // If we have a specific target, steer toward it instead of flock center
    // This extends the original Flock.cs for goal-seeking behavior
    FVector CohesionTarget = bHasTarget ? TargetLocation : AveragePosition;

    // Port of Lines 88-91:
    // Vector3 towardsCenter = AveragePosition - boid.Position;
    // float distanceToCenter = towardsCenter.Length;
    // if (distanceToCenter <= 0) return result;
    FVector TowardsTarget = CohesionTarget - MyPosition;
    float DistanceToTarget = TowardsTarget.Size();

    if (DistanceToTarget <= 0.0f)
    {
        return FVector::ZeroVector;
    }

    // Port of Line 93:
    // Vector3 cohesionInfluence = towardsCenter / distanceToCenter;
    FVector CohesionInfluence = TowardsTarget / DistanceToTarget;

    // Port of Lines 94-98:
    // if (distanceToCenter < FlockRadius && FlockRadius > 0)
    //     cohesionInfluence *= (distanceToCenter / FlockRadius);
    // This weakens the pull as we get closer to the center
    if (DistanceToTarget < FlockRadius && FlockRadius > 0.0f)
    {
        CohesionInfluence *= (DistanceToTarget / FlockRadius);
    }

    // Port of Line 101:
    // result = cohesionInfluence * CohesionStrength;
    return CohesionInfluence * CohesionStrength;
}

// ============================================================================
// SEPARATION - Port of Flock.cs CalculateSeparationAcceleration() (Lines 105-140)
// ============================================================================

FVector UFlyingSteeringComponent::CalculateSeparationForce()
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return FVector::ZeroVector;
    }

    FVector MyPosition = Owner->GetActorLocation();

    // Port of Line 108:
    // Vector3 separationSum = Vector3.Empty;
    FVector SeparationSum = FVector::ZeroVector;

    TArray<AActor*> FlockMembers = GetNearbyFlockMembers();

    // Port of Lines 109-132 (the foreach loop):
    for (AActor* OtherActor : FlockMembers)
    {
        // Port of Lines 111-112:
        // if (otherBoid == null || otherBoid == boid) continue;
        if (OtherActor == Owner)
        {
            continue;
        }

        // Port of Lines 113-117:
        // Vector3 distanceVictor = boid.Position - otherBoid.Position;
        // float distance = distanceVictor.Length;
        // float safeDistance = boid.SafeRadius + otherBoid.SafeRadius;
        FVector AwayVector = MyPosition - OtherActor->GetActorLocation();
        float Distance = AwayVector.Size();

        // Port of Line 118:
        // if (distance < safeDistance)
        if (Distance < SafeRadius && Distance > 0.0f)
        {
            // Port of Lines 120-124 (prevent division by zero):
            if (Distance < KINDA_SMALL_NUMBER)
            {
                AwayVector = FVector(0.1f, 0.1f, 0.0f);
                Distance = AwayVector.Size();
            }

            // Port of Lines 126-127:
            // Vector3 awayDirection = distanceVictor / distance;
            FVector AwayDirection = AwayVector / Distance;

            // Port of Lines 128-129:
            // float influenceStrength = (safeDistance - distance) / safeDistance;
            float InfluenceStrength = (SafeRadius - Distance) / SafeRadius;

            // Port of Line 130:
            // separationSum += awayDirection * influenceStrength;
            SeparationSum += AwayDirection * InfluenceStrength;
        }
    }

    // Port of Lines 135-138 (clamp to unit length):
    if (SeparationSum.Size() > 1.0f)
    {
        SeparationSum = SeparationSum.GetSafeNormal();
    }

    // Port of Line 139:
    // return separationSum * SeparationStrength;
    return SeparationSum * SeparationStrength;
}

// ============================================================================
// TARGET MANAGEMENT
// ============================================================================

void UFlyingSteeringComponent::SetTargetLocation(const FVector& Target)
{
    TargetLocation = Target;
    bHasTarget = true;
    TargetActor.Reset();

    UE_LOG(LogFlockSteering, Verbose,
        TEXT("[%s] Target set to location: %s"),
        *GetOwner()->GetName(),
        *Target.ToString());
}

void UFlyingSteeringComponent::SetTargetActor(AActor* Target)
{
    if (Target)
    {
        TargetActor = Target;
        TargetLocation = Target->GetActorLocation();
        bHasTarget = true;

        UE_LOG(LogFlockSteering, Display,
            TEXT("[%s] Now tracking actor: %s"),
            *GetOwner()->GetName(),
            *Target->GetName());
    }
    else
    {
        ClearTarget();
    }
}

void UFlyingSteeringComponent::ClearTarget()
{
    bHasTarget = false;
    TargetActor.Reset();
}

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

// Port of Flock.cs CalculateAverages() (Lines 37-63)
void UFlyingSteeringComponent::CalculateFlockAverages()
{
    TArray<AActor*> FlockMembers = GetNearbyFlockMembers();

    // Port of Lines 39-44:
    // if (Boids == null || Boids.Count == 0) { ... return; }
    if (FlockMembers.Num() == 0)
    {
        AveragePosition = GetOwner()->GetActorLocation();
        AverageVelocity = FVector::ZeroVector;
        return;
    }

    // Port of Lines 46-48:
    // Vector3 positionSum = Vector3.Empty;
    // Vector3 velocitySum = Vector3.Empty;
    FVector PositionSum = FVector::ZeroVector;
    FVector VelocitySum = FVector::ZeroVector;

    // Port of Lines 50-59 (foreach loop):
    for (AActor* Member : FlockMembers)
    {
        PositionSum += Member->GetActorLocation();
        VelocitySum += GetActorVelocity(Member);
    }

    // Port of Lines 61-62:
    // AveragePosition = positionSum / Boids.Count;
    // AverageForward = velocitySum / Boids.Count;
    AveragePosition = PositionSum / FlockMembers.Num();
    AverageVelocity = VelocitySum / FlockMembers.Num();
}

TArray<AActor*> UFlyingSteeringComponent::GetNearbyFlockMembers()
{
    TArray<AActor*> Result;
    AActor* Owner = GetOwner();

    if (!Owner || !GetWorld())
    {
        return Result;
    }

    FVector MyPosition = Owner->GetActorLocation();

    // Find all actors with matching flock tag within radius
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor* Actor = *It;

        // Skip self
        if (Actor == Owner)
        {
            continue;
        }

        // Check tag match (if tag is set)
        if (!FlockTag.IsNone() && !Actor->ActorHasTag(FlockTag))
        {
            continue;
        }

        // Check distance
        float Distance = FVector::Dist(MyPosition, Actor->GetActorLocation());
        if (Distance <= FlockRadius)
        {
            Result.Add(Actor);
        }
    }

    return Result;
}

FVector UFlyingSteeringComponent::GetActorVelocity(AActor* Actor) const
{
    if (!Actor)
    {
        return FVector::ZeroVector;
    }

    // Try movement component first (most accurate)
    if (APawn* Pawn = Cast<APawn>(Actor))
    {
        if (UMovementComponent* MoveComp = Pawn->GetMovementComponent())
        {
            return MoveComp->Velocity;
        }
    }

    return FVector::ZeroVector;
}
