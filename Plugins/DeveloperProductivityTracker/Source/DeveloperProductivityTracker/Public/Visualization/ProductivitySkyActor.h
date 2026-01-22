// ============================================================================
// ProductivitySkyActor.h
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Main actor that renders the productivity-aware sky visualization.
// Spawns and manages celestial bodies, sky dome, and atmosphere effects.
//
// Architecture:
// Composite actor managing multiple visual components.
// Subscribes to TimeOfDaySubsystem for state updates.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProductivitySkyActor.generated.h"

class UProductivitySkyConfig;
class UCelestialBodyComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UCLASS()
class DEVELOPERPRODUCTIVITYTRACKER_API AProductivitySkyActor : public AActor
{
	GENERATED_BODY()

public:
	AProductivitySkyActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// ========================================================================
	// CONFIGURATION
	// ========================================================================

	// Sky configuration asset
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Sky")
	UProductivitySkyConfig* SkyConfig;

	// ========================================================================
	// METHODS
	// ========================================================================

	// Apply a new sky configuration
	UFUNCTION(BlueprintCallable, Category = "Productivity|Sky")
	void ApplySkyConfig(UProductivitySkyConfig* Config);

	// Update all visuals for current time
	UFUNCTION(BlueprintCallable, Category = "Productivity|Sky")
	void UpdateForTimeOfDay(float TimeOfDay);

	// Apply wellness tint overlay
	UFUNCTION(BlueprintCallable, Category = "Productivity|Sky")
	void ApplyWellnessTint(const FLinearColor& Tint);

protected:
	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootSceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* SkyDomeMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCelestialBodyComponent* SunComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCelestialBodyComponent* BlueMoonComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UCelestialBodyComponent* OrangeMoonComponent;

private:
	// Materials
	UPROPERTY()
	UMaterialInstanceDynamic* SkyMaterial;

	// State
	float CurrentTimeOfDay;
	FLinearColor CurrentWellnessTint;

	// Initialization
	void InitializeComponents();
	void InitializeSkyDome();
	void InitializeCelestialBodies();
	void SubscribeToSubsystems();

	// Updates
	void UpdateSkyColors(float TimeOfDay);
	void UpdateCelestialPositions(float TimeOfDay);
	void UpdateStarVisibility(float TimeOfDay);

	// Event handlers
	UFUNCTION()
	void HandleTimeOfDayChanged(float NewTimeOfDay);
};
