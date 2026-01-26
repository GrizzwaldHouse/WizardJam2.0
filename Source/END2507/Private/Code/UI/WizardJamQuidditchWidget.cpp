// ============================================================================
// WizardJamQuidditchWidget.cpp
// Implementation of Quidditch scoreboard widget with WorldSignalEmitter integration
//
// Developer: Marcus Daley
// Date: January 26, 2026
// Project: WizardJam2.0 (END2507)
//
// SIGNAL INTEGRATION:
// - Listens for SignalTypeNames::QuidditchMatchStart to start timer
// - Listens for SignalTypeNames::QuidditchMatchEnd to stop timer
// - Broadcasts OnMatchTimerExpired when timer reaches zero
// ============================================================================

#include "Code/UI/WizardJamQuidditchWidget.h"
#include "Code/GameModes/WizardJamGameMode.h"
#include "Code/Utilities/SignalTypes.h"
#include "Code/Actors/WorldSignalEmitter.h"
#include "Components/TextBlock.h"

DEFINE_LOG_CATEGORY(LogQuidditchWidget);

// ============================================================================
// LIFECYCLE
// ============================================================================

void UWizardJamQuidditchWidget::NativeConstruct()
{
    Super::NativeConstruct();

    UE_LOG(LogQuidditchWidget, Log, TEXT("WizardJamQuidditchWidget NativeConstruct"));

    // Initialize state
    bTimerRunning = false;
    bMatchEnded = false;
    MatchTimeRemaining = 0.0f;
    MatchDuration = DefaultMatchDuration;

    // Initialize cached scores (observer pattern - updated via delegates only)
    CachedPlayerScore = 0;
    CachedAIScore = 0;

    // Set default team labels if configured
    if (!DefaultPlayerTeamName.IsEmpty() && PlayerScoreLabel)
    {
        PlayerScoreLabel->SetText(DefaultPlayerTeamName);
    }
    if (!DefaultAITeamName.IsEmpty() && AIScoreLabel)
    {
        AIScoreLabel->SetText(DefaultAITeamName);
    }

    // Initialize scores to 0
    if (PlayerScoreText)
    {
        PlayerScoreText->SetText(FText::FromString(TEXT("0")));
    }
    if (AIScoreText)
    {
        AIScoreText->SetText(FText::FromString(TEXT("0")));
    }

    // Initialize timer display
    if (MatchTimerText)
    {
        MatchTimerText->SetText(FText::FromString(FormatTime(DefaultMatchDuration)));
    }

    // Set initial status
    if (MatchStatusText)
    {
        MatchStatusText->SetText(FText::FromString(TEXT("WAITING TO START")));
    }

    // Bind to GameMode delegates
    BindToGameMode();

    // Bind to global world signals
    BindToWorldSignals();

    UE_LOG(LogQuidditchWidget, Log, TEXT("WizardJamQuidditchWidget initialization complete"));
}

void UWizardJamQuidditchWidget::NativeDestruct()
{
    // Clean unbind
    UnbindFromGameMode();
    UnbindFromWorldSignals();

    Super::NativeDestruct();
}

void UWizardJamQuidditchWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // Update timer if running
    if (bTimerRunning && !bMatchEnded)
    {
        MatchTimeRemaining -= InDeltaTime;

        if (MatchTimeRemaining <= 0.0f)
        {
            MatchTimeRemaining = 0.0f;
            OnTimerExpired();
        }

        // Update display
        UpdateTimer(MatchTimeRemaining);
    }
}

// ============================================================================
// GAMEMODE BINDING
// ============================================================================

void UWizardJamQuidditchWidget::BindToGameMode()
{
    // Get GameMode ONLY for delegate binding, NOT for queries
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogQuidditchWidget, Warning, TEXT("No world - cannot bind to GameMode"));
        return;
    }

    CachedGameMode = Cast<AWizardJamGameMode>(World->GetAuthGameMode());
    if (!CachedGameMode)
    {
        UE_LOG(LogQuidditchWidget, Warning, TEXT("GameMode is not WizardJamGameMode - score updates may not work"));
        return;
    }

    // Bind to score changes delegate (observer pattern)
    CachedGameMode->OnScoreChanged.AddDynamic(this, &UWizardJamQuidditchWidget::HandleScoreChanged);

    // Initialize displays to 0 - delegate will update when scores change
    // NO POLLING - we only update via HandleScoreChanged delegate
    UpdatePlayerScore(0, 0);
    UpdateAIScore(0, 0);

    UE_LOG(LogQuidditchWidget, Log, TEXT("Bound to GameMode OnScoreChanged delegate"));
}

void UWizardJamQuidditchWidget::UnbindFromGameMode()
{
    if (CachedGameMode)
    {
        CachedGameMode->OnScoreChanged.RemoveDynamic(this, &UWizardJamQuidditchWidget::HandleScoreChanged);
        CachedGameMode = nullptr;
    }
}

