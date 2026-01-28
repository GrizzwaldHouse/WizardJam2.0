// ============================================================================
// WizardJamQuidditchWidget.cpp
// Developer: Marcus Daley
// Date: January 5, 2026
// Project: WizardJam
// ============================================================================
// Purpose:
// Implementation of the separate Quidditch scoreboard widget.
// Binds to GameMode scoring and timer delegates and updates UI accordingly.
//
// Key Implementation Details:
// - Self-contained widget that manages its own GameMode binding
// - Can be created/destroyed independently of main HUD
// - Formats timer as MM:SS for clean display
// - Supports optional team name labels for customization
// - ADDED: Binds to OnMatchTimerUpdated for live countdown
//
// Observer Pattern:
// - GameMode broadcasts -> Widget handles -> Updates UI
// - No polling, pure delegate-driven updates
// ============================================================================

#include "Code/UI/WizardJamQuidditchWidget.h"
#include "Code/GameModes/QuidditchGameMode.h"
#include "Components/TextBlock.h"

DEFINE_LOG_CATEGORY_STATIC(LogQuidditchWidget, Log, All);

// ============================================================================
// LIFECYCLE
// ============================================================================

void UWizardJamQuidditchWidget::NativeConstruct()
{
    Super::NativeConstruct();

    UE_LOG(LogQuidditchWidget, Display, TEXT("[QuidditchWidget] NativeConstruct called"));

    // Initialize score displays to zero
    if (PlayerScoreText)
    {
        PlayerScoreText->SetText(FText::FromString(TEXT("0")));
    }

    if (AIScoreText)
    {
        AIScoreText->SetText(FText::FromString(TEXT("0")));
    }

    // Initialize timer to show starting value (will be updated immediately by first broadcast)
    if (MatchTimerText)
    {
        MatchTimerText->SetText(FText::FromString(TEXT("05:00")));
    }

    // Set default team labels if configured
    if (PlayerScoreLabel && !DefaultPlayerTeamName.IsEmpty())
    {
        PlayerScoreLabel->SetText(DefaultPlayerTeamName);
    }

    if (AIScoreLabel && !DefaultAITeamName.IsEmpty())
    {
        AIScoreLabel->SetText(DefaultAITeamName);
    }

    // Bind to GameMode scoring events
    BindToGameMode();

    UE_LOG(LogQuidditchWidget, Display, TEXT("[QuidditchWidget] Widget initialized"));
}

void UWizardJamQuidditchWidget::NativeDestruct()
{
    UnbindFromGameMode();

    Super::NativeDestruct();
}

// ============================================================================
// GAMEMODE BINDING
// UPDATED: Now also binds to OnMatchTimerUpdated
// ============================================================================

void UWizardJamQuidditchWidget::BindToGameMode()
{
    if (!GetWorld())
    {
        UE_LOG(LogQuidditchWidget, Warning, TEXT("[QuidditchWidget] Cannot bind - World is null"));
        return;
    }

    CachedQuidditchGameMode = Cast<AQuidditchGameMode>(GetWorld()->GetAuthGameMode());
    if (!CachedQuidditchGameMode)
    {
        UE_LOG(LogQuidditchWidget, Warning,
            TEXT("[QuidditchWidget] Cannot bind - GameMode is not QuidditchGameMode"));
        return;
    }

    // Bind to QuidditchGameMode scoring delegate
    CachedQuidditchGameMode->OnQuidditchTeamScored.AddDynamic(
        this, &UWizardJamQuidditchWidget::HandleTeamScored);

    // Bind to match start for score reset
    CachedQuidditchGameMode->OnMatchStarted.AddDynamic(
        this, &UWizardJamQuidditchWidget::HandleMatchStarted);

    // Bind to snitch caught for match end
    CachedQuidditchGameMode->OnSnitchCaught.AddDynamic(
        this, &UWizardJamQuidditchWidget::HandleSnitchCaught);

    UE_LOG(LogQuidditchWidget, Display,
        TEXT("[QuidditchWidget] Bound to QuidditchGameMode delegates"));
}

void UWizardJamQuidditchWidget::UnbindFromGameMode()
{
    if (CachedQuidditchGameMode)
    {
        CachedQuidditchGameMode->OnQuidditchTeamScored.RemoveDynamic(
            this, &UWizardJamQuidditchWidget::HandleTeamScored);
        CachedQuidditchGameMode->OnMatchStarted.RemoveDynamic(
            this, &UWizardJamQuidditchWidget::HandleMatchStarted);
        CachedQuidditchGameMode->OnSnitchCaught.RemoveDynamic(
            this, &UWizardJamQuidditchWidget::HandleSnitchCaught);

        UE_LOG(LogQuidditchWidget, Display, TEXT("[QuidditchWidget] Unbound from QuidditchGameMode"));
    }

    CachedQuidditchGameMode = nullptr;
}

