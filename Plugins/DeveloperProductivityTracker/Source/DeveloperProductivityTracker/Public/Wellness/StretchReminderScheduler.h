// ============================================================================
// StretchReminderScheduler.h
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Schedules and manages stretch/exercise reminders for developer wellness.
// Tracks reminder history and adapts to user behavior patterns.
//
// Architecture:
// Timer-based scheduling with snooze and skip functionality.
// Integrates with notification system for non-intrusive reminders.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPath.h"
#include "StretchReminderScheduler.generated.h"

// Log category
DECLARE_LOG_CATEGORY_EXTERN(LogStretchReminder, Log, All);

// Stretch exercise definition
USTRUCT(BlueprintType)
struct DEVELOPERPRODUCTIVITYTRACKER_API FStretchExercise
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Exercise")
	FString Name;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Exercise")
	FString Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Exercise")
	FString TargetArea; // Neck, Back, Wrists, Eyes, etc.

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Exercise")
	int32 DurationSeconds;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Exercise")
	int32 Repetitions;

	// Optional reference to a demonstration image texture
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Exercise")
	FSoftObjectPath DemonstrationImage;

	// Optional video/media URL for the exercise
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Exercise")
	FString VideoURL;

	// Difficulty level (1-5 stars)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Exercise",
		meta = (ClampMin = "1", ClampMax = "5"))
	int32 Difficulty;

	// Whether this exercise requires standing up
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Exercise")
	bool bRequiresStanding;

	FStretchExercise()
		: DurationSeconds(30)
		, Repetitions(1)
		, Difficulty(1)
		, bRequiresStanding(false)
	{
	}

	FStretchExercise(const FString& InName, const FString& InDesc, const FString& InTarget,
		int32 InDuration, int32 InDifficulty = 1, bool bInRequiresStanding = false)
		: Name(InName)
		, Description(InDesc)
		, TargetArea(InTarget)
		, DurationSeconds(InDuration)
		, Repetitions(1)
		, Difficulty(InDifficulty)
		, bRequiresStanding(bInRequiresStanding)
	{
	}
};

// Reminder event data
USTRUCT(BlueprintType)
struct DEVELOPERPRODUCTIVITYTRACKER_API FStretchReminderEvent
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reminder")
	FDateTime ScheduledTime;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reminder")
	FDateTime ActualTime;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reminder")
	bool bWasAccepted;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reminder")
	bool bWasSnoozed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reminder")
	bool bWasSkipped;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Reminder")
	FStretchExercise Exercise;

	FStretchReminderEvent()
		: ScheduledTime(FDateTime::MinValue())
		, ActualTime(FDateTime::MinValue())
		, bWasAccepted(false)
		, bWasSnoozed(false)
		, bWasSkipped(false)
	{
	}
};

// Delegate declarations
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStretchReminderTriggered, const FStretchExercise&, Exercise);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStretchReminderSnoozed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStretchReminderSkipped);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStretchReminderCompleted);

UCLASS(BlueprintType)
class DEVELOPERPRODUCTIVITYTRACKER_API UStretchReminderScheduler : public UObject
{
	GENERATED_BODY()

public:
	UStretchReminderScheduler();

	// Update timers (call each frame)
	void Tick(float DeltaTime);

	// ========================================================================
	// CONTROLS
	// ========================================================================

	// Start the reminder scheduler
	UFUNCTION(BlueprintCallable, Category = "Productivity|Stretch")
	void StartScheduler();

	// Stop the reminder scheduler
	UFUNCTION(BlueprintCallable, Category = "Productivity|Stretch")
	void StopScheduler();

	// Snooze current reminder
	UFUNCTION(BlueprintCallable, Category = "Productivity|Stretch")
	void SnoozeReminder(float SnoozeMinutes = 5.0f);

	// Skip current reminder
	UFUNCTION(BlueprintCallable, Category = "Productivity|Stretch")
	void SkipReminder();

	// Mark current stretch as completed
	UFUNCTION(BlueprintCallable, Category = "Productivity|Stretch")
	void CompleteStretch();

	// Trigger a reminder manually
	UFUNCTION(BlueprintCallable, Category = "Productivity|Stretch")
	void TriggerReminderNow();

	// ========================================================================
	// QUERIES
	// ========================================================================

	// Check if scheduler is active
	UFUNCTION(BlueprintPure, Category = "Productivity|Stretch")
	bool IsSchedulerActive() const { return bIsActive; }

	// Check if a reminder is currently showing
	UFUNCTION(BlueprintPure, Category = "Productivity|Stretch")
	bool IsReminderActive() const { return bReminderActive; }

	// Get time until next reminder
	UFUNCTION(BlueprintPure, Category = "Productivity|Stretch")
	float GetSecondsUntilNextReminder() const;

	// Get formatted time until next reminder
	UFUNCTION(BlueprintPure, Category = "Productivity|Stretch")
	FString GetFormattedTimeUntilNext() const;

	// Get current exercise
	UFUNCTION(BlueprintPure, Category = "Productivity|Stretch")
	FStretchExercise GetCurrentExercise() const { return CurrentExercise; }

	// Get reminder history
	UFUNCTION(BlueprintPure, Category = "Productivity|Stretch")
	TArray<FStretchReminderEvent> GetReminderHistory() const { return ReminderHistory; }

	// Get today's completed stretch count
	UFUNCTION(BlueprintPure, Category = "Productivity|Stretch")
	int32 GetTodayCompletedCount() const;

	// Get available exercises
	UFUNCTION(BlueprintPure, Category = "Productivity|Stretch")
	TArray<FStretchExercise> GetAvailableExercises() const { return AvailableExercises; }

	// ========================================================================
	// CONFIGURATION
	// ========================================================================

	// Interval between reminders in minutes
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stretch|Config",
		meta = (ClampMin = "15.0", ClampMax = "120.0"))
	float ReminderIntervalMinutes;

	// Default snooze duration in minutes
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stretch|Config",
		meta = (ClampMin = "1.0", ClampMax = "30.0"))
	float DefaultSnoozeMinutes;

	// Randomize exercise selection
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stretch|Config")
	bool bRandomizeExercises;

	// Maximum reminder events to retain in history
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stretch|Config",
		meta = (ClampMin = "10", ClampMax = "500"))
	int32 MaxHistoryEvents;

	// ========================================================================
	// DELEGATES
	// ========================================================================

	UPROPERTY(BlueprintAssignable, Category = "Productivity|Events")
	FOnStretchReminderTriggered OnStretchReminderTriggered;

	UPROPERTY(BlueprintAssignable, Category = "Productivity|Events")
	FOnStretchReminderSnoozed OnStretchReminderSnoozed;

	UPROPERTY(BlueprintAssignable, Category = "Productivity|Events")
	FOnStretchReminderSkipped OnStretchReminderSkipped;

	UPROPERTY(BlueprintAssignable, Category = "Productivity|Events")
	FOnStretchReminderCompleted OnStretchReminderCompleted;

private:
	// State
	bool bIsActive;
	bool bReminderActive;
	float TimeSinceLastReminder;
	int32 CurrentExerciseIndex;
	FStretchExercise CurrentExercise;

	// History
	TArray<FStretchReminderEvent> ReminderHistory;
	FStretchReminderEvent CurrentReminderEvent;

	// Exercise library
	TArray<FStretchExercise> AvailableExercises;

	// Internal methods
	void InitializeExerciseLibrary();
	FStretchExercise SelectNextExercise();
	void TriggerReminder();
	void RecordReminderEvent(bool bAccepted, bool bSnoozed, bool bSkipped);
};
