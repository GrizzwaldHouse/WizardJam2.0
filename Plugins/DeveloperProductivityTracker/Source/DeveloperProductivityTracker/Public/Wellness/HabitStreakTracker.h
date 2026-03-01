// ============================================================================
// HabitStreakTracker.h
// Developer: Marcus Daley
// Date: February 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Tracks daily wellness habits (stretches, breaks, pomodoros) and maintains
// streak data for consecutive days of meeting all goals.
//
// Architecture:
// UObject that records daily habit completions, persists to JSON,
// and broadcasts delegates on goal/milestone achievements.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "HabitStreakTracker.generated.h"

// Log category
DECLARE_LOG_CATEGORY_EXTERN(LogHabitStreak, Log, All);

// Record of a single day's habits
USTRUCT(BlueprintType)
struct DEVELOPERPRODUCTIVITYTRACKER_API FDailyHabitRecord
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Habits")
	FDateTime Date;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Habits")
	int32 StretchesCompleted;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Habits")
	int32 BreaksTaken;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Habits")
	int32 PomodorosCompleted;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Habits")
	bool bMetStretchGoal;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Habits")
	bool bMetBreakGoal;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Habits")
	bool bMetPomodoroGoal;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Habits")
	bool bMetAllGoals;

	FDailyHabitRecord()
		: Date(FDateTime::Today())
		, StretchesCompleted(0)
		, BreaksTaken(0)
		, PomodorosCompleted(0)
		, bMetStretchGoal(false)
		, bMetBreakGoal(false)
		, bMetPomodoroGoal(false)
		, bMetAllGoals(false)
	{
	}
};

// Aggregate streak data
USTRUCT(BlueprintType)
struct DEVELOPERPRODUCTIVITYTRACKER_API FHabitStreakData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Streak")
	int32 CurrentStreak;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Streak")
	int32 LongestStreak;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Streak")
	int32 TotalDaysTracked;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Streak")
	FDateTime LastTrackedDate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Streak")
	TArray<FDailyHabitRecord> History;

	FHabitStreakData()
		: CurrentStreak(0)
		, LongestStreak(0)
		, TotalDaysTracked(0)
		, LastTrackedDate(FDateTime::MinValue())
	{
	}
};

// Delegate declarations
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDailyGoalMet, FName, GoalType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllDailyGoalsMet);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStreakMilestone, int32, StreakDays);

UCLASS(BlueprintType)
class DEVELOPERPRODUCTIVITYTRACKER_API UHabitStreakTracker : public UObject
{
	GENERATED_BODY()

public:
	UHabitStreakTracker();

	// ========================================================================
	// RECORDING
	// ========================================================================

	// Record that a stretch was completed
	UFUNCTION(BlueprintCallable, Category = "Productivity|Habits")
	void RecordStretchCompleted();

	// Record that a break was taken
	UFUNCTION(BlueprintCallable, Category = "Productivity|Habits")
	void RecordBreakTaken();

	// Record that a Pomodoro work interval was completed
	UFUNCTION(BlueprintCallable, Category = "Productivity|Habits")
	void RecordPomodoroCompleted();

	// ========================================================================
	// QUERIES
	// ========================================================================

	// Get today's habit record
	UFUNCTION(BlueprintPure, Category = "Productivity|Habits")
	FDailyHabitRecord GetTodayRecord() const { return CurrentDayRecord; }

	// Get full streak data
	UFUNCTION(BlueprintPure, Category = "Productivity|Habits")
	FHabitStreakData GetStreakData() const { return StreakData; }

	// Get current consecutive day streak
	UFUNCTION(BlueprintPure, Category = "Productivity|Habits")
	int32 GetCurrentStreak() const { return StreakData.CurrentStreak; }

	// Get longest streak ever
	UFUNCTION(BlueprintPure, Category = "Productivity|Habits")
	int32 GetLongestStreak() const { return StreakData.LongestStreak; }

	// Get today's progress as 0.0-1.0 (fraction of goals met)
	UFUNCTION(BlueprintPure, Category = "Productivity|Habits")
	float GetTodayProgress() const;

	// Check if all daily goals are met today
	UFUNCTION(BlueprintPure, Category = "Productivity|Habits")
	bool HasMetTodayGoals() const { return CurrentDayRecord.bMetAllGoals; }

	// ========================================================================
	// PERSISTENCE
	// ========================================================================

	// Save streak data to JSON file
	UFUNCTION(BlueprintCallable, Category = "Productivity|Habits")
	void SaveToJson();

	// Load streak data from JSON file
	UFUNCTION(BlueprintCallable, Category = "Productivity|Habits")
	void LoadFromJson();

	// ========================================================================
	// CONFIGURATION
	// ========================================================================

	// Daily stretch goal
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Habits|Goals",
		meta = (ClampMin = "1", ClampMax = "20"))
	int32 DailyStretchGoal;

	// Daily break goal
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Habits|Goals",
		meta = (ClampMin = "1", ClampMax = "20"))
	int32 DailyBreakGoal;

	// Daily Pomodoro goal
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Habits|Goals",
		meta = (ClampMin = "1", ClampMax = "16"))
	int32 DailyPomodoroGoal;

	// Maximum days of history to retain
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Habits|Config",
		meta = (ClampMin = "7", ClampMax = "365"))
	int32 MaxHistoryDays;

	// ========================================================================
	// DELEGATES
	// ========================================================================

	// Fires when a specific daily goal is met (Stretch, Break, or Pomodoro)
	UPROPERTY(BlueprintAssignable, Category = "Productivity|Events")
	FOnDailyGoalMet OnDailyGoalMet;

	// Fires when all 3 daily goals are met
	UPROPERTY(BlueprintAssignable, Category = "Productivity|Events")
	FOnAllDailyGoalsMet OnAllDailyGoalsMet;

	// Fires at streak milestones (3, 7, 14, 30, 60, 90 days)
	UPROPERTY(BlueprintAssignable, Category = "Productivity|Events")
	FOnStreakMilestone OnStreakMilestone;

private:
	// Current day's record
	FDailyHabitRecord CurrentDayRecord;

	// Aggregate streak data
	FHabitStreakData StreakData;

	// If the date rolled over, finalize previous day and start fresh
	void CheckAndAdvanceDay();

	// Recalculate streak from history
	void UpdateStreakFromHistory();

	// Evaluate today's goals against targets
	void EvaluateDailyGoals();

	// Fire milestone delegates if a threshold was crossed
	void CheckMilestones(int32 OldStreak, int32 NewStreak);

	// Get the file path for JSON persistence
	FString GetSaveFilePath() const;

	// Trim history to MaxHistoryDays
	void TrimHistory();
};
