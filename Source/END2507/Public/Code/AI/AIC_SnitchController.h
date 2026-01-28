// ============================================================================
// AIC_SnitchController.h
// Developer: Marcus Daley
// Date: January 24, 2026
// Project: END2507
// ============================================================================
// Purpose:
// Lightweight AI Controller for the Golden Snitch. Uses perception to detect
// Seekers (event-driven) instead of polling every tick.
//
// Architecture:
// - Inherits from AAIController for perception support
// - 360-degree sight (peripheral vision = 180)
// - Detects all affiliations (enemies, friendlies, neutrals)
// - Broadcasts OnPursuerDetected/OnPursuerLost to SnitchBall
// - NO behavior tree - Snitch handles its own movement
//
// Usage:
// 1. SnitchBall sets AIControllerClass = AIC_SnitchController
// 2. Controller auto-possesses on spawn
// 3. SnitchBall binds to controller's perception delegates
// 4. Movement logic stays in SnitchBall::UpdateMovement()
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "AIC_SnitchController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;

// Delegate for Snitch to respond to perception events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPursuerPerceptionChanged, AActor*, Pursuer);

DECLARE_LOG_CATEGORY_EXTERN(LogSnitchController, Log, All);

UCLASS()
class END2507_API AAIC_SnitchController : public AAIController
{
	GENERATED_BODY()

public:
	AAIC_SnitchController();

	// ========================================================================
	// DELEGATES - Observer Pattern for SnitchBall to bind
	// ========================================================================

	// Broadcast when a new pursuer enters detection range
	UPROPERTY(BlueprintAssignable, Category = "Snitch|Perception")
	FOnPursuerPerceptionChanged OnPursuerDetected;

	// Broadcast when a pursuer leaves detection range
	UPROPERTY(BlueprintAssignable, Category = "Snitch|Perception")
	FOnPursuerPerceptionChanged OnPursuerLost;

	// ========================================================================
	// PUBLIC API
	// ========================================================================

	// Get all currently tracked pursuers
	UFUNCTION(BlueprintPure, Category = "Snitch|Perception")
	TArray<AActor*> GetCurrentPursuers() const { return TrackedPursuers; }

	// Get count of active pursuers
	UFUNCTION(BlueprintPure, Category = "Snitch|Perception")
	int32 GetPursuerCount() const { return TrackedPursuers.Num(); }

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;

	// ========================================================================
	// COMPONENTS
	// ========================================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Snitch|Perception")
	UAIPerceptionComponent* AIPerceptionComp;

	UPROPERTY()
	UAISenseConfig_Sight* SightConfig;

	// ========================================================================
	// PERCEPTION CONFIGURATION
	// ========================================================================

	// How far Snitch can see pursuers
	UPROPERTY(EditDefaultsOnly, Category = "Snitch|Perception")
	float DetectionRadius;

	// Slightly larger to prevent flickering
	UPROPERTY(EditDefaultsOnly, Category = "Snitch|Perception")
	float LoseDetectionRadius;

	// Tags that identify valid pursuers (designer-configurable)
	// Default: "Seeker", "Flying", "Player"
	UPROPERTY(EditDefaultsOnly, Category = "Snitch|Perception")
	TArray<FName> ValidPursuerTags;

private:
	// Currently tracked pursuers (maintained by perception events)
	UPROPERTY()
	TArray<AActor*> TrackedPursuers;

	// Perception callback - Nick Penney pattern
	UFUNCTION()
	void HandlePerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	// Check if actor qualifies as a pursuer
	bool IsPursuer(AActor* Actor) const;

	void SetupPerception();
};
