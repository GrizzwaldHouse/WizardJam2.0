// ============================================================================
// SteamDeckInputModule.h
// Developer: Marcus Daley
// Date: February 11, 2026
// Project: SteamDeckInput Plugin
// ============================================================================
// Purpose:
// Module interface for the SteamDeckInput plugin.
// Handles module startup, shutdown, and provides logging category for the plugin.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSteamDeckInput, Log, All);

class FSteamDeckInputModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
