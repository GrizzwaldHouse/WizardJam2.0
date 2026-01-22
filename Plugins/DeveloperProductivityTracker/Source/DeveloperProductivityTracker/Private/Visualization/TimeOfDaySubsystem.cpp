// ============================================================================
// TimeOfDaySubsystem.cpp
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Implementation of time-of-day management for sky visualization.
// ============================================================================

#include "Visualization/TimeOfDaySubsystem.h"
#include "Visualization/ProductivitySkyConfig.h"
#include "Core/SessionTrackingSubsystem.h"
#include "Core/ProductivityTrackerSettings.h"
#include "Wellness/BreakWellnessSubsystem.h"

UTimeOfDaySubsystem::UTimeOfDaySubsystem()
	: bIsEnabled(true)
	, CurrentTimeOfDay(0.25f)
	, PreviousTimeOfDay(0.25f)
	, CurrentWellnessTint(FLinearColor::White)
	, TargetWellnessTint(FLinearColor::White)
	, WellnessTintTransitionProgress(1.0f)
	, SkyConfig(nullptr)
{
}

void UTimeOfDaySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Load settings
	const UProductivityTrackerSettings* Settings = UProductivityTrackerSettings::Get();
	if (Settings)
	{
		bIsEnabled = Settings->bEnableSkyVisualization;

		// Load default sky config if specified
		if (!Settings->DefaultSkyConfigPath.IsNull())
		{
			SkyConfig = Cast<UProductivitySkyConfig>(Settings->DefaultSkyConfigPath.TryLoad());
		}
	}

	// Subscribe to wellness status changes
	UBreakWellnessSubsystem* WellnessSubsystem = GEditor->GetEditorSubsystem<UBreakWellnessSubsystem>();
	if (WellnessSubsystem)
	{
		WellnessSubsystem->OnWellnessStatusChanged.AddDynamic(this, &UTimeOfDaySubsystem::HandleWellnessStatusChanged);
	}

	// Set initial time
	if (SkyConfig)
	{
		CurrentTimeOfDay = SkyConfig->SessionStartTimeOfDay;
	}

	UE_LOG(LogProductivitySky, Log, TEXT("TimeOfDaySubsystem initialized (Enabled: %s)"),
		bIsEnabled ? TEXT("Yes") : TEXT("No"));
}

void UTimeOfDaySubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UTimeOfDaySubsystem::Tick(float DeltaTime)
{
	if (!bIsEnabled)
	{
		return;
	}

	UpdateTimeOfDay(DeltaTime);
	UpdateWellnessTint(DeltaTime);
}

TStatId UTimeOfDaySubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTimeOfDaySubsystem, STATGROUP_Tickables);
}

FString UTimeOfDaySubsystem::GetTimeDisplayString() const
{
	// Convert 0-1 time to 24-hour format
	float Hours24 = CurrentTimeOfDay * 24.0f;
	int32 Hours = FMath::FloorToInt(Hours24);
	int32 Minutes = FMath::FloorToInt(FMath::Fmod(Hours24, 1.0f) * 60.0f);

	// Convert to 12-hour format
	bool bIsPM = Hours >= 12;
	int32 Hours12 = Hours % 12;
	if (Hours12 == 0) Hours12 = 12;

	return FString::Printf(TEXT("%d:%02d %s"), Hours12, Minutes, bIsPM ? TEXT("PM") : TEXT("AM"));
}

bool UTimeOfDaySubsystem::IsDaytime() const
{
	if (SkyConfig)
	{
		return SkyConfig->IsSunVisibleAtTime(CurrentTimeOfDay);
	}
	return CurrentTimeOfDay > 0.25f && CurrentTimeOfDay < 0.75f;
}

void UTimeOfDaySubsystem::SetSkyConfig(UProductivitySkyConfig* Config)
{
	SkyConfig = Config;
	UE_LOG(LogProductivitySky, Log, TEXT("Sky config set: %s"),
		Config ? *Config->GetName() : TEXT("None"));
}

