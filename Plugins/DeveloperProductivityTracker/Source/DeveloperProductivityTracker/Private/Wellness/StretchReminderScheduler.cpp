// ============================================================================
// StretchReminderScheduler.cpp
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Implementation of stretch reminder scheduling and exercise management.
// ============================================================================

#include "Wellness/StretchReminderScheduler.h"

DEFINE_LOG_CATEGORY(LogStretchReminder);

UStretchReminderScheduler::UStretchReminderScheduler()
	: ReminderIntervalMinutes(45.0f)
	, DefaultSnoozeMinutes(5.0f)
	, bRandomizeExercises(true)
	, MaxHistoryEvents(100)
	, bIsActive(false)
	, bReminderActive(false)
	, TimeSinceLastReminder(0.0f)
	, CurrentExerciseIndex(0)
{
	InitializeExerciseLibrary();
}

void UStretchReminderScheduler::Tick(float DeltaTime)
{
	if (!bIsActive || bReminderActive)
	{
		return;
	}

	TimeSinceLastReminder += DeltaTime;

	float IntervalSeconds = ReminderIntervalMinutes * 60.0f;
	if (TimeSinceLastReminder >= IntervalSeconds)
	{
		TriggerReminder();
	}
}

// ============================================================================
// CONTROLS
// ============================================================================

void UStretchReminderScheduler::StartScheduler()
{
	if (bIsActive)
	{
		return;
	}

	bIsActive = true;
	TimeSinceLastReminder = 0.0f;

	UE_LOG(LogStretchReminder, Log, TEXT("Stretch reminder scheduler started - Interval: %.0f minutes"),
		ReminderIntervalMinutes);
}

void UStretchReminderScheduler::StopScheduler()
{
	if (!bIsActive)
	{
		return;
	}

	bIsActive = false;
	bReminderActive = false;

	UE_LOG(LogStretchReminder, Log, TEXT("Stretch reminder scheduler stopped"));
}

void UStretchReminderScheduler::SnoozeReminder(float SnoozeMinutes)
{
	if (!bReminderActive)
	{
		return;
	}

	// Record snooze
	RecordReminderEvent(false, true, false);

	bReminderActive = false;
	TimeSinceLastReminder = (ReminderIntervalMinutes - SnoozeMinutes) * 60.0f;

	UE_LOG(LogStretchReminder, Log, TEXT("Reminder snoozed for %.0f minutes"), SnoozeMinutes);

	OnStretchReminderSnoozed.Broadcast();
}

void UStretchReminderScheduler::SkipReminder()
{
	if (!bReminderActive)
	{
		return;
	}

	// Record skip
	RecordReminderEvent(false, false, true);

	bReminderActive = false;
	TimeSinceLastReminder = 0.0f;

	UE_LOG(LogStretchReminder, Log, TEXT("Reminder skipped"));

	OnStretchReminderSkipped.Broadcast();
}

void UStretchReminderScheduler::CompleteStretch()
{
	if (!bReminderActive)
	{
		return;
	}

	// Record completion
	RecordReminderEvent(true, false, false);

	bReminderActive = false;
	TimeSinceLastReminder = 0.0f;

	UE_LOG(LogStretchReminder, Log, TEXT("Stretch completed: %s"), *CurrentExercise.Name);

	OnStretchReminderCompleted.Broadcast();
}

void UStretchReminderScheduler::TriggerReminderNow()
{
	if (bReminderActive)
	{
		return;
	}

	TriggerReminder();
}

// ============================================================================
// QUERIES
// ============================================================================

float UStretchReminderScheduler::GetSecondsUntilNextReminder() const
{
	if (!bIsActive || bReminderActive)
	{
		return 0.0f;
	}

	float IntervalSeconds = ReminderIntervalMinutes * 60.0f;
	return FMath::Max(0.0f, IntervalSeconds - TimeSinceLastReminder);
}

FString UStretchReminderScheduler::GetFormattedTimeUntilNext() const
{
	float Remaining = GetSecondsUntilNextReminder();
	int32 Minutes = FMath::FloorToInt(Remaining / 60.0f);
	int32 Seconds = FMath::FloorToInt(FMath::Fmod(Remaining, 60.0f));

	return FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
}

