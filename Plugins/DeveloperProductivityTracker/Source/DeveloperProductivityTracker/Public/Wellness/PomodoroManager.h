// ============================================================================
// PomodoroManager.h
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Manages Pomodoro work/break cycles for productivity optimization.
// Default: 25 min work, 5 min short break, 15 min long break after 4 cycles.
//
// Architecture:
// State machine design for clean state transitions.
// Delegate-driven for loose coupling with UI and notification systems.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "PomodoroManager.generated.h"

// Log category
DECLARE_LOG_CATEGORY_EXTERN(LogPomodoro, Log, All);

// Pomodoro state machine states
UENUM(BlueprintType)
enum class EPomodoroState : uint8
{
	Inactive    UMETA(DisplayName = "Inactive"),    // Timer not running
	Working     UMETA(DisplayName = "Working"),     // Work interval active
	ShortBreak  UMETA(DisplayName = "Short Break"), // Short break interval
	LongBreak   UMETA(DisplayName = "Long Break"),  // Long break after N work intervals
	Paused      UMETA(DisplayName = "Paused")       // Timer paused
};

// Delegate declarations
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPomodoroStateChanged, EPomodoroState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPomodoroTimerTick, float, ElapsedSeconds, float, TotalSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPomodoroIntervalCompleted, EPomodoroState, CompletedState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPomodoroWorkSessionCompleted, int32, TotalWorkIntervals);

// Pomodoro statistics for current session
USTRUCT(BlueprintType)
struct DEVELOPERPRODUCTIVITYTRACKER_API FPomodoroStatistics
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Statistics")
	int32 CompletedWorkIntervals;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Statistics")
	int32 CompletedShortBreaks;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Statistics")
	int32 CompletedLongBreaks;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Statistics")
	float TotalWorkSeconds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Statistics")
	float TotalBreakSeconds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Statistics")
	int32 SkippedIntervals;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Statistics")
	FDateTime SessionStartTime;

	FPomodoroStatistics()
		: CompletedWorkIntervals(0)
		, CompletedShortBreaks(0)
		, CompletedLongBreaks(0)
		, TotalWorkSeconds(0.0f)
		, TotalBreakSeconds(0.0f)
		, SkippedIntervals(0)
		, SessionStartTime(FDateTime::MinValue())
	{
	}

	void Reset()
	{
		CompletedWorkIntervals = 0;
		CompletedShortBreaks = 0;
		CompletedLongBreaks = 0;
		TotalWorkSeconds = 0.0f;
		TotalBreakSeconds = 0.0f;
		SkippedIntervals = 0;
		SessionStartTime = FDateTime::MinValue();
	}
};

UCLASS(BlueprintType)
class DEVELOPERPRODUCTIVITYTRACKER_API UPomodoroManager : public UObject
{
	GENERATED_BODY()

public:
	UPomodoroManager();

	// Update the timer (call each frame)
	void Tick(float DeltaTime);

	// ========================================================================
	// CONTROLS
	// ========================================================================

	// Start the Pomodoro timer (begins with work interval)
	UFUNCTION(BlueprintCallable, Category = "Productivity|Pomodoro")
	void StartPomodoro();

	// Stop the Pomodoro timer completely
	UFUNCTION(BlueprintCallable, Category = "Productivity|Pomodoro")
	void StopPomodoro();

	// Pause the current interval
	UFUNCTION(BlueprintCallable, Category = "Productivity|Pomodoro")
	void PausePomodoro();

	// Resume from pause
	UFUNCTION(BlueprintCallable, Category = "Productivity|Pomodoro")
	void ResumePomodoro();

	// Skip the current interval and move to the next
	UFUNCTION(BlueprintCallable, Category = "Productivity|Pomodoro")
	void SkipCurrentInterval();

	// Reset to initial state
	UFUNCTION(BlueprintCallable, Category = "Productivity|Pomodoro")
	void ResetPomodoro();

	// ========================================================================
	// QUERIES
	// ========================================================================

