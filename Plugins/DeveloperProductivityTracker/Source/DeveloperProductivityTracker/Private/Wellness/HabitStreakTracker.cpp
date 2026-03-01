// ============================================================================
// HabitStreakTracker.cpp
// Developer: Marcus Daley
// Date: February 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Implementation of habit streak tracking with JSON persistence.
// Tracks daily wellness habits and maintains consecutive-day streaks.
// ============================================================================

#include "Wellness/HabitStreakTracker.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "HAL/PlatformFileManager.h"

DEFINE_LOG_CATEGORY(LogHabitStreak);

// Milestone thresholds that trigger delegate broadcasts
static const TArray<int32> StreakMilestones = { 3, 7, 14, 30, 60, 90 };

UHabitStreakTracker::UHabitStreakTracker()
	: DailyStretchGoal(3)
	, DailyBreakGoal(4)
	, DailyPomodoroGoal(4)
	, MaxHistoryDays(90)
{
	// Start with a fresh record for today
	CurrentDayRecord.Date = FDateTime::Today();
}

// ============================================================================
// RECORDING
// ============================================================================

void UHabitStreakTracker::RecordStretchCompleted()
{
	CheckAndAdvanceDay();

	CurrentDayRecord.StretchesCompleted++;

	UE_LOG(LogHabitStreak, Verbose, TEXT("Stretch recorded (%d/%d)"),
		CurrentDayRecord.StretchesCompleted, DailyStretchGoal);

	EvaluateDailyGoals();
	SaveToJson();
}

void UHabitStreakTracker::RecordBreakTaken()
{
	CheckAndAdvanceDay();

	CurrentDayRecord.BreaksTaken++;

	UE_LOG(LogHabitStreak, Verbose, TEXT("Break recorded (%d/%d)"),
		CurrentDayRecord.BreaksTaken, DailyBreakGoal);

	EvaluateDailyGoals();
	SaveToJson();
}

void UHabitStreakTracker::RecordPomodoroCompleted()
{
	CheckAndAdvanceDay();

	CurrentDayRecord.PomodorosCompleted++;

	UE_LOG(LogHabitStreak, Verbose, TEXT("Pomodoro recorded (%d/%d)"),
		CurrentDayRecord.PomodorosCompleted, DailyPomodoroGoal);

	EvaluateDailyGoals();
	SaveToJson();
}

// ============================================================================
// QUERIES
// ============================================================================

float UHabitStreakTracker::GetTodayProgress() const
{
	// Each goal category contributes 1/3 of total progress
	int32 GoalsMet = 0;
	if (CurrentDayRecord.bMetStretchGoal) GoalsMet++;
	if (CurrentDayRecord.bMetBreakGoal) GoalsMet++;
	if (CurrentDayRecord.bMetPomodoroGoal) GoalsMet++;

	static constexpr int32 TotalGoalCategories = 3;
	return static_cast<float>(GoalsMet) / static_cast<float>(TotalGoalCategories);
}

// ============================================================================
// PERSISTENCE
// ============================================================================

