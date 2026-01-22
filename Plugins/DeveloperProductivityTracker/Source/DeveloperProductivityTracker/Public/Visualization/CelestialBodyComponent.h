// ============================================================================
// CelestialBodyComponent.h
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Component for rendering celestial bodies (sun, moons) in the sky.
// Handles orbital motion and visual appearance based on time of day.
//
// Architecture:
// Scene Component with mesh and material control.
// Driven by TimeOfDaySubsystem for position updates.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "CelestialBodyComponent.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

// Type of celestial body
UENUM(BlueprintType)
enum class ECelestialBodyType : uint8
{
	Sun         UMETA(DisplayName = "Sun"),
	BlueMoon    UMETA(DisplayName = "Blue Moon"),
	OrangeMoon  UMETA(DisplayName = "Orange Moon")
};

UCLASS(ClassGroup = (Productivity), meta = (BlueprintSpawnableComponent))
class DEVELOPERPRODUCTIVITYTRACKER_API UCelestialBodyComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UCelestialBodyComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ========================================================================
	// CONFIGURATION
	// ========================================================================

	// Type of this celestial body
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Celestial")
	ECelestialBodyType BodyType;

	// Base color of the body
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Celestial")
	FLinearColor BaseColor;

	// Emissive strength multiplier
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Celestial",
		meta = (ClampMin = "0.0", ClampMax = "20.0"))
	float EmissiveStrength;

	// Visual scale
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Celestial",
		meta = (ClampMin = "1.0", ClampMax = "1000.0"))
	float BodyScale;

	// Orbit radius from center
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Celestial|Orbit",
		meta = (ClampMin = "100.0", ClampMax = "50000.0"))
	float OrbitRadius;

	// Phase offset (0.0-1.0)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Celestial|Orbit",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PhaseOffset;

	// Orbit speed multiplier
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Celestial|Orbit",
		meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float OrbitSpeedMultiplier;

	// ========================================================================
	// METHODS
	// ========================================================================

	// Update position based on time of day
	UFUNCTION(BlueprintCallable, Category = "Celestial")
	void UpdatePosition(float TimeOfDay);

	// Set visibility with fade
	UFUNCTION(BlueprintCallable, Category = "Celestial")
	void SetVisibilitySmooth(bool bNewVisibility, float FadeDuration = 1.0f);

	// Get current visibility alpha
	UFUNCTION(BlueprintPure, Category = "Celestial")
	float GetCurrentAlpha() const { return CurrentAlpha; }

private:
	// Visual components
	UPROPERTY()
	UStaticMeshComponent* MeshComponent;

	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMaterial;

	// State
	float CurrentAlpha;
	float TargetAlpha;
	float FadeSpeed;

	// Methods
	void InitializeVisuals();
	void UpdateMaterial();
	FVector CalculateOrbitalPosition(float TimeOfDay) const;
};
