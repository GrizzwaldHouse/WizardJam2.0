// ============================================================================
// SnitchBall.cpp
// Golden Snitch implementation with world collision avoidance
//
// Developer: Marcus Daley
// Date: January 25, 2026
// Project: WizardJam (END2507)
// ============================================================================

#include "Code/Quidditch/SnitchBall.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"

DEFINE_LOG_CATEGORY(LogSnitch);

// ============================================================================
// CONSTRUCTOR - Initialize components and default values
// ============================================================================

ASnitchBall::ASnitchBall()
	// Movement configuration
	: BaseSpeed(800.0f)
	, MaxEvasionSpeed(1500.0f)
	, TurnRate(180.0f)
	, DirectionChangeInterval(1.5f)
	, DirectionChangeVariance(0.5f)
	// Avoidance configuration
	, ObstacleCheckDistance(200.0f)
	, ObstacleAvoidanceStrength(3.0f)
	, MinHeightAboveGround(100.0f)
	, MaxHeightAboveGround(1500.0f)
	// Evasion configuration
	, EvasionTriggerDistance(500.0f)
	, EvasionSpeedMultiplier(1.8f)
	// Debug
	, bShowDebugVisualization(true)
	, DebugTextScale(1.0f)
	, DebugTextLineSpacing(20.0f)
	// Runtime state
	, CurrentDirection(FVector::ForwardVector)
	, DirectionChangeTimer(0.0f)
	, CurrentSpeed(800.0f)
	, bIsEvading(false)
{
	PrimaryActorTick.bCanEverTick = true;

	// Create collision sphere as root
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetSphereRadius(30.0f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionSphere->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	// Block WorldStatic (floors, walls) so we can detect them
	CollisionSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	CollisionSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECollisionResponse::ECR_Block);
	CollisionSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECollisionResponse::ECR_Overlap);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECollisionResponse::ECR_Overlap);
	RootComponent = CollisionSphere;

	// Create mesh component for visuals
	SnitchMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SnitchMesh"));
	SnitchMesh->SetupAttachment(RootComponent);
	SnitchMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Create floating pawn movement for smooth flying
	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComponent"));
	MovementComponent->MaxSpeed = MaxEvasionSpeed;
	MovementComponent->Acceleration = 2000.0f;
	MovementComponent->Deceleration = 1000.0f;
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void ASnitchBall::BeginPlay()
{
	Super::BeginPlay();

	// Initialize with random direction
	ChooseNewDirection();
	CurrentSpeed = BaseSpeed;

	UE_LOG(LogSnitch, Display,
		TEXT("[%s] Snitch spawned at %s | Speed: %.0f | Debug: %s"),
		*GetName(),
		*GetActorLocation().ToString(),
		CurrentSpeed,
		bShowDebugVisualization ? TEXT("ON") : TEXT("OFF"));
}

void ASnitchBall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update direction change timer
	DirectionChangeTimer -= DeltaTime;
	if (DirectionChangeTimer <= 0.0f)
	{
		ChooseNewDirection();
	}

	// Check for player proximity and update evasion state
	UpdateEvasionState();

	// Calculate avoidance forces from obstacles
	FVector AvoidanceForce = CalculateAvoidanceForce();
	FVector HeightCorrection = CalculateHeightCorrection();

	// Combine desired direction with avoidance
	FVector DesiredDirection = CurrentDirection + AvoidanceForce + HeightCorrection;
	DesiredDirection.Normalize();

	// Smoothly rotate toward desired direction
	FVector CurrentVelocity = MovementComponent->Velocity;
	if (!CurrentVelocity.IsNearlyZero())
	{
		FVector SmoothedDirection = FMath::VInterpNormalRotationTo(
			CurrentVelocity.GetSafeNormal(),
			DesiredDirection,
			DeltaTime,
			TurnRate
		);
		DesiredDirection = SmoothedDirection;
	}

	// Apply movement input
	AddMovementInput(DesiredDirection, CurrentSpeed / MovementComponent->MaxSpeed);

	// Debug visualization
	if (bShowDebugVisualization)
	{
		DrawDebugInfo();
	}
}

void ASnitchBall::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	// Snitch has no player input - AI controlled
}

// ============================================================================
// MOVEMENT FUNCTIONS
// ============================================================================

void ASnitchBall::ChooseNewDirection()
{
	// Random direction with bias toward horizontal movement
	float RandomYaw = FMath::RandRange(-180.0f, 180.0f);
	float RandomPitch = FMath::RandRange(-30.0f, 30.0f); // Less extreme vertical

	FRotator RandomRotation(RandomPitch, RandomYaw, 0.0f);
	CurrentDirection = RandomRotation.Vector();

	// Set timer for next direction change
	DirectionChangeTimer = DirectionChangeInterval +
		FMath::RandRange(-DirectionChangeVariance, DirectionChangeVariance);

	UE_LOG(LogSnitch, Verbose,
		TEXT("[%s] New direction: %s | Next change in: %.1fs"),
		*GetName(),
		*CurrentDirection.ToString(),
		DirectionChangeTimer);
}

