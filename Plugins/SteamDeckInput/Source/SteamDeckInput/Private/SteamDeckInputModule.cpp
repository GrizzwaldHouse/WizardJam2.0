// ============================================================================
// SteamDeckInputModule.cpp
// Developer: Marcus Daley
// Date: February 11, 2026
// Project: SteamDeckInput Plugin
// ============================================================================
// Purpose:
// Implementation of the SteamDeckInput module interface.
// Logs module load and unload events.
// ============================================================================

#include "SteamDeckInputModule.h"

DEFINE_LOG_CATEGORY(LogSteamDeckInput);

void FSteamDeckInputModule::StartupModule()
{
    UE_LOG(LogSteamDeckInput, Log, TEXT("SteamDeckInput module loaded"));
}

void FSteamDeckInputModule::ShutdownModule()
{
    UE_LOG(LogSteamDeckInput, Log, TEXT("SteamDeckInput module unloaded"));
}

IMPLEMENT_MODULE(FSteamDeckInputModule, SteamDeckInput)
