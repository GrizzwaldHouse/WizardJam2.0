// ============================================================================
// SnitchBall.h
// Golden Snitch for Quidditch matches - erratic flying ball that avoids capture
//
// Developer: Marcus Daley
// Date: January 25, 2026
// Project: WizardJam (END2507)
//
// BEHAVIOR:
// - Flies erratically in random directions
// - Detects floors/walls via line traces and steers away
// - Speeds up when players get close (evasion behavior)
// - Can be caught by Seekers for 150 points
//
// USAGE:
// 1. Create Blueprint child class (BP_GoldenSnitch)
// 2. Set mesh and materials in Blueprint
// 3. Configure movement parameters in Details panel
// 4. Place in level or spawn from QuidditchGameMode
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "SnitchBall.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSnitch, Log, All);

class USphereComponent;
class UStaticMeshComponent;
class UFloatingPawnMovement;

UCLASS()
class END2507_API ASnitchBall : public APawn
{
	GENERATED_BODY()

public:
	ASnitchBall();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// ========================================================================
	// COMPONENTS
	// ========================================================================

protected:
	// Collision sphere - root component for physics queries
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* CollisionSphere;

	// Visual mesh - designer sets this in Blueprint
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* SnitchMesh;

	// Movement component for smooth flying
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UFloatingPawnMovement* MovementComponent;

	// ========================================================================
	// MOVEMENT CONFIGURATION - Designer tunes in Blueprint
	// ========================================================================

	// Base movement speed in units per second
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Snitch|Movement",
		meta = (ClampMin = "100.0", ClampMax = "3000.0"))
	float BaseSpeed;

	// Maximum speed when evading players
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Snitch|Movement",
		meta = (ClampMin = "500.0", ClampMax = "5000.0"))
	float MaxEvasionSpeed;

	// How quickly the snitch changes direction (degrees per second)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Snitch|Movement",
		meta = (ClampMin = "30.0", ClampMax = "360.0"))
	float TurnRate;

	// Time between random direction changes
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Snitch|Movement",
		meta = (ClampMin = "0.5", ClampMax = "5.0"))
	float DirectionChangeInterval;

	// Random variance added to change interval
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Snitch|Movement",
		meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float DirectionChangeVariance;

	// ========================================================================
	// WORLD AVOIDANCE - Prevents going through floors/walls
	// ========================================================================

	// Distance to check for obstacles ahead
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Snitch|Avoidance",
		meta = (ClampMin = "50.0", ClampMax = "500.0"))
	float ObstacleCheckDistance;

	// How strongly to steer away from obstacles (multiplier)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Snitch|Avoidance",
		meta = (ClampMin = "1.0", ClampMax = "10.0"))
	float ObstacleAvoidanceStrength;

	// Minimum height above ground
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Snitch|Avoidance",
		meta = (ClampMin = "50.0", ClampMax = "500.0"))
	float MinHeightAboveGround;

	// Maximum height above ground (keeps snitch in play area)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Snitch|Avoidance",
		meta = (ClampMin = "500.0", ClampMax = "5000.0"))
	float MaxHeightAboveGround;

	// ========================================================================
	// EVASION BEHAVIOR - Speeds up when players approach
	// ========================================================================

	// Distance at which snitch starts evading
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Snitch|Evasion",
		meta = (ClampMin = "200.0", ClampMax = "2000.0"))
	float EvasionTriggerDistance;

	// Speed multiplier when evading
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Snitch|Evasion",
		meta = (ClampMin = "1.0", ClampMax = "3.0"))
	float EvasionSpeedMultiplier;

	// ========================================================================
	// DEBUG VISUALIZATION
	// ========================================================================

	// Enable debug drawing (movement vectors, obstacle traces)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snitch|Debug")
	bool bShowDebugVisualization;

	// Scale for on-screen debug text
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snitch|Debug",
		meta = (ClampMin = "0.5", ClampMax = "3.0"))
	float DebugTextScale;

	// Vertical offset between debug text lines
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snitch|Debug",
		meta = (ClampMin = "10.0", ClampMax = "50.0"))
	float DebugTextLineSpacing;

private:
	// ========================================================================
	// RUNTIME STATE
	// ========================================================================

	// Current desired movement direction
	FVector CurrentDirection;

	// Timer for direction changes
	float DirectionChangeTimer;

	// Current speed (modified by evasion)
	float CurrentSpeed;

	// Is currently evading a player?
	bool bIsEvading;

	// ========================================================================
	// INTERNAL FUNCTIONS
	// ========================================================================

	// Pick a new random direction (constrained by height limits)
	void ChooseNewDirection();

	// Check for obstacles and modify direction to avoid them
	FVector CalculateAvoidanceForce();

	// Check ground distance and modify direction if too low/high
	FVector CalculateHeightCorrection();

	// Check for nearby players and calculate evasion
	void UpdateEvasionState();

	// Draw debug visualization (lines, text)
	void DrawDebugInfo();

	// Line trace helper that respects collision channels
	bool TraceForObstacle(const FVector& Start, const FVector& End, FHitResult& OutHit);
};
