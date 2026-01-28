// ============================================================================
// WizardJamResultsWidget.cpp
// Developer: Marcus Daley
// Date: January 7, 2026
// ============================================================================
// Implementation Notes:
// - NO Kismet/GameplayStatics usage
// - Navigation handled via delegates (Observer pattern)
// - Sound played through owning PlayerController
// - Timer accessed through GetWorld()->GetTimerManager()
// ============================================================================

#include "Code/UI/WizardJamResultsWidget.h"
#include "Code/ButtonWidgetComponent.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/AudioComponent.h"
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY(LogWizardJamResults);

// ============================================================================
// LIFECYCLE
// ============================================================================

void UWizardJamResultsWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // ========================================================================
    // BIND BUTTON DELEGATES
    // ========================================================================

    if (RestartButton)
    {
        RestartButton->OnClickedEvent.AddDynamic(this, &UWizardJamResultsWidget::OnRestartClicked);
        RestartButton->SetButtonText(FText::FromString(TEXT("Play Again")));
        UE_LOG(LogWizardJamResults, Display, TEXT("[ResultsWidget] RestartButton bound"));
    }
    else
    {
        UE_LOG(LogWizardJamResults, Error,
            TEXT("[ResultsWidget] RestartButton is NULL! Create WBP_GameButton named 'RestartButton'"));
    }

    if (MenuButton)
    {
        MenuButton->OnClickedEvent.AddDynamic(this, &UWizardJamResultsWidget::OnMenuClicked);
        MenuButton->SetButtonText(FText::FromString(TEXT("Main Menu")));
        UE_LOG(LogWizardJamResults, Display, TEXT("[ResultsWidget] MenuButton bound"));
    }
    else
    {
        UE_LOG(LogWizardJamResults, Error,
            TEXT("[ResultsWidget] MenuButton is NULL! Create WBP_GameButton named 'MenuButton'"));
    }

    // ========================================================================
    // VALIDATE CONFIGURATIONS
    // ========================================================================

    if (ResultConfigurations.Num() == 0)
    {
        UE_LOG(LogWizardJamResults, Warning,
            TEXT("[ResultsWidget] No ResultConfigurations defined! Add entries in Blueprint."));
    }
    else
    {
        UE_LOG(LogWizardJamResults, Display,
            TEXT("[ResultsWidget] Loaded %d result configurations"), ResultConfigurations.Num());
    }
}

void UWizardJamResultsWidget::NativeDestruct()
{
    // Clear any running timers to prevent crashes
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(AutoReturnTimerHandle);
    }

    Super::NativeDestruct();
}

// ============================================================================
// PUBLIC API
// ============================================================================

void UWizardJamResultsWidget::ShowResults(const FMatchSummary& MatchData)
{
    CurrentResultType = MatchData.ResultType;

    // Find the configuration for this result type
    const FResultConfiguration* Config = FindResultConfiguration(MatchData.ResultType);

    if (!Config)
    {
        // Try default result type
        Config = FindResultConfiguration(DefaultResultType);

        if (!Config)
        {
            UE_LOG(LogWizardJamResults, Error,
                TEXT("[ResultsWidget] No configuration found for '%s' and no default set!"),
                *MatchData.ResultType.ToString());
            return;
        }

        UE_LOG(LogWizardJamResults, Warning,
            TEXT("[ResultsWidget] Result type '%s' not found, using default '%s'"),
            *MatchData.ResultType.ToString(), *DefaultResultType.ToString());
    }

    // Apply the configuration
    ApplyResultConfiguration(*Config);

    // Update score display
    UpdateScoreDisplay(MatchData);

    // Configure buttons
    ConfigureButtons(*Config);

    // Play sound if configured
    if (Config->ResultSound)
    {
        PlayResultSound(Config->ResultSound);
    }

    UE_LOG(LogWizardJamResults, Display,
        TEXT("[ResultsWidget] Showing result: %s | Primary: %d | Secondary: %d"),
        *MatchData.ResultType.ToString(), MatchData.PrimaryScore, MatchData.SecondaryScore);
}

void UWizardJamResultsWidget::ShowSimpleResult(bool bPlayerWon, int32 PlayerScore, int32 OpponentScore)
{
    // Build match summary with standard victory/defeat type
    FMatchSummary Summary;
    Summary.ResultType = bPlayerWon ? FName("Victory") : FName("Defeat");
    Summary.PrimaryScore = PlayerScore;
    Summary.SecondaryScore = OpponentScore;
    Summary.PrimaryScoreLabel = FText::FromString(TEXT("PLAYER"));
    Summary.SecondaryScoreLabel = FText::FromString(TEXT("OPPONENT"));

    ShowResults(Summary);
}

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

const FResultConfiguration* UWizardJamResultsWidget::FindResultConfiguration(FName ResultType) const
{
    for (const FResultConfiguration& Config : ResultConfigurations)
    {
        if (Config.ResultType == ResultType)
        {
            return &Config;
        }
    }

    return nullptr;
}

