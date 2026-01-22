// ============================================================================
// BreakQualityEvaluator.cpp
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Implementation of break quality evaluation and feedback generation.
// ============================================================================

#include "Wellness/BreakQualityEvaluator.h"

DEFINE_LOG_CATEGORY(LogBreakQuality);

UBreakQualityEvaluator::UBreakQualityEvaluator()
	: DurationWeight(0.4f)
	, DisengagementWeight(0.35f)
	, MovementWeight(0.25f)
	, ExcellentThreshold(80.0f)
	, GoodThreshold(60.0f)
	, PartialThreshold(40.0f)
	, IdealMinimumBreakSeconds(300.0f)  // 5 minutes
	, IdealMaximumBreakSeconds(900.0f)  // 15 minutes
{
}

FBreakQualityReport UBreakQualityEvaluator::EvaluateBreak(
	float DurationSeconds,
	bool bScreenWasOff,
	int32 AppSwitchCount,
	bool bMovementDetected)
{
	FBreakQualityReport Report;

	// Store input data
	Report.BreakDurationSeconds = DurationSeconds;
	Report.bScreenWasOff = bScreenWasOff;
	Report.AppSwitchCount = AppSwitchCount;
	Report.bMovementDetected = bMovementDetected;

	// Calculate component scores
	Report.DurationScore = CalculateDurationScore(DurationSeconds);
	Report.DisengagementScore = CalculateDisengagementScore(bScreenWasOff, AppSwitchCount);
	Report.MovementScore = CalculateMovementScore(bMovementDetected, DurationSeconds);

	// Consistency score based on how balanced the components are
	float Variance = FMath::Abs(Report.DurationScore - Report.DisengagementScore)
		+ FMath::Abs(Report.DisengagementScore - Report.MovementScore)
		+ FMath::Abs(Report.MovementScore - Report.DurationScore);
	Report.ConsistencyScore = FMath::Max(0.0f, 100.0f - (Variance / 3.0f));

	// Calculate weighted overall score
	float TotalWeight = DurationWeight + DisengagementWeight + MovementWeight;
	Report.OverallScore = (
		Report.DurationScore * DurationWeight +
		Report.DisengagementScore * DisengagementWeight +
		Report.MovementScore * MovementWeight
	) / TotalWeight;

	// Determine tier
	Report.QualityTier = DetermineQualityTier(Report.OverallScore);

	// Generate feedback
	Report.Feedback = GenerateFeedback(Report);
	Report.Suggestions = GenerateSuggestions(Report);

	UE_LOG(LogBreakQuality, Log, TEXT("Break evaluated - Score: %.1f (%s), Duration: %.0fs, Screen Off: %s, Movement: %s"),
		Report.OverallScore,
		*Report.GetTierDisplayString(),
		DurationSeconds,
		bScreenWasOff ? TEXT("Yes") : TEXT("No"),
		bMovementDetected ? TEXT("Yes") : TEXT("No"));

	return Report;
}

FBreakQualityReport UBreakQualityEvaluator::EvaluateBreakSimple(float DurationSeconds)
{
	// Assume decent disengagement and no movement info
	return EvaluateBreak(DurationSeconds, true, 0, false);
}

float UBreakQualityEvaluator::GetRecommendedBreakDuration(float WorkSessionMinutes) const
{
	// Rule of thumb: 5-15 minute break for every 25-60 minutes of work
	// Scale based on work duration

	if (WorkSessionMinutes < 25.0f)
	{
		return 5.0f; // 5 minute break
	}
	else if (WorkSessionMinutes < 50.0f)
	{
		return 5.0f + (WorkSessionMinutes - 25.0f) / 5.0f; // 5-10 minutes
	}
	else if (WorkSessionMinutes < 90.0f)
	{
		return 10.0f + (WorkSessionMinutes - 50.0f) / 8.0f; // 10-15 minutes
	}
	else
	{
		return 15.0f + (WorkSessionMinutes - 90.0f) / 20.0f; // 15-20+ minutes
	}
}

