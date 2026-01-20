// ============================================================================
// BroomTypes.h
// Shared types for the modular broom flight system
//
// Developer: Marcus Daley
// Date: January 20, 2026
// Project: WizardJam
// ============================================================================
// PURPOSE:
// Defines FBroomConfiguration struct that holds ALL broom behavior settings.
// Designers configure these values in BroomActor child Blueprints.
// AC_BroomComponent reads from this struct - no hardcoded values in component.
//
// ARCHITECTURE:
// This follows the same pattern as ElementTypes.h / ElementDatabase.h:
// - Struct defined once in types header
// - BroomActor exposes struct for designer configuration
// - Component reads from struct at runtime
//
// DESIGNER WORKFLOW:
// 1. Create child Blueprint of BroomActor (e.g., BP_Broom_Quidditch)
// 2. Configure BroomConfiguration struct in Details panel
// 3. Place broom in level or spawn dynamically
// 4. Component automatically uses those settings when player mounts
//
// EXPANDING THE SYSTEM:
// To add new broom behaviors (racing, stealth, combat):
// 1. Add new properties to FBroomConfiguration
// 2. AC_BroomComponent checks those properties
// 3. Designer creates new child Blueprint with desired settings
// 4. NO enum changes, NO recompile for designers
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BroomTypes.generated.h"

// ============================================================================
// FBROOM CONFIGURATION
// All tunable broom settings in one designer-editable struct
// ============================================================================

USTRUCT(BlueprintType)
struct  END2507_API FBroomConfiguration
{
    GENERATED_BODY()

    // ========================================================================
    // IDENTITY
    // ========================================================================

    // Display name for UI and tooltips
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Broom|Identity")
    FText BroomDisplayName;

    // Description shown in inventory or selection UI
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Broom|Identity")
    FText BroomDescription;

    // ========================================================================
    // DURATION BEHAVIOR
    // Controls whether flight is time-limited or infinite
    // ========================================================================

    // If true, flight lasts until EndFlight() is called (Quidditch, cutscenes)
    // If false, flight ends when Duration expires OR stamina depletes
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Broom|Duration")
    bool bInfiniteDuration;

