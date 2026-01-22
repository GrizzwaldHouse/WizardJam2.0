// ============================================================================
// BreakWellnessSubsystem.cpp
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Implementation of the central wellness coordinator subsystem.
// ============================================================================

#include "Wellness/BreakWellnessSubsystem.h"
#include "Core/ProductivityTrackerSettings.h"

DEFINE_LOG_CATEGORY(LogWellness);

UBreakWellnessSubsystem::UBreakWellnessSubsystem()
	: bIsEnabled(true)
	, CurrentWellnessStatus(EWellnessStatus::Good)
	, PreviousWellnessStatus(EWellnessStatus::Good)
	, SecondsSinceLastBreak(0.0f)
	, LastBreakEndTime(FDateTime::Now())
	, PomodoroManager(nullptr)
	, SmartBreakDetector(nullptr)
	, BreakQualityEvaluator(nullptr)
	, StretchReminderScheduler(nullptr)
	, MinutesBeforeBreakSuggestion(45.0f)
	, MinutesBeforeOverworkedWarning(90.0f)
	, TodayWorkSeconds(0.0f)
	, TodayBreakSeconds(0.0f)
{
}

void UBreakWellnessSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogWellness, Log, TEXT("BreakWellnessSubsystem initializing..."));

	// Load settings
	const UProductivityTrackerSettings* Settings = UProductivityTrackerSettings::Get();
	if (Settings)
	{
		bIsEnabled = Settings->bEnableWellnessFeatures;
	}

	// Initialize components
	InitializeComponents();

	UE_LOG(LogWellness, Log, TEXT("BreakWellnessSubsystem initialized (Enabled: %s)"),
		bIsEnabled ? TEXT("Yes") : TEXT("No"));
}

void UBreakWellnessSubsystem::Deinitialize()
{
	UE_LOG(LogWellness, Log, TEXT("BreakWellnessSubsystem deinitializing..."));

	// Stop all schedulers
	if (StretchReminderScheduler)
	{
		StretchReminderScheduler->StopScheduler();
	}

	if (PomodoroManager)
	{
		PomodoroManager->StopPomodoro();
	}

	Super::Deinitialize();
}

void UBreakWellnessSubsystem::Tick(float DeltaTime)
{
	if (!bIsEnabled)
	{
		return;
	}

	// Check if we're on a detected break
	bool bOnBreak = SmartBreakDetector && SmartBreakDetector->IsOnDetectedBreak();
	bool bOnPomodoroBreak = PomodoroManager &&
		(PomodoroManager->GetCurrentState() == EPomodoroState::ShortBreak ||
		 PomodoroManager->GetCurrentState() == EPomodoroState::LongBreak);

	// Update time trackers
	if (bOnBreak || bOnPomodoroBreak)
	{
		TodayBreakSeconds += DeltaTime;
		SecondsSinceLastBreak = 0.0f;
	}
	else
	{
		TodayWorkSeconds += DeltaTime;
		SecondsSinceLastBreak += DeltaTime;
	}

	// Tick components
	if (PomodoroManager)
	{
		PomodoroManager->Tick(DeltaTime);
	}

	if (SmartBreakDetector)
	{
		SmartBreakDetector->Tick(DeltaTime);
	}

	if (StretchReminderScheduler)
	{
		StretchReminderScheduler->Tick(DeltaTime);
	}

	// Update wellness status
	UpdateWellnessStatus();
}

TStatId UBreakWellnessSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UBreakWellnessSubsystem, STATGROUP_Tickables);
}

// ============================================================================
// WELLNESS STATUS
// ============================================================================

FWellnessStatistics UBreakWellnessSubsystem::GetWellnessStatistics() const
{
	FWellnessStatistics Stats;

	Stats.TodayWorkMinutes = TodayWorkSeconds / 60.0f;
	Stats.TodayBreakMinutes = TodayBreakSeconds / 60.0f;
	Stats.MinutesSinceLastBreak = SecondsSinceLastBreak / 60.0f;
	Stats.CurrentStatus = CurrentWellnessStatus;

	// Pomodoro stats
	if (PomodoroManager)
	{
		Stats.TodayPomodorosCompleted = PomodoroManager->GetStatistics().CompletedWorkIntervals;
	}

	// Stretch stats
	if (StretchReminderScheduler)
	{
		Stats.TodayStretchesCompleted = StretchReminderScheduler->GetTodayCompletedCount();
	}

	// Calculate average break quality
	if (TodayBreakQualities.Num() > 0)
	{
		float Sum = 0.0f;
		for (float Quality : TodayBreakQualities)
		{
			Sum += Quality;
		}
		Stats.AverageBreakQuality = Sum / static_cast<float>(TodayBreakQualities.Num());
	}

	return Stats;
}