float UBreakQualityEvaluator::CalculateDurationScore(float DurationSeconds) const
{
	// Score based on how close to ideal range
	if (DurationSeconds < 60.0f)
	{
		// Too short - scale from 0 to 50
		return (DurationSeconds / 60.0f) * 50.0f;
	}
	else if (DurationSeconds < IdealMinimumBreakSeconds)
	{
		// Approaching ideal - scale from 50 to 80
		float Progress = (DurationSeconds - 60.0f) / (IdealMinimumBreakSeconds - 60.0f);
		return 50.0f + Progress * 30.0f;
	}
	else if (DurationSeconds <= IdealMaximumBreakSeconds)
	{
		// In ideal range - full score
		return 100.0f;
	}
	else if (DurationSeconds < IdealMaximumBreakSeconds * 2.0f)
	{
		// Getting too long - gradual decrease
		float Excess = (DurationSeconds - IdealMaximumBreakSeconds) / IdealMaximumBreakSeconds;
		return FMath::Max(60.0f, 100.0f - Excess * 40.0f);
	}
	else
	{
		// Way too long - might indicate forgot to return
		return 40.0f;
	}
}

float UBreakQualityEvaluator::CalculateDisengagementScore(bool bScreenOff, int32 AppSwitches) const
{
	float Score = 0.0f;

	// Screen being off is a major indicator of good disengagement
	if (bScreenOff)
	{
		Score += 70.0f;
	}
	else
	{
		Score += 30.0f; // Base score for taking a break at all
	}

	// App switches indicate checking things during break
	if (AppSwitches == 0)
	{
		Score += 30.0f;
	}
	else if (AppSwitches <= 2)
	{
		Score += 20.0f;
	}
	else if (AppSwitches <= 5)
	{
		Score += 10.0f;
	}
	// More than 5 switches - no bonus

	return FMath::Clamp(Score, 0.0f, 100.0f);
}

float UBreakQualityEvaluator::CalculateMovementScore(bool bMovementDetected, float DurationSeconds) const
{
	// Movement during breaks is beneficial for health

	if (bMovementDetected)
	{
		// Great - they got up and moved
		return 100.0f;
	}
	else if (DurationSeconds < 120.0f)
	{
		// Short break without movement is fine
		return 60.0f;
	}
	else if (DurationSeconds < 300.0f)
	{
		// Moderate break - movement would help
		return 40.0f;
	}
	else
	{
		// Long break without movement - should have moved
		return 20.0f;
	}
}

EBreakQualityTier UBreakQualityEvaluator::DetermineQualityTier(float Score) const
{
	if (Score >= ExcellentThreshold)
	{
		return EBreakQualityTier::Excellent;
	}
	else if (Score >= GoodThreshold)
	{
		return EBreakQualityTier::Good;
	}
	else if (Score >= PartialThreshold)
	{
		return EBreakQualityTier::Partial;
	}
	else
	{
		return EBreakQualityTier::Minimal;
	}
}

FString UBreakQualityEvaluator::GenerateFeedback(const FBreakQualityReport& Report) const
{
	switch (Report.QualityTier)
	{
	case EBreakQualityTier::Excellent:
		return TEXT("Great break! You stepped away completely and gave yourself proper rest.");

	case EBreakQualityTier::Good:
		return TEXT("Good break. You took time to rest, though there's room for improvement.");

	case EBreakQualityTier::Partial:
		return TEXT("Partial break. Consider stepping away more completely next time.");

	case EBreakQualityTier::Minimal:
		return TEXT("Brief pause. Try taking a longer, more complete break for better recovery.");

	default:
		return TEXT("Break recorded.");
	}
}

TArray<FString> UBreakQualityEvaluator::GenerateSuggestions(const FBreakQualityReport& Report) const
{
	TArray<FString> Suggestions;

	// Duration suggestions
	if (Report.DurationScore < 60.0f)
	{
		if (Report.BreakDurationSeconds < IdealMinimumBreakSeconds)
		{
			Suggestions.Add(FString::Printf(TEXT("Try extending your breaks to at least %.0f minutes for better recovery."),
				IdealMinimumBreakSeconds / 60.0f));
		}
	}

	// Disengagement suggestions
	if (Report.DisengagementScore < 60.0f)
	{
		if (!Report.bScreenWasOff)
		{
			Suggestions.Add(TEXT("Consider locking your screen or stepping away from the computer during breaks."));
		}
		if (Report.AppSwitchCount > 2)
		{
			Suggestions.Add(TEXT("Avoid checking apps or emails during your break - give your mind a rest too."));
		}
	}

	// Movement suggestions
	if (Report.MovementScore < 60.0f && Report.BreakDurationSeconds > 120.0f)
	{
		Suggestions.Add(TEXT("Use break time to stretch or walk around - it helps both body and mind."));
	}

	// General suggestions based on tier
	if (Report.QualityTier == EBreakQualityTier::Minimal)
	{
		Suggestions.Add(TEXT("Remember: quality breaks improve focus and productivity when you return to work."));
	}

	return Suggestions;
}