// ============================================================================
// WORLD SIGNAL INTEGRATION
// ============================================================================

void UWizardJamQuidditchWidget::BindToWorldSignals()
{
    // Bind to the static global signal delegate
    GlobalSignalHandle = AWorldSignalEmitter::OnAnySignalEmittedGlobal.AddUObject(
        this, &UWizardJamQuidditchWidget::HandleGlobalSignal);

    UE_LOG(LogQuidditchWidget, Log, TEXT("Bound to WorldSignalEmitter global signals"));
}

void UWizardJamQuidditchWidget::UnbindFromWorldSignals()
{
    if (GlobalSignalHandle.IsValid())
    {
        AWorldSignalEmitter::OnAnySignalEmittedGlobal.Remove(GlobalSignalHandle);
        GlobalSignalHandle.Reset();
    }
}

void UWizardJamQuidditchWidget::HandleGlobalSignal(const FSignalData& SignalData)
{
    UE_LOG(LogQuidditchWidget, Log, TEXT("Received signal: %s"), *SignalData.SignalType.ToString());

    // Handle Quidditch Match Start signal
    if (SignalData.SignalType == SignalTypeNames::QuidditchMatchStart)
    {
        UE_LOG(LogQuidditchWidget, Log, TEXT("Quidditch Match Start signal received - starting timer"));

        // Start the match timer
        StartMatchTimer(DefaultMatchDuration > 0.0f ? DefaultMatchDuration : 600.0f); // Default 10 minutes

        // Update status
        if (MatchStatusText)
        {
            MatchStatusText->SetText(FText::FromString(TEXT("MATCH IN PROGRESS")));
        }
    }
    // Handle Quidditch Match End signal
    else if (SignalData.SignalType == SignalTypeNames::QuidditchMatchEnd)
    {
        UE_LOG(LogQuidditchWidget, Log, TEXT("Quidditch Match End signal received - stopping timer"));

        StopMatchTimer();

        // Determine winner based on cached scores (observer pattern - no GameMode query)
        bool bPlayerWon = CachedPlayerScore > CachedAIScore;

        OnMatchEnded(bPlayerWon, SignalData.CustomData);
    }
}

// ============================================================================
// SCORE UPDATES
// ============================================================================

void UWizardJamQuidditchWidget::UpdatePlayerScore(int32 NewScore, int32 PointsAdded)
{
    if (!PlayerScoreText)
    {
        UE_LOG(LogQuidditchWidget, Warning, TEXT("PlayerScoreText widget not bound"));
        return;
    }

    PlayerScoreText->SetText(FText::AsNumber(NewScore));

    // Optional flash effect for score increase
    if (PointsAdded > 0)
    {
        FlashScoreText(PlayerScoreText, true);
    }

    UE_LOG(LogQuidditchWidget, Log, TEXT("Player score updated: %d (+%d)"), NewScore, PointsAdded);
}

void UWizardJamQuidditchWidget::UpdateAIScore(int32 NewScore, int32 PointsAdded)
{
    if (!AIScoreText)
    {
        UE_LOG(LogQuidditchWidget, Warning, TEXT("AIScoreText widget not bound"));
        return;
    }

    AIScoreText->SetText(FText::AsNumber(NewScore));

    // Optional flash effect for score increase
    if (PointsAdded > 0)
    {
        FlashScoreText(AIScoreText, false); // false = opponent scored
    }

    UE_LOG(LogQuidditchWidget, Log, TEXT("AI score updated: %d (+%d)"), NewScore, PointsAdded);
}

void UWizardJamQuidditchWidget::HandleScoreChanged(int32 PlayerScore, int32 AIScore, AActor* ScoringActor)
{
    // Calculate delta from cached values (observer pattern)
    int32 PlayerDelta = PlayerScore - CachedPlayerScore;
    int32 AIDelta = AIScore - CachedAIScore;

    // Update cached state
    CachedPlayerScore = PlayerScore;
    CachedAIScore = AIScore;

    // Update display with calculated deltas for flash effect
    UpdatePlayerScore(PlayerScore, PlayerDelta);
    UpdateAIScore(AIScore, AIDelta);

    UE_LOG(LogQuidditchWidget, Log, TEXT("Score changed via delegate: Player=%d (+%d), AI=%d (+%d), Scorer=%s"),
        PlayerScore, PlayerDelta, AIScore, AIDelta,
        ScoringActor ? *ScoringActor->GetName() : TEXT("Unknown"));
}

// ============================================================================
// TIMER
// ============================================================================

