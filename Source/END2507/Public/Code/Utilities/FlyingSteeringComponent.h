// ============================================================================
// FlyingSteeringComponent.h
// Port of Full Sail Flock.cs to UE5 ActorComponent
//
// Developer: Marcus Daley
// Date: January 20, 2026
// Project: WizardJam / GAR (END2507)
//
// ALGORITHM SOURCE: FullSailAFI.SteeringBehaviors.StudentAI.Flock
// Original file: Flock.cs, Lines 1-182
//
// This component implements three Reynolds flocking behaviors:
//
// 1. ALIGNMENT (Flock.cs Line 65-81):
//    Steer to match the average velocity direction of nearby teammates.
//    Makes agents fly in the same direction as their flock.
//
// 2. COHESION (Flock.cs Line 83-103):
//    Steer toward the center of nearby teammates (or a target point).
//    Pulls scattered agents back toward the group.
//
// 3. SEPARATION (Flock.cs Line 105-140):
//    Steer away from nearby teammates to avoid collision.
//    Prevents agents from stacking on top of each other.
//
// HOW QUIDDITCH POSITIONS USE THIS:
// - SEEKER: Cohesion toward Snitch, minimal Alignment, low Separation
// - CHASER: Full flocking for team coordination, Cohesion toward goal when scoring
// - BEATER: Separation disabled (uses pursuit instead), Cohesion toward target
// - KEEPER: Strong Cohesion to goal center, Position hold behavior
//
// USAGE:
// 1. Add component to any AI pawn
// 2. Set FlockTag to identify flock members ("Team0_Chasers", etc.)
// 3. Call CalculateSteeringForce() from BT Service each tick
// 4. Apply returned force vector to movement input
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FlyingSteeringComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogFlockSteering, Log, All);

UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent),
    DisplayName = "Flying Steering (Flock.cs Port)")
class END2507_API UFlyingSteeringComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UFlyingSteeringComponent();

    // ========================================================================
    // MAIN API - Call from Behavior Tree Service or AI Controller
    // ========================================================================

    // Returns combined steering force from all enabled behaviors
    // This is the main function you call every tick
    // Port of: Flock.cs Update() method, Lines 146-179
    UFUNCTION(BlueprintCallable, Category = "Steering")
    FVector CalculateSteeringForce(float DeltaTime);

    // ========================================================================
    // INDIVIDUAL BEHAVIORS - For debugging or role-specific overrides
    // ========================================================================

    // Alignment: Match average velocity of nearby flock members
    // Port of: Flock.cs CalculateAlignmentAcceleration(), Lines 65-81
    // Math: AlignmentInfluence = AverageVelocity / MaxSpeed, clamped to unit length
    UFUNCTION(BlueprintCallable, Category = "Steering|Behaviors")
    FVector CalculateAlignmentForce();

    // Cohesion: Steer toward center of flock (or target if set)
    // Port of: Flock.cs CalculateCohesionAcceleration(), Lines 83-103
    // Math: CohesionInfluence = (AveragePosition - MyPosition).Normalized * (Distance/Radius)
    UFUNCTION(BlueprintCallable, Category = "Steering|Behaviors")
    FVector CalculateCohesionForce();

    // Separation: Push away from nearby flock members
    // Port of: Flock.cs CalculateSeparationAcceleration(), Lines 105-140
    // Math: For each nearby agent, add AwayDirection * (1 - Distance/SafeDistance)
    UFUNCTION(BlueprintCallable, Category = "Steering|Behaviors")
    FVector CalculateSeparationForce();

    // ========================================================================
    // TARGET OVERRIDE - For goal seeking, ball chasing, player pursuit
    // ========================================================================

    // Sets a target location for Cohesion to steer toward
    // When set, Cohesion ignores flock center and steers toward this point
    UFUNCTION(BlueprintCallable, Category = "Steering|Target")
    void SetTargetLocation(const FVector& Target);

    // Sets a target actor to track (automatically updates target location)
    UFUNCTION(BlueprintCallable, Category = "Steering|Target")
    void SetTargetActor(AActor* Target);

    // Clears target, returns Cohesion to normal flock center behavior
    UFUNCTION(BlueprintCallable, Category = "Steering|Target")
    void ClearTarget();

    // Query if we have an active target
    UFUNCTION(BlueprintPure, Category = "Steering|Target")
    bool HasTarget() const { return bHasTarget; }

    // Get current target location
    UFUNCTION(BlueprintPure, Category = "Steering|Target")
    FVector GetTargetLocation() const { return TargetLocation; }

    // ========================================================================
    // BEHAVIOR TOGGLES - Enable/disable for role-specific configurations
    // ========================================================================

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Steering|Toggles")
    bool bEnableAlignment;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Steering|Toggles")
    bool bEnableCohesion;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Steering|Toggles")
    bool bEnableSeparation;

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    // ========================================================================
    // CONFIGURATION - Direct port of Flock.cs properties
    // Designer tunes these in Blueprint class defaults
    // ========================================================================

    // Alignment strength multiplier
    // Higher = agents match teammate velocity more strongly
    // Flock.cs constructor default: 5.0f (Line 23)
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Steering|Strengths",
        meta = (ClampMin = "0.0", ClampMax = "20.0"))
    float AlignmentStrength;

    // Cohesion strength multiplier
    // Higher = agents pull toward center more strongly
    // Flock.cs constructor default: 5.0f (Line 24)
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Steering|Strengths",
        meta = (ClampMin = "0.0", ClampMax = "20.0"))
    float CohesionStrength;

    // Separation strength multiplier
    // Higher = agents push away from each other more strongly
    // Flock.cs constructor default: 5.0f (Line 25)
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Steering|Strengths",
        meta = (ClampMin = "0.0", ClampMax = "20.0"))
    float SeparationStrength;

    // Radius within which to consider other agents as flock members
    // Flock.cs constructor default: 50.0f (Line 26)
    // Scaled 10x for UE units: 500.0f
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Steering|Detection",
        meta = (ClampMin = "100.0", ClampMax = "2000.0"))
    float FlockRadius;

    // Distance at which separation force begins pushing
    // Agents closer than this trigger separation
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Steering|Detection",
        meta = (ClampMin = "50.0", ClampMax = "500.0"))
    float SafeRadius;

    // Tag to identify flock members
    // Only actors with this tag are considered for flocking calculations
    // Example: "Team0_Chasers", "Team1_Chasers"
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Steering|Detection")
    FName FlockTag;

    // Maximum speed for velocity calculations
    // Used in Alignment normalization
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Steering|Movement")
    float MaxSpeed;

private:
    // ========================================================================
    // RUNTIME STATE - Mirrors Flock.cs instance variables
    // ========================================================================

    // Average position of all flock members (Flock.cs Line 15)
    FVector AveragePosition;

    // Average velocity of all flock members (Flock.cs Line 16: AverageForward)
    FVector AverageVelocity;

    // Current target location for Cohesion override
    FVector TargetLocation;

    // Whether we have an active target
    bool bHasTarget;

    // Target actor being tracked
    UPROPERTY()
    TWeakObjectPtr<AActor> TargetActor;

    // ========================================================================
    // INTERNAL FUNCTIONS - Direct ports of Flock.cs methods
    // ========================================================================

    // Calculate average position and velocity of nearby flock members
    // Port of: Flock.cs Lines 37-63
    void CalculateFlockAverages();

    // Get all actors within FlockRadius that have matching FlockTag
    TArray<AActor*> GetNearbyFlockMembers();

    // Get velocity of an actor (from movement component)
    FVector GetActorVelocity(AActor* Actor) const;
};
