// ============================================================================
// ProductivitySkyConfig.cpp
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Implementation of the productivity sky configuration data asset.
// ============================================================================

#include "Visualization/ProductivitySkyConfig.h"

DEFINE_LOG_CATEGORY(LogProductivitySky);

UProductivitySkyConfig::UProductivitySkyConfig()
	: WorkDayCycleDurationSeconds(28800.0f) // 8 hours
	, TimeScaleMultiplier(1.0f)
	, SessionStartTimeOfDay(0.25f) // Dawn/6am
	, SkyColorCurve(nullptr)
	, HorizonColorCurve(nullptr)
	, SkyBrightnessMultiplier(1.0f)
	, CloudCoverageCurve(nullptr)
	, SunColorCurve(nullptr)
	, SunBaseIntensity(10.0f)
	, SunIntensityCurve(nullptr)
	, SunriseTime(0.25f)
	, SunsetTime(0.75f)
	, SunDiskSize(2.0f)
	, BlueMoonColor(0.4f, 0.6f, 1.0f)
	, OrangeMoonColor(1.0f, 0.6f, 0.3f)
	, MoonEmissiveStrength(2.0f)
	, MoonScale(100.0f)
	, MoonOrbitRadius(5000.0f)
	, OrangeMoonPhaseOffset(0.33f)
	, MoonOrbitSpeedMultiplier(1.2f)
	, bEnableStars(true)
	, StarCount(500)
	, StarSize(4.0f)
	, StarsAppearTime(0.7f)
	, StarsDisappearTime(0.3f)
	, StarTwinkleSpeed(1.0f)
	, BreakApproachingTint(1.0f, 0.95f, 0.8f)
	, BreakRecommendedTint(1.0f, 0.85f, 0.6f)
	, BreakOverdueTint(1.0f, 0.7f, 0.5f)
	, OnBreakTint(0.8f, 0.9f, 1.0f)
	, WellnessTransitionDuration(30.0f)
	, AmbientColorCurve(nullptr)
	, AmbientIntensityCurve(nullptr)
{
}

FLinearColor UProductivitySkyConfig::GetSkyColorAtTime(float TimeOfDay) const
{
	if (SkyColorCurve)
	{
		return SkyColorCurve->GetLinearColorValue(TimeOfDay);
	}

	// Default fallback - simple day/night
	float DayFactor = FMath::Clamp(FMath::Sin(TimeOfDay * PI * 2.0f - PI * 0.5f) * 0.5f + 0.5f, 0.0f, 1.0f);

	FLinearColor NightColor(0.02f, 0.03f, 0.08f);
	FLinearColor DayColor(0.4f, 0.6f, 1.0f);

	return FMath::Lerp(NightColor, DayColor, DayFactor);
}

FLinearColor UProductivitySkyConfig::GetSunColorAtTime(float TimeOfDay) const
{
	if (SunColorCurve)
	{
		return SunColorCurve->GetLinearColorValue(TimeOfDay);
	}

	// Default fallback - warm at sunrise/sunset, white at noon
	float SunAngle = (TimeOfDay - 0.25f) / 0.5f; // 0 at sunrise, 1 at sunset

	if (SunAngle < 0.0f || SunAngle > 1.0f)
	{
		return FLinearColor::Black; // Sun not visible
	}

	// Warmer colors near horizon
	float HorizonFactor = 1.0f - FMath::Abs(SunAngle - 0.5f) * 2.0f;
	HorizonFactor = FMath::Pow(HorizonFactor, 2.0f);

	FLinearColor NoonColor(1.0f, 1.0f, 0.95f);
	FLinearColor HorizonColor(1.0f, 0.7f, 0.4f);

	return FMath::Lerp(HorizonColor, NoonColor, HorizonFactor);
}

float UProductivitySkyConfig::GetSunIntensityAtTime(float TimeOfDay) const
{
	if (SunIntensityCurve)
	{
		return SunIntensityCurve->GetFloatValue(TimeOfDay) * SunBaseIntensity;
	}

	// Default fallback - sine curve during day
	if (!IsSunVisibleAtTime(TimeOfDay))
	{
		return 0.0f;
	}

	float SunProgress = (TimeOfDay - SunriseTime) / (SunsetTime - SunriseTime);
	float SunHeight = FMath::Sin(SunProgress * PI);

	return SunHeight * SunBaseIntensity;
}

bool UProductivitySkyConfig::IsSunVisibleAtTime(float TimeOfDay) const
{
	return TimeOfDay >= SunriseTime && TimeOfDay <= SunsetTime;
}

float UProductivitySkyConfig::GetStarVisibilityAtTime(float TimeOfDay) const
{
	if (!bEnableStars)
	{
		return 0.0f;
	}

	// Stars visible from StarsAppearTime through midnight to StarsDisappearTime
	if (TimeOfDay >= StarsAppearTime)
	{
		// Evening fade in
		float FadeIn = (TimeOfDay - StarsAppearTime) / (1.0f - StarsAppearTime);
		return FMath::Clamp(FadeIn, 0.0f, 1.0f);
	}
	else if (TimeOfDay <= StarsDisappearTime)
	{
		// Morning fade out
		float FadeOut = 1.0f - (TimeOfDay / StarsDisappearTime);
		return FMath::Clamp(FadeOut, 0.0f, 1.0f);
	}

	return 0.0f;
}
