// ============================================================================
// SnitchBall.h
// Developer: Marcus Daley
// Date: January 24, 2026
// Project: END2507
// ============================================================================
// Purpose:
// The Golden Snitch - a fast-moving, evasive target for Seekers.
// Uses configurable movement patterns and evades nearby pursuers.
//
// Movement Algorithm:
// - Wander: Random direction changes at configurable intervals
// - Evade: Flees from nearby pursuers (Seekers) when within EvadeRadius
// - Boundary: Soft constraint keeping Snitch within PlayAreaExtent
//
// Usage:
// 1. Create Blueprint child (BP_GoldenSnitch)
// 2. Assign visual mesh and golden material
// 3. Configure movement and evasion parameters
// 4. Place in level and set PlayAreaCenter
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "SnitchBall.generated.h"

// Forward declarations
class USphereComponent;
class UStaticMeshComponent;
class UAIPerceptionStimuliSourceComponent;
class AAIC_SnitchController;

DECLARE_LOG_CATEGORY_EXTERN(LogSnitchBall, Log, All);

UCLASS()
class END2507_API ASnitchBall : public APawn
{
    GENERATED_BODY()

public:
    ASnitchBall();

    // Override to bind to controller perception delegates
    virtual void PossessedBy(AController* NewController) override;

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // ========================================================================
    // COMPONENTS
    // ========================================================================

protected:
    // Root collision sphere
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* CollisionSphere;

    // Visual mesh (golden ball with wings)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* SnitchMesh;

    // AI Perception - so Seekers can detect us
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UAIPerceptionStimuliSourceComponent* PerceptionSource;

    // ========================================================================
    // MOVEMENT CONFIGURATION (Designer-tunable)
    // ========================================================================

    // Base movement speed when not evading
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
    float BaseSpeed;

    // Maximum speed when evading pursuers
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
    float MaxEvadeSpeed;

    // How quickly Snitch can change direction (degrees/second)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
    float TurnRate;

    // Time between random direction changes
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
    float DirectionChangeInterval;

    // Random variance in direction change timing
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement")
    float DirectionChangeVariance;

    // ========================================================================
    // EVASION CONFIGURATION
    // ========================================================================

    // Distance at which Snitch detects pursuers
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Evasion")
    float DetectionRadius;

    // Distance at which Snitch starts evading
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Evasion")
    float EvadeRadius;

    // How strongly to evade (multiplier on evasion force)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Evasion")
    float EvadeStrength;

    // ========================================================================
    // BOUNDARY CONFIGURATION
    // Designer assigns a volume actor in the level, or sets manual values.
    // The boundary system uses pure 3D math - works at any altitude.
    // ========================================================================

    // Optional reference to a volume actor that defines the play area.
    // If assigned, PlayAreaCenter and PlayAreaExtent are read from this actor at BeginPlay.
    // If null, the Snitch uses the manual values below.
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Bounds")
    AActor* PlayAreaVolumeRef;

    // Center of play area - set manually or read from PlayAreaVolumeRef
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Bounds")
    FVector PlayAreaCenter;

    // Half-extents of play area box - set manually or read from PlayAreaVolumeRef
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Bounds")
    FVector PlayAreaExtent;

    // Strength of the soft push when approaching boundaries
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bounds")
    float BoundaryForce;

    // ========================================================================
    // OBSTACLE DETECTION CONFIGURATION
    // Multi-directional ray casting for collision avoidance.
    // Rays check forward, down, left, right, and down-forward directions.
    // ========================================================================

    // Distance to check for obstacles in each direction (units)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Obstacle Detection")
    float ObstacleCheckDistance;

    // How strongly to steer away from obstacles (multiplier on avoidance force)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Obstacle Detection")
    float ObstacleAvoidanceStrength;

    // Collision channel to check against for obstacles
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Obstacle Detection")
    TEnumAsByte<ECollisionChannel> ObstacleChannel;

    // ========================================================================
    // HEIGHT CONSTRAINTS
    // Keep Snitch within vertical bounds relative to ground.
    // Ground height is detected via downward line trace each frame.
    // ========================================================================

    // Minimum height above ground - Snitch won't fly lower than this
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Height Constraints")
    float MinHeightAboveGround;

    // Maximum height above ground - Snitch won't fly higher than this
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Height Constraints")
    float MaxHeightAboveGround;

    // How far down to trace for ground detection (units)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Height Constraints")
    float GroundTraceDistance;

    // ========================================================================
    // DEBUG
    // ========================================================================

    // Show debug visualization
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Debug")
    bool bShowDebug;

    // ========================================================================
    // RUNTIME STATE
    // ========================================================================

private:
    // Current movement direction
    FVector CurrentDirection;

    // Current speed (interpolates between Base and MaxEvade)
    float CurrentSpeed;

    // Timer for direction changes
    float DirectionChangeTimer;

    // Next direction change time
    float NextDirectionChangeTime;

    // Cached controller reference for perception access
    UPROPERTY()
    AAIC_SnitchController* SnitchController;

    // Current ground height below Snitch (updated each frame for debug)
    float CurrentGroundHeight;

    // Last detected obstacle hit results (stored for debug visualization)
    TArray<FHitResult> LastObstacleHits;

    // Track evasion state changes for logging (prevents spam)
    bool bWasEvadingLastFrame;

    // ========================================================================
    // MOVEMENT FUNCTIONS
    // ========================================================================

    // Calculate next movement direction (random wandering)
    FVector CalculateWanderDirection() const;

    // Calculate evasion vector from nearby pursuers (uses perception data)
    FVector CalculateEvadeVector() const;

    // Calculate boundary avoidance force
    FVector CalculateBoundaryForce() const;

    // Check for obstacles in 5 directions (forward, down, left, right, down-forward)
    // Returns combined avoidance vector weighted by proximity
    FVector CalculateObstacleAvoidance();

    // Get current ground height via downward line trace
    float GetGroundHeight() const;

    // Enforce height constraints on desired location
    // Clamps Z between MinHeightAboveGround and MaxHeightAboveGround
    FVector EnforceHeightConstraints(const FVector& DesiredLocation) const;

    // Blend all forces and update position
    void UpdateMovement(float DeltaTime);

    // Draw debug visualization (calls both enhanced and on-screen methods)
    void DrawDebugInfo() const;

    // Enhanced in-world debug drawing with colored rays and boundaries
    void DrawEnhancedDebugInfo() const;

    // On-screen text debug display (properly spaced, not clumped)
    void DrawOnScreenDebugText() const;

    // ========================================================================
    // PERCEPTION EVENT HANDLERS - Observer Pattern
    // ========================================================================

    // Called by controller when a new pursuer enters detection range
    UFUNCTION()
    void HandlePursuerDetected(AActor* Pursuer);

    // Called by controller when a pursuer leaves detection range
    UFUNCTION()
    void HandlePursuerLost(AActor* Pursuer);

    // ========================================================================
    // CATCH DETECTION
    // ========================================================================

    // Called when a Seeker overlaps the collision sphere
    UFUNCTION()
    void OnSnitchOverlap(UPrimitiveComponent* OverlappedComponent,
                          AActor* OtherActor,
                          UPrimitiveComponent* OtherComp,
                          int32 OtherBodyIndex,
                          bool bFromSweep,
                          const FHitResult& SweepResult);
};