void UHabitStreakTracker::SaveToJson()
{
	TSharedRef<FJsonObject> RootObject = MakeShared<FJsonObject>();

	// Streak metadata
	RootObject->SetNumberField(TEXT("currentStreak"), StreakData.CurrentStreak);
	RootObject->SetNumberField(TEXT("longestStreak"), StreakData.LongestStreak);
	RootObject->SetNumberField(TEXT("totalDaysTracked"), StreakData.TotalDaysTracked);
	RootObject->SetStringField(TEXT("lastTrackedDate"), StreakData.LastTrackedDate.ToIso8601());

	// Current day record
	TSharedRef<FJsonObject> TodayObject = MakeShared<FJsonObject>();
	TodayObject->SetStringField(TEXT("date"), CurrentDayRecord.Date.ToIso8601());
	TodayObject->SetNumberField(TEXT("stretches"), CurrentDayRecord.StretchesCompleted);
	TodayObject->SetNumberField(TEXT("breaks"), CurrentDayRecord.BreaksTaken);
	TodayObject->SetNumberField(TEXT("pomodoros"), CurrentDayRecord.PomodorosCompleted);
	TodayObject->SetBoolField(TEXT("metStretchGoal"), CurrentDayRecord.bMetStretchGoal);
	TodayObject->SetBoolField(TEXT("metBreakGoal"), CurrentDayRecord.bMetBreakGoal);
	TodayObject->SetBoolField(TEXT("metPomodoroGoal"), CurrentDayRecord.bMetPomodoroGoal);
	TodayObject->SetBoolField(TEXT("metAllGoals"), CurrentDayRecord.bMetAllGoals);
	RootObject->SetObjectField(TEXT("today"), TodayObject);

	// History array
	TArray<TSharedPtr<FJsonValue>> HistoryArray;
	for (const FDailyHabitRecord& Record : StreakData.History)
	{
		TSharedRef<FJsonObject> RecordObject = MakeShared<FJsonObject>();
		RecordObject->SetStringField(TEXT("date"), Record.Date.ToIso8601());
		RecordObject->SetNumberField(TEXT("stretches"), Record.StretchesCompleted);
		RecordObject->SetNumberField(TEXT("breaks"), Record.BreaksTaken);
		RecordObject->SetNumberField(TEXT("pomodoros"), Record.PomodorosCompleted);
		RecordObject->SetBoolField(TEXT("metAllGoals"), Record.bMetAllGoals);
		HistoryArray.Add(MakeShared<FJsonValueObject>(RecordObject));
	}
	RootObject->SetArrayField(TEXT("history"), HistoryArray);

	// Serialize and write
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject, Writer);

	FString FilePath = GetSaveFilePath();

	// Ensure directory exists
	FString Directory = FPaths::GetPath(FilePath);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.CreateDirectoryTree(*Directory);

	if (FFileHelper::SaveStringToFile(OutputString, *FilePath))
	{
		UE_LOG(LogHabitStreak, Verbose, TEXT("Saved habit data to: %s"), *FilePath);
	}
	else
	{
		UE_LOG(LogHabitStreak, Warning, TEXT("Failed to save habit data to: %s"), *FilePath);
	}
}

void UHabitStreakTracker::LoadFromJson()
{
	FString FilePath = GetSaveFilePath();
	FString JsonString;

	if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
	{
		UE_LOG(LogHabitStreak, Log, TEXT("No existing habit data found at: %s (starting fresh)"), *FilePath);
		return;
	}

	TSharedPtr<FJsonObject> RootObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		UE_LOG(LogHabitStreak, Warning, TEXT("Failed to parse habit data JSON"));
		return;
	}

	// Streak metadata
	StreakData.CurrentStreak = RootObject->GetIntegerField(TEXT("currentStreak"));
	StreakData.LongestStreak = RootObject->GetIntegerField(TEXT("longestStreak"));
	StreakData.TotalDaysTracked = RootObject->GetIntegerField(TEXT("totalDaysTracked"));

	FString LastDateStr;
	if (RootObject->TryGetStringField(TEXT("lastTrackedDate"), LastDateStr))
	{
		FDateTime::ParseIso8601(*LastDateStr, StreakData.LastTrackedDate);
	}

	// Current day record
	const TSharedPtr<FJsonObject>* TodayObject;
	if (RootObject->TryGetObjectField(TEXT("today"), TodayObject))
	{
		FString DateStr;
		if ((*TodayObject)->TryGetStringField(TEXT("date"), DateStr))
		{
			FDateTime::ParseIso8601(*DateStr, CurrentDayRecord.Date);
		}
		CurrentDayRecord.StretchesCompleted = (*TodayObject)->GetIntegerField(TEXT("stretches"));
		CurrentDayRecord.BreaksTaken = (*TodayObject)->GetIntegerField(TEXT("breaks"));
		CurrentDayRecord.PomodorosCompleted = (*TodayObject)->GetIntegerField(TEXT("pomodoros"));
		CurrentDayRecord.bMetStretchGoal = (*TodayObject)->GetBoolField(TEXT("metStretchGoal"));
		CurrentDayRecord.bMetBreakGoal = (*TodayObject)->GetBoolField(TEXT("metBreakGoal"));
		CurrentDayRecord.bMetPomodoroGoal = (*TodayObject)->GetBoolField(TEXT("metPomodoroGoal"));
		CurrentDayRecord.bMetAllGoals = (*TodayObject)->GetBoolField(TEXT("metAllGoals"));
	}

	// History
	StreakData.History.Empty();
	const TArray<TSharedPtr<FJsonValue>>* HistoryArray;
	if (RootObject->TryGetArrayField(TEXT("history"), HistoryArray))
	{
		for (const TSharedPtr<FJsonValue>& Value : *HistoryArray)
		{
			const TSharedPtr<FJsonObject>& RecordObject = Value->AsObject();
			if (!RecordObject.IsValid())
			{
				continue;
			}

			FDailyHabitRecord Record;

			FString DateStr;
			if (RecordObject->TryGetStringField(TEXT("date"), DateStr))
			{
				FDateTime::ParseIso8601(*DateStr, Record.Date);
			}
			Record.StretchesCompleted = RecordObject->GetIntegerField(TEXT("stretches"));
			Record.BreaksTaken = RecordObject->GetIntegerField(TEXT("breaks"));
			Record.PomodorosCompleted = RecordObject->GetIntegerField(TEXT("pomodoros"));
			Record.bMetAllGoals = RecordObject->GetBoolField(TEXT("metAllGoals"));

			StreakData.History.Add(Record);
		}
	}

	UE_LOG(LogHabitStreak, Log, TEXT("Loaded habit data: %d days history, current streak: %d, longest: %d"),
		StreakData.History.Num(), StreakData.CurrentStreak, StreakData.LongestStreak);

	// Day may have advanced since last save
	CheckAndAdvanceDay();
}

