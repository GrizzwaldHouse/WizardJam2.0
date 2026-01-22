// ============================================================================
// BreakQualityEvaluator.h
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Evaluates whether a break provided actual rest.
// Factors: duration, movement, screen off, not checking phone/apps.
//
// Architecture:
// Weighted scoring system for break quality assessment.
// Provides actionable feedback for improving breaks.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BreakQualityEvaluator.generated.h"

// Log category
DECLARE_LOG_CATEGORY_EXTERN(LogBreakQuality, Log, All);

// Quality tier classification
UENUM(BlueprintType)
enum class EBreakQualityTier : uint8
{
	Excellent   UMETA(DisplayName = "Excellent"),   // 80-100
	Good        UMETA(DisplayName = "Good"),        // 60-79
	Partial     UMETA(DisplayName = "Partial"),     // 40-59
	Minimal     UMETA(DisplayName = "Minimal")      // 0-39
};

// Detailed break quality report
USTRUCT(BlueprintType)
struct DEVELOPERPRODUCTIVITYTRACKER_API FBreakQualityReport
{
	GENERATED_BODY()

	// Overall quality score (0-100)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quality")
	float OverallScore;

	// Quality tier classification
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quality")
	EBreakQualityTier QualityTier;

	// Individual component scores (0-100)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quality")
	float DurationScore;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quality")
	float DisengagementScore;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quality")
	float MovementScore;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quality")
	float ConsistencyScore;

	// Input data
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input")
	float BreakDurationSeconds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input")
	bool bScreenWasOff;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input")
	int32 AppSwitchCount;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input")
	bool bMovementDetected;

	// Human-readable feedback
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Feedback")
	FString Feedback;

	// Specific improvement suggestions
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Feedback")
	TArray<FString> Suggestions;

	FBreakQualityReport()
		: OverallScore(0.0f)
		, QualityTier(EBreakQualityTier::Minimal)
		, DurationScore(0.0f)
		, DisengagementScore(0.0f)
		, MovementScore(0.0f)
		, ConsistencyScore(0.0f)
		, BreakDurationSeconds(0.0f)
		, bScreenWasOff(false)
		, AppSwitchCount(0)
		, bMovementDetected(false)
	{
	}

	// Get tier as display string
	FString GetTierDisplayString() const
	{
		switch (QualityTier)
		{
		case EBreakQualityTier::Excellent:
			return TEXT("Excellent");
		case EBreakQualityTier::Good:
			return TEXT("Good");
		case EBreakQualityTier::Partial:
			return TEXT("Partial");
		case EBreakQualityTier::Minimal:
			return TEXT("Minimal");
		default:
			return TEXT("Unknown");
		}
	}

	// Get color for UI display
	FLinearColor GetTierColor() const
	{
		switch (QualityTier)
		{
		case EBreakQualityTier::Excellent:
			return FLinearColor(0.0f, 0.8f, 0.2f); // Green
		case EBreakQualityTier::Good:
			return FLinearColor(0.6f, 0.8f, 0.0f); // Yellow-Green
		case EBreakQualityTier::Partial:
			return FLinearColor(1.0f, 0.6f, 0.0f); // Orange
		case EBreakQualityTier::Minimal:
			return FLinearColor(0.8f, 0.2f, 0.2f); // Red
		default:
			return FLinearColor::Gray;
		}
	}
};

UCLASS(BlueprintType)
class DEVELOPERPRODUCTIVITYTRACKER_API UBreakQualityEvaluator : public UObject
{
	GENERATED_BODY()

public:
	UBreakQualityEvaluator();

	// ========================================================================
	// EVALUATION
	// ========================================================================

	// Evaluate break quality with full parameters
	UFUNCTION(BlueprintCallable, Category = "Productivity|Break")
	FBreakQualityReport EvaluateBreak(
		float DurationSeconds,
		bool bScreenWasOff,
		int32 AppSwitchCount,
		bool bMovementDetected
	);

	// Quick evaluation with just duration
	UFUNCTION(BlueprintCallable, Category = "Productivity|Break")
	FBreakQualityReport EvaluateBreakSimple(float DurationSeconds);

	// Get recommended break duration based on work session length
	UFUNCTION(BlueprintPure, Category = "Productivity|Break")
	float GetRecommendedBreakDuration(float WorkSessionMinutes) const;

	// ========================================================================
	// CONFIGURATION - WEIGHTS
	// ========================================================================

	// Weight for duration in overall score
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quality|Weights",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DurationWeight;

	// Weight for disengagement (screen off, no app switches)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quality|Weights",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DisengagementWeight;

	// Weight for movement during break
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quality|Weights",
		meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MovementWeight;

	// ========================================================================
	// CONFIGURATION - THRESHOLDS
	// ========================================================================

	// Score threshold for Excellent tier
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quality|Thresholds",
		meta = (ClampMin = "50.0", ClampMax = "100.0"))
	float ExcellentThreshold;

	// Score threshold for Good tier
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quality|Thresholds",
		meta = (ClampMin = "30.0", ClampMax = "80.0"))
	float GoodThreshold;

	// Score threshold for Partial tier
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quality|Thresholds",
		meta = (ClampMin = "10.0", ClampMax = "60.0"))
	float PartialThreshold;

	// ========================================================================
	// CONFIGURATION - DURATION TARGETS
	// ========================================================================

	// Ideal minimum break duration in seconds
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quality|Duration",
		meta = (ClampMin = "60.0", ClampMax = "600.0"))
	float IdealMinimumBreakSeconds;

	// Ideal maximum break duration in seconds
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quality|Duration",
		meta = (ClampMin = "300.0", ClampMax = "1800.0"))
	float IdealMaximumBreakSeconds;

private:
	// Score calculation methods
	float CalculateDurationScore(float DurationSeconds) const;
	float CalculateDisengagementScore(bool bScreenOff, int32 AppSwitches) const;
	float CalculateMovementScore(bool bMovementDetected, float DurationSeconds) const;

	// Tier determination
	EBreakQualityTier DetermineQualityTier(float Score) const;

	// Feedback generation
	FString GenerateFeedback(const FBreakQualityReport& Report) const;
	TArray<FString> GenerateSuggestions(const FBreakQualityReport& Report) const;
};
