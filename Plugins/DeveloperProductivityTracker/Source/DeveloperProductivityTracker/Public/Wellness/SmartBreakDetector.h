// ============================================================================
// SmartBreakDetector.h
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Automatically detects when developer steps away from workstation.
// Combines multiple signals: no input, screen lock, no app focus, etc.
//
// Architecture:
// Multi-signal confidence scoring for accurate break detection.
// Hysteresis to prevent rapid state toggling.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "SmartBreakDetector.generated.h"

// Log category
DECLARE_LOG_CATEGORY_EXTERN(LogSmartBreak, Log, All);

// Signals used to detect break state
USTRUCT(BlueprintType)
struct DEVELOPERPRODUCTIVITYTRACKER_API FBreakDetectionSignals
{
	GENERATED_BODY()

	// Is the screen/workstation locked?
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Signals")
	bool bScreenLocked;

	// No keyboard/mouse input detected
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Signals")
	bool bNoInputDetected;

	// No productive application is focused
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Signals")
	bool bNoProductiveAppFocused;

	// Mouse has not moved
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Signals")
	bool bMouseIdle;

	// Keyboard has not been used
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Signals")
	bool bKeyboardIdle;

	// Time since last input
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Signals")
	float SecondsSinceLastInput;

	// Unreal Editor has lost focus
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Signals")
	bool bEditorLostFocus;

	FBreakDetectionSignals()
		: bScreenLocked(false)
		, bNoInputDetected(false)
		, bNoProductiveAppFocused(false)
		, bMouseIdle(false)
		, bKeyboardIdle(false)
		, SecondsSinceLastInput(0.0f)
		, bEditorLostFocus(false)
	{
	}

	// Calculate confidence score (0.0-1.0) that user is on break
	float CalculateBreakConfidence() const
	{
		float Confidence = 0.0f;

		// Screen lock is a strong indicator
		Confidence += bScreenLocked ? 0.4f : 0.0f;

		// No input is a moderate indicator
		Confidence += bNoInputDetected ? 0.3f : 0.0f;

		// No productive app focused
		Confidence += bNoProductiveAppFocused ? 0.2f : 0.0f;

		// Both mouse and keyboard idle
		Confidence += (bMouseIdle && bKeyboardIdle) ? 0.1f : 0.0f;

		return FMath::Clamp(Confidence, 0.0f, 1.0f);
	}

	// Get a description of active signals
	FString GetActiveSignalsDescription() const
	{
		TArray<FString> ActiveSignals;

		if (bScreenLocked) ActiveSignals.Add(TEXT("Screen Locked"));
		if (bNoInputDetected) ActiveSignals.Add(TEXT("No Input"));
		if (bNoProductiveAppFocused) ActiveSignals.Add(TEXT("No Productive App"));
		if (bMouseIdle && bKeyboardIdle) ActiveSignals.Add(TEXT("Input Devices Idle"));
		if (bEditorLostFocus) ActiveSignals.Add(TEXT("Editor Not Focused"));

		return FString::Join(ActiveSignals, TEXT(", "));
	}
};

// Break event data
USTRUCT(BlueprintType)
struct DEVELOPERPRODUCTIVITYTRACKER_API FDetectedBreak
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Break")
	FDateTime StartTime;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Break")
	FDateTime EndTime;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Break")
	float DurationSeconds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Break")
	float AverageConfidence;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Break")
	FBreakDetectionSignals PeakSignals;

	FDetectedBreak()
		: StartTime(FDateTime::MinValue())
		, EndTime(FDateTime::MinValue())
		, DurationSeconds(0.0f)
		, AverageConfidence(0.0f)
	{
	}
};

// Delegate declarations
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBreakDetected, float, Confidence);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBreakEnded, const FDetectedBreak&, BreakData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBreakSuggested, float, SuggestedDurationMinutes);

UCLASS(BlueprintType)
class DEVELOPERPRODUCTIVITYTRACKER_API USmartBreakDetector : public UObject
{
	GENERATED_BODY()

public:
	USmartBreakDetector();

	// Update detection (call each frame)
	void Tick(float DeltaTime);

	// ========================================================================
	// QUERIES
	// ========================================================================

	// Check if currently on a detected break
	UFUNCTION(BlueprintPure, Category = "Productivity|Break")
	bool IsOnDetectedBreak() const { return bIsOnBreak; }

	// Get duration of current break
	UFUNCTION(BlueprintPure, Category = "Productivity|Break")
	float GetCurrentBreakDuration() const;

	// Get current detection signals
	UFUNCTION(BlueprintPure, Category = "Productivity|Break")
	FBreakDetectionSignals GetCurrentSignals() const { return CurrentSignals; }

	// Get current break confidence
	UFUNCTION(BlueprintPure, Category = "Productivity|Break")
	float GetCurrentConfidence() const;

	// Get recent breaks
	UFUNCTION(BlueprintPure, Category = "Productivity|Break")
	TArray<FDetectedBreak> GetRecentBreaks() const { return RecentBreaks; }

	// Get total break time today
	UFUNCTION(BlueprintPure, Category = "Productivity|Break")
	float GetTodayBreakTimeSeconds() const;

	// ========================================================================
	// CONFIGURATION
	// ========================================================================

	// Confidence threshold to start detecting a break
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Break|Config",
		meta = (ClampMin = "0.3", ClampMax = "0.9"))
	float ConfidenceThresholdToStartBreak;

	// Confidence threshold to end a break
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Break|Config",
		meta = (ClampMin = "0.1", ClampMax = "0.5"))
	float ConfidenceThresholdToEndBreak;

	// Confidence threshold to suggest taking a break
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Break|Config",
		meta = (ClampMin = "0.2", ClampMax = "0.6"))
	float ConfidenceThresholdToSuggestBreak;

	// Minimum duration in seconds for a break to be recorded
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Break|Config",
		meta = (ClampMin = "30.0", ClampMax = "300.0"))
	float MinimumBreakDurationSeconds;

	// Seconds of inactivity before considering "no input"
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Break|Config",
		meta = (ClampMin = "30.0", ClampMax = "300.0"))
	float InactivityThresholdSeconds;

	// ========================================================================
	// DELEGATES
	// ========================================================================

	// Broadcast when a break is detected
	UPROPERTY(BlueprintAssignable, Category = "Productivity|Events")
	FOnBreakDetected OnBreakDetected;

	// Broadcast when a break ends
	UPROPERTY(BlueprintAssignable, Category = "Productivity|Events")
	FOnBreakEnded OnBreakEnded;

	// Broadcast when a break is suggested
	UPROPERTY(BlueprintAssignable, Category = "Productivity|Events")
	FOnBreakSuggested OnBreakSuggested;

private:
	// State
	bool bIsOnBreak;
	FDateTime BreakStartTime;
	FBreakDetectionSignals CurrentSignals;
	float ConfidenceAccumulator;
	int32 ConfidenceSamples;

	// Recent break history
	TArray<FDetectedBreak> RecentBreaks;

	// Hysteresis
	float HysteresisTimer;

	// Internal methods
	void UpdateDetectionSignals();
	void StartBreak(float Confidence);
	void EndBreak();
	bool CheckScreenLockState();
	float GetSecondsSinceLastInput() const;
};