int32 UStretchReminderScheduler::GetTodayCompletedCount() const
{
	int32 Count = 0;
	FDateTime Today = FDateTime::Today();

	for (const FStretchReminderEvent& Event : ReminderHistory)
	{
		if (Event.bWasAccepted && Event.ActualTime >= Today)
		{
			Count++;
		}
	}

	return Count;
}

// ============================================================================
// PRIVATE METHODS
// ============================================================================

void UStretchReminderScheduler::InitializeExerciseLibrary()
{
	// Neck stretches (seated, low difficulty)
	AvailableExercises.Add(FStretchExercise(
		TEXT("Neck Tilt"),
		TEXT("Slowly tilt your head to the left, hold for 15 seconds, then tilt to the right."),
		TEXT("Neck"), 30, 1, false
	));

	AvailableExercises.Add(FStretchExercise(
		TEXT("Neck Rotation"),
		TEXT("Slowly rotate your head in a circular motion, first clockwise, then counter-clockwise."),
		TEXT("Neck"), 30, 1, false
	));

	// Shoulder stretches (seated, low difficulty)
	AvailableExercises.Add(FStretchExercise(
		TEXT("Shoulder Shrugs"),
		TEXT("Raise both shoulders up towards your ears, hold for 5 seconds, then release. Repeat 5 times."),
		TEXT("Shoulders"), 30, 1, false
	));

	AvailableExercises.Add(FStretchExercise(
		TEXT("Shoulder Rolls"),
		TEXT("Roll your shoulders forward 5 times, then backward 5 times."),
		TEXT("Shoulders"), 30, 1, false
	));

	// Wrist stretches (seated, low difficulty - critical for developers)
	AvailableExercises.Add(FStretchExercise(
		TEXT("Wrist Extension"),
		TEXT("Extend your arm, palm up. Use the other hand to gently pull fingers back. Hold 15 seconds each side."),
		TEXT("Wrists"), 30, 1, false
	));

	AvailableExercises.Add(FStretchExercise(
		TEXT("Wrist Circles"),
		TEXT("Make circles with your wrists, 10 times clockwise, then 10 times counter-clockwise."),
		TEXT("Wrists"), 20, 1, false
	));

	// Back stretches (mixed difficulty)
	AvailableExercises.Add(FStretchExercise(
		TEXT("Seated Twist"),
		TEXT("Sit up straight, twist your torso to the left, hold 15 seconds. Repeat on the right."),
		TEXT("Back"), 30, 2, false
	));

	AvailableExercises.Add(FStretchExercise(
		TEXT("Cat-Cow Stretch"),
		TEXT("If space allows, get on hands and knees. Arch your back up, then dip it down. Repeat 5 times."),
		TEXT("Back"), 45, 3, true
	));

	// Eye exercises (seated, lowest difficulty)
	AvailableExercises.Add(FStretchExercise(
		TEXT("20-20-20 Rule"),
		TEXT("Look at something 20 feet away for 20 seconds. This reduces eye strain from screens."),
		TEXT("Eyes"), 20, 1, false
	));

	AvailableExercises.Add(FStretchExercise(
		TEXT("Eye Circles"),
		TEXT("Without moving your head, roll your eyes in circles. 5 times clockwise, 5 times counter-clockwise."),
		TEXT("Eyes"), 20, 1, false
	));

	// Standing stretches (higher difficulty)
	AvailableExercises.Add(FStretchExercise(
		TEXT("Standing Stretch"),
		TEXT("Stand up, reach your arms overhead, and stretch your whole body. Hold for 10 seconds."),
		TEXT("Full Body"), 15, 2, true
	));

	AvailableExercises.Add(FStretchExercise(
		TEXT("Calf Raises"),
		TEXT("Stand and raise onto your toes, hold briefly, then lower. Repeat 10 times."),
		TEXT("Legs"), 30, 2, true
	));

	// === NEW EXERCISES (expanded library) ===

	// Chest and upper body
	AvailableExercises.Add(FStretchExercise(
		TEXT("Chest Opener"),
		TEXT("Clasp your hands behind your back, straighten your arms and lift slightly. Open your chest and hold for 20 seconds."),
		TEXT("Chest"), 30, 2, false
	));

	// Hips (seated)
	AvailableExercises.Add(FStretchExercise(
		TEXT("Seated Hip Stretch"),
		TEXT("Cross your right ankle over your left knee. Gently press the right knee down. Hold 15 seconds, then switch."),
		TEXT("Hips"), 30, 2, false
	));

	// Hands (seated, critical for devs)
	AvailableExercises.Add(FStretchExercise(
		TEXT("Finger Stretches"),
		TEXT("Spread your fingers wide, hold 5 seconds, then make a fist. Repeat 5 times. Then touch each finger to your thumb."),
		TEXT("Hands"), 20, 1, false
	));

	// Neck (seated, posture correction)
	AvailableExercises.Add(FStretchExercise(
		TEXT("Chin Tucks"),
		TEXT("Sit tall, draw your chin straight back (making a double chin). Hold 5 seconds. Repeat 5 times. Combats forward head posture."),
		TEXT("Neck"), 20, 1, false
	));

	// Arms (standing)
	AvailableExercises.Add(FStretchExercise(
		TEXT("Wall Push-ups"),
		TEXT("Stand arm's length from a wall. Place palms on wall at shoulder height. Do 10 slow push-ups against the wall."),
		TEXT("Arms"), 45, 3, true
	));

	// Legs (standing)
	AvailableExercises.Add(FStretchExercise(
		TEXT("Leg Swings"),
		TEXT("Hold a wall or chair for balance. Swing one leg forward and back 10 times, then switch legs. Keep core engaged."),
		TEXT("Legs"), 30, 3, true
	));

	// Breathing (seated, recovery)
	AvailableExercises.Add(FStretchExercise(
		TEXT("Deep Breathing"),
		TEXT("Inhale deeply for 4 seconds, hold for 4, exhale for 6. Repeat 5 times. Activates the parasympathetic nervous system."),
		TEXT("Full Body"), 60, 1, false
	));

	// Core (standing, high difficulty)
	AvailableExercises.Add(FStretchExercise(
		TEXT("Desk Plank"),
		TEXT("Place your hands on the edge of your desk, walk your feet back to a plank position. Hold for 20 seconds."),
		TEXT("Core"), 30, 4, true
	));

	UE_LOG(LogStretchReminder, Log, TEXT("Initialized %d stretch exercises"), AvailableExercises.Num());
}

