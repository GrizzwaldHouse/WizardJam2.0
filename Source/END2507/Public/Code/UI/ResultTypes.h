#pragma once

#include "CoreMinimal.h"
#include "ResultTypes.generated.h"

// ============================================================================
// RESULT CONFIGURATION STRUCT
// Designer creates one entry per result type
// ============================================================================

USTRUCT(BlueprintType)
struct FResultConfiguration
{
    GENERATED_BODY()

    // ========================================================================
    // IDENTIFIER
    // ========================================================================

    // Unique name for this result type
    // Examples: "QuidditchVictory", "BossDefeated", "SurvivalComplete"
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Result")
    FName ResultType;

    // ========================================================================
    // VISUAL CONFIGURATION
    // ========================================================================

    // Background image for this result
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Result|Visuals")
    UTexture2D* BackgroundTexture;

    // Title text displayed (e.g., "VICTORY!", "BOSS DEFEATED!", "SURVIVED!")
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Result|Visuals")
    FText TitleText;

    // Color for the title text
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Result|Visuals")
    FLinearColor TitleColor;

    // Optional subtitle (e.g., "Quidditch Match Complete", "Wave 5 Cleared")
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Result|Visuals")
    FText SubtitleText;

    // ========================================================================
    // BEHAVIOR CONFIGURATION
    // ========================================================================

    // Should buttons be visible? (false for victories with auto-return)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Result|Behavior")
    bool bShowButtons;

    // Should auto-return to menu after delay?
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Result|Behavior")
    bool bAutoReturn;

    // Delay before auto-return (if enabled)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Result|Behavior",
        meta = (EditCondition = "bAutoReturn", ClampMin = "1.0", ClampMax = "30.0"))
    float AutoReturnDelay;

    // ========================================================================
    // AUDIO (Optional - for future use)
    // ========================================================================

    // Sound to play when this result displays
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Result|Audio")
    USoundBase* ResultSound;

    // ========================================================================
    // CONSTRUCTOR - Default values
    // ========================================================================

    FResultConfiguration()
        : ResultType(NAME_None)
        , BackgroundTexture(nullptr)
        , TitleText(FText::FromString(TEXT("RESULT")))
        , TitleColor(FLinearColor::White)
        , SubtitleText(FText::GetEmpty())
        , bShowButtons(true)
        , bAutoReturn(false)
        , AutoReturnDelay(5.0f)
        , ResultSound(nullptr)
    {
    }
};

// ============================================================================
// MATCH SUMMARY STRUCT
// Runtime data passed to results widget
// ============================================================================

USTRUCT(BlueprintType)
struct FMatchSummary
{
    GENERATED_BODY()

    // Which result configuration to use
    UPROPERTY(BlueprintReadWrite, Category = "Match")
    FName ResultType;

    // ========================================================================
    // SCORE DATA (Generic - works for any mode)
    // ========================================================================

    // Primary score (player points, enemies killed, time survived, etc.)
    UPROPERTY(BlueprintReadWrite, Category = "Match|Scores")
    int32 PrimaryScore;

    // Secondary score (opponent points, optional)
    UPROPERTY(BlueprintReadWrite, Category = "Match|Scores")
    int32 SecondaryScore;

    // ========================================================================
    // COLLECTION DATA
    // ========================================================================

    // Items collected (spells, coins, pickups, etc.)
    UPROPERTY(BlueprintReadWrite, Category = "Match|Collection")
    int32 ItemsCollected;

    // Total possible items
    UPROPERTY(BlueprintReadWrite, Category = "Match|Collection")
    int32 TotalItems;

    // ========================================================================
    // TIME DATA
    // ========================================================================

    // Time elapsed or remaining
    UPROPERTY(BlueprintReadWrite, Category = "Match|Time")
    float TimeValue;

    // Is this time elapsed (true) or time remaining (false)?
    UPROPERTY(BlueprintReadWrite, Category = "Match|Time")
    bool bIsTimeElapsed;

    // ========================================================================
    // DISPLAY LABELS (Designer can customize per game mode)
    // ========================================================================

    // Label for primary score (e.g., "Player", "Kills", "Points")
    UPROPERTY(BlueprintReadWrite, Category = "Match|Labels")
    FText PrimaryScoreLabel;

    // Label for secondary score (e.g., "AI", "Deaths", "Opponent")
    UPROPERTY(BlueprintReadWrite, Category = "Match|Labels")
    FText SecondaryScoreLabel;

    // Label for collection (e.g., "Spells", "Coins", "Stars")
    UPROPERTY(BlueprintReadWrite, Category = "Match|Labels")
    FText CollectionLabel;

    // ========================================================================
    // CONSTRUCTOR
    // ========================================================================

    FMatchSummary()
        : ResultType(NAME_None)
        , PrimaryScore(0)
        , SecondaryScore(0)
        , ItemsCollected(0)
        , TotalItems(0)
        , TimeValue(0.0f)
        , bIsTimeElapsed(true)
        , PrimaryScoreLabel(FText::FromString(TEXT("Score")))
        , SecondaryScoreLabel(FText::FromString(TEXT("Opponent")))
        , CollectionLabel(FText::FromString(TEXT("Collected")))
    {
    }
};