void UWizardJamResultsWidget::ApplyResultConfiguration(const FResultConfiguration& Config)
{
    // ========================================================================
    // SET BACKGROUND
    // ========================================================================

    if (ResultsBackground)
    {
        if (Config.BackgroundTexture)
        {
            ResultsBackground->SetBrushFromTexture(Config.BackgroundTexture);
            ResultsBackground->SetVisibility(ESlateVisibility::Visible);

            UE_LOG(LogWizardJamResults, Display,
                TEXT("[ResultsWidget] Background set: %s"), *Config.BackgroundTexture->GetName());
        }
        else
        {
            // No texture configured - hide background or use default color
            ResultsBackground->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    // ========================================================================
    // SET TITLE
    // ========================================================================

    if (TitleText)
    {
        TitleText->SetText(Config.TitleText);
        TitleText->SetColorAndOpacity(FSlateColor(Config.TitleColor));
    }

    // ========================================================================
    // SET SUBTITLE (Optional)
    // ========================================================================

    if (SubtitleText)
    {
        if (!Config.SubtitleText.IsEmpty())
        {
            SubtitleText->SetText(Config.SubtitleText);
            SubtitleText->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            SubtitleText->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

void UWizardJamResultsWidget::UpdateScoreDisplay(const FMatchSummary& MatchData)
{
    // ========================================================================
    // PRIMARY SCORE
    // ========================================================================

    if (PrimaryScoreLabelText)
    {
        PrimaryScoreLabelText->SetText(MatchData.PrimaryScoreLabel);
    }

    if (PrimaryScoreText)
    {
        PrimaryScoreText->SetText(FText::AsNumber(MatchData.PrimaryScore));
    }

    // ========================================================================
    // SECONDARY SCORE (Optional - hide if not used)
    // ========================================================================

    if (SecondaryScoreLabelText)
    {
        SecondaryScoreLabelText->SetText(MatchData.SecondaryScoreLabel);
    }

    if (SecondaryScoreText)
    {
        SecondaryScoreText->SetText(FText::AsNumber(MatchData.SecondaryScore));
    }

    // ========================================================================
    // COLLECTION DISPLAY
    // ========================================================================

    if (CollectionText)
    {
        if (MatchData.TotalItems > 0)
        {
            FText CollectionFormatted = FText::Format(
                FText::FromString(TEXT("{0}: {1} / {2}")),
                MatchData.CollectionLabel,
                FText::AsNumber(MatchData.ItemsCollected),
                FText::AsNumber(MatchData.TotalItems)
            );
            CollectionText->SetText(CollectionFormatted);
            CollectionText->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            CollectionText->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

void UWizardJamResultsWidget::ConfigureButtons(const FResultConfiguration& Config)
{
    // ========================================================================
    // BUTTON VISIBILITY
    // ========================================================================

    if (ButtonArea)
    {
        ESlateVisibility ButtonVisibility = Config.bShowButtons
            ? ESlateVisibility::Visible
            : ESlateVisibility::Collapsed;

        ButtonArea->SetVisibility(ButtonVisibility);

        // TODO: Implement FocusButton() on UButtonWidgetComponent for keyboard navigation
        // if (Config.bShowButtons && RestartButton)
        // {
        //     RestartButton->FocusButton();
        // }
    }

    // ========================================================================
    // AUTO-RETURN TIMER
    // ========================================================================

    if (Config.bAutoReturn)
    {
        StartAutoReturnTimer(Config.AutoReturnDelay);
    }
}

void UWizardJamResultsWidget::PlayResultSound(USoundBase* Sound)
{
    if (!Sound)
    {
        return;
    }

    // Get owning player controller to play sound
    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        UE_LOG(LogWizardJamResults, Warning,
            TEXT("[ResultsWidget] Cannot play sound - no owning player"));
        return;
    }

    // Play 2D sound through the player controller's audio component
    // ClientPlaySound is a built-in RPC-safe method on PlayerController
    PC->ClientPlaySound(Sound);

    UE_LOG(LogWizardJamResults, Display,
        TEXT("[ResultsWidget] Playing result sound: %s"), *Sound->GetName());
}

void UWizardJamResultsWidget::StartAutoReturnTimer(float Delay)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogWizardJamResults, Warning,
            TEXT("[ResultsWidget] Cannot start timer - no world context"));
        return;
    }

    World->GetTimerManager().SetTimer(
        AutoReturnTimerHandle,
        this,
        &UWizardJamResultsWidget::OnAutoReturnTimerFired,
        Delay,
        false
    );

    UE_LOG(LogWizardJamResults, Display,
        TEXT("[ResultsWidget] Auto-return timer started: %.1f seconds"), Delay);
}

void UWizardJamResultsWidget::OnAutoReturnTimerFired()
{
    UE_LOG(LogWizardJamResults, Display, TEXT("[ResultsWidget] Auto-return timer fired"));

    // Broadcast delegate - PlayerController handles the actual navigation
    OnAutoReturnTriggered.Broadcast();
}

// ============================================================================
// BUTTON CALLBACKS
// ============================================================================

void UWizardJamResultsWidget::OnRestartClicked()
{
    UE_LOG(LogWizardJamResults, Display, TEXT("[ResultsWidget] Restart clicked"));

    // Clear timer if running
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(AutoReturnTimerHandle);
    }

    // Broadcast delegate - PlayerController handles the actual level reload
    OnRestartRequested.Broadcast();
}

void UWizardJamResultsWidget::OnMenuClicked()
{
    UE_LOG(LogWizardJamResults, Display, TEXT("[ResultsWidget] Menu clicked"));

    // Clear timer if running
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(AutoReturnTimerHandle);
    }

    // Broadcast delegate - PlayerController handles the actual navigation
    OnMenuRequested.Broadcast();
}