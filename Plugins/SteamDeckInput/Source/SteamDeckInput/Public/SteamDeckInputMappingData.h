// ============================================================================
// SteamDeckInputMappingData.h
// Developer: Marcus Daley
// Date: February 11, 2026
// Project: SteamDeckInput Plugin
// ============================================================================
// Purpose:
// Data asset for storing game-specific Steam Deck input mappings.
// Defines button mappings, gyro settings, trackpad modes, and layered contexts.
// Designers create these as primary data assets to configure per-game input schemes.
//
// Designer Usage:
// 1. Create Data Asset: Right-click -> Miscellaneous -> Data Asset -> SteamDeckInputMappingData
// 2. Set GameName and SteamAppID
// 3. Assign DefaultContext (IMC_SteamDeck_Default)
// 4. Configure LayeredContexts (Menu, Flight, Vehicle, etc.)
// 5. Fill ButtonMappings array with action->button associations
// 6. Tune gyro and trackpad settings
// 7. Reference this asset in project settings or load dynamically
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SteamDeckInputMappingData.generated.h"

// Forward declarations
class UInputMappingContext;
class UInputAction;

// ============================================================================
// FSteamDeckButtonMapping
// Struct representing a single button mapping for a specific action
// ============================================================================
USTRUCT(BlueprintType)
struct FSteamDeckButtonMapping
{
    GENERATED_BODY()

    // Display name for UI (e.g., "Jump", "Fire", "Interact")
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button Mapping")
    FText ActionDisplayName;

    // Reference to the Input Action this mapping applies to
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button Mapping")
    TSoftObjectPtr<UInputAction> InputAction;

    // Primary button name (e.g., "SteamDeck_A", "SteamDeck_RightTrigger")
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button Mapping")
    FName PrimarySteamDeckButton;

    // Optional secondary button (for alternate bindings)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button Mapping")
    FName SecondarySteamDeckButton;

    // Whether this mapping can be changed by the user
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button Mapping")
    bool bIsRemappable = true;

    // Category for UI grouping (Movement, Combat, UI, etc.)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button Mapping")
    FName Category;
};

// ============================================================================
// USteamDeckInputMappingData
// Primary data asset for game-specific Steam Deck configurations
// ============================================================================
UCLASS(BlueprintType)
class STEAMDECKINPUT_API USteamDeckInputMappingData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    // ========================================================================
    // Game Identification
    // ========================================================================

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Info")
    FString GameName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Game Info")
    int32 SteamAppID = 0;

    // ========================================================================
    // Input Mapping Contexts
    // ========================================================================

    // Default IMC applied when Steam Deck mode is active
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Contexts")
    TSoftObjectPtr<UInputMappingContext> DefaultContext;

    // Named layers for different game modes (Menu, Flight, Vehicle, etc.)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Contexts")
    TMap<FName, TSoftObjectPtr<UInputMappingContext>> LayeredContexts;

    // ========================================================================
    // Button Mappings
    // ========================================================================

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Button Mappings")
    TArray<FSteamDeckButtonMapping> ButtonMappings;

    // ========================================================================
    // Gyro Configuration
    // ========================================================================

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gyro Settings")
    bool bGyroEnabledByDefault = false;

    // OnRightTrackpadTouch, OnADS, AlwaysOn, etc.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gyro Settings")
    FName GyroActivationMode = TEXT("OnRightTrackpadTouch");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gyro Settings", meta = (ClampMin = "0.1", ClampMax = "3.0"))
    float DefaultGyroSensitivity = 1.0f;

    // ========================================================================
    // Trackpad Configuration
    // ========================================================================

    // MouseLike, JoystickEmu, RadialMenu, etc.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trackpad Settings")
    FName RightTrackpadMode = TEXT("MouseLike");

    // DPadEmulation, RadialMenu, ScrollWheel, etc.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trackpad Settings")
    FName LeftTrackpadMode = TEXT("DPadEmulation");

    // ========================================================================
    // UPrimaryDataAsset Interface
    // ========================================================================

    virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
