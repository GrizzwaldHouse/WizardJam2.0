// ============================================================================
// ProductivityTrackerSettings.h
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// User-configurable settings for the Developer Productivity Tracker plugin.
// Settings are exposed through the Project Settings panel in the editor.
//
// Architecture:
// UDeveloperSettings subclass that integrates with Unreal's settings system.
// All settings use EditDefaultsOnly - configured in Project Settings, not instances.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ProductivityTrackerSettings.generated.h"

// Log category for settings operations
DECLARE_LOG_CATEGORY_EXTERN(LogProductivitySettings, Log, All);

// Delegate broadcast when settings change
DECLARE_MULTICAST_DELEGATE(FOnProductivitySettingsChanged);

UCLASS(Config = Editor, DefaultConfig, meta = (DisplayName = "Developer Productivity Tracker"))
class DEVELOPERPRODUCTIVITYTRACKER_API UProductivityTrackerSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UProductivityTrackerSettings();

	// ========================================================================
	// SESSION TRACKING SETTINGS
	// ========================================================================

	// Enable automatic session tracking when the editor starts
	UPROPERTY(Config, EditDefaultsOnly, Category = "Session Tracking")
	bool bAutoStartSession;

	// Interval in seconds between activity snapshots
	UPROPERTY(Config, EditDefaultsOnly, Category = "Session Tracking",
		meta = (ClampMin = "5.0", ClampMax = "300.0", Units = "Seconds"))
	float SnapshotIntervalSeconds;

	// Seconds of inactivity before marking state as "Thinking"
	UPROPERTY(Config, EditDefaultsOnly, Category = "Session Tracking",
		meta = (ClampMin = "30.0", ClampMax = "600.0", Units = "Seconds"))
	float ThinkingThresholdSeconds;

	// Seconds of inactivity before marking state as "Away"
	UPROPERTY(Config, EditDefaultsOnly, Category = "Session Tracking",
		meta = (ClampMin = "60.0", ClampMax = "1800.0", Units = "Seconds"))
	float AwayThresholdSeconds;

	// Automatically recover incomplete sessions from crashes
	UPROPERTY(Config, EditDefaultsOnly, Category = "Session Tracking")
	bool bAutoRecoverSessions;

	// ========================================================================
	// EXTERNAL ACTIVITY MONITORING
	// ========================================================================

	// Enable monitoring of external development applications
	UPROPERTY(Config, EditDefaultsOnly, Category = "External Monitoring")
	bool bEnableExternalMonitoring;

	// Interval in seconds between process scans
	UPROPERTY(Config, EditDefaultsOnly, Category = "External Monitoring",
		meta = (ClampMin = "1.0", ClampMax = "30.0", Units = "Seconds", EditCondition = "bEnableExternalMonitoring"))
	float ProcessScanIntervalSeconds;

	// Enable file system monitoring for source file changes
	UPROPERTY(Config, EditDefaultsOnly, Category = "External Monitoring",
		meta = (EditCondition = "bEnableExternalMonitoring"))
	bool bEnableFileMonitoring;

	// How long a file change is considered "recent" for activity detection
	UPROPERTY(Config, EditDefaultsOnly, Category = "External Monitoring",
		meta = (ClampMin = "30.0", ClampMax = "600.0", Units = "Seconds", EditCondition = "bEnableFileMonitoring"))
	float RecentModificationThresholdSeconds;

	// Additional directories to monitor for file changes (beyond project Source)
	UPROPERTY(Config, EditDefaultsOnly, Category = "External Monitoring",
		meta = (EditCondition = "bEnableFileMonitoring"))
	TArray<FDirectoryPath> AdditionalSourceDirectories;

	// ========================================================================
	// WELLNESS SETTINGS
	// ========================================================================

	// Enable break and wellness features
	UPROPERTY(Config, EditDefaultsOnly, Category = "Wellness")
	bool bEnableWellnessFeatures;

	// Enable Pomodoro timer functionality
	UPROPERTY(Config, EditDefaultsOnly, Category = "Wellness|Pomodoro",
		meta = (EditCondition = "bEnableWellnessFeatures"))
	bool bEnablePomodoro;

	// Work interval duration in minutes
	UPROPERTY(Config, EditDefaultsOnly, Category = "Wellness|Pomodoro",
		meta = (ClampMin = "15.0", ClampMax = "60.0", Units = "Minutes", EditCondition = "bEnablePomodoro"))
	float PomodoroWorkMinutes;

	// Short break duration in minutes
	UPROPERTY(Config, EditDefaultsOnly, Category = "Wellness|Pomodoro",
		meta = (ClampMin = "3.0", ClampMax = "15.0", Units = "Minutes", EditCondition = "bEnablePomodoro"))
	float PomodoroShortBreakMinutes;

	// Long break duration in minutes
	UPROPERTY(Config, EditDefaultsOnly, Category = "Wellness|Pomodoro",
		meta = (ClampMin = "10.0", ClampMax = "45.0", Units = "Minutes", EditCondition = "bEnablePomodoro"))
	float PomodoroLongBreakMinutes;

	// Number of work intervals before a long break
	UPROPERTY(Config, EditDefaultsOnly, Category = "Wellness|Pomodoro",
		meta = (ClampMin = "2", ClampMax = "8", EditCondition = "bEnablePomodoro"))
	int32 PomodoroIntervalsBeforeLongBreak;

	// Enable smart break detection (auto-detect when user steps away)
	UPROPERTY(Config, EditDefaultsOnly, Category = "Wellness|Breaks",
		meta = (EditCondition = "bEnableWellnessFeatures"))
	bool bEnableSmartBreakDetection;

	// Enable stretch reminders
	UPROPERTY(Config, EditDefaultsOnly, Category = "Wellness|Stretching",
		meta = (EditCondition = "bEnableWellnessFeatures"))
	bool bEnableStretchReminders;

	// Interval between stretch reminders in minutes
	UPROPERTY(Config, EditDefaultsOnly, Category = "Wellness|Stretching",
		meta = (ClampMin = "15.0", ClampMax = "120.0", Units = "Minutes", EditCondition = "bEnableStretchReminders"))
	float StretchReminderIntervalMinutes;

	// Show popup window for stretch exercises (vs notification only)
	UPROPERTY(Config, EditDefaultsOnly, Category = "Wellness|Stretching",
		meta = (EditCondition = "bEnableStretchReminders"))
	bool bShowExercisePopup;

	// ========================================================================
	// HABIT STREAK SETTINGS
	// ========================================================================

	// Enable habit streak tracking
	UPROPERTY(Config, EditDefaultsOnly, Category = "Wellness|Habits",
		meta = (EditCondition = "bEnableWellnessFeatures"))
	bool bEnableHabitStreaks;

	// Daily stretch goal for streaks
	UPROPERTY(Config, EditDefaultsOnly, Category = "Wellness|Habits",
		meta = (ClampMin = "1", ClampMax = "20", EditCondition = "bEnableHabitStreaks"))
	int32 DailyStretchGoal;

	// Daily break goal for streaks
	UPROPERTY(Config, EditDefaultsOnly, Category = "Wellness|Habits",
		meta = (ClampMin = "1", ClampMax = "20", EditCondition = "bEnableHabitStreaks"))
	int32 DailyBreakGoal;

	// Daily Pomodoro goal for streaks
	UPROPERTY(Config, EditDefaultsOnly, Category = "Wellness|Habits",
		meta = (ClampMin = "1", ClampMax = "16", EditCondition = "bEnableHabitStreaks"))
	int32 DailyPomodoroGoal;

	// ========================================================================
	// HTTP API SETTINGS
	// ========================================================================

	// Enable HTTP API for external tool integration
	UPROPERTY(Config, EditDefaultsOnly, Category = "Wellness|API",
		meta = (EditCondition = "bEnableWellnessFeatures"))
	bool bEnableHttpApi;

	// Port for the HTTP API server
	UPROPERTY(Config, EditDefaultsOnly, Category = "Wellness|API",
		meta = (ClampMin = "1024", ClampMax = "65535", EditCondition = "bEnableHttpApi"))
	int32 HttpApiPort;

	// ========================================================================
	// VISUALIZATION SETTINGS
	// ========================================================================

	// Enable atmospheric sky visualization
	UPROPERTY(Config, EditDefaultsOnly, Category = "Visualization")
	bool bEnableSkyVisualization;

	// Enable wellness atmosphere effects (tint changes based on break status)
	UPROPERTY(Config, EditDefaultsOnly, Category = "Visualization",
		meta = (EditCondition = "bEnableSkyVisualization"))
	bool bEnableWellnessAtmosphere;

	// Path to the default sky configuration data asset
	UPROPERTY(Config, EditDefaultsOnly, Category = "Visualization",
		meta = (EditCondition = "bEnableSkyVisualization"))
	FSoftObjectPath DefaultSkyConfigPath;

	// ========================================================================
	// NOTIFICATION SETTINGS
	// ========================================================================

	// Enable toast notifications
	UPROPERTY(Config, EditDefaultsOnly, Category = "Notifications")
	bool bEnableNotifications;

	// Enable sound effects for notifications
	UPROPERTY(Config, EditDefaultsOnly, Category = "Notifications",
		meta = (EditCondition = "bEnableNotifications"))
	bool bEnableNotificationSounds;

	// Duration notifications remain visible in seconds
	UPROPERTY(Config, EditDefaultsOnly, Category = "Notifications",
		meta = (ClampMin = "2.0", ClampMax = "30.0", Units = "Seconds", EditCondition = "bEnableNotifications"))
	float NotificationDurationSeconds;

	// ========================================================================
	// PRIVACY SETTINGS
	// ========================================================================

	// Store application names in activity data (privacy consideration)
	UPROPERTY(Config, EditDefaultsOnly, Category = "Privacy")
	bool bStoreApplicationNames;

	// Store file paths in activity data (privacy consideration)
	UPROPERTY(Config, EditDefaultsOnly, Category = "Privacy")
	bool bStoreFilePaths;

	// Number of days to retain detailed session data
	UPROPERTY(Config, EditDefaultsOnly, Category = "Privacy",
		meta = (ClampMin = "7", ClampMax = "365", Units = "Days"))
	int32 DataRetentionDays;

	// ========================================================================
	// SECURITY SETTINGS
	// ========================================================================

	// Enable checksum verification for data integrity
	UPROPERTY(Config, EditDefaultsOnly, Category = "Security")
	bool bEnableChecksumVerification;

	// Warn when data tampering is detected
	UPROPERTY(Config, EditDefaultsOnly, Category = "Security",
		meta = (EditCondition = "bEnableChecksumVerification"))
	bool bWarnOnTamperDetection;

	// ========================================================================
	// DATA EXPORT SETTINGS
	// ========================================================================

	// Default directory for data exports
	UPROPERTY(Config, EditDefaultsOnly, Category = "Export")
	FDirectoryPath DefaultExportDirectory;

	// Include snapshots in session exports (can make files large)
	UPROPERTY(Config, EditDefaultsOnly, Category = "Export")
	bool bIncludeSnapshotsInExport;

	// ========================================================================
	// METHODS
	// ========================================================================

	// Get the singleton settings instance
	static UProductivityTrackerSettings* Get();

	// UDeveloperSettings interface
	virtual FName GetContainerName() const override { return TEXT("Project"); }
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
	virtual FName GetSectionName() const override { return TEXT("Developer Productivity Tracker"); }

#if WITH_EDITOR
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	// Delegate for settings changes
	FOnProductivitySettingsChanged OnSettingsChanged;

private:
	// Validate settings values
	void ValidateSettings();
};
