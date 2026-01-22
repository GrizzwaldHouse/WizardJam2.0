// ============================================================================
// NotificationSubsystem.cpp
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================

#include "UI/NotificationSubsystem.h"
#include "Core/ProductivityTrackerSettings.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

DEFINE_LOG_CATEGORY(LogProductivityNotification);

UNotificationSubsystem::UNotificationSubsystem()
	: bNotificationsEnabled(true)
	, bSoundsEnabled(false)
{
}

void UNotificationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Load settings
	const UProductivityTrackerSettings* Settings = UProductivityTrackerSettings::Get();
	if (Settings)
	{
		bNotificationsEnabled = Settings->bEnableNotifications;
		bSoundsEnabled = Settings->bEnableNotificationSounds;
	}

	UE_LOG(LogProductivityNotification, Log, TEXT("NotificationSubsystem initialized (Enabled: %s)"),
		bNotificationsEnabled ? TEXT("Yes") : TEXT("No"));
}

void UNotificationSubsystem::Deinitialize()
{
	DismissAllNotifications();
	Super::Deinitialize();
}

void UNotificationSubsystem::ShowNotification(const FProductivityNotification& Notification)
{
	if (!bNotificationsEnabled)
	{
		return;
	}

	DisplaySlateNotification(Notification);

	if (bSoundsEnabled)
	{
		PlayNotificationSound(Notification.Type);
	}

	UE_LOG(LogProductivityNotification, Log, TEXT("Notification shown: %s - %s"),
		*Notification.Title.ToString(), *Notification.Message.ToString());
}

void UNotificationSubsystem::ShowSimpleNotification(const FText& Title, const FText& Message, ENotificationType Type)
{
	FProductivityNotification Notification;
	Notification.Title = Title;
	Notification.Message = Message;
	Notification.Type = Type;
	Notification.Priority = ENotificationPriority::Normal;

	ShowNotification(Notification);
}

void UNotificationSubsystem::ShowBreakReminder(float MinutesWorked)
{
	FProductivityNotification Notification;
	Notification.Title = NSLOCTEXT("Productivity", "BreakReminderTitle", "Time for a Break");
	Notification.Message = FText::Format(
		NSLOCTEXT("Productivity", "BreakReminderMessage", "You've been working for {0} minutes. Consider taking a short break to stay productive."),
		FText::AsNumber(FMath::RoundToInt(MinutesWorked)));
	Notification.Type = ENotificationType::Break;
	Notification.Priority = ENotificationPriority::High;
	Notification.DurationSeconds = 10.0f;
	Notification.bHasActions = true;
	Notification.ActionButtonText = TEXT("Take Break");
	Notification.DismissButtonText = TEXT("Later");

	ShowNotification(Notification);
}

void UNotificationSubsystem::ShowPomodoroNotification(const FText& Message, bool bIsBreakTime)
{
	FProductivityNotification Notification;
	Notification.Title = bIsBreakTime
		? NSLOCTEXT("Productivity", "PomodoroBreakTitle", "Break Time!")
		: NSLOCTEXT("Productivity", "PomodoroWorkTitle", "Back to Work!");
	Notification.Message = Message;
	Notification.Type = ENotificationType::Pomodoro;
	Notification.Priority = ENotificationPriority::High;
	Notification.DurationSeconds = 8.0f;

	ShowNotification(Notification);
}

void UNotificationSubsystem::ShowStretchReminder(const FString& ExerciseName, const FString& ExerciseDescription)
{
	FProductivityNotification Notification;
	Notification.Title = NSLOCTEXT("Productivity", "StretchTitle", "Stretch Break");
	Notification.Message = FText::Format(
		NSLOCTEXT("Productivity", "StretchMessage", "{0}\n\n{1}"),
		FText::FromString(ExerciseName),
		FText::FromString(ExerciseDescription));
	Notification.Type = ENotificationType::Stretch;
	Notification.Priority = ENotificationPriority::Normal;
	Notification.DurationSeconds = 15.0f;
	Notification.bHasActions = true;
	Notification.ActionButtonText = TEXT("Done");
	Notification.DismissButtonText = TEXT("Skip");

	ShowNotification(Notification);
}

