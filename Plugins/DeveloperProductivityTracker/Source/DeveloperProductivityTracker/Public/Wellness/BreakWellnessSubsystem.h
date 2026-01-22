// ============================================================================
// BreakWellnessSubsystem.h
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Central coordinator for all wellness features including Pomodoro timer,
// smart break detection, break quality evaluation, and stretch reminders.
//
// Architecture:
// Editor Subsystem that owns and coordinates all wellness components.
// Provides unified API for UI and external integrations.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "Tickable.h"
#include "PomodoroManager.h"
#include "SmartBreakDetector.h"
#include "BreakQualityEvaluator.h"
#include "StretchReminderScheduler.h"
#include "BreakWellnessSubsystem.generated.h"

// Log category
DECLARE_LOG_CATEGORY_EXTERN(LogWellness, Log, All);

// Overall wellness status
UENUM(BlueprintType)
enum class EWellnessStatus : uint8
{
	Optimal      UMETA(DisplayName = "Optimal"),      // Recent quality break, good rhythm
	Good         UMETA(DisplayName = "Good"),         // Working well, no concerns
	NeedBreak    UMETA(DisplayName = "Need Break"),   // Been working too long
	OnBreak      UMETA(DisplayName = "On Break"),     // Currently on break
	Overworked   UMETA(DisplayName = "Overworked")    // Extended time without break
};

// Wellness statistics
USTRUCT(BlueprintType)
struct DEVELOPERPRODUCTIVITYTRACKER_API FWellnessStatistics
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Statistics")
	float TodayWorkMinutes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Statistics")
	float TodayBreakMinutes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Statistics")
	int32 TodayPomodorosCompleted;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Statistics")
	int32 TodayStretchesCompleted;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Statistics")
	float AverageBreakQuality;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Statistics")
	float MinutesSinceLastBreak;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Statistics")
	EWellnessStatus CurrentStatus;

	FWellnessStatistics()
		: TodayWorkMinutes(0.0f)
		, TodayBreakMinutes(0.0f)
		, TodayPomodorosCompleted(0)
		, TodayStretchesCompleted(0)
		, AverageBreakQuality(0.0f)
		, MinutesSinceLastBreak(0.0f)
		, CurrentStatus(EWellnessStatus::Good)
	{
	}
};

// Delegate for wellness status changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWellnessStatusChanged, EWellnessStatus, NewStatus);

UCLASS()
class DEVELOPERPRODUCTIVITYTRACKER_API UBreakWellnessSubsystem : public UEditorSubsystem, public FTickableEditorObject
{
	GENERATED_BODY()

public:
	UBreakWellnessSubsystem();

	// ========================================================================
	// USubsystem Interface
	// ========================================================================

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ========================================================================
	// FTickableEditorObject Interface
	// ========================================================================

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return bIsEnabled; }

	// ========================================================================
	// COMPONENT ACCESS
	// ========================================================================

	// Get the Pomodoro manager
	UFUNCTION(BlueprintPure, Category = "Productivity|Wellness")
	UPomodoroManager* GetPomodoroManager() const { return PomodoroManager; }

	// Get the smart break detector
	UFUNCTION(BlueprintPure, Category = "Productivity|Wellness")
	USmartBreakDetector* GetSmartBreakDetector() const { return SmartBreakDetector; }

	// Get the break quality evaluator
	UFUNCTION(BlueprintPure, Category = "Productivity|Wellness")
	UBreakQualityEvaluator* GetBreakQualityEvaluator() const { return BreakQualityEvaluator; }

	// Get the stretch reminder scheduler
	UFUNCTION(BlueprintPure, Category = "Productivity|Wellness")
	UStretchReminderScheduler* GetStretchReminderScheduler() const { return StretchReminderScheduler; }

	// ========================================================================
	// WELLNESS STATUS
	// ========================================================================

	// Get current wellness status
	UFUNCTION(BlueprintPure, Category = "Productivity|Wellness")
	EWellnessStatus GetCurrentWellnessStatus() const { return CurrentWellnessStatus; }

	// Get wellness statistics
	UFUNCTION(BlueprintPure, Category = "Productivity|Wellness")
	FWellnessStatistics GetWellnessStatistics() const;

	// Get status as display string
	UFUNCTION(BlueprintPure, Category = "Productivity|Wellness")
	FString GetWellnessStatusDisplayString() const;

	// Get status color for UI
	UFUNCTION(BlueprintPure, Category = "Productivity|Wellness")
	FLinearColor GetWellnessStatusColor() const;

	// Get time since last break in minutes
	UFUNCTION(BlueprintPure, Category = "Productivity|Wellness")
	float GetMinutesSinceLastBreak() const;

	// ========================================================================
	// QUICK ACTIONS
	// ========================================================================

	// Start a quick break (pauses Pomodoro if active)
	UFUNCTION(BlueprintCallable, Category = "Productivity|Wellness")
	void StartQuickBreak();

	// End a break and resume work
	UFUNCTION(BlueprintCallable, Category = "Productivity|Wellness")
	void EndBreakAndResume();

	// ========================================================================
	// CONFIGURATION
	// ========================================================================

	// Enable/disable wellness features
	UFUNCTION(BlueprintCallable, Category = "Productivity|Wellness")
	void SetWellnessEnabled(bool bEnabled);

	// Check if wellness features are enabled
	UFUNCTION(BlueprintPure, Category = "Productivity|Wellness")
	bool IsWellnessEnabled() const { return bIsEnabled; }

	// Minutes of work before suggesting a break
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wellness|Config",
		meta = (ClampMin = "15.0", ClampMax = "120.0"))
	float MinutesBeforeBreakSuggestion;

	// Minutes of work before showing overworked warning
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wellness|Config",
		meta = (ClampMin = "60.0", ClampMax = "240.0"))
	float MinutesBeforeOverworkedWarning;

	// ========================================================================
	// DELEGATES
	// ========================================================================

	UPROPERTY(BlueprintAssignable, Category = "Productivity|Events")
	FOnWellnessStatusChanged OnWellnessStatusChanged;

private:
	// State
	bool bIsEnabled;
	EWellnessStatus CurrentWellnessStatus;
	EWellnessStatus PreviousWellnessStatus;
	float SecondsSinceLastBreak;
	FDateTime LastBreakEndTime;

	// Components
	UPROPERTY()
	UPomodoroManager* PomodoroManager;

	UPROPERTY()
	USmartBreakDetector* SmartBreakDetector;

	UPROPERTY()
	UBreakQualityEvaluator* BreakQualityEvaluator;

	UPROPERTY()
	UStretchReminderScheduler* StretchReminderScheduler;

	// Today's statistics
	float TodayWorkSeconds;
	float TodayBreakSeconds;
	TArray<float> TodayBreakQualities;

	// Internal methods
	void InitializeComponents();
	void UpdateWellnessStatus();
	EWellnessStatus CalculateWellnessStatus() const;

	// Event handlers
	UFUNCTION()
	void HandlePomodoroStateChanged(EPomodoroState NewState);

	UFUNCTION()
	void HandleBreakDetected(float Confidence);

	UFUNCTION()
	void HandleBreakEnded(const FDetectedBreak& BreakData);

	UFUNCTION()
	void HandleStretchCompleted();
};
