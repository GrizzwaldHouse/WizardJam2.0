// ============================================================================
// ExercisePopupWidget.cpp
// Developer: Marcus Daley
// Date: February 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Implementation of the Slate exercise popup and its UObject manager.
// ============================================================================

#include "Wellness/ExercisePopupWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SBoxPanel.h"
#include "Styling/AppStyle.h"
#include "Framework/Application/SlateApplication.h"

DEFINE_LOG_CATEGORY(LogExercisePopup);

// Popup window dimensions
static const FVector2D PopupWindowSize(450.0, 380.0);

// ============================================================================
// SExercisePopupWidget - Slate Implementation
// ============================================================================

SExercisePopupWidget::~SExercisePopupWidget()
{
	StopTimer();
}

void SExercisePopupWidget::Construct(const FArguments& InArgs)
{
	DisplayedExercise = InArgs._Exercise;
	OnCompleteAction = InArgs._OnComplete;
	OnSnoozeAction = InArgs._OnSnooze;
	OnSkipAction = InArgs._OnSkip;
	ElapsedTime = 0.0f;
	bTimerRunning = false;

	// Main layout
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(16.0f)
		[
			SNew(SVerticalBox)

			// Exercise name (large header)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 4)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(STextBlock)
					.Text(this, &SExercisePopupWidget::GetExerciseNameText)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(this, &SExercisePopupWidget::GetDifficultyText)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
					.ColorAndOpacity(FLinearColor(1.0f, 0.8f, 0.0f))
				]
			]

			// Target area and duration
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 12)
			[
				SNew(STextBlock)
				.Text(this, &SExercisePopupWidget::GetExerciseDetailsText)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)))
			]

			// Description (wrapping text)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 16)
			[
				SNew(STextBlock)
				.Text(this, &SExercisePopupWidget::GetExerciseDescriptionText)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
				.AutoWrapText(true)
			]

			// Timer section
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 8)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"))
				.Padding(12.0f)
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0, 0, 0, 6)
					[
						SNew(STextBlock)
						.Text(this, &SExercisePopupWidget::GetTimerText)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
						.Justification(ETextJustify::Center)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SProgressBar)
						.Percent(this, &SExercisePopupWidget::GetTimerProgress)
						.FillColorAndOpacity(FLinearColor(0.2f, 0.8f, 0.2f))
					]
				]
			]

			// Spacer
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SSpacer)
			]

			// Buttons row
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0, 8, 0, 0)
			[
				SNew(SHorizontalBox)

				// Complete button (green tint)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0, 0, 8, 0)
				[
					SNew(SButton)
					.OnClicked(this, &SExercisePopupWidget::OnCompleteClicked)
					.ButtonColorAndOpacity(FLinearColor(0.15f, 0.5f, 0.15f))
					.ContentPadding(FMargin(16, 8))
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Complete")))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
						.ColorAndOpacity(FSlateColor(FLinearColor::White))
					]
				]

				// Snooze button (yellow tint)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0, 0, 8, 0)
				[
					SNew(SButton)
					.OnClicked(this, &SExercisePopupWidget::OnSnoozeClicked)
					.ButtonColorAndOpacity(FLinearColor(0.6f, 0.5f, 0.1f))
					.ContentPadding(FMargin(16, 8))
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Snooze")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
						.ColorAndOpacity(FSlateColor(FLinearColor::White))
					]
				]

				// Skip button (gray)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.OnClicked(this, &SExercisePopupWidget::OnSkipClicked)
					.ContentPadding(FMargin(16, 8))
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Skip")))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 12))
					]
				]
			]
		]
	];
}

void SExercisePopupWidget::StartTimer()
{
	ElapsedTime = 0.0f;
	bTimerRunning = true;

	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateSP(this, &SExercisePopupWidget::OnTick),
		0.1f // Update every 100ms for smooth progress
	);
}

void SExercisePopupWidget::StopTimer()
{
	bTimerRunning = false;

	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}
}

bool SExercisePopupWidget::OnTick(float DeltaTime)
{
	if (!bTimerRunning)
	{
		return false; // Stop ticking
	}

	ElapsedTime += DeltaTime;

	// Auto-complete when timer finishes
	float TotalDuration = static_cast<float>(DisplayedExercise.DurationSeconds);
	if (ElapsedTime >= TotalDuration)
	{
		bTimerRunning = false;
		OnCompleteAction.ExecuteIfBound();
		return false;
	}

	return true; // Keep ticking
}

// ============================================================================
// TEXT ACCESSORS
// ============================================================================

FText SExercisePopupWidget::GetExerciseNameText() const
{
	return FText::FromString(DisplayedExercise.Name);
}

FText SExercisePopupWidget::GetExerciseDetailsText() const
{
	FString Standing = DisplayedExercise.bRequiresStanding ? TEXT("Standing") : TEXT("Seated");
	return FText::FromString(FString::Printf(TEXT("Target: %s  |  Duration: %ds  |  %s"),
		*DisplayedExercise.TargetArea,
		DisplayedExercise.DurationSeconds,
		*Standing));
}

FText SExercisePopupWidget::GetExerciseDescriptionText() const
{
	return FText::FromString(DisplayedExercise.Description);
}