FString UBreakWellnessSubsystem::GetWellnessStatusDisplayString() const
{
	switch (CurrentWellnessStatus)
	{
	case EWellnessStatus::Optimal:
		return TEXT("Optimal - Well rested and productive");
	case EWellnessStatus::Good:
		return TEXT("Good - Working well");
	case EWellnessStatus::NeedBreak:
		return TEXT("Consider taking a break");
	case EWellnessStatus::OnBreak:
		return TEXT("On break - Good job!");
	case EWellnessStatus::Overworked:
		return TEXT("You should take a break soon");
	default:
		return TEXT("Unknown");
	}
}

FLinearColor UBreakWellnessSubsystem::GetWellnessStatusColor() const
{
	switch (CurrentWellnessStatus)
	{
	case EWellnessStatus::Optimal:
		return FLinearColor(0.2f, 0.8f, 0.2f); // Green
	case EWellnessStatus::Good:
		return FLinearColor(0.6f, 0.8f, 0.2f); // Yellow-green
	case EWellnessStatus::NeedBreak:
		return FLinearColor(1.0f, 0.8f, 0.0f); // Yellow
	case EWellnessStatus::OnBreak:
		return FLinearColor(0.2f, 0.6f, 1.0f); // Blue
	case EWellnessStatus::Overworked:
		return FLinearColor(1.0f, 0.4f, 0.2f); // Orange-red
	default:
		return FLinearColor::Gray;
	}
}

float UBreakWellnessSubsystem::GetMinutesSinceLastBreak() const
{
	return SecondsSinceLastBreak / 60.0f;
}

// ============================================================================
// QUICK ACTIONS
// ============================================================================

void UBreakWellnessSubsystem::StartQuickBreak()
{
	// Pause Pomodoro if it's in work mode
	if (PomodoroManager && PomodoroManager->GetCurrentState() == EPomodoroState::Working)
	{
		PomodoroManager->PausePomodoro();
	}

	// Pause stretch reminders
	if (StretchReminderScheduler)
	{
		StretchReminderScheduler->StopScheduler();
	}

	UE_LOG(LogWellness, Log, TEXT("Quick break started"));
}

void UBreakWellnessSubsystem::EndBreakAndResume()
{
	// Reset break tracking
	SecondsSinceLastBreak = 0.0f;
	LastBreakEndTime = FDateTime::Now();

	// Resume Pomodoro if it was paused
	if (PomodoroManager && PomodoroManager->GetCurrentState() == EPomodoroState::Paused)
	{
		PomodoroManager->ResumePomodoro();
	}

	// Resume stretch reminders
	const UProductivityTrackerSettings* Settings = UProductivityTrackerSettings::Get();
	if (StretchReminderScheduler && Settings && Settings->bEnableStretchReminders)
	{
		StretchReminderScheduler->StartScheduler();
	}

	UE_LOG(LogWellness, Log, TEXT("Break ended, resuming work"));
}

void UBreakWellnessSubsystem::SetWellnessEnabled(bool bEnabled)
{
	bIsEnabled = bEnabled;

	if (!bIsEnabled)
	{
		// Stop all wellness features
		if (PomodoroManager)
		{
			PomodoroManager->StopPomodoro();
		}
		if (StretchReminderScheduler)
		{
			StretchReminderScheduler->StopScheduler();
		}
	}

	UE_LOG(LogWellness, Log, TEXT("Wellness features %s"), bEnabled ? TEXT("enabled") : TEXT("disabled"));
}

// ============================================================================
// PRIVATE METHODS
// ============================================================================

