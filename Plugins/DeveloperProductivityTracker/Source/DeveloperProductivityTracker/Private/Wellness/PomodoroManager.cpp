// ============================================================================
// PomodoroManager.cpp
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Implementation of the Pomodoro timer state machine.
// ============================================================================

#include "Wellness/PomodoroManager.h"

DEFINE_LOG_CATEGORY(LogPomodoro);

UPomodoroManager::UPomodoroManager()
	: WorkIntervalMinutes(25.0f)
	, ShortBreakMinutes(5.0f)
	, LongBreakMinutes(15.0f)
	, WorkIntervalsBeforeLongBreak(4)
	, bAutoStartNextInterval(false)
	, CurrentState(EPomodoroState::Inactive)
	, StateBeforePause(EPomodoroState::Inactive)
	, CurrentIntervalElapsed(0.0f)
	, WorkIntervalsSinceLastLongBreak(0)
{
}

void UPomodoroManager::Tick(float DeltaTime)
{
	if (CurrentState == EPomodoroState::Inactive || CurrentState == EPomodoroState::Paused)
	{
		return;
	}

	// Update elapsed time
	CurrentIntervalElapsed += DeltaTime;

	// Update statistics
	if (CurrentState == EPomodoroState::Working)
	{
		Statistics.TotalWorkSeconds += DeltaTime;
	}
	else if (CurrentState == EPomodoroState::ShortBreak || CurrentState == EPomodoroState::LongBreak)
	{
		Statistics.TotalBreakSeconds += DeltaTime;
	}

	// Broadcast tick
	float TotalSeconds = GetCurrentIntervalDuration();
	OnPomodoroTimerTick.Broadcast(CurrentIntervalElapsed, TotalSeconds);

	// Check if interval is complete
	if (CurrentIntervalElapsed >= TotalSeconds)
	{
		OnIntervalComplete();
	}
}

// ============================================================================
// CONTROLS
// ============================================================================

void UPomodoroManager::StartPomodoro()
{
	if (CurrentState != EPomodoroState::Inactive)
	{
		UE_LOG(LogPomodoro, Warning, TEXT("Pomodoro already running"));
		return;
	}

	// Initialize statistics
	Statistics.Reset();
	Statistics.SessionStartTime = FDateTime::Now();
	WorkIntervalsSinceLastLongBreak = 0;

	// Start with work interval
	TransitionToState(EPomodoroState::Working);

	UE_LOG(LogPomodoro, Log, TEXT("Pomodoro started - Work interval: %.0f minutes"), WorkIntervalMinutes);
}

void UPomodoroManager::StopPomodoro()
{
	if (CurrentState == EPomodoroState::Inactive)
	{
		return;
	}

	UE_LOG(LogPomodoro, Log, TEXT("Pomodoro stopped - Completed %d work intervals"),
		Statistics.CompletedWorkIntervals);

	TransitionToState(EPomodoroState::Inactive);
}

void UPomodoroManager::PausePomodoro()
{
	if (CurrentState == EPomodoroState::Inactive || CurrentState == EPomodoroState::Paused)
	{
		return;
	}

	StateBeforePause = CurrentState;
	TransitionToState(EPomodoroState::Paused);

	UE_LOG(LogPomodoro, Log, TEXT("Pomodoro paused"));
}

void UPomodoroManager::ResumePomodoro()
{
	if (CurrentState != EPomodoroState::Paused)
	{
		return;
	}

	// Don't reset elapsed time - resume where we left off
	CurrentState = StateBeforePause;
	OnPomodoroStateChanged.Broadcast(CurrentState);

	UE_LOG(LogPomodoro, Log, TEXT("Pomodoro resumed - State: %s"), *GetStateDisplayName());
}

void UPomodoroManager::SkipCurrentInterval()
{
	if (CurrentState == EPomodoroState::Inactive || CurrentState == EPomodoroState::Paused)
	{
		return;
	}

	Statistics.SkippedIntervals++;

	UE_LOG(LogPomodoro, Log, TEXT("Skipped %s interval"), *GetStateDisplayName());

	TransitionToNextState();
}

void UPomodoroManager::ResetPomodoro()
{
	Statistics.Reset();
	WorkIntervalsSinceLastLongBreak = 0;
	CurrentIntervalElapsed = 0.0f;

	if (CurrentState != EPomodoroState::Inactive)
	{
		TransitionToState(EPomodoroState::Working);
	}

	UE_LOG(LogPomodoro, Log, TEXT("Pomodoro reset"));
}

// ============================================================================
// QUERIES
// ============================================================================

float UPomodoroManager::GetRemainingSeconds() const
{
	float TotalSeconds = GetCurrentIntervalDuration();
	return FMath::Max(0.0f, TotalSeconds - CurrentIntervalElapsed);
}

