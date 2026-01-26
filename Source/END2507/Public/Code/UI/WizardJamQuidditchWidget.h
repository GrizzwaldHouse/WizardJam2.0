// ============================================================================
// WizardJamQuidditchWidget.h
// Quidditch match scoreboard widget displaying scores, timer, and match state
//
// Developer: Marcus Daley
// Date: January 26, 2026
// Project: WizardJam2.0 (END2507)
//
// PURPOSE:
// Displays real-time Quidditch match information:
// - Player team score vs AI team score
// - Match timer (countdown or elapsed)
// - Team names with customizable labels
//
// DESIGNER WORKFLOW:
// 1. Create WBP_WizardJamQuidditchWidget Blueprint inheriting from this class
// 2. Add text widgets with EXACT names matching BindWidgetOptional properties
// 3. Set default team names in Blueprint defaults
// 4. Assign to WizardJamHUDWidget's QuidditchWidgetClass
//
// WIDGET NAMING CONVENTION:
// - PlayerScoreText, AIScoreText (score displays)
// - PlayerScoreLabel, AIScoreLabel (team name labels)
// - MatchTimerText (countdown timer)
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WizardJamQuidditchWidget.generated.h"

// Forward declarations
class AWizardJamGameMode;
class UTextBlock;
struct FSignalData;

DECLARE_LOG_CATEGORY_EXTERN(LogQuidditchWidget, Log, All);

// Delegate for timer expiration - GameMode can bind to handle match timeout
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMatchTimerExpired);

UCLASS()
class END2507_API UWizardJamQuidditchWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // ========================================================================
    // PUBLIC API - Score Updates
    // Can be called directly or triggered via delegates
    // ========================================================================

    // Update player team's displayed score
    // PointsAdded is for optional animation/feedback (flash effect)
    UFUNCTION(BlueprintCallable, Category = "Quidditch|Scores")
    void UpdatePlayerScore(int32 NewScore, int32 PointsAdded = 0);

    // Update AI team's displayed score
    UFUNCTION(BlueprintCallable, Category = "Quidditch|Scores")
    void UpdateAIScore(int32 NewScore, int32 PointsAdded = 0);

    // ========================================================================
    // PUBLIC API - Timer
    // ========================================================================

    // Update the match timer display
    // TimeRemaining in seconds - formatted as MM:SS
    UFUNCTION(BlueprintCallable, Category = "Quidditch|Timer")
    void UpdateTimer(float TimeRemaining);

    // Start the match timer (call from GameMode)
    UFUNCTION(BlueprintCallable, Category = "Quidditch|Timer")
    void StartMatchTimer(float MatchDurationSeconds);

    // Stop/pause the match timer
    UFUNCTION(BlueprintCallable, Category = "Quidditch|Timer")
    void StopMatchTimer();

    // ========================================================================
    // PUBLIC API - Team Labels
    // ========================================================================

    // Set custom team names (e.g., "WIZARDS" vs "SERPENTS")
    UFUNCTION(BlueprintCallable, Category = "Quidditch|Teams")
    void SetTeamLabels(const FText& PlayerTeamName, const FText& AITeamName);

    // ========================================================================
    // PUBLIC API - Match State
    // ========================================================================

    // Called when match ends - updates display
    UFUNCTION(BlueprintCallable, Category = "Quidditch|Match")
    void OnMatchEnded(bool bPlayerWon, const FString& Reason);

    // ========================================================================
    // DELEGATES
    // ========================================================================

    // Broadcast when match timer reaches zero
    // GameMode binds to this to handle timeout condition
    UPROPERTY(BlueprintAssignable, Category = "Quidditch|Events")
    FOnMatchTimerExpired OnMatchTimerExpired;

protected:
    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // ========================================================================
    // WIDGET REFERENCES
    // Names MUST match exactly in UMG Blueprint
    // ========================================================================

    // Score displays
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* PlayerScoreText;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* AIScoreText;

    // Team name labels
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* PlayerScoreLabel;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* AIScoreLabel;

    // Match timer
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* MatchTimerText;

    // Optional match status text ("MATCH IN PROGRESS", "PLAYER WINS!")
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* MatchStatusText;

    // ========================================================================
    // CONFIGURATION
    // Designer sets in Blueprint defaults
    // ========================================================================

    // Default player team name (shown in PlayerScoreLabel)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quidditch|Config")
    FText DefaultPlayerTeamName;

    // Default AI team name (shown in AIScoreLabel)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quidditch|Config")
    FText DefaultAITeamName;

    // Match duration in seconds (0 = no timer, unlimited)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quidditch|Config")
    float DefaultMatchDuration;

    // ========================================================================
    // GAMEMODE BINDING
    // ========================================================================

    // Bind to GameMode score delegates
    void BindToGameMode();

    // Unbind from GameMode
    void UnbindFromGameMode();

    // Cached GameMode reference
    UPROPERTY()
    AWizardJamGameMode* CachedGameMode;

    // ========================================================================
    // WORLD SIGNAL INTEGRATION
    // Timer starts when QuidditchMatchStart signal is received
    // ========================================================================

    // Bind to global signal emitter delegate
    void BindToWorldSignals();

    // Unbind from global signals
    void UnbindFromWorldSignals();

    // Handle global signal emission
    void HandleGlobalSignal(const FSignalData& SignalData);

    // Handle for global signal binding (needed for removal)
    FDelegateHandle GlobalSignalHandle;

    // ========================================================================
    // TIMER STATE
    // ========================================================================

    // Is timer currently running?
    bool bTimerRunning;

    // Time remaining in match (seconds)
    float MatchTimeRemaining;

    // Total match duration (for reset/display)
    float MatchDuration;

private:
    // ========================================================================
    // DELEGATE HANDLERS
    // ========================================================================

    // Handler for GameMode's OnScoreChanged delegate
    // Signature: FOnScoreChanged(int32 PlayerScore, int32 AIScore, AActor* ScoringActor)
    UFUNCTION()
    void HandleScoreChanged(int32 PlayerScore, int32 AIScore, AActor* ScoringActor);

    // Handler for GameMode's OnMatchEnded delegate
    // Signature: FOnMatchEnded(bool bPlayerWon, const FString& Reason)
    UFUNCTION()
    void HandleMatchEnded(bool bPlayerWon, const FString& Reason);

    // ========================================================================
    // INTERNAL HELPERS
    // ========================================================================

    // Format seconds to MM:SS string
    FString FormatTime(float Seconds) const;

    // Flash score text for feedback (optional visual effect)
    void FlashScoreText(UTextBlock* ScoreText, bool bPositive);

    // Called when timer reaches zero
    void OnTimerExpired();

    // Track if match has ended to prevent duplicate handling
    bool bMatchEnded;

    // ========================================================================
    // CACHED SCORE STATE (Observer Pattern)
    // Updated ONLY via HandleScoreChanged delegate - never query GameMode
    // ========================================================================

    int32 CachedPlayerScore;
    int32 CachedAIScore;
};