void UBreakWellnessSubsystem::InitializeComponents()
{
	const UProductivityTrackerSettings* Settings = UProductivityTrackerSettings::Get();

	// Create Pomodoro manager
	PomodoroManager = NewObject<UPomodoroManager>(this);
	if (Settings)
	{
		PomodoroManager->WorkIntervalMinutes = Settings->PomodoroWorkMinutes;
		PomodoroManager->ShortBreakMinutes = Settings->PomodoroShortBreakMinutes;
		PomodoroManager->LongBreakMinutes = Settings->PomodoroLongBreakMinutes;
		PomodoroManager->WorkIntervalsBeforeLongBreak = Settings->PomodoroIntervalsBeforeLongBreak;
	}
	PomodoroManager->OnPomodoroStateChanged.AddDynamic(this, &UBreakWellnessSubsystem::HandlePomodoroStateChanged);

	// Create smart break detector
	SmartBreakDetector = NewObject<USmartBreakDetector>(this);
	SmartBreakDetector->OnBreakDetected.AddDynamic(this, &UBreakWellnessSubsystem::HandleBreakDetected);
	SmartBreakDetector->OnBreakEnded.AddDynamic(this, &UBreakWellnessSubsystem::HandleBreakEnded);

	// Create break quality evaluator
	BreakQualityEvaluator = NewObject<UBreakQualityEvaluator>(this);

	// Create stretch reminder scheduler
	StretchReminderScheduler = NewObject<UStretchReminderScheduler>(this);
	if (Settings)
	{
		StretchReminderScheduler->ReminderIntervalMinutes = Settings->StretchReminderIntervalMinutes;
	}
	StretchReminderScheduler->OnStretchReminderCompleted.AddDynamic(this, &UBreakWellnessSubsystem::HandleStretchCompleted);

	// Auto-start stretch reminders if enabled
	if (Settings && Settings->bEnableStretchReminders)
	{
		StretchReminderScheduler->StartScheduler();
	}

	UE_LOG(LogWellness, Log, TEXT("Wellness components initialized"));
}

void UBreakWellnessSubsystem::UpdateWellnessStatus()
{
	EWellnessStatus NewStatus = CalculateWellnessStatus();

	if (NewStatus != CurrentWellnessStatus)
	{
		PreviousWellnessStatus = CurrentWellnessStatus;
		CurrentWellnessStatus = NewStatus;

		UE_LOG(LogWellness, Log, TEXT("Wellness status changed: %s -> %s"),
			*UEnum::GetValueAsString(PreviousWellnessStatus),
			*UEnum::GetValueAsString(CurrentWellnessStatus));

		OnWellnessStatusChanged.Broadcast(CurrentWellnessStatus);
	}
}

EWellnessStatus UBreakWellnessSubsystem::CalculateWellnessStatus() const
{
	// Check if on break
	bool bOnBreak = (SmartBreakDetector && SmartBreakDetector->IsOnDetectedBreak()) ||
		(PomodoroManager &&
		 (PomodoroManager->GetCurrentState() == EPomodoroState::ShortBreak ||
		  PomodoroManager->GetCurrentState() == EPomodoroState::LongBreak));

	if (bOnBreak)
	{
		return EWellnessStatus::OnBreak;
	}

	float MinutesSinceBreak = SecondsSinceLastBreak / 60.0f;

	// Check for overworked
	if (MinutesSinceBreak > MinutesBeforeOverworkedWarning)
	{
		return EWellnessStatus::Overworked;
	}

	// Check for need break
	if (MinutesSinceBreak > MinutesBeforeBreakSuggestion)
	{
		return EWellnessStatus::NeedBreak;
	}

	// Check for optimal (had a quality break recently)
	if (MinutesSinceBreak < 15.0f && TodayBreakQualities.Num() > 0)
	{
		float LastQuality = TodayBreakQualities.Last();
		if (LastQuality >= 60.0f)
		{
			return EWellnessStatus::Optimal;
		}
	}

	return EWellnessStatus::Good;
}

void UBreakWellnessSubsystem::HandlePomodoroStateChanged(EPomodoroState NewState)
{
	UE_LOG(LogWellness, Verbose, TEXT("Pomodoro state changed to: %s"),
		*UEnum::GetValueAsString(NewState));

	// Reset break timer when transitioning from break to work
	if (NewState == EPomodoroState::Working)
	{
		SecondsSinceLastBreak = 0.0f;
		LastBreakEndTime = FDateTime::Now();
	}
}

void UBreakWellnessSubsystem::HandleBreakDetected(float Confidence)
{
	UE_LOG(LogWellness, Log, TEXT("Smart break detected (Confidence: %.2f)"), Confidence);
}

void UBreakWellnessSubsystem::HandleBreakEnded(const FDetectedBreak& BreakData)
{
	// Evaluate break quality
	if (BreakQualityEvaluator)
	{
		FBreakQualityReport Report = BreakQualityEvaluator->EvaluateBreak(
			BreakData.DurationSeconds,
			true, // Assume screen was off during detected break
			0,    // No app switch data
			false // No movement data
		);

		TodayBreakQualities.Add(Report.OverallScore);

		UE_LOG(LogWellness, Log, TEXT("Break quality: %.1f (%s) - %s"),
			Report.OverallScore,
			*Report.GetTierDisplayString(),
			*Report.Feedback);
	}

	// Reset break timer
	SecondsSinceLastBreak = 0.0f;
	LastBreakEndTime = FDateTime::Now();
}

void UBreakWellnessSubsystem::HandleStretchCompleted()
{
	UE_LOG(LogWellness, Log, TEXT("Stretch exercise completed"));
}