    // Maximum flight time in seconds (only used if bInfiniteDuration = false)
    // Set to 0 for stamina-only gating (no time limit)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Broom|Duration",
        meta = (EditCondition = "!bInfiniteDuration", ClampMin = "0.0"))
    float FlightDuration;

    // ========================================================================
    // STAMINA BEHAVIOR
    // How stamina is consumed and regenerated during flight
    // ========================================================================

    // If true, stamina only drains while player is providing movement input
    // If false, stamina drains constantly while flying
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Broom|Stamina")
    bool bDrainOnlyWhenMoving;

    // If true, stamina regenerates when player is idle (no input) while flying
    // Allows "catch your breath" gameplay - stop moving to recover
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Broom|Stamina",
        meta = (EditCondition = "bDrainOnlyWhenMoving"))
    bool bRegenWhenIdle;

    // If true, player is forced to dismount when stamina hits zero
    // If false, player can remain mounted but cannot move until stamina regens
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Broom|Stamina")
    bool bDismountOnStaminaDepletion;

    // Stamina drain per second during normal flight
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Broom|Stamina",
        meta = (ClampMin = "0.0"))
    float BaseStaminaDrainRate;

    // Additional stamina drain per second while boosting
    // Total boost drain = BaseStaminaDrainRate + BoostStaminaDrainRate
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Broom|Stamina",
        meta = (ClampMin = "0.0"))
    float BoostStaminaDrainRate;

    // Stamina regen per second while idle (only if bRegenWhenIdle = true)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Broom|Stamina",
        meta = (EditCondition = "bRegenWhenIdle", ClampMin = "0.0"))
    float IdleStaminaRegenRate;

    // Minimum stamina required to start flying (0-1 percentage)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Broom|Stamina",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MinStaminaToFly;

    // ========================================================================
    // SPEED SETTINGS
    // Movement speeds for different flight modes
    // ========================================================================

    // Normal flight speed
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Broom|Speed",
        meta = (ClampMin = "0.0"))
    float FlySpeed;

    // Speed while holding boost button
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Broom|Speed",
        meta = (ClampMin = "0.0"))
    float BoostSpeed;

    // Vertical movement speed (ascend/descend)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Broom|Speed",
        meta = (ClampMin = "0.0"))
    float VerticalSpeed;

    // ========================================================================
    // DECELERATION
    // Optional momentum system - broom slows down gradually when input stops
    // ========================================================================

    // If true, broom continues moving after input stops and gradually slows
    // If false, broom stops instantly when input stops
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Broom|Deceleration")
    bool bUseDeceleration;

    // How quickly the broom slows down (units per second squared)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Broom|Deceleration",
        meta = (EditCondition = "bUseDeceleration", ClampMin = "0.0"))
    float DecelerationRate;

    // Speed below which we snap to zero (prevents infinite tiny drift)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Broom|Deceleration",
        meta = (EditCondition = "bUseDeceleration", ClampMin = "0.0"))
    float MinSpeedThreshold;

    // ========================================================================
    // MOUNTING
    // Socket configuration for different character types
    // ========================================================================

    // Socket name for player character mounting
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Broom|Mounting")
    FName PlayerMountSocket;

    // Socket name for AI/mannequin mounting (falls back to PlayerMountSocket if empty)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Broom|Mounting")
    FName AIMountSocket;

    // ========================================================================
    // CHANNEL REQUIREMENT
    // Optional - require player to have unlocked this broom type
    // ========================================================================

    // Channel player must have to use this broom (e.g., "BroomFlight", "QuidditchBroom")
    // Leave empty for no requirement
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Broom|Requirements")
    FName RequiredChannel;

    // ========================================================================
    // CONSTRUCTOR - Default values for a basic broom
    // Designers override in Blueprint children
    // ========================================================================

    FBroomConfiguration()
        : BroomDisplayName(FText::FromString(TEXT("Basic Broom")))
        , BroomDescription(FText::FromString(TEXT("A standard flying broom.")))
        , bInfiniteDuration(false)
        , FlightDuration(30.0f)
        , bDrainOnlyWhenMoving(false)
        , bRegenWhenIdle(false)
        , bDismountOnStaminaDepletion(true)
        , BaseStaminaDrainRate(15.0f)
        , BoostStaminaDrainRate(30.0f)
        , IdleStaminaRegenRate(10.0f)
        , MinStaminaToFly(0.1f)
        , FlySpeed(600.0f)
        , BoostSpeed(1200.0f)
        , VerticalSpeed(400.0f)
        , bUseDeceleration(false)
        , DecelerationRate(400.0f)
        , MinSpeedThreshold(10.0f)
        , PlayerMountSocket(NAME_None)
        , AIMountSocket(NAME_None)
        , RequiredChannel(NAME_None)
    {
    }

    // ========================================================================
    // PRESET FACTORIES
    // Static functions that return pre-configured structs for common broom types
    // NOTE: UFUNCTION cannot be used in USTRUCT - these are C++ only
    // For Blueprint access, use a UBlueprintFunctionLibrary wrapper
    // ========================================================================

    // Returns configuration for a free-flight exploration broom
    // - Timed duration (30 seconds)
    // - Constant drain while flying
    // - Dismounts on stamina depletion
    static FBroomConfiguration GetFreeFlightPreset()
    {
        FBroomConfiguration Config;
        Config.BroomDisplayName = FText::FromString(TEXT("Free Flight Broom"));
        Config.BroomDescription = FText::FromString(TEXT("Explore freely for 30 seconds."));
        Config.bInfiniteDuration = false;
        Config.FlightDuration = 30.0f;
        Config.bDrainOnlyWhenMoving = false;
        Config.bRegenWhenIdle = false;
        Config.bDismountOnStaminaDepletion = true;
        return Config;
    }

    // Returns configuration for a Quidditch gameplay broom
    // - Infinite duration (match-based)
    // - Drains only when moving
    // - Regens when idle (catch your breath)
    // - No dismount on depletion (just can't move)
    static FBroomConfiguration GetQuidditchPreset()
    {
        FBroomConfiguration Config;
        Config.BroomDisplayName = FText::FromString(TEXT("Quidditch Broom"));
        Config.BroomDescription = FText::FromString(TEXT("Optimized for Quidditch gameplay."));
        Config.bInfiniteDuration = true;
        Config.FlightDuration = 0.0f;
        Config.bDrainOnlyWhenMoving = true;
        Config.bRegenWhenIdle = true;
        Config.bDismountOnStaminaDepletion = false;
        Config.BaseStaminaDrainRate = 10.0f;
        Config.IdleStaminaRegenRate = 12.0f;
        Config.FlySpeed = 800.0f;
        Config.BoostSpeed = 1500.0f;
        Config.RequiredChannel = FName(TEXT("QuidditchBroom"));
        return Config;
    }

    // Returns configuration for a racing broom (future use)
    // - Very fast, high stamina consumption
    // - No deceleration (responsive controls)
    static FBroomConfiguration GetRacingPreset()
    {
        FBroomConfiguration Config;
        Config.BroomDisplayName = FText::FromString(TEXT("Racing Broom"));
        Config.BroomDescription = FText::FromString(TEXT("A high-speed broom for racing."));
        Config.bInfiniteDuration = false;
        Config.FlightDuration = 60.0f;
        Config.bDrainOnlyWhenMoving = true;
        Config.bRegenWhenIdle = false;
        Config.bDismountOnStaminaDepletion = true;
        Config.BaseStaminaDrainRate = 25.0f;
        Config.BoostStaminaDrainRate = 50.0f;
        Config.FlySpeed = 1200.0f;
        Config.BoostSpeed = 2000.0f;
        Config.bUseDeceleration = false;
        return Config;
    }
};