// ============================================================================
// SteamDeckInputSettings.cpp
// Developer: Marcus Daley
// Date: February 11, 2026
// Project: SteamDeckInput Plugin
// ============================================================================
// Purpose:
// Implementation of plugin settings.
// Sets defaults and configures the settings category.
// ============================================================================

#include "SteamDeckInputSettings.h"

USteamDeckInputSettings::USteamDeckInputSettings()
{
    // Set default values
    bAutoDetectSteamDeck = true;
    bShowSteamDeckButtonPrompts = true;
    bPersistUserSettings = true;
}

FName USteamDeckInputSettings::GetCategoryName() const
{
    return TEXT("Plugins");
}
