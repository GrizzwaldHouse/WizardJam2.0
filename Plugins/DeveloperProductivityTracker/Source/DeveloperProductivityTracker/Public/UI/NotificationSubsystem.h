// ============================================================================
// NotificationSubsystem.h
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Manages non-intrusive notifications for productivity events.
// Toast-style notifications for breaks, Pomodoro, stretches, etc.
//
// Architecture:
// Editor Subsystem that queues and displays notifications.
// Integrates with Slate for editor UI presentation.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "NotificationSubsystem.generated.h"

// Log category
DECLARE_LOG_CATEGORY_EXTERN(LogProductivityNotification, Log, All);

// Notification priority levels
UENUM(BlueprintType)
enum class ENotificationPriority : uint8
{
	Low      UMETA(DisplayName = "Low"),       // Informational
	Normal   UMETA(DisplayName = "Normal"),    // Standard notifications
	High     UMETA(DisplayName = "High"),      // Important - break reminders
	Urgent   UMETA(DisplayName = "Urgent")     // Critical - overwork warnings
};

// Notification type for styling
UENUM(BlueprintType)
enum class ENotificationType : uint8
{
	Information  UMETA(DisplayName = "Information"),
	Success      UMETA(DisplayName = "Success"),
	Warning      UMETA(DisplayName = "Warning"),
	Break        UMETA(DisplayName = "Break"),
	Pomodoro     UMETA(DisplayName = "Pomodoro"),
	Stretch      UMETA(DisplayName = "Stretch"),
	Achievement  UMETA(DisplayName = "Achievement")
};

// Notification data
USTRUCT(BlueprintType)
struct DEVELOPERPRODUCTIVITYTRACKER_API FProductivityNotification
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notification")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notification")
	FText Message;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notification")
	ENotificationType Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notification")
	ENotificationPriority Priority;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notification")
	float DurationSeconds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notification")
	bool bHasActions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notification")
	FString ActionButtonText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notification")
	FString DismissButtonText;

	FProductivityNotification()
		: Type(ENotificationType::Information)
		, Priority(ENotificationPriority::Normal)
		, DurationSeconds(5.0f)
		, bHasActions(false)
	{
	}
};

// Delegate for notification actions
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNotificationAction, const FString&, ActionId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNotificationDismissed);

UCLASS()
class DEVELOPERPRODUCTIVITYTRACKER_API UNotificationSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	UNotificationSubsystem();

	// USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ========================================================================
	// NOTIFICATION METHODS
	// ========================================================================

	// Show a notification
	UFUNCTION(BlueprintCallable, Category = "Productivity|Notifications")
	void ShowNotification(const FProductivityNotification& Notification);

	// Show a simple text notification
	UFUNCTION(BlueprintCallable, Category = "Productivity|Notifications")
	void ShowSimpleNotification(const FText& Title, const FText& Message, ENotificationType Type = ENotificationType::Information);

	// Show a break reminder notification
	UFUNCTION(BlueprintCallable, Category = "Productivity|Notifications")
	void ShowBreakReminder(float MinutesWorked);

	// Show Pomodoro notification
	UFUNCTION(BlueprintCallable, Category = "Productivity|Notifications")
	void ShowPomodoroNotification(const FText& Message, bool bIsBreakTime);

	// Show stretch reminder
	UFUNCTION(BlueprintCallable, Category = "Productivity|Notifications")
	void ShowStretchReminder(const FString& ExerciseName, const FString& ExerciseDescription);

	// Show session summary
	UFUNCTION(BlueprintCallable, Category = "Productivity|Notifications")
	void ShowSessionSummary(float TotalMinutes, float ActivePercentage);

	// Dismiss all notifications
	UFUNCTION(BlueprintCallable, Category = "Productivity|Notifications")
	void DismissAllNotifications();

	// ========================================================================
	// CONFIGURATION
	// ========================================================================

	// Enable/disable notifications
	UFUNCTION(BlueprintCallable, Category = "Productivity|Notifications")
	void SetNotificationsEnabled(bool bEnabled);

	// Check if notifications are enabled
	UFUNCTION(BlueprintPure, Category = "Productivity|Notifications")
	bool AreNotificationsEnabled() const { return bNotificationsEnabled; }

	// Set whether to play sounds
	UFUNCTION(BlueprintCallable, Category = "Productivity|Notifications")
	void SetSoundsEnabled(bool bEnabled);

	// ========================================================================
	// DELEGATES
	// ========================================================================

	UPROPERTY(BlueprintAssignable, Category = "Productivity|Events")
	FOnNotificationAction OnNotificationAction;

	UPROPERTY(BlueprintAssignable, Category = "Productivity|Events")
	FOnNotificationDismissed OnNotificationDismissed;

private:
	// State
	bool bNotificationsEnabled;
	bool bSoundsEnabled;

	// Methods
	void DisplaySlateNotification(const FProductivityNotification& Notification);
	FLinearColor GetColorForType(ENotificationType Type) const;
	void PlayNotificationSound(ENotificationType Type);
};
