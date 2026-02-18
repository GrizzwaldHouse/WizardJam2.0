// ============================================================================
// QuidditchDebugWidget.cpp
// Developer: Marcus Daley
// Date: February 15, 2026
// Project: WizardJam
// ============================================================================
// Purpose:
// Implementation of the Quidditch debug instrumentation widget.
// Pure delegate-driven updates - no Tick, no polling.
// ============================================================================

#include "Code/UI/QuidditchDebugWidget.h"
#include "Code/GameModes/QuidditchGameMode.h"
#include "Components/TextBlock.h"

DEFINE_LOG_CATEGORY_STATIC(LogQuidditchDebug, Log, All);

// ============================================================================
// LIFECYCLE
// ============================================================================

void UQuidditchDebugWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Initialize displays to starting values
    if (AgentReadyText)
    {
        AgentReadyText->SetText(FText::FromString(TEXT("Ready Agents: 0 / ?")));
    }

    if (MatchStateText)
    {
        MatchStateText->SetText(FText::FromString(TEXT("State: Initializing")));
    }

    if (CountdownText)
    {
        CountdownText->SetText(FText::FromString(TEXT("Countdown: Inactive")));
    }

    BindToGameMode();

    // If GameMode already has state, query it now so display is correct immediately
    if (CachedGameMode)
    {
        int32 ReadyCount = CachedGameMode->GetAgentsReadyCount();
        int32 RequiredCount = CachedGameMode->GetRequiredAgentCount();
        HandleAgentCountUpdated(ReadyCount, RequiredCount);

        EQuidditchMatchState CurrentState = CachedGameMode->GetMatchState();
        if (MatchStateText)
        {
            MatchStateText->SetText(FText::FromString(
                FString::Printf(TEXT("State: %s"), *MatchStateToString(CurrentState))));
        }
    }

    UE_LOG(LogQuidditchDebug, Display, TEXT("[QuidditchDebugWidget] Initialized"));
}

void UQuidditchDebugWidget::NativeDestruct()
{
    UnbindFromGameMode();

    Super::NativeDestruct();
}

// ============================================================================
// GAMEMODE BINDING
// ============================================================================

void UQuidditchDebugWidget::BindToGameMode()
{
    if (!GetWorld())
    {
        UE_LOG(LogQuidditchDebug, Warning, TEXT("[QuidditchDebugWidget] Cannot bind - World is null"));
        return;
    }

    CachedGameMode = Cast<AQuidditchGameMode>(GetWorld()->GetAuthGameMode());
    if (!CachedGameMode)
    {
        UE_LOG(LogQuidditchDebug, Warning,
            TEXT("[QuidditchDebugWidget] Cannot bind - GameMode is not QuidditchGameMode"));
        return;
    }

    CachedGameMode->OnAgentCountUpdated.AddDynamic(
        this, &UQuidditchDebugWidget::HandleAgentCountUpdated);

    CachedGameMode->OnMatchStateChanged.AddDynamic(
        this, &UQuidditchDebugWidget::HandleMatchStateChanged);

    CachedGameMode->OnCountdownTickBroadcast.AddDynamic(
        this, &UQuidditchDebugWidget::HandleCountdownTick);

    CachedGameMode->OnMatchStarted.AddDynamic(
        this, &UQuidditchDebugWidget::HandleMatchStarted);

    CachedGameMode->OnMatchEnded.AddDynamic(
        this, &UQuidditchDebugWidget::HandleMatchEnded);

    UE_LOG(LogQuidditchDebug, Display,
        TEXT("[QuidditchDebugWidget] Bound to QuidditchGameMode delegates"));
}