// ============================================================================
// DELEGATE HANDLERS
// ============================================================================

void UWizardJamQuidditchWidget::HandlePlayerScored(int32 NewScore, int32 PointsAdded)
{
    UpdatePlayerScore(NewScore, PointsAdded);
}

void UWizardJamQuidditchWidget::HandleAIScored(int32 NewScore, int32 PointsAdded)
{
    UpdateAIScore(NewScore, PointsAdded);
}

// ============================================================================
// ADDED: Timer delegate handler
// Called every frame by GameMode::Tick()
// Simply forwards to UpdateTimer() which formats as MM:SS
// ============================================================================

void UWizardJamQuidditchWidget::HandleTimerUpdated(float TimeRemaining)
{
    UpdateTimer(TimeRemaining);
}

// ============================================================================
// PUBLIC API
// ============================================================================

void UWizardJamQuidditchWidget::UpdatePlayerScore(int32 NewScore, int32 PointsAdded)
{
    if (!PlayerScoreText)
    {
        UE_LOG(LogQuidditchWidget, Warning, TEXT("[QuidditchWidget] PlayerScoreText not bound"));
        return;
    }

    PlayerScoreText->SetText(FText::AsNumber(NewScore));

    UE_LOG(LogQuidditchWidget, Display, TEXT("[QuidditchWidget] Player score: %d (+%d)"),
        NewScore, PointsAdded);
}

void UWizardJamQuidditchWidget::UpdateAIScore(int32 NewScore, int32 PointsAdded)
{
    if (!AIScoreText)
    {
        UE_LOG(LogQuidditchWidget, Warning, TEXT("[QuidditchWidget] AIScoreText not bound"));
        return;
    }

    AIScoreText->SetText(FText::AsNumber(NewScore));

    UE_LOG(LogQuidditchWidget, Display, TEXT("[QuidditchWidget] AI score: %d (+%d)"),
        NewScore, PointsAdded);
}

void UWizardJamQuidditchWidget::UpdateTimer(float TimeRemaining)
{
    if (!MatchTimerText)
    {
        return;
    }

    // Clamp to zero (no negative display)
    float ClampedTime = FMath::Max(0.0f, TimeRemaining);

    // Format time as MM:SS
    int32 Minutes = FMath::FloorToInt(ClampedTime / 60.0f);
    int32 Seconds = FMath::FloorToInt(FMath::Fmod(ClampedTime, 60.0f));

    FString TimeString = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
    MatchTimerText->SetText(FText::FromString(TimeString));
}

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

    UE_LOG(LogQuidditchWidget, Display, TEXT("[QuidditchWidget] Team labels set: '%s' vs '%s'"),
        *PlayerTeamName.ToString(), *AITeamName.ToString());
}

// ============================================================================
// QUIDDITCH GAMEMODE DELEGATE HANDLERS
// ============================================================================

void UWizardJamQuidditchWidget::HandleTeamScored(EQuidditchTeam ScoringTeam, int32 Points, int32 TotalScore)
{
    // TeamA = Player team, TeamB = AI team (configurable)
    if (ScoringTeam == EQuidditchTeam::TeamA)
    {
        UpdatePlayerScore(TotalScore, Points);
    }
    else if (ScoringTeam == EQuidditchTeam::TeamB)
    {
        UpdateAIScore(TotalScore, Points);
    }
}

void UWizardJamQuidditchWidget::HandleMatchStarted(float CountdownSeconds)
{
    // Reset scores to zero on match start
    if (PlayerScoreText)
    {
        PlayerScoreText->SetText(FText::FromString(TEXT("0")));
    }
    if (AIScoreText)
    {
        AIScoreText->SetText(FText::FromString(TEXT("0")));
    }

    UE_LOG(LogQuidditchWidget, Display, TEXT("[QuidditchWidget] Match started - scores reset"));
}

void UWizardJamQuidditchWidget::HandleSnitchCaught(APawn* CatchingSeeker, EQuidditchTeam WinningTeam)
{
    UE_LOG(LogQuidditchWidget, Display,
        TEXT("[QuidditchWidget] Snitch caught by %s! Winner: %s"),
        CatchingSeeker ? *CatchingSeeker->GetName() : TEXT("Unknown"),
        WinningTeam == EQuidditchTeam::TeamA ? TEXT("Team A") : TEXT("Team B"));

    // Future: Could trigger results screen or victory animation here
}