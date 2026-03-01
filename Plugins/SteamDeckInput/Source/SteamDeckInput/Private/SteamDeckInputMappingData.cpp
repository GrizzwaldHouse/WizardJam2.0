// ============================================================================
// SteamDeckInputMappingData.cpp
// Developer: Marcus Daley
// Date: February 11, 2026
// Project: SteamDeckInput Plugin
// ============================================================================
// Purpose:
// Implementation of the Steam Deck input mapping data asset.
// Provides primary asset ID for asset management.
// ============================================================================

#include "SteamDeckInputMappingData.h"

FPrimaryAssetId USteamDeckInputMappingData::GetPrimaryAssetId() const
{
    return FPrimaryAssetId(TEXT("SteamDeckMapping"), GetFName());
}