// ============================================================================
// PRIVATE METHODS
// ============================================================================

void UHabitStreakTracker::CheckAndAdvanceDay()
{
	FDateTime Today = FDateTime::Today();

	if (CurrentDayRecord.Date == Today)
	{
		// Same day, nothing to do
		return;
	}

	// Day changed - finalize previous day record if it had any activity
	if (CurrentDayRecord.StretchesCompleted > 0 ||
		CurrentDayRecord.BreaksTaken > 0 ||
		CurrentDayRecord.PomodorosCompleted > 0)
	{
		StreakData.History.Add(CurrentDayRecord);
		StreakData.TotalDaysTracked++;
		StreakData.LastTrackedDate = CurrentDayRecord.Date;

		UE_LOG(LogHabitStreak, Log, TEXT("Day finalized [%s]: Stretches=%d Breaks=%d Pomodoros=%d AllGoals=%s"),
			*CurrentDayRecord.Date.ToString(),
			CurrentDayRecord.StretchesCompleted,
			CurrentDayRecord.BreaksTaken,
			CurrentDayRecord.PomodorosCompleted,
			CurrentDayRecord.bMetAllGoals ? TEXT("YES") : TEXT("no"));
	}

	// Recalculate streaks from full history
	UpdateStreakFromHistory();

	// Trim old history
	TrimHistory();

	// Start fresh for today
	CurrentDayRecord = FDailyHabitRecord();
	CurrentDayRecord.Date = Today;
}

void UHabitStreakTracker::UpdateStreakFromHistory()
{
	if (StreakData.History.Num() == 0)
	{
		StreakData.CurrentStreak = 0;
		return;
	}

	int32 OldStreak = StreakData.CurrentStreak;

	// Sort history by date descending to walk backward from most recent
	StreakData.History.Sort([](const FDailyHabitRecord& A, const FDailyHabitRecord& B)
	{
		return A.Date > B.Date;
	});

	// Count consecutive days where all goals were met, starting from most recent
	int32 Streak = 0;
	FDateTime ExpectedDate = FDateTime::Today() - FTimespan(1, 0, 0, 0); // Yesterday

	for (const FDailyHabitRecord& Record : StreakData.History)
	{
		if (Record.Date == ExpectedDate && Record.bMetAllGoals)
		{
			Streak++;
			ExpectedDate -= FTimespan(1, 0, 0, 0);
		}
		else if (Record.Date < ExpectedDate)
		{
			// Gap in days - streak broken
			break;
		}
		else if (Record.Date == ExpectedDate && !Record.bMetAllGoals)
		{
			// Day exists but goals not met - streak broken
			break;
		}
	}

	StreakData.CurrentStreak = Streak;

	if (Streak > StreakData.LongestStreak)
	{
		StreakData.LongestStreak = Streak;
	}

	CheckMilestones(OldStreak, Streak);

	UE_LOG(LogHabitStreak, Log, TEXT("Streak updated: %d days (longest: %d)"),
		StreakData.CurrentStreak, StreakData.LongestStreak);
}