FText SExercisePopupWidget::GetTimerText() const
{
	float Remaining = FMath::Max(0.0f, static_cast<float>(DisplayedExercise.DurationSeconds) - ElapsedTime);
	int32 Minutes = FMath::FloorToInt(Remaining / 60.0f);
	int32 Seconds = FMath::FloorToInt(FMath::Fmod(Remaining, 60.0f));

	return FText::FromString(FString::Printf(TEXT("Timer: %d:%02d"), Minutes, Seconds));
}

FText SExercisePopupWidget::GetDifficultyText() const
{
	// Star rating using Unicode filled/empty stars
	FString Stars;
	for (int32 i = 1; i <= 5; i++)
	{
		Stars += (i <= DisplayedExercise.Difficulty) ? TEXT("\u2605") : TEXT("\u2606");
	}
	return FText::FromString(Stars);
}

TOptional<float> SExercisePopupWidget::GetTimerProgress() const
{
	float TotalDuration = static_cast<float>(DisplayedExercise.DurationSeconds);
	if (TotalDuration <= 0.0f)
	{
		return 0.0f;
	}
	return FMath::Clamp(ElapsedTime / TotalDuration, 0.0f, 1.0f);
}

// ============================================================================
// BUTTON HANDLERS
// ============================================================================

FReply SExercisePopupWidget::OnCompleteClicked()
{
	StopTimer();
	OnCompleteAction.ExecuteIfBound();
	return FReply::Handled();
}

FReply SExercisePopupWidget::OnSnoozeClicked()
{
	StopTimer();
	OnSnoozeAction.ExecuteIfBound();
	return FReply::Handled();
}

FReply SExercisePopupWidget::OnSkipClicked()
{
	StopTimer();
	OnSkipAction.ExecuteIfBound();
	return FReply::Handled();
}

// ============================================================================
// UExercisePopupManager - UObject Manager Implementation
// ============================================================================

UExercisePopupManager::UExercisePopupManager()
{
}

void UExercisePopupManager::ShowPopup(const FStretchExercise& Exercise)
{
	// Dismiss existing popup if one is open
	if (IsPopupVisible())
	{
		DismissPopup();
	}

	UE_LOG(LogExercisePopup, Log, TEXT("Showing exercise popup: %s (%s)"),
		*Exercise.Name, *Exercise.TargetArea);

	// Create the Slate widget
	PopupWidget = SNew(SExercisePopupWidget)
		.Exercise(Exercise)
		.OnComplete(FOnExerciseAction::CreateUObject(this, &UExercisePopupManager::HandleComplete))
		.OnSnooze(FOnExerciseAction::CreateUObject(this, &UExercisePopupManager::HandleSnooze))
		.OnSkip(FOnExerciseAction::CreateUObject(this, &UExercisePopupManager::HandleSkip));

	// Create the window
	PopupWindow = SNew(SWindow)
		.Title(FText::FromString(TEXT("Stretch Reminder")))
		.ClientSize(PopupWindowSize)
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.IsTopmostWindow(true)
		.SizingRule(ESizingRule::FixedSize)
		[
			PopupWidget.ToSharedRef()
		];

	// Handle external window close (user clicks X)
	PopupWindow->SetOnWindowClosed(
		FOnWindowClosed::CreateUObject(this, &UExercisePopupManager::HandleWindowClosed));

	// Add to Slate and show
	FSlateApplication::Get().AddWindow(PopupWindow.ToSharedRef());

	// Start the exercise countdown timer
	PopupWidget->StartTimer();
}

void UExercisePopupManager::DismissPopup()
{
	if (PopupWidget.IsValid())
	{
		PopupWidget->StopTimer();
		PopupWidget.Reset();
	}

	if (PopupWindow.IsValid())
	{
		PopupWindow->RequestDestroyWindow();
		PopupWindow.Reset();
	}
}

bool UExercisePopupManager::IsPopupVisible() const
{
	return PopupWindow.IsValid() && PopupWindow->IsVisible();
}

void UExercisePopupManager::HandleComplete()
{
	UE_LOG(LogExercisePopup, Log, TEXT("Exercise completed via popup"));
	OnExercisePopupAction.Broadcast(FName(TEXT("Complete")));
	DismissPopup();
}

void UExercisePopupManager::HandleSnooze()
{
	UE_LOG(LogExercisePopup, Log, TEXT("Exercise snoozed via popup"));
	OnExercisePopupAction.Broadcast(FName(TEXT("Snooze")));
	DismissPopup();
}

void UExercisePopupManager::HandleSkip()
{
	UE_LOG(LogExercisePopup, Log, TEXT("Exercise skipped via popup"));
	OnExercisePopupAction.Broadcast(FName(TEXT("Skip")));
	DismissPopup();
}

void UExercisePopupManager::HandleWindowClosed(const TSharedRef<SWindow>& Window)
{
	// User closed via title bar X - treat as skip
	UE_LOG(LogExercisePopup, Log, TEXT("Exercise popup closed via window button - treating as skip"));

	if (PopupWidget.IsValid())
	{
		PopupWidget->StopTimer();
		PopupWidget.Reset();
	}

	PopupWindow.Reset();
	OnExercisePopupAction.Broadcast(FName(TEXT("Skip")));
}
