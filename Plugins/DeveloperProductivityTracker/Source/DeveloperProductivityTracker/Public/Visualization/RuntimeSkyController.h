// ============================================================================
// RuntimeSkyController.h
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Runtime actor that drives sky visualization during gameplay.
// Use this for in-game demos and PIE preview of the productivity sky system.
//
// Usage:
// 1. Place BP_RuntimeSkyController in your level
// 2. Assign a ProductivitySkyConfig asset
// 3. Press Play - sky will cycle through time automatically
// 4. Use SetTimeOfDay() or TimeSpeed to control visualization
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RuntimeSkyController.generated.h"

class UProductivitySkyConfig;
class AProductivitySkyActor;
class ADirectionalLight;
class ASkyLight;
class ASkyAtmosphere;

// Delegate for time changes - lets other systems react
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRuntimeTimeChanged, float, TimeOfDay);

UCLASS(Blueprintable, BlueprintType)
class DEVELOPERPRODUCTIVITYTRACKER_API ARuntimeSkyController : public AActor
{
	GENERATED_BODY()

public:
	ARuntimeSkyController();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// ========================================================================
	// CONFIGURATION
	// ========================================================================

	// Sky configuration asset - assign in Blueprint
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sky Config")
	UProductivitySkyConfig* SkyConfig;

	// Enable automatic time progression
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Control")
	bool bAutoProgressTime;

	// Speed multiplier for time (1.0 = config speed, 10.0 = 10x faster for demos)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Control", meta = (ClampMin = "0.1", ClampMax = "100.0"))
	float TimeSpeed;

	// Current time of day (0.0 = midnight, 0.25 = dawn, 0.5 = noon, 0.75 = dusk)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Control", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CurrentTimeOfDay;

	// Simulate wellness states for demo
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wellness Demo")
	bool bSimulateWellnessChanges;

	// ========================================================================
	// REFERENCES - auto-found or manually assigned
	// ========================================================================

	// The sky actor to control (auto-found if not set)
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "References")
	AProductivitySkyActor* SkyActor;

	// Directional light to rotate with sun (auto-found if not set)
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "References")
	ADirectionalLight* SunLight;

	// Sky light to adjust intensity (auto-found if not set)
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "References")
	ASkyLight* SkyLight;

	// Sky atmosphere for color adjustments (auto-found if not set)
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "References")
	ASkyAtmosphere* SkyAtmosphere;

	// ========================================================================
	// METHODS
	// ========================================================================

	// Set time of day directly (useful for Blueprint scrubbing)
	UFUNCTION(BlueprintCallable, Category = "Time Control")
	void SetTimeOfDay(float NewTime);

	// Jump forward/backward in time
	UFUNCTION(BlueprintCallable, Category = "Time Control")
	void AdvanceTime(float Hours);

	// Get human-readable time string
	UFUNCTION(BlueprintPure, Category = "Time Control")
	FString GetTimeDisplayString() const;

	// Check if daytime
	UFUNCTION(BlueprintPure, Category = "Time Control")
	bool IsDaytime() const;

	// Force refresh of all visuals
	UFUNCTION(BlueprintCallable, Category = "Sky Control")
	void RefreshSkyVisuals();

	// Set wellness tint manually for demo
	UFUNCTION(BlueprintCallable, Category = "Wellness Demo")
	void SetWellnessTint(FLinearColor Tint);

	// Simulate different wellness states
	UFUNCTION(BlueprintCallable, Category = "Wellness Demo")
	void SimulateWellnessState(int32 StateIndex);

	// ========================================================================
	// EVENTS
	// ========================================================================

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnRuntimeTimeChanged OnTimeChanged;

protected:
	// Find references in the level
	void FindLevelReferences();

	// Update sun directional light rotation
	void UpdateSunLightRotation(float TimeOfDay);

	// Update sky light intensity
	void UpdateSkyLightIntensity(float TimeOfDay);

	// Update sky atmosphere colors
	void UpdateAtmosphereColors(float TimeOfDay);

private:
	float PreviousTimeOfDay;
	FLinearColor CurrentWellnessTint;
	float WellnessSimulationTimer;
	int32 CurrentWellnessState;
};