void UQuidditchDebugWidget::UnbindFromGameMode()
{
    if (CachedGameMode)
    {
        CachedGameMode->OnAgentCountUpdated.RemoveDynamic(
            this, &UQuidditchDebugWidget::HandleAgentCountUpdated);

        CachedGameMode->OnMatchStateChanged.RemoveDynamic(
            this, &UQuidditchDebugWidget::HandleMatchStateChanged);

        CachedGameMode->OnCountdownTickBroadcast.RemoveDynamic(
            this, &UQuidditchDebugWidget::HandleCountdownTick);

        CachedGameMode->OnMatchStarted.RemoveDynamic(
            this, &UQuidditchDebugWidget::HandleMatchStarted);

        CachedGameMode->OnMatchEnded.RemoveDynamic(
            this, &UQuidditchDebugWidget::HandleMatchEnded);

        UE_LOG(LogQuidditchDebug, Display,
            TEXT("[QuidditchDebugWidget] Unbound from QuidditchGameMode"));
    }

    CachedGameMode = nullptr;
}

// ============================================================================
// DELEGATE HANDLERS
// ============================================================================

void UQuidditchDebugWidget::HandleAgentCountUpdated(int32 CurrentReadyCount, int32 RequiredCount)
{
    if (AgentReadyText)
    {
        AgentReadyText->SetText(FText::FromString(
            FString::Printf(TEXT("Ready Agents: %d / %d"), CurrentReadyCount, RequiredCount)));
    }
}

void UQuidditchDebugWidget::HandleMatchStateChanged(EQuidditchMatchState OldState, EQuidditchMatchState NewState)
{
    if (MatchStateText)
    {
        MatchStateText->SetText(FText::FromString(
            FString::Printf(TEXT("State: %s"), *MatchStateToString(NewState))));
    }

    // Reset countdown display when entering non-countdown states
    if (NewState != EQuidditchMatchState::Countdown && CountdownText)
    {
        CountdownText->SetText(FText::FromString(TEXT("Countdown: Inactive")));
    }
}

void UQuidditchDebugWidget::HandleCountdownTick(int32 SecondsRemaining)
{
    if (CountdownText)
    {
        CountdownText->SetText(FText::FromString(
            FString::Printf(TEXT("Countdown: %d"), SecondsRemaining)));
    }
}

void UQuidditchDebugWidget::HandleMatchStarted(float CountdownSeconds)
{
    if (MatchStateText)
    {
        MatchStateText->SetText(FText::FromString(TEXT("State: InProgress")));
    }

    if (CountdownText)
    {
        CountdownText->SetText(FText::FromString(TEXT("Countdown: Complete")));
    }

    UE_LOG(LogQuidditchDebug, Display, TEXT("[QuidditchDebugWidget] Match started"));
}

void UQuidditchDebugWidget::HandleMatchEnded()
{
    if (MatchStateText)
    {
        MatchStateText->SetText(FText::FromString(TEXT("State: Ended")));
    }

    if (CountdownText)
    {
        CountdownText->SetText(FText::FromString(TEXT("Countdown: Inactive")));
    }

    UE_LOG(LogQuidditchDebug, Display, TEXT("[QuidditchDebugWidget] Match ended"));
}

// ============================================================================
// HELPERS
// ============================================================================

FString UQuidditchDebugWidget::MatchStateToString(EQuidditchMatchState State)
{
    switch (State)
    {
    case EQuidditchMatchState::Initializing:    return TEXT("Initializing");
    case EQuidditchMatchState::FlyingToStart:   return TEXT("Flying to Positions");
    case EQuidditchMatchState::WaitingForReady: return TEXT("Waiting for Agents");
    case EQuidditchMatchState::Countdown:       return TEXT("Countdown");
    case EQuidditchMatchState::InProgress:      return TEXT("In Progress");
    case EQuidditchMatchState::PlayerJoining:   return TEXT("Player Joining");
    case EQuidditchMatchState::SnitchCaught:    return TEXT("Snitch Caught!");
    case EQuidditchMatchState::Ended:           return TEXT("Match Over");
    default:                                    return TEXT("Unknown");
    }
}