void UTimeOfDaySubsystem::SetEnabled(bool bEnabled)
{
	bIsEnabled = bEnabled;
}

void UTimeOfDaySubsystem::SetTimeOfDay(float Time)
{
	CurrentTimeOfDay = FMath::Fmod(Time, 1.0f);
	if (CurrentTimeOfDay < 0.0f)
	{
		CurrentTimeOfDay += 1.0f;
	}

	OnTimeOfDayChanged.Broadcast(CurrentTimeOfDay);
}

void UTimeOfDaySubsystem::ResetToSessionStart()
{
	if (SkyConfig)
	{
		CurrentTimeOfDay = SkyConfig->SessionStartTimeOfDay;
	}
	else
	{
		CurrentTimeOfDay = 0.25f;
	}

	OnTimeOfDayChanged.Broadcast(CurrentTimeOfDay);
}

void UTimeOfDaySubsystem::UpdateTimeOfDay(float DeltaTime)
{
	if (!SkyConfig)
	{
		return;
	}

	// Get elapsed session time
	USessionTrackingSubsystem* SessionSubsystem = GEditor->GetEditorSubsystem<USessionTrackingSubsystem>();
	if (!SessionSubsystem || !SessionSubsystem->IsSessionActive())
	{
		return;
	}

	float ElapsedSeconds = SessionSubsystem->GetElapsedSeconds();

	// Map session time to time of day
	float CycleDuration = SkyConfig->WorkDayCycleDurationSeconds;
	float TimeScale = SkyConfig->TimeScaleMultiplier;

	float ScaledElapsed = ElapsedSeconds * TimeScale;
	float TimeProgress = FMath::Fmod(ScaledElapsed / CycleDuration, 1.0f);

	PreviousTimeOfDay = CurrentTimeOfDay;
	CurrentTimeOfDay = FMath::Fmod(SkyConfig->SessionStartTimeOfDay + TimeProgress, 1.0f);

	// Broadcast if changed significantly
	if (FMath::Abs(CurrentTimeOfDay - PreviousTimeOfDay) > 0.001f)
	{
		OnTimeOfDayChanged.Broadcast(CurrentTimeOfDay);
	}
}

void UTimeOfDaySubsystem::UpdateWellnessTint(float DeltaTime)
{
	if (!SkyConfig)
	{
		return;
	}

	// Interpolate toward target tint
	if (WellnessTintTransitionProgress < 1.0f)
	{
		float TransitionSpeed = 1.0f / SkyConfig->WellnessTransitionDuration;
		WellnessTintTransitionProgress = FMath::Min(1.0f, WellnessTintTransitionProgress + DeltaTime * TransitionSpeed);

		CurrentWellnessTint = FMath::Lerp(CurrentWellnessTint, TargetWellnessTint, WellnessTintTransitionProgress);
	}
}

void UTimeOfDaySubsystem::HandleWellnessStatusChanged(EWellnessStatus NewStatus)
{
	TargetWellnessTint = GetTintForWellnessStatus(NewStatus);
	WellnessTintTransitionProgress = 0.0f;

	UE_LOG(LogProductivitySky, Verbose, TEXT("Wellness tint transitioning for status: %s"),
		*UEnum::GetValueAsString(NewStatus));
}

FLinearColor UTimeOfDaySubsystem::GetTintForWellnessStatus(EWellnessStatus Status) const
{
	if (!SkyConfig)
	{
		return FLinearColor::White;
	}

	switch (Status)
	{
	case EWellnessStatus::NeedBreak:
		return SkyConfig->BreakApproachingTint;
	case EWellnessStatus::Overworked:
		return SkyConfig->BreakOverdueTint;
	case EWellnessStatus::OnBreak:
		return SkyConfig->OnBreakTint;
	case EWellnessStatus::Optimal:
	case EWellnessStatus::Good:
	default:
		return FLinearColor::White;
	}
}
