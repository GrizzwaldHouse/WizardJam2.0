// ============================================================================
// QuidditchDebugWidget.h
// Developer: Marcus Daley
// Date: February 15, 2026
// Project: WizardJam
// ============================================================================
// Purpose:
// Debug instrumentation layer for Quidditch match observability.
// Shows agent ready count, match state, and countdown in real-time.
// Event-driven only - NO Tick polling. Binds to GameMode delegates.
//
// This is NOT a gameplay UI widget. This is a production-level debug HUD
// for verifying the staging -> countdown -> match start pipeline visually.
//
// Observer Pattern Bindings:
// - OnAgentCountUpdated -> HandleAgentCountUpdated -> UpdateAgentReadyText
// - OnMatchStateChanged -> HandleMatchStateChanged -> UpdateMatchStateText
// - OnCountdownTickBroadcast -> HandleCountdownTick -> UpdateCountdownText
// - OnMatchStarted -> HandleMatchStarted -> Update all
// - OnMatchEnded -> HandleMatchEnded -> Update all
//
// Designer Workflow:
// 1. Create WBP_QuidditchDebugHUD Blueprint (parent: QuidditchDebugWidget)
// 2. Add TextBlocks named: AgentReadyText, MatchStateText, CountdownText
// 3. Create widget and add to viewport from Level BP or PlayerController
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Code/Quidditch/QuidditchTypes.h"
#include "QuidditchDebugWidget.generated.h"

class UTextBlock;
class AQuidditchGameMode;

UCLASS()
class END2507_API UQuidditchDebugWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // ========================================================================
    // WIDGET REFERENCES (Bound in Blueprint via BindWidgetOptional)
    // ========================================================================

    // Shows "Ready Agents: 0 / 1"
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* AgentReadyText;

    // Shows "State: WaitingForReady"
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* MatchStateText;

    // Shows "Countdown: 3" or "Countdown: Inactive"
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* CountdownText;

    // ========================================================================
    // GAMEMODE BINDING
    // ========================================================================

    UPROPERTY()
    AQuidditchGameMode* CachedGameMode;

    void BindToGameMode();
    void UnbindFromGameMode();

private:
    // ========================================================================
    // DELEGATE HANDLERS (UFUNCTION required for dynamic delegate binding)
    // ========================================================================

    UFUNCTION()
    void HandleAgentCountUpdated(int32 CurrentReadyCount, int32 RequiredCount);

    UFUNCTION()
    void HandleMatchStateChanged(EQuidditchMatchState OldState, EQuidditchMatchState NewState);

    UFUNCTION()
    void HandleCountdownTick(int32 SecondsRemaining);

    UFUNCTION()
    void HandleMatchStarted(float CountdownSeconds);

    UFUNCTION()
    void HandleMatchEnded();

    // ========================================================================
    // HELPERS
    // ========================================================================

    // Convert match state enum to human-readable text
    static FString MatchStateToString(EQuidditchMatchState State);
};