	// Get current state
	UFUNCTION(BlueprintPure, Category = "Productivity|Pomodoro")
	EPomodoroState GetCurrentState() const { return CurrentState; }

	// Get remaining time in current interval
	UFUNCTION(BlueprintPure, Category = "Productivity|Pomodoro")
	float GetRemainingSeconds() const;

	// Get elapsed time in current interval
	UFUNCTION(BlueprintPure, Category = "Productivity|Pomodoro")
	float GetElapsedSeconds() const { return CurrentIntervalElapsed; }

	// Get total duration of current interval
	UFUNCTION(BlueprintPure, Category = "Productivity|Pomodoro")
	float GetCurrentIntervalDuration() const;

	// Get progress as 0.0-1.0
	UFUNCTION(BlueprintPure, Category = "Productivity|Pomodoro")
	float GetIntervalProgress() const;

	// Get number of completed work intervals
	UFUNCTION(BlueprintPure, Category = "Productivity|Pomodoro")
	int32 GetCompletedWorkIntervals() const { return Statistics.CompletedWorkIntervals; }

	// Get work intervals until long break
	UFUNCTION(BlueprintPure, Category = "Productivity|Pomodoro")
	int32 GetIntervalsUntilLongBreak() const;

	// Get session statistics
	UFUNCTION(BlueprintPure, Category = "Productivity|Pomodoro")
	FPomodoroStatistics GetStatistics() const { return Statistics; }

	// Get formatted remaining time (MM:SS)
	UFUNCTION(BlueprintPure, Category = "Productivity|Pomodoro")
	FString GetFormattedRemainingTime() const;

	// Get state display name
	UFUNCTION(BlueprintPure, Category = "Productivity|Pomodoro")
	FString GetStateDisplayName() const;

	// Check if timer is running (not inactive or paused)
	UFUNCTION(BlueprintPure, Category = "Productivity|Pomodoro")
	bool IsRunning() const;

	// ========================================================================
	// CONFIGURATION
	// ========================================================================

	// Work interval duration in minutes
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pomodoro|Config",
		meta = (ClampMin = "15.0", ClampMax = "60.0"))
	float WorkIntervalMinutes;

	// Short break duration in minutes
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pomodoro|Config",
		meta = (ClampMin = "3.0", ClampMax = "15.0"))
	float ShortBreakMinutes;

	// Long break duration in minutes
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pomodoro|Config",
		meta = (ClampMin = "10.0", ClampMax = "45.0"))
	float LongBreakMinutes;

	// Number of work intervals before long break
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pomodoro|Config",
		meta = (ClampMin = "2", ClampMax = "8"))
	int32 WorkIntervalsBeforeLongBreak;

	// Auto-start next interval after break
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pomodoro|Config")
	bool bAutoStartNextInterval;

	// ========================================================================
	// DELEGATES
	// ========================================================================

	// Broadcast when state changes
	UPROPERTY(BlueprintAssignable, Category = "Productivity|Events")
	FOnPomodoroStateChanged OnPomodoroStateChanged;

	// Broadcast every tick with timing info
	UPROPERTY(BlueprintAssignable, Category = "Productivity|Events")
	FOnPomodoroTimerTick OnPomodoroTimerTick;

	// Broadcast when an interval completes
	UPROPERTY(BlueprintAssignable, Category = "Productivity|Events")
	FOnPomodoroIntervalCompleted OnPomodoroIntervalCompleted;

	// Broadcast when a full work session (all intervals before long break) completes
	UPROPERTY(BlueprintAssignable, Category = "Productivity|Events")
	FOnPomodoroWorkSessionCompleted OnPomodoroWorkSessionCompleted;

private:
	// State
	EPomodoroState CurrentState;
	EPomodoroState StateBeforePause;
	float CurrentIntervalElapsed;
	int32 WorkIntervalsSinceLastLongBreak;

	// Statistics
	FPomodoroStatistics Statistics;

	// State machine
	void TransitionToState(EPomodoroState NewState);
	void TransitionToNextState();
	void OnIntervalComplete();
	EPomodoroState DetermineNextState() const;
};
