// ============================================================================
// SteamDeckInputSettings.h
// Developer: Marcus Daley
// Date: February 11, 2026
// Project: SteamDeckInput Plugin
// ============================================================================
// Purpose:
// Project settings for the SteamDeckInput plugin.
// Configurable via Editor Preferences -> Plugins -> Steam Deck Input.
//
// Designer Usage:
// 1. Open Project Settings -> Plugins -> Steam Deck Input
// 2. Set DefaultMappingData to your game's SteamDeckInputMappingData asset
// 3. Enable/disable auto-detection, button prompts, and user settings persistence
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SteamDeckInputSettings.generated.h"

// Forward declarations
class USteamDeckInputMappingData;

// ============================================================================
// USteamDeckInputSettings
// Developer settings for Steam Deck input configuration
// ============================================================================
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Steam Deck Input"))
class STEAMDECKINPUT_API USteamDeckInputSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    USteamDeckInputSettings();

    // ========================================================================
    // Settings
    // ========================================================================

    // Default mapping data asset for this game
    UPROPERTY(config, EditAnywhere, Category = "General", meta = (AllowedClasses = "/Script/SteamDeckInput.SteamDeckInputMappingData"))
    TSoftObjectPtr<USteamDeckInputMappingData> DefaultMappingData;

    // Automatically detect Steam Deck hardware on startup
    UPROPERTY(config, EditAnywhere, Category = "General")
    bool bAutoDetectSteamDeck = true;

    // Show Steam Deck button prompts in UI when Steam Deck mode is active
    UPROPERTY(config, EditAnywhere, Category = "UI")
    bool bShowSteamDeckButtonPrompts = true;

    // Save user-customized input settings to disk
    UPROPERTY(config, EditAnywhere, Category = "User Settings")
    bool bPersistUserSettings = true;

    // ========================================================================
    // UDeveloperSettings Interface
    // ========================================================================

    virtual FName GetCategoryName() const override;
};
