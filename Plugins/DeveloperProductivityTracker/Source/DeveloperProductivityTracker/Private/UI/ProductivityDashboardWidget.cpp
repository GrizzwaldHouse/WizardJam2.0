// ============================================================================
// ProductivityDashboardWidget.cpp
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================

#include "UI/ProductivityDashboardWidget.h"
#include "Core/SessionTrackingSubsystem.h"
#include "Wellness/BreakWellnessSubsystem.h"
#include "UI/NotificationSubsystem.h"
#include "Editor.h"

UProductivityDashboardWidget::UProductivityDashboardWidget()
	: SessionSubsystem(nullptr)
	, WellnessSubsystem(nullptr)
	, NotificationSubsystem(nullptr)
{
}

void UProductivityDashboardWidget::NativeConstruct()
{
	Super::NativeConstruct();
	CacheSubsystems();
}

void UProductivityDashboardWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Re-cache if needed
	if (!SessionSubsystem || !WellnessSubsystem)
	{
		CacheSubsystems();
	}
}

// ============================================================================
// SESSION DATA
// ============================================================================

FString UProductivityDashboardWidget::GetElapsedTimeFormatted() const
{
	if (!SessionSubsystem)
	{
		return TEXT("00:00:00");
	}
	return SessionSubsystem->GetFormattedElapsedTime();
}

FString UProductivityDashboardWidget::GetActivityStateText() const
{
	if (!SessionSubsystem)
	{
		return TEXT("Not Tracking");
	}
	return SessionSubsystem->GetActivityStateDisplayString();
}

float UProductivityDashboardWidget::GetActivePercentage() const
{
	if (!SessionSubsystem || !SessionSubsystem->IsSessionActive())
	{
		return 0.0f;
	}

	FSessionRecord Record = SessionSubsystem->GetCurrentSessionRecord();
	return Record.ActivitySummary.GetActivePercentage();
}

bool UProductivityDashboardWidget::IsSessionActive() const
{
	return SessionSubsystem && SessionSubsystem->IsSessionActive();
}

// ============================================================================
// POMODORO DATA
// ============================================================================

FString UProductivityDashboardWidget::GetPomodoroTimeFormatted() const
{
	if (!WellnessSubsystem)
	{
		return TEXT("--:--");
	}

	UPomodoroManager* Pomodoro = WellnessSubsystem->GetPomodoroManager();
	if (!Pomodoro)
	{
		return TEXT("--:--");
	}

	return Pomodoro->GetFormattedRemainingTime();
}

FString UProductivityDashboardWidget::GetPomodoroStateText() const
{
	if (!WellnessSubsystem)
	{
		return TEXT("Inactive");
	}

	UPomodoroManager* Pomodoro = WellnessSubsystem->GetPomodoroManager();
	if (!Pomodoro)
	{
		return TEXT("Inactive");
	}

	return Pomodoro->GetStateDisplayName();
}

int32 UProductivityDashboardWidget::GetCompletedPomodoros() const
{
	if (!WellnessSubsystem)
	{
		return 0;
	}

	UPomodoroManager* Pomodoro = WellnessSubsystem->GetPomodoroManager();
	if (!Pomodoro)
	{
		return 0;
	}

	return Pomodoro->GetCompletedWorkIntervals();
}

float UProductivityDashboardWidget::GetPomodoroProgress() const
{
	if (!WellnessSubsystem)
	{
		return 0.0f;
	}

	UPomodoroManager* Pomodoro = WellnessSubsystem->GetPomodoroManager();
	if (!Pomodoro)
	{
		return 0.0f;
	}

	return Pomodoro->GetIntervalProgress();
}

// ============================================================================
// WELLNESS DATA
// ============================================================================

FString UProductivityDashboardWidget::GetWellnessStatusText() const
{
	if (!WellnessSubsystem)
	{
		return TEXT("Unknown");
	}
	return WellnessSubsystem->GetWellnessStatusDisplayString();
}

FLinearColor UProductivityDashboardWidget::GetWellnessStatusColor() const
{
	if (!WellnessSubsystem)
	{
		return FLinearColor::Gray;
	}
	return WellnessSubsystem->GetWellnessStatusColor();
}

float UProductivityDashboardWidget::GetMinutesSinceBreak() const
{
	if (!WellnessSubsystem)
	{
		return 0.0f;
	}
	return WellnessSubsystem->GetMinutesSinceLastBreak();
}

float UProductivityDashboardWidget::GetTodayWorkHours() const
{
	if (!SessionSubsystem)
	{
		return 0.0f;
	}
	return SessionSubsystem->GetTodayTotalSeconds() / 3600.0f;
}

// ============================================================================
// SESSION CONTROLS
// ============================================================================

void UProductivityDashboardWidget::ToggleSession()
{
	if (!SessionSubsystem)
	{
		return;
	}

	if (SessionSubsystem->IsSessionActive())
	{
		SessionSubsystem->EndSession();
	}
	else
	{
		SessionSubsystem->StartSession();
	}
}

void UProductivityDashboardWidget::TogglePause()
{
	if (!SessionSubsystem || !SessionSubsystem->IsSessionActive())
	{
		return;
	}

	if (SessionSubsystem->IsSessionPaused())
	{
		SessionSubsystem->ResumeSession();
	}
	else
	{
		SessionSubsystem->PauseSession();
	}
}

// ============================================================================
// POMODORO CONTROLS
// ============================================================================

void UProductivityDashboardWidget::StartPomodoro()
{
	if (!WellnessSubsystem)
	{
		return;
	}

	UPomodoroManager* Pomodoro = WellnessSubsystem->GetPomodoroManager();
	if (Pomodoro)
	{
		Pomodoro->StartPomodoro();
	}
}

void UProductivityDashboardWidget::StopPomodoro()
{
	if (!WellnessSubsystem)
	{
		return;
	}

	UPomodoroManager* Pomodoro = WellnessSubsystem->GetPomodoroManager();
	if (Pomodoro)
	{
		Pomodoro->StopPomodoro();
	}
}

void UProductivityDashboardWidget::SkipPomodoroInterval()
{
	if (!WellnessSubsystem)
	{
		return;
	}

	UPomodoroManager* Pomodoro = WellnessSubsystem->GetPomodoroManager();
	if (Pomodoro)
	{
		Pomodoro->SkipCurrentInterval();
	}
}

// ============================================================================
// QUICK ACTIONS
// ============================================================================

void UProductivityDashboardWidget::TakeQuickBreak()
{
	if (WellnessSubsystem)
	{
		WellnessSubsystem->StartQuickBreak();
	}
}

void UProductivityDashboardWidget::EndBreak()
{
	if (WellnessSubsystem)
	{
		WellnessSubsystem->EndBreakAndResume();
	}
}

void UProductivityDashboardWidget::TriggerStretchNow()
{
	if (!WellnessSubsystem)
	{
		return;
	}

	UStretchReminderScheduler* Stretches = WellnessSubsystem->GetStretchReminderScheduler();
	if (Stretches)
	{
		Stretches->TriggerReminderNow();
	}
}

void UProductivityDashboardWidget::CacheSubsystems()
{
	if (GEditor)
	{
		SessionSubsystem = GEditor->GetEditorSubsystem<USessionTrackingSubsystem>();
		WellnessSubsystem = GEditor->GetEditorSubsystem<UBreakWellnessSubsystem>();
		NotificationSubsystem = GEditor->GetEditorSubsystem<UNotificationSubsystem>();
	}
}
