// ============================================================================
// WizardJamResultsWidget.h
// Developer: Marcus Daley
// Date: January 7, 2026
// ============================================================================
// Purpose:
// Scalable results widget that supports unlimited result types.
// Designers configure result variations in Blueprint without C++ changes.
//
// Architecture:
// - FName-based result types (not hardcoded enum)
// - TArray of FResultConfiguration for designer customization
// - FMatchSummary carries runtime data from any game mode
// - Delegates broadcast user actions (Observer pattern)
// - NO Kismet/GameplayStatics - PlayerController handles navigation
//
// Supports:
// - Quidditch matches (Player vs AI scoring)
// - Boss battles (Victory/Defeat)
// - Survival modes (Time survived)
// - Collection modes (Items gathered)
// - Any future game mode
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Code/UI/ResultTypes.h"
#include "WizardJamResultsWidget.generated.h"

// Forward declarations
class UImage;
class UTextBlock;
class UVerticalBox;
class UButtonWidgetComponent;
class USoundBase;
class UAudioComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogWizardJamResults, Log, All);

// ============================================================================
// DELEGATE DECLARATIONS
// Widget broadcasts these - PlayerController listens and handles navigation
// ============================================================================

// Fired when player clicks "Play Again" button
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRestartRequested);

// Fired when player clicks "Main Menu" button
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMenuRequested);

// Fired when auto-return timer completes
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAutoReturnTriggered);

// ============================================================================
// WIZARDJAM RESULTS WIDGET
// ============================================================================

UCLASS()
class END2507_API UWizardJamResultsWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // ========================================================================
    // DELEGATES - PlayerController binds to these
    // ========================================================================

    // Broadcast when restart button clicked
    UPROPERTY(BlueprintAssignable, Category = "Results|Events")
    FOnRestartRequested OnRestartRequested;

    // Broadcast when menu button clicked
    UPROPERTY(BlueprintAssignable, Category = "Results|Events")
    FOnMenuRequested OnMenuRequested;

    // Broadcast when auto-return timer fires
    UPROPERTY(BlueprintAssignable, Category = "Results|Events")
    FOnAutoReturnTriggered OnAutoReturnTriggered;

    // ========================================================================
    // PUBLIC API
    // ========================================================================

    // Shows results using the specified result type and match data
    // This is the main entry point - call this from GameMode or PlayerController
    UFUNCTION(BlueprintCallable, Category = "Results")
    void ShowResults(const FMatchSummary& MatchData);

    // Convenience function for simple win/lose scenarios
    // Automatically picks "Victory" or "Defeat" result type based on bPlayerWon
    UFUNCTION(BlueprintCallable, Category = "Results")
    void ShowSimpleResult(bool bPlayerWon, int32 PlayerScore, int32 OpponentScore);

    // Returns the currently displayed result type
    UFUNCTION(BlueprintPure, Category = "Results")
    FName GetCurrentResultType() const { return CurrentResultType; }

protected:
    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // ========================================================================
    // DESIGNER CONFIGURATION
    // All result types configured here - no C++ changes needed to add more
    // ========================================================================

    // Array of all possible result configurations
    // Designer adds entries for each result type (Victory, Defeat, BossKilled, etc.)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Results|Configuration")
    TArray<FResultConfiguration> ResultConfigurations;

    // Default result type if requested type not found
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Results|Configuration")
    FName DefaultResultType;

    // ========================================================================
    // BOUND WIDGETS - Designer creates these in UMG
    // ========================================================================

    // Background image - changes based on result type
    UPROPERTY(meta = (BindWidget))
    UImage* ResultsBackground;

    // Semi-transparent overlay for text readability
    UPROPERTY(meta = (BindWidgetOptional))
    UImage* DarkOverlay;

    // Main title text (VICTORY!, DEFEATED!, etc.)
    UPROPERTY(meta = (BindWidget))
    UTextBlock* TitleText;

    // Optional subtitle
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* SubtitleText;

    // Primary score label (e.g., "PLAYER", "KILLS")
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* PrimaryScoreLabelText;

    // Primary score value
    UPROPERTY(meta = (BindWidget))
    UTextBlock* PrimaryScoreText;

    // Secondary score label (e.g., "AI", "DEATHS")
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* SecondaryScoreLabelText;

    // Secondary score value
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* SecondaryScoreText;

    // Collection display (e.g., "Spells: 3 / 4")
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* CollectionText;

    // Container for buttons (hidden on some result types)
    UPROPERTY(meta = (BindWidget))
    UVerticalBox* ButtonArea;

    // Play Again button
    UPROPERTY(meta = (BindWidget))
    UButtonWidgetComponent* RestartButton;

    // Main Menu button
    UPROPERTY(meta = (BindWidget))
    UButtonWidgetComponent* MenuButton;

private:
    // ========================================================================
    // INTERNAL HELPERS
    // ========================================================================

    // Finds configuration for the given result type
    // Returns nullptr if not found
    const FResultConfiguration* FindResultConfiguration(FName ResultType) const;

    // Applies a result configuration to the widget visuals
    void ApplyResultConfiguration(const FResultConfiguration& Config);

    // Updates score display with match data
    void UpdateScoreDisplay(const FMatchSummary& MatchData);

    // Configures button visibility and behavior
    void ConfigureButtons(const FResultConfiguration& Config);

    // Plays the result sound through owning player
    void PlayResultSound(USoundBase* Sound);

    // Starts auto-return timer if enabled
    void StartAutoReturnTimer(float Delay);

    // Called when auto-return timer fires
    void OnAutoReturnTimerFired();

    // ========================================================================
    // BUTTON CALLBACKS
    // ========================================================================

    UFUNCTION()
    void OnRestartClicked();

    UFUNCTION()
    void OnMenuClicked();

    // ========================================================================
    // STATE
    // ========================================================================

    // Timer handle for auto-return
    FTimerHandle AutoReturnTimerHandle;

    // Currently displayed result type
    FName CurrentResultType;
};