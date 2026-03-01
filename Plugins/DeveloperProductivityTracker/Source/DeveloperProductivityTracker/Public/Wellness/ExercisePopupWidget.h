// ============================================================================
// ExercisePopupWidget.h
// Developer: Marcus Daley
// Date: February 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Slate-based popup window that displays stretch/exercise reminders with
// visual layout, countdown timer, and action buttons (Complete/Snooze/Skip).
//
// Architecture:
// SExercisePopupWidget is the Slate compound widget for the popup content.
// UExercisePopupManager is the UObject manager that creates/destroys the
// popup window and bridges between the StretchReminderScheduler and the UI.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Containers/Ticker.h"
#include "Wellness/StretchReminderScheduler.h"
#include "ExercisePopupWidget.generated.h"

// Log category
DECLARE_LOG_CATEGORY_EXTERN(LogExercisePopup, Log, All);

// ============================================================================
// Slate Widget - The popup content
// ============================================================================

// Callback types for button actions
DECLARE_DELEGATE(FOnExerciseAction);

class SExercisePopupWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SExercisePopupWidget)
		: _Exercise(FStretchExercise())
	{}
		SLATE_ARGUMENT(FStretchExercise, Exercise)
		SLATE_EVENT(FOnExerciseAction, OnComplete)
		SLATE_EVENT(FOnExerciseAction, OnSnooze)
		SLATE_EVENT(FOnExerciseAction, OnSkip)
	SLATE_END_ARGS()

	// Ensure ticker is cleaned up if widget is destroyed while timer runs
	virtual ~SExercisePopupWidget();

	void Construct(const FArguments& InArgs);

	// Start the countdown timer
	void StartTimer();

	// Stop the countdown timer
	void StopTimer();

private:
	// The exercise being displayed
	FStretchExercise DisplayedExercise;

	// Timer state
	float ElapsedTime;
	bool bTimerRunning;
	FTSTicker::FDelegateHandle TickerHandle;

	// Callbacks
	FOnExerciseAction OnCompleteAction;
	FOnExerciseAction OnSnoozeAction;
	FOnExerciseAction OnSkipAction;

	// Ticker callback
	bool OnTick(float DeltaTime);

	// UI text accessors for Slate binding
	FText GetExerciseNameText() const;
	FText GetExerciseDetailsText() const;
	FText GetExerciseDescriptionText() const;
	FText GetTimerText() const;
	FText GetDifficultyText() const;
	TOptional<float> GetTimerProgress() const;

	// Button handlers
	FReply OnCompleteClicked();
	FReply OnSnoozeClicked();
	FReply OnSkipClicked();
};

// ============================================================================
// Manager UObject - Owns the popup window lifecycle
// ============================================================================

// Delegate for popup actions
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnExercisePopupAction, FName, Action);

UCLASS(BlueprintType)
class DEVELOPERPRODUCTIVITYTRACKER_API UExercisePopupManager : public UObject
{
	GENERATED_BODY()

public:
	UExercisePopupManager();

	// ========================================================================
	// CONTROLS
	// ========================================================================

	// Show the exercise popup window
	UFUNCTION(BlueprintCallable, Category = "Productivity|Popup")
	void ShowPopup(const FStretchExercise& Exercise);

	// Close the popup window
	UFUNCTION(BlueprintCallable, Category = "Productivity|Popup")
	void DismissPopup();

	// Check if popup is currently visible
	UFUNCTION(BlueprintPure, Category = "Productivity|Popup")
	bool IsPopupVisible() const;

	// ========================================================================
	// DELEGATES
	// ========================================================================

	// Fires when user clicks Complete, Snooze, or Skip
	UPROPERTY(BlueprintAssignable, Category = "Productivity|Events")
	FOnExercisePopupAction OnExercisePopupAction;

private:
	// The Slate window and widget
	TSharedPtr<SWindow> PopupWindow;
	TSharedPtr<SExercisePopupWidget> PopupWidget;

	// Button action handlers called by the Slate widget
	void HandleComplete();
	void HandleSnooze();
	void HandleSkip();

	// Cleanup when window is closed externally
	void HandleWindowClosed(const TSharedRef<SWindow>& Window);
};