FVector ASnitchBall::CalculateAvoidanceForce()
{
	FVector AvoidanceForce = FVector::ZeroVector;
	FVector CurrentLocation = GetActorLocation();
	FHitResult Hit;

	// Check multiple directions for obstacles
	// Forward
	if (TraceForObstacle(CurrentLocation, CurrentLocation + CurrentDirection * ObstacleCheckDistance, Hit))
	{
		// Push away from obstacle
		FVector AwayFromObstacle = Hit.Normal * ObstacleAvoidanceStrength;
		AvoidanceForce += AwayFromObstacle;

		if (bShowDebugVisualization)
		{
			DrawDebugLine(GetWorld(), Hit.ImpactPoint, Hit.ImpactPoint + Hit.Normal * 100.0f,
				FColor::Red, false, -1.0f, 0, 3.0f);
		}
	}

	// Down (floor check)
	FVector DownCheck = CurrentLocation + FVector(0, 0, -ObstacleCheckDistance);
	if (TraceForObstacle(CurrentLocation, DownCheck, Hit))
	{
		float DistanceToFloor = Hit.Distance;
		if (DistanceToFloor < MinHeightAboveGround)
		{
			// Strong upward force when too close to floor
			float Urgency = 1.0f - (DistanceToFloor / MinHeightAboveGround);
			AvoidanceForce += FVector::UpVector * ObstacleAvoidanceStrength * Urgency * 2.0f;
		}
	}

	// Side checks (left and right)
	FVector RightDir = FVector::CrossProduct(CurrentDirection, FVector::UpVector).GetSafeNormal();
	FVector LeftDir = -RightDir;

	if (TraceForObstacle(CurrentLocation, CurrentLocation + RightDir * ObstacleCheckDistance * 0.5f, Hit))
	{
		AvoidanceForce += LeftDir * ObstacleAvoidanceStrength * 0.5f;
	}
	if (TraceForObstacle(CurrentLocation, CurrentLocation + LeftDir * ObstacleCheckDistance * 0.5f, Hit))
	{
		AvoidanceForce += RightDir * ObstacleAvoidanceStrength * 0.5f;
	}

	return AvoidanceForce;
}

FVector ASnitchBall::CalculateHeightCorrection()
{
	FVector Correction = FVector::ZeroVector;
	FVector CurrentLocation = GetActorLocation();

	// Trace down to find ground
	FHitResult Hit;
	FVector TraceStart = CurrentLocation;
	FVector TraceEnd = CurrentLocation - FVector(0, 0, MaxHeightAboveGround + 500.0f);

	if (TraceForObstacle(TraceStart, TraceEnd, Hit))
	{
		float HeightAboveGround = Hit.Distance;

		// Too low - push up
		if (HeightAboveGround < MinHeightAboveGround)
		{
			float Urgency = 1.0f - (HeightAboveGround / MinHeightAboveGround);
			Correction.Z = Urgency * 2.0f;
		}
		// Too high - push down gently
		else if (HeightAboveGround > MaxHeightAboveGround)
		{
			float ExcessHeight = HeightAboveGround - MaxHeightAboveGround;
			Correction.Z = -FMath::Min(ExcessHeight / 500.0f, 1.0f);
		}
	}

	return Correction;
}

void ASnitchBall::UpdateEvasionState()
{
	// Find nearest player
	APawn* NearestPlayer = nullptr;
	float NearestDistance = EvasionTriggerDistance;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			if (APawn* PlayerPawn = PC->GetPawn())
			{
				float Distance = FVector::Dist(GetActorLocation(), PlayerPawn->GetActorLocation());
				if (Distance < NearestDistance)
				{
					NearestDistance = Distance;
					NearestPlayer = PlayerPawn;
				}
			}
		}
	}

	// Update evasion state and speed
	bool bWasEvading = bIsEvading;
	bIsEvading = (NearestPlayer != nullptr);

	if (bIsEvading)
	{
		// Speed up and steer away from player
		CurrentSpeed = FMath::Lerp(CurrentSpeed, BaseSpeed * EvasionSpeedMultiplier, 0.1f);

		// Add evasion direction (away from player)
		FVector AwayFromPlayer = (GetActorLocation() - NearestPlayer->GetActorLocation()).GetSafeNormal();
		CurrentDirection = FMath::VInterpNormalRotationTo(CurrentDirection, AwayFromPlayer, GetWorld()->DeltaTimeSeconds, TurnRate * 2.0f);
	}
	else
	{
		// Return to base speed
		CurrentSpeed = FMath::Lerp(CurrentSpeed, BaseSpeed, 0.05f);
	}

	// Log state changes
	if (bIsEvading != bWasEvading)
	{
		UE_LOG(LogSnitch, Display,
			TEXT("[%s] Evasion %s | Speed: %.0f"),
			*GetName(),
			bIsEvading ? TEXT("STARTED") : TEXT("ENDED"),
			CurrentSpeed);
	}
}