void UNotificationSubsystem::ShowSessionSummary(float TotalMinutes, float ActivePercentage)
{
	FProductivityNotification Notification;
	Notification.Title = NSLOCTEXT("Productivity", "SessionSummaryTitle", "Session Complete");
	Notification.Message = FText::Format(
		NSLOCTEXT("Productivity", "SessionSummaryMessage", "You worked for {0} minutes with {1}% active time. Great job!"),
		FText::AsNumber(FMath::RoundToInt(TotalMinutes)),
		FText::AsNumber(FMath::RoundToInt(ActivePercentage)));
	Notification.Type = ENotificationType::Success;
	Notification.Priority = ENotificationPriority::Normal;
	Notification.DurationSeconds = 8.0f;

	ShowNotification(Notification);
}

void UNotificationSubsystem::DismissAllNotifications()
{
	// Note: FSlateNotificationManager doesn't expose active notifications directly
	// Notifications will expire naturally based on their ExpireDuration
}

void UNotificationSubsystem::SetNotificationsEnabled(bool bEnabled)
{
	bNotificationsEnabled = bEnabled;
}

void UNotificationSubsystem::SetSoundsEnabled(bool bEnabled)
{
	bSoundsEnabled = bEnabled;
}

void UNotificationSubsystem::DisplaySlateNotification(const FProductivityNotification& Notification)
{
	FNotificationInfo Info(Notification.Message);
	Info.bFireAndForget = true;
	Info.FadeOutDuration = 0.5f;
	Info.ExpireDuration = Notification.DurationSeconds;
	Info.bUseThrobber = false;
	Info.bUseSuccessFailIcons = false;

	// Set hyperlink text as title
	Info.Hyperlink = FSimpleDelegate();
	Info.HyperlinkText = Notification.Title;

	// Add action buttons if specified
	if (Notification.bHasActions)
	{
		Info.ButtonDetails.Add(FNotificationButtonInfo(
			FText::FromString(Notification.ActionButtonText),
			FText::GetEmpty(),
			FSimpleDelegate::CreateLambda([this]()
			{
				OnNotificationAction.Broadcast(TEXT("action"));
			}),
			SNotificationItem::CS_None
		));

		Info.ButtonDetails.Add(FNotificationButtonInfo(
			FText::FromString(Notification.DismissButtonText),
			FText::GetEmpty(),
			FSimpleDelegate::CreateLambda([this]()
			{
				OnNotificationDismissed.Broadcast();
			}),
			SNotificationItem::CS_None
		));
	}

	FSlateNotificationManager::Get().AddNotification(Info);
}

FLinearColor UNotificationSubsystem::GetColorForType(ENotificationType Type) const
{
	switch (Type)
	{
	case ENotificationType::Success:
		return FLinearColor(0.2f, 0.8f, 0.2f);
	case ENotificationType::Warning:
		return FLinearColor(1.0f, 0.8f, 0.0f);
	case ENotificationType::Break:
		return FLinearColor(0.2f, 0.6f, 1.0f);
	case ENotificationType::Pomodoro:
		return FLinearColor(1.0f, 0.4f, 0.4f);
	case ENotificationType::Stretch:
		return FLinearColor(0.6f, 0.8f, 0.2f);
	case ENotificationType::Achievement:
		return FLinearColor(1.0f, 0.8f, 0.2f);
	case ENotificationType::Information:
	default:
		return FLinearColor(0.5f, 0.5f, 0.5f);
	}
}

void UNotificationSubsystem::PlayNotificationSound(ENotificationType Type)
{
	// Sound playback would be implemented here
	// For now, we skip as it requires audio assets
}
