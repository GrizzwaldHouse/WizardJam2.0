// ============================================================================
// SmartBreakDetector.cpp
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Implementation of smart break detection using multi-signal analysis.
// ============================================================================

#include "Wellness/SmartBreakDetector.h"
#include "Framework/Application/SlateApplication.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

DEFINE_LOG_CATEGORY(LogSmartBreak);

// Hysteresis time to prevent rapid break toggling
static const float HysteresisDelay = 5.0f;

USmartBreakDetector::USmartBreakDetector()
	: ConfidenceThresholdToStartBreak(0.6f)
	, ConfidenceThresholdToEndBreak(0.3f)
	, ConfidenceThresholdToSuggestBreak(0.4f)
	, MinimumBreakDurationSeconds(60.0f)
	, InactivityThresholdSeconds(120.0f)
	, bIsOnBreak(false)
	, BreakStartTime(FDateTime::MinValue())
	, ConfidenceAccumulator(0.0f)
	, ConfidenceSamples(0)
	, HysteresisTimer(0.0f)
{
}

void USmartBreakDetector::Tick(float DeltaTime)
{
	// Update detection signals
	UpdateDetectionSignals();

	// Calculate current confidence
	float Confidence = CurrentSignals.CalculateBreakConfidence();

	// Handle hysteresis
	if (HysteresisTimer > 0.0f)
	{
		HysteresisTimer -= DeltaTime;
		return;
	}

	if (!bIsOnBreak)
	{
		// Check if we should start detecting a break
		if (Confidence >= ConfidenceThresholdToStartBreak)
		{
			StartBreak(Confidence);
		}
		// Check if we should suggest a break
		else if (Confidence >= ConfidenceThresholdToSuggestBreak)
		{
			// Calculate suggested break duration based on work time
			// This could be enhanced with more sophisticated logic
			float SuggestedMinutes = 5.0f;
			OnBreakSuggested.Broadcast(SuggestedMinutes);
		}
	}
	else
	{
		// Accumulate confidence for averaging
		ConfidenceAccumulator += Confidence;
		ConfidenceSamples++;

		// Check if break should end
		if (Confidence < ConfidenceThresholdToEndBreak)
		{
			EndBreak();
		}
	}
}

float USmartBreakDetector::GetCurrentBreakDuration() const
{
	if (!bIsOnBreak)
	{
		return 0.0f;
	}

	FTimespan Duration = FDateTime::Now() - BreakStartTime;
	return static_cast<float>(Duration.GetTotalSeconds());
}

float USmartBreakDetector::GetCurrentConfidence() const
{
	return CurrentSignals.CalculateBreakConfidence();
}

float USmartBreakDetector::GetTodayBreakTimeSeconds() const
{
	float TotalSeconds = 0.0f;
	FDateTime Today = FDateTime::Today();

	for (const FDetectedBreak& Break : RecentBreaks)
	{
		if (Break.StartTime >= Today)
		{
			TotalSeconds += Break.DurationSeconds;
		}
	}

	// Add current break if active
	if (bIsOnBreak)
	{
		TotalSeconds += GetCurrentBreakDuration();
	}

	return TotalSeconds;
}

void USmartBreakDetector::UpdateDetectionSignals()
{
	// Reset signals
	CurrentSignals = FBreakDetectionSignals();

	// Check screen lock
	CurrentSignals.bScreenLocked = CheckScreenLockState();

	// Check input activity
	CurrentSignals.SecondsSinceLastInput = GetSecondsSinceLastInput();
	CurrentSignals.bNoInputDetected = CurrentSignals.SecondsSinceLastInput > InactivityThresholdSeconds;

	// Check mouse and keyboard idle separately
	CurrentSignals.bMouseIdle = CurrentSignals.SecondsSinceLastInput > (InactivityThresholdSeconds * 0.5f);
	CurrentSignals.bKeyboardIdle = CurrentSignals.SecondsSinceLastInput > (InactivityThresholdSeconds * 0.5f);

	// Check editor focus
	if (FSlateApplication::IsInitialized())
	{
		CurrentSignals.bEditorLostFocus = !FSlateApplication::Get().IsActive();
	}

	// Check productive app focus (this would integrate with ExternalActivityMonitor)
	// For now, we use editor focus as a proxy
	CurrentSignals.bNoProductiveAppFocused = CurrentSignals.bEditorLostFocus;
}

void USmartBreakDetector::StartBreak(float Confidence)
{
	bIsOnBreak = true;
	BreakStartTime = FDateTime::Now();
	ConfidenceAccumulator = Confidence;
	ConfidenceSamples = 1;

	UE_LOG(LogSmartBreak, Log, TEXT("Break detected (Confidence: %.2f) - Signals: %s"),
		Confidence, *CurrentSignals.GetActiveSignalsDescription());

	OnBreakDetected.Broadcast(Confidence);
}

void USmartBreakDetector::EndBreak()
{
	float Duration = GetCurrentBreakDuration();

	// Only record if break was long enough
	if (Duration >= MinimumBreakDurationSeconds)
	{
		FDetectedBreak Break;
		Break.StartTime = BreakStartTime;
		Break.EndTime = FDateTime::Now();
		Break.DurationSeconds = Duration;
		Break.AverageConfidence = ConfidenceSamples > 0
			? ConfidenceAccumulator / static_cast<float>(ConfidenceSamples)
			: 0.0f;
		Break.PeakSignals = CurrentSignals;

		RecentBreaks.Add(Break);

		// Keep only last 50 breaks
		while (RecentBreaks.Num() > 50)
		{
			RecentBreaks.RemoveAt(0);
		}

		UE_LOG(LogSmartBreak, Log, TEXT("Break ended - Duration: %.1f seconds, Avg Confidence: %.2f"),
			Duration, Break.AverageConfidence);

		OnBreakEnded.Broadcast(Break);
	}
	else
	{
		UE_LOG(LogSmartBreak, Verbose, TEXT("Break too short (%.1f seconds) - not recorded"), Duration);
	}

	// Reset state with hysteresis
	bIsOnBreak = false;
	BreakStartTime = FDateTime::MinValue();
	ConfidenceAccumulator = 0.0f;
	ConfidenceSamples = 0;
	HysteresisTimer = HysteresisDelay;
}

bool USmartBreakDetector::CheckScreenLockState()
{
#if PLATFORM_WINDOWS
	// Check if a desktop switch has occurred (screen lock indicator)
	HDESK hDesk = OpenInputDesktop(0, 0, DESKTOP_READOBJECTS);
	if (hDesk == nullptr)
	{
		// Cannot access input desktop - likely locked
		return true;
	}

	// Check desktop name
	TCHAR DesktopName[256] = { 0 };
	DWORD NameLength = 0;
	GetUserObjectInformation(hDesk, UOI_NAME, DesktopName, sizeof(DesktopName), &NameLength);
	CloseDesktop(hDesk);

	// "Winlogon" desktop indicates lock screen
	FString Name(DesktopName);
	if (Name.Contains(TEXT("Winlogon")))
	{
		return true;
	}

	return false;
#else
	// Platform not supported - assume not locked
	return false;
#endif
}

float USmartBreakDetector::GetSecondsSinceLastInput() const
{
	if (!FSlateApplication::IsInitialized())
	{
		return 0.0f;
	}

	double LastInteraction = FSlateApplication::Get().GetLastUserInteractionTime();
	double CurrentTime = FSlateApplication::Get().GetCurrentTime();

	return static_cast<float>(CurrentTime - LastInteraction);
}
