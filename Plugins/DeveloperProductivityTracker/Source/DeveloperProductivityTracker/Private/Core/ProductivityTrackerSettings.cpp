// ============================================================================
// ProductivityTrackerSettings.cpp
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Implementation of user-configurable settings for the productivity tracker.
// Handles settings validation and change notifications.
// ============================================================================

#include "Core/ProductivityTrackerSettings.h"

DEFINE_LOG_CATEGORY(LogProductivitySettings);

UProductivityTrackerSettings::UProductivityTrackerSettings()
	: bAutoStartSession(true)
	, SnapshotIntervalSeconds(30.0f)
	, ThinkingThresholdSeconds(120.0f)
	, AwayThresholdSeconds(300.0f)
	, bAutoRecoverSessions(true)
	, bEnableExternalMonitoring(false)
	, ProcessScanIntervalSeconds(5.0f)
	, bEnableFileMonitoring(false)
	, RecentModificationThresholdSeconds(120.0f)
	, bEnableWellnessFeatures(true)
	, bEnablePomodoro(true)
	, PomodoroWorkMinutes(25.0f)
	, PomodoroShortBreakMinutes(5.0f)
	, PomodoroLongBreakMinutes(15.0f)
	, PomodoroIntervalsBeforeLongBreak(4)
	, bEnableSmartBreakDetection(true)
	, bEnableStretchReminders(true)
	, StretchReminderIntervalMinutes(45.0f)
	, bEnableSkyVisualization(true)
	, bEnableWellnessAtmosphere(true)
	, bEnableNotifications(true)
	, bEnableNotificationSounds(false)
	, NotificationDurationSeconds(5.0f)
	, bStoreApplicationNames(true)
	, bStoreFilePaths(false)
	, DataRetentionDays(30)
	, bEnableChecksumVerification(true)
	, bWarnOnTamperDetection(true)
	, bIncludeSnapshotsInExport(false)
{
	UE_LOG(LogProductivitySettings, Log, TEXT("ProductivityTrackerSettings constructed with default values"));
}

UProductivityTrackerSettings* UProductivityTrackerSettings::Get()
{
	return GetMutableDefault<UProductivityTrackerSettings>();
}

#if WITH_EDITOR

FText UProductivityTrackerSettings::GetSectionText() const
{
	return NSLOCTEXT("ProductivityTracker", "SettingsSectionText", "Developer Productivity Tracker");
}

FText UProductivityTrackerSettings::GetSectionDescription() const
{
	return NSLOCTEXT("ProductivityTracker", "SettingsSectionDescription",
		"Configure session tracking, external monitoring, wellness features, and visualization settings.");
}

void UProductivityTrackerSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.Property
		? PropertyChangedEvent.Property->GetFName()
		: NAME_None;

	UE_LOG(LogProductivitySettings, Log, TEXT("Settings property changed: %s"), *PropertyName.ToString());

	// Validate settings after any change
	ValidateSettings();

	// Broadcast change notification
	OnSettingsChanged.Broadcast();

	// Save config immediately
	SaveConfig();
}

#endif

void UProductivityTrackerSettings::ValidateSettings()
{
	// Ensure away threshold is greater than thinking threshold
	if (AwayThresholdSeconds <= ThinkingThresholdSeconds)
	{
		AwayThresholdSeconds = ThinkingThresholdSeconds + 60.0f;
		UE_LOG(LogProductivitySettings, Warning,
			TEXT("Away threshold must be greater than thinking threshold. Adjusted to %.1f seconds."),
			AwayThresholdSeconds);
	}

	// Ensure Pomodoro long break is longer than short break
	if (PomodoroLongBreakMinutes <= PomodoroShortBreakMinutes)
	{
		PomodoroLongBreakMinutes = PomodoroShortBreakMinutes * 3.0f;
		UE_LOG(LogProductivitySettings, Warning,
			TEXT("Long break must be longer than short break. Adjusted to %.1f minutes."),
			PomodoroLongBreakMinutes);
	}

	// Ensure work interval is at least twice the short break
	if (PomodoroWorkMinutes < PomodoroShortBreakMinutes * 2.0f)
	{
		UE_LOG(LogProductivitySettings, Warning,
			TEXT("Work interval (%.1f min) is unusually short compared to break (%.1f min)."),
			PomodoroWorkMinutes, PomodoroShortBreakMinutes);
	}

	// Validate data retention
	if (DataRetentionDays < 7)
	{
		DataRetentionDays = 7;
		UE_LOG(LogProductivitySettings, Warning,
			TEXT("Data retention must be at least 7 days. Adjusted to %d days."),
			DataRetentionDays);
	}

	// Validate snapshot interval
	if (SnapshotIntervalSeconds < 5.0f)
	{
		SnapshotIntervalSeconds = 5.0f;
		UE_LOG(LogProductivitySettings, Warning,
			TEXT("Snapshot interval too low. Adjusted to %.1f seconds for performance."),
			SnapshotIntervalSeconds);
	}

	// Log current configuration summary
	UE_LOG(LogProductivitySettings, Verbose,
		TEXT("Settings validated - AutoStart: %s, External: %s, Wellness: %s, Sky: %s"),
		bAutoStartSession ? TEXT("Yes") : TEXT("No"),
		bEnableExternalMonitoring ? TEXT("Yes") : TEXT("No"),
		bEnableWellnessFeatures ? TEXT("Yes") : TEXT("No"),
		bEnableSkyVisualization ? TEXT("Yes") : TEXT("No"));
}
