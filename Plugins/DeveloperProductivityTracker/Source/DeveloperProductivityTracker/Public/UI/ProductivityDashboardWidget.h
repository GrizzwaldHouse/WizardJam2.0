// ============================================================================
// ProductivityDashboardWidget.h
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Main dashboard widget for viewing productivity statistics.
// Displays session info, wellness status, Pomodoro timer, and history.
//
// Architecture:
// Editor Utility Widget base for dockable UI panel.
// Pulls data from various subsystems for display.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "Core/SessionData.h"
#include "Wellness/PomodoroManager.h"
#include "ProductivityDashboardWidget.generated.h"

class USessionTrackingSubsystem;
class UBreakWellnessSubsystem;
class UNotificationSubsystem;

UCLASS()
class DEVELOPERPRODUCTIVITYTRACKER_API UProductivityDashboardWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:
	UProductivityDashboardWidget();

	// ========================================================================
	// LIFECYCLE
	// ========================================================================

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// ========================================================================
	// SESSION DATA
	// ========================================================================

	// Get current session elapsed time formatted
	UFUNCTION(BlueprintPure, Category = "Dashboard")
	FString GetElapsedTimeFormatted() const;

	// Get current activity state text
	UFUNCTION(BlueprintPure, Category = "Dashboard")
	FString GetActivityStateText() const;

	// Get active percentage for current session
	UFUNCTION(BlueprintPure, Category = "Dashboard")
	float GetActivePercentage() const;

	// Check if session is active
	UFUNCTION(BlueprintPure, Category = "Dashboard")
	bool IsSessionActive() const;

	// ========================================================================
	// POMODORO DATA
	// ========================================================================

	// Get Pomodoro remaining time formatted
	UFUNCTION(BlueprintPure, Category = "Dashboard")
	FString GetPomodoroTimeFormatted() const;

	// Get Pomodoro state text
	UFUNCTION(BlueprintPure, Category = "Dashboard")
	FString GetPomodoroStateText() const;

	// Get completed work intervals
	UFUNCTION(BlueprintPure, Category = "Dashboard")
	int32 GetCompletedPomodoros() const;

	// Get Pomodoro progress (0-1)
	UFUNCTION(BlueprintPure, Category = "Dashboard")
	float GetPomodoroProgress() const;

	// ========================================================================
	// WELLNESS DATA
	// ========================================================================

	// Get wellness status text
	UFUNCTION(BlueprintPure, Category = "Dashboard")
	FString GetWellnessStatusText() const;

	// Get wellness status color
	UFUNCTION(BlueprintPure, Category = "Dashboard")
	FLinearColor GetWellnessStatusColor() const;

	// Get minutes since last break
	UFUNCTION(BlueprintPure, Category = "Dashboard")
	float GetMinutesSinceBreak() const;

	// Get today's total work hours
	UFUNCTION(BlueprintPure, Category = "Dashboard")
	float GetTodayWorkHours() const;

	// ========================================================================
	// SESSION CONTROLS
	// ========================================================================

	// Toggle session start/end
	UFUNCTION(BlueprintCallable, Category = "Dashboard")
	void ToggleSession();

	// Pause/resume session
	UFUNCTION(BlueprintCallable, Category = "Dashboard")
	void TogglePause();

	// ========================================================================
	// POMODORO CONTROLS
	// ========================================================================

	// Start Pomodoro timer
	UFUNCTION(BlueprintCallable, Category = "Dashboard")
	void StartPomodoro();

	// Stop Pomodoro timer
	UFUNCTION(BlueprintCallable, Category = "Dashboard")
	void StopPomodoro();

	// Skip current Pomodoro interval
	UFUNCTION(BlueprintCallable, Category = "Dashboard")
	void SkipPomodoroInterval();

	// ========================================================================
	// QUICK ACTIONS
	// ========================================================================

	// Take a quick break
	UFUNCTION(BlueprintCallable, Category = "Dashboard")
	void TakeQuickBreak();

	// End break and resume
	UFUNCTION(BlueprintCallable, Category = "Dashboard")
	void EndBreak();

	// Trigger stretch reminder now
	UFUNCTION(BlueprintCallable, Category = "Dashboard")
	void TriggerStretchNow();

protected:
	// Cached subsystem references
	UPROPERTY()
	USessionTrackingSubsystem* SessionSubsystem;

	UPROPERTY()
	UBreakWellnessSubsystem* WellnessSubsystem;

	UPROPERTY()
	UNotificationSubsystem* NotificationSubsystem;

private:
	// Cache subsystem references
	void CacheSubsystems();
};