void UWizardJamQuidditchWidget::UpdateTimer(float TimeRemaining)
{
    if (!MatchTimerText)
    {
        return;
    }

    MatchTimerText->SetText(FText::FromString(FormatTime(TimeRemaining)));

    // Optional: Change color when time is low
    if (TimeRemaining <= 60.0f) // Last minute
    {
        MatchTimerText->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
    }
    else if (TimeRemaining <= 120.0f) // Last 2 minutes
    {
        MatchTimerText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.5f, 0.0f, 1.0f))); // Orange
    }
    else
    {
        MatchTimerText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    }
}

void UWizardJamQuidditchWidget::StartMatchTimer(float MatchDurationSeconds)
{
    MatchDuration = MatchDurationSeconds;
    MatchTimeRemaining = MatchDurationSeconds;
    bTimerRunning = true;
    bMatchEnded = false;

    UE_LOG(LogQuidditchWidget, Log, TEXT("Match timer started: %.0f seconds"), MatchDurationSeconds);

    // Update display immediately
    UpdateTimer(MatchTimeRemaining);
}

void UWizardJamQuidditchWidget::StopMatchTimer()
{
    bTimerRunning = false;

    UE_LOG(LogQuidditchWidget, Log, TEXT("Match timer stopped at %.1f seconds remaining"), MatchTimeRemaining);
}

void UWizardJamQuidditchWidget::OnTimerExpired()
{
    UE_LOG(LogQuidditchWidget, Log, TEXT("Match timer expired!"));

    bTimerRunning = false;
    bMatchEnded = true;

    // Update status
    if (MatchStatusText)
    {
        MatchStatusText->SetText(FText::FromString(TEXT("TIME'S UP!")));
    }

    // Broadcast timer expiration - GameMode should handle this
    OnMatchTimerExpired.Broadcast();

    // Determine winner based on cached scores (observer pattern - no GameMode query)
    bool bPlayerWon = CachedPlayerScore > CachedAIScore;
    FString Reason = FString::Printf(TEXT("Time expired - Final Score: %d to %d"),
        CachedPlayerScore, CachedAIScore);

    OnMatchEnded(bPlayerWon, Reason);
}

// ============================================================================
// TEAM LABELS
// ============================================================================

void UWizardJamQuidditchWidget::SetTeamLabels(const FText& PlayerTeamName, const FText& AITeamName)
{
    if (PlayerScoreLabel)
    {
        PlayerScoreLabel->SetText(PlayerTeamName);
    }
    if (AIScoreLabel)
    {
        AIScoreLabel->SetText(AITeamName);
    }

    UE_LOG(LogQuidditchWidget, Log, TEXT("Team labels set: %s vs %s"),
        *PlayerTeamName.ToString(), *AITeamName.ToString());
}

// ============================================================================
// MATCH STATE
// ============================================================================

void UWizardJamQuidditchWidget::OnMatchEnded(bool bPlayerWon, const FString& Reason)
{
    if (bMatchEnded)
    {
        // Already handled
        return;
    }

    bMatchEnded = true;
    StopMatchTimer();

    UE_LOG(LogQuidditchWidget, Log, TEXT("Match ended: %s - %s"),
        bPlayerWon ? TEXT("PLAYER WINS") : TEXT("AI WINS"), *Reason);

    // Update status text
    if (MatchStatusText)
    {
        if (bPlayerWon)
        {
            MatchStatusText->SetText(FText::FromString(TEXT("VICTORY!")));
            MatchStatusText->SetColorAndOpacity(FSlateColor(FLinearColor::Green));
        }
        else
        {
            MatchStatusText->SetText(FText::FromString(TEXT("DEFEAT")));
            MatchStatusText->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
        }
    }
}

void UWizardJamQuidditchWidget::HandleMatchEnded(bool bPlayerWon, const FString& Reason)
{
    OnMatchEnded(bPlayerWon, Reason);
}

// ============================================================================
// HELPERS
// ============================================================================

FString UWizardJamQuidditchWidget::FormatTime(float Seconds) const
{
    // Clamp to non-negative
    Seconds = FMath::Max(0.0f, Seconds);

    int32 Minutes = FMath::FloorToInt(Seconds / 60.0f);
    int32 RemainingSeconds = FMath::FloorToInt(FMath::Fmod(Seconds, 60.0f));

    return FString::Printf(TEXT("%02d:%02d"), Minutes, RemainingSeconds);
}

void UWizardJamQuidditchWidget::FlashScoreText(UTextBlock* ScoreText, bool bPositive)
{
    if (!ScoreText)
    {
        return;
    }

    // Simple flash effect - set to highlight color
    // In a full implementation, this would use an animation or timer to fade back
    FLinearColor FlashColor = bPositive ?
        FLinearColor::Green :  // Player scored
        FLinearColor::Red;     // Opponent scored

    ScoreText->SetColorAndOpacity(FSlateColor(FlashColor));

    // Note: To properly reset the color, you'd use a timer or UMG animation
    // For now, the color will stay until next update
    // TODO: Add timer to reset color after 0.5 seconds
}