FStretchExercise UStretchReminderScheduler::SelectNextExercise()
{
	if (AvailableExercises.Num() == 0)
	{
		return FStretchExercise();
	}

	if (bRandomizeExercises)
	{
		int32 Index = FMath::RandRange(0, AvailableExercises.Num() - 1);
		return AvailableExercises[Index];
	}
	else
	{
		// Sequential selection
		FStretchExercise Exercise = AvailableExercises[CurrentExerciseIndex];
		CurrentExerciseIndex = (CurrentExerciseIndex + 1) % AvailableExercises.Num();
		return Exercise;
	}
}

void UStretchReminderScheduler::TriggerReminder()
{
	CurrentExercise = SelectNextExercise();
	bReminderActive = true;

	// Start tracking this reminder
	CurrentReminderEvent = FStretchReminderEvent();
	CurrentReminderEvent.ScheduledTime = FDateTime::Now();
	CurrentReminderEvent.Exercise = CurrentExercise;

	UE_LOG(LogStretchReminder, Log, TEXT("Stretch reminder triggered: %s (%s)"),
		*CurrentExercise.Name, *CurrentExercise.TargetArea);

	OnStretchReminderTriggered.Broadcast(CurrentExercise);
}

void UStretchReminderScheduler::RecordReminderEvent(bool bAccepted, bool bSnoozed, bool bSkipped)
{
	CurrentReminderEvent.ActualTime = FDateTime::Now();
	CurrentReminderEvent.bWasAccepted = bAccepted;
	CurrentReminderEvent.bWasSnoozed = bSnoozed;
	CurrentReminderEvent.bWasSkipped = bSkipped;

	ReminderHistory.Add(CurrentReminderEvent);

	// Trim oldest events when history exceeds configured limit
	while (ReminderHistory.Num() > MaxHistoryEvents)
	{
		ReminderHistory.RemoveAt(0);
	}
}
