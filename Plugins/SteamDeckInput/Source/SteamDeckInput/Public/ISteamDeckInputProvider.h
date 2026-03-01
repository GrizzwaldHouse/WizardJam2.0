// ============================================================================
// ISteamDeckInputProvider.h
// Developer: Marcus Daley
// Date: February 11, 2026
// Project: SteamDeckInput Plugin
// ============================================================================
// Purpose:
// Interface for classes that provide Steam Deck input detection and mode switching.
// Defines the contract for querying input capabilities (gyro, trackpads) and
// managing input modes (Desktop, Gamepad, SteamDeck, Custom).
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ISteamDeckInputProvider.generated.h"

// ============================================================================
// ESteamDeckInputMode
// Input mode enum for different control schemes
// ============================================================================
UENUM(BlueprintType)
enum class ESteamDeckInputMode : uint8
{
    Desktop         UMETA(DisplayName = "Desktop (Keyboard & Mouse)"),
    Gamepad         UMETA(DisplayName = "Standard Gamepad"),
    SteamDeck       UMETA(DisplayName = "Steam Deck"),
    Custom          UMETA(DisplayName = "Custom Configuration")
};

// ============================================================================
// UISteamDeckInputProvider
// UInterface boilerplate for the provider interface
// ============================================================================
UINTERFACE(MinimalAPI, Blueprintable)
class UISteamDeckInputProvider : public UInterface
{
    GENERATED_BODY()
};

// ============================================================================
// IISteamDeckInputProvider
// Pure virtual interface for input mode detection and management
// ============================================================================
class STEAMDECKINPUT_API IISteamDeckInputProvider
{
    GENERATED_BODY()

public:
    // Detects the current input mode based on hardware and configuration
    virtual ESteamDeckInputMode DetectInputMode() const = 0;

    // Sets the current input mode (may trigger IMC remapping)
    virtual void SetInputMode(ESteamDeckInputMode NewMode) = 0;

    // Returns the currently active input mode
    virtual ESteamDeckInputMode GetCurrentInputMode() const = 0;

    // Returns true if gyroscope hardware is available
    virtual bool IsGyroAvailable() const = 0;

    // Returns true if trackpad hardware is available
    virtual bool AreTrackpadsAvailable() const = 0;
};