void UHabitStreakTracker::EvaluateDailyGoals()
{
	bool bWasStretchGoalMet = CurrentDayRecord.bMetStretchGoal;
	bool bWasBreakGoalMet = CurrentDayRecord.bMetBreakGoal;
	bool bWasPomodoroGoalMet = CurrentDayRecord.bMetPomodoroGoal;
	bool bWasAllGoalsMet = CurrentDayRecord.bMetAllGoals;

	// Evaluate each goal
	CurrentDayRecord.bMetStretchGoal = CurrentDayRecord.StretchesCompleted >= DailyStretchGoal;
	CurrentDayRecord.bMetBreakGoal = CurrentDayRecord.BreaksTaken >= DailyBreakGoal;
	CurrentDayRecord.bMetPomodoroGoal = CurrentDayRecord.PomodorosCompleted >= DailyPomodoroGoal;
	CurrentDayRecord.bMetAllGoals =
		CurrentDayRecord.bMetStretchGoal &&
		CurrentDayRecord.bMetBreakGoal &&
		CurrentDayRecord.bMetPomodoroGoal;

	// Fire delegates for newly-met goals
	if (CurrentDayRecord.bMetStretchGoal && !bWasStretchGoalMet)
	{
		UE_LOG(LogHabitStreak, Log, TEXT("Daily stretch goal met! (%d/%d)"),
			CurrentDayRecord.StretchesCompleted, DailyStretchGoal);
		OnDailyGoalMet.Broadcast(FName(TEXT("Stretch")));
	}

	if (CurrentDayRecord.bMetBreakGoal && !bWasBreakGoalMet)
	{
		UE_LOG(LogHabitStreak, Log, TEXT("Daily break goal met! (%d/%d)"),
			CurrentDayRecord.BreaksTaken, DailyBreakGoal);
		OnDailyGoalMet.Broadcast(FName(TEXT("Break")));
	}

	if (CurrentDayRecord.bMetPomodoroGoal && !bWasPomodoroGoalMet)
	{
		UE_LOG(LogHabitStreak, Log, TEXT("Daily Pomodoro goal met! (%d/%d)"),
			CurrentDayRecord.PomodorosCompleted, DailyPomodoroGoal);
		OnDailyGoalMet.Broadcast(FName(TEXT("Pomodoro")));
	}

	if (CurrentDayRecord.bMetAllGoals && !bWasAllGoalsMet)
	{
		UE_LOG(LogHabitStreak, Log, TEXT("All daily goals met!"));
		OnAllDailyGoalsMet.Broadcast();

		// Immediately update streak since all goals met today
		int32 OldStreak = StreakData.CurrentStreak;
		StreakData.CurrentStreak++;
		if (StreakData.CurrentStreak > StreakData.LongestStreak)
		{
			StreakData.LongestStreak = StreakData.CurrentStreak;
		}
		CheckMilestones(OldStreak, StreakData.CurrentStreak);
	}
}

void UHabitStreakTracker::CheckMilestones(int32 OldStreak, int32 NewStreak)
{
	for (int32 Milestone : StreakMilestones)
	{
		// Fire if we crossed this milestone threshold
		if (NewStreak >= Milestone && OldStreak < Milestone)
		{
			UE_LOG(LogHabitStreak, Log, TEXT("Streak milestone reached: %d days!"), Milestone);
			OnStreakMilestone.Broadcast(Milestone);
		}
	}
}

FString UHabitStreakTracker::GetSaveFilePath() const
{
	return FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("ProductivityTracker"),
		TEXT("HabitStreaks.json")
	);
}

void UHabitStreakTracker::TrimHistory()
{
	if (StreakData.History.Num() <= MaxHistoryDays)
	{
		return;
	}

	// Sort by date ascending so oldest are first
	StreakData.History.Sort([](const FDailyHabitRecord& A, const FDailyHabitRecord& B)
	{
		return A.Date < B.Date;
	});

	int32 ToRemove = StreakData.History.Num() - MaxHistoryDays;
	StreakData.History.RemoveAt(0, ToRemove);

	UE_LOG(LogHabitStreak, Verbose, TEXT("Trimmed %d old habit records (keeping %d days)"),
		ToRemove, MaxHistoryDays);
}