// ============================================================================
// DEBUG VISUALIZATION
// ============================================================================

void ASnitchBall::DrawDebugInfo()
{
	FVector Location = GetActorLocation();
	FVector Velocity = MovementComponent->Velocity;

	// Draw velocity vector (green)
	DrawDebugDirectionalArrow(GetWorld(),
		Location,
		Location + Velocity.GetSafeNormal() * 150.0f,
		50.0f,
		FColor::Green,
		false, -1.0f, 0, 3.0f);

	// Draw desired direction (yellow)
	DrawDebugDirectionalArrow(GetWorld(),
		Location,
		Location + CurrentDirection * 100.0f,
		30.0f,
		FColor::Yellow,
		false, -1.0f, 0, 2.0f);

	// Draw obstacle check rays (cyan)
	DrawDebugLine(GetWorld(),
		Location,
		Location + CurrentDirection * ObstacleCheckDistance,
		FColor::Cyan,
		false, -1.0f, 0, 1.0f);

	// Draw height zone (orange lines for min/max height)
	FHitResult Hit;
	FVector GroundTrace = Location - FVector(0, 0, MaxHeightAboveGround + 500.0f);
	if (TraceForObstacle(Location, GroundTrace, Hit))
	{
		FVector GroundPoint = Hit.ImpactPoint;

		// Min height line
		DrawDebugLine(GetWorld(),
			GroundPoint + FVector(-50, 0, MinHeightAboveGround),
			GroundPoint + FVector(50, 0, MinHeightAboveGround),
			FColor::Orange, false, -1.0f, 0, 2.0f);

		// Max height line
		DrawDebugLine(GetWorld(),
			GroundPoint + FVector(-50, 0, MaxHeightAboveGround),
			GroundPoint + FVector(50, 0, MaxHeightAboveGround),
			FColor::Purple, false, -1.0f, 0, 2.0f);
	}

	// On-screen debug text - properly spaced
	if (GEngine)
	{
		int32 LineIndex = 0;
		FColor TextColor = bIsEvading ? FColor::Red : FColor::Green;

		// Use unique keys for each line to prevent overlap
		GEngine->AddOnScreenDebugMessage(
			GetUniqueID() * 10 + LineIndex++,
			0.0f,
			TextColor,
			FString::Printf(TEXT("=== SNITCH DEBUG ==="))
		);

		GEngine->AddOnScreenDebugMessage(
			GetUniqueID() * 10 + LineIndex++,
			0.0f,
			FColor::White,
			FString::Printf(TEXT("Location: %s"), *Location.ToString())
		);

		GEngine->AddOnScreenDebugMessage(
			GetUniqueID() * 10 + LineIndex++,
			0.0f,
			FColor::White,
			FString::Printf(TEXT("Speed: %.0f / %.0f"), Velocity.Size(), CurrentSpeed)
		);

		GEngine->AddOnScreenDebugMessage(
			GetUniqueID() * 10 + LineIndex++,
			0.0f,
			FColor::White,
			FString::Printf(TEXT("Direction: %s"), *CurrentDirection.ToCompactString())
		);

		GEngine->AddOnScreenDebugMessage(
			GetUniqueID() * 10 + LineIndex++,
			0.0f,
			bIsEvading ? FColor::Red : FColor::Green,
			FString::Printf(TEXT("Evading: %s"), bIsEvading ? TEXT("YES") : TEXT("NO"))
		);

		GEngine->AddOnScreenDebugMessage(
			GetUniqueID() * 10 + LineIndex++,
			0.0f,
			FColor::Cyan,
			FString::Printf(TEXT("Next Dir Change: %.1fs"), DirectionChangeTimer)
		);
	}
}

bool ASnitchBall::TraceForObstacle(const FVector& Start, const FVector& End, FHitResult& OutHit)
{
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	// Trace against WorldStatic only (floors, walls, boundaries)
	return GetWorld()->LineTraceSingleByChannel(
		OutHit,
		Start,
		End,
		ECC_WorldStatic,
		QueryParams
	);
}