float UPomodoroManager::GetCurrentIntervalDuration() const
{
	switch (CurrentState)
	{
	case EPomodoroState::Working:
		return WorkIntervalMinutes * 60.0f;
	case EPomodoroState::ShortBreak:
		return ShortBreakMinutes * 60.0f;
	case EPomodoroState::LongBreak:
		return LongBreakMinutes * 60.0f;
	default:
		return 0.0f;
	}
}

float UPomodoroManager::GetIntervalProgress() const
{
	float TotalSeconds = GetCurrentIntervalDuration();
	if (TotalSeconds <= 0.0f)
	{
		return 0.0f;
	}
	return FMath::Clamp(CurrentIntervalElapsed / TotalSeconds, 0.0f, 1.0f);
}

int32 UPomodoroManager::GetIntervalsUntilLongBreak() const
{
	return FMath::Max(0, WorkIntervalsBeforeLongBreak - WorkIntervalsSinceLastLongBreak);
}

FString UPomodoroManager::GetFormattedRemainingTime() const
{
	float Remaining = GetRemainingSeconds();
	int32 Minutes = FMath::FloorToInt(Remaining / 60.0f);
	int32 Seconds = FMath::FloorToInt(FMath::Fmod(Remaining, 60.0f));

	return FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
}

FString UPomodoroManager::GetStateDisplayName() const
{
	switch (CurrentState)
	{
	case EPomodoroState::Inactive:
		return TEXT("Inactive");
	case EPomodoroState::Working:
		return TEXT("Working");
	case EPomodoroState::ShortBreak:
		return TEXT("Short Break");
	case EPomodoroState::LongBreak:
		return TEXT("Long Break");
	case EPomodoroState::Paused:
		return TEXT("Paused");
	default:
		return TEXT("Unknown");
	}
}

bool UPomodoroManager::IsRunning() const
{
	return CurrentState == EPomodoroState::Working ||
		   CurrentState == EPomodoroState::ShortBreak ||
		   CurrentState == EPomodoroState::LongBreak;
}

// ============================================================================
// PRIVATE METHODS
// ============================================================================

void UPomodoroManager::TransitionToState(EPomodoroState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	EPomodoroState OldState = CurrentState;
	CurrentState = NewState;
	CurrentIntervalElapsed = 0.0f;

	UE_LOG(LogPomodoro, Verbose, TEXT("Pomodoro state: %s -> %s"),
		*UEnum::GetValueAsString(OldState),
		*UEnum::GetValueAsString(NewState));

	OnPomodoroStateChanged.Broadcast(CurrentState);
}

void UPomodoroManager::TransitionToNextState()
{
	EPomodoroState NextState = DetermineNextState();
	TransitionToState(NextState);
}

void UPomodoroManager::OnIntervalComplete()
{
	EPomodoroState CompletedState = CurrentState;

	// Update statistics based on completed state
	switch (CompletedState)
	{
	case EPomodoroState::Working:
		Statistics.CompletedWorkIntervals++;
		WorkIntervalsSinceLastLongBreak++;
		break;
	case EPomodoroState::ShortBreak:
		Statistics.CompletedShortBreaks++;
		break;
	case EPomodoroState::LongBreak:
		Statistics.CompletedLongBreaks++;
		WorkIntervalsSinceLastLongBreak = 0;
		OnPomodoroWorkSessionCompleted.Broadcast(Statistics.CompletedWorkIntervals);
		break;
	default:
		break;
	}

	UE_LOG(LogPomodoro, Log, TEXT("Completed %s interval (Total work: %d)"),
		*GetStateDisplayName(), Statistics.CompletedWorkIntervals);

	// Broadcast completion
	OnPomodoroIntervalCompleted.Broadcast(CompletedState);

	// Auto-transition or pause
	if (bAutoStartNextInterval)
	{
		TransitionToNextState();
	}
	else
	{
		// Wait for user to acknowledge
		StateBeforePause = DetermineNextState();
		TransitionToState(EPomodoroState::Paused);
	}
}

EPomodoroState UPomodoroManager::DetermineNextState() const
{
	switch (CurrentState)
	{
	case EPomodoroState::Working:
		// After work: short or long break
		if (WorkIntervalsSinceLastLongBreak >= WorkIntervalsBeforeLongBreak)
		{
			return EPomodoroState::LongBreak;
		}
		return EPomodoroState::ShortBreak;

	case EPomodoroState::ShortBreak:
	case EPomodoroState::LongBreak:
		// After break: work
		return EPomodoroState::Working;

	case EPomodoroState::Paused:
		// Resume to whatever was planned
		return StateBeforePause;

	default:
		return EPomodoroState::Working;
	}
}
