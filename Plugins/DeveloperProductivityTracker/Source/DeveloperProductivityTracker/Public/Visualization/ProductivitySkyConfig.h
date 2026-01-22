// ============================================================================
// ProductivitySkyConfig.h
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Data Asset for configuring the productivity sky visualization.
// Designers can create different themes without touching code.
//
// Architecture:
// UDataAsset for editor-configurable settings.
// Curve assets for smooth time-based transitions.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Curves/CurveLinearColor.h"
#include "Curves/CurveFloat.h"
#include "ProductivitySkyConfig.generated.h"

// Log category
DECLARE_LOG_CATEGORY_EXTERN(LogProductivitySky, Log, All);

UCLASS(BlueprintType)
class DEVELOPERPRODUCTIVITYTRACKER_API UProductivitySkyConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	UProductivitySkyConfig();

	// ========================================================================
	// TIME CONFIGURATION
	// ========================================================================

	// Duration of full day cycle in seconds (28800 = 8 hours)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Time",
		meta = (ClampMin = "60.0", ClampMax = "86400.0"))
	float WorkDayCycleDurationSeconds;

	// Time scale multiplier (1.0 = real-time)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Time",
		meta = (ClampMin = "0.1", ClampMax = "10.0"))
	float TimeScaleMultiplier;

	// Starting time when session begins (0.25 = dawn/6am)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Time",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SessionStartTimeOfDay;

	// ========================================================================
	// SKY COLORS
	// ========================================================================

	// Color of the upper sky dome over time (0=midnight, 0.5=noon)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sky")
	UCurveLinearColor* SkyColorCurve;

	// Color of the horizon over time
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sky")
	UCurveLinearColor* HorizonColorCurve;

	// Overall sky brightness multiplier
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sky",
		meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float SkyBrightnessMultiplier;

	// Cloud coverage over time (0=clear, 1=overcast)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sky")
	UCurveFloat* CloudCoverageCurve;

	// ========================================================================
	// SUN CONFIGURATION
	// ========================================================================

	// Sun color over time
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sun")
	UCurveLinearColor* SunColorCurve;

	// Base sun intensity
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sun",
		meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float SunBaseIntensity;

	// Sun intensity curve over time
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sun")
	UCurveFloat* SunIntensityCurve;

	// Time of sunrise (0.0-0.5)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sun",
		meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float SunriseTime;

	// Time of sunset (0.5-1.0)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sun",
		meta = (ClampMin = "0.5", ClampMax = "1.0"))
	float SunsetTime;

	// Sun disk size in degrees
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sun",
		meta = (ClampMin = "0.5", ClampMax = "10.0"))
	float SunDiskSize;

	// ========================================================================
	// MOON CONFIGURATION
	// ========================================================================

	// Blue moon base color
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Moons")
	FLinearColor BlueMoonColor;

	// Orange moon base color
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Moons")
	FLinearColor OrangeMoonColor;

	// Moon emissive strength multiplier
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Moons",
		meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float MoonEmissiveStrength;

	// Moon visual scale
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Moons",
		meta = (ClampMin = "10.0", ClampMax = "1000.0"))
	float MoonScale;

	// Distance of moon from sky center
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Moons",
		meta = (ClampMin = "1000.0", ClampMax = "10000.0"))
	float MoonOrbitRadius;

	// Phase offset between the two moons (0.0-1.0)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Moons",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float OrangeMoonPhaseOffset;

	// Moon orbit speed relative to sun
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Moons",
		meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float MoonOrbitSpeedMultiplier;

	// ========================================================================
	// STAR CONFIGURATION
	// ========================================================================

	// Enable star rendering
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stars")
	bool bEnableStars;

	// Number of stars to render
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stars",
		meta = (ClampMin = "100", ClampMax = "2000", EditCondition = "bEnableStars"))
	int32 StarCount;

	// Base star size in screen units
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stars",
		meta = (ClampMin = "1.0", ClampMax = "20.0", EditCondition = "bEnableStars"))
	float StarSize;

	// Time of day when stars start appearing (0.0-1.0)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stars",
		meta = (ClampMin = "0.5", ClampMax = "1.0", EditCondition = "bEnableStars"))
	float StarsAppearTime;

	// Time of day when stars fully disappear (0.0-1.0)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stars",
		meta = (ClampMin = "0.0", ClampMax = "0.5", EditCondition = "bEnableStars"))
	float StarsDisappearTime;

	// Star twinkle speed
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stars",
		meta = (ClampMin = "0.0", ClampMax = "5.0", EditCondition = "bEnableStars"))
	float StarTwinkleSpeed;

	// ========================================================================
	// WELLNESS ATMOSPHERE STATES
	// ========================================================================

	// Sky tint when break is approaching
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wellness")
	FLinearColor BreakApproachingTint;

	// Sky tint when break is recommended
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wellness")
	FLinearColor BreakRecommendedTint;

	// Sky tint when break is overdue
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wellness")
	FLinearColor BreakOverdueTint;

	// Tint when on a break (calm/peaceful)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wellness")
	FLinearColor OnBreakTint;

	// Transition duration for wellness tints (seconds)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wellness",
		meta = (ClampMin = "1.0", ClampMax = "120.0"))
	float WellnessTransitionDuration;

	// ========================================================================
	// AMBIENT LIGHTING
	// ========================================================================

	// Ambient light color over time
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ambient")
	UCurveLinearColor* AmbientColorCurve;

	// Ambient intensity over time
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ambient")
	UCurveFloat* AmbientIntensityCurve;

	// ========================================================================
	// HELPER METHODS
	// ========================================================================

	// Sample sky color at a given time of day
	UFUNCTION(BlueprintPure, Category = "Sky")
	FLinearColor GetSkyColorAtTime(float TimeOfDay) const;

	// Sample sun color at a given time of day
	UFUNCTION(BlueprintPure, Category = "Sky")
	FLinearColor GetSunColorAtTime(float TimeOfDay) const;

	// Get sun intensity at a given time of day
	UFUNCTION(BlueprintPure, Category = "Sky")
	float GetSunIntensityAtTime(float TimeOfDay) const;

	// Check if sun is visible at a given time
	UFUNCTION(BlueprintPure, Category = "Sky")
	bool IsSunVisibleAtTime(float TimeOfDay) const;

	// Get star visibility alpha at a given time
	UFUNCTION(BlueprintPure, Category = "Sky")
	float GetStarVisibilityAtTime(float TimeOfDay) const;
};
