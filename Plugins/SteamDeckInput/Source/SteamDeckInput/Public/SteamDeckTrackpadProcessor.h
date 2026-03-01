// ============================================================================
// SteamDeckTrackpadProcessor.h
// Developer: Marcus Daley
// Date: February 11, 2026
// Project: SteamDeckInput Plugin
// ============================================================================
// Purpose:
// Processes touchpad input from the Steam Deck's dual trackpads.
// Supports multiple modes: mouse-like, joystick emulation, D-pad, radial menu,
// scroll wheel, flick stick, and disabled.
//
// Designer Usage:
// 1. Access via USteamDeckInputSubsystem::GetTrackpadProcessor()
// 2. Call SetLeftTrackpadMode() and SetRightTrackpadMode() to configure behavior
// 3. Call ProcessLeftTrackpad() and ProcessRightTrackpad() each frame
// 4. For D-pad mode, use GetDPadDirection() to get discrete direction names
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SteamDeckTrackpadProcessor.generated.h"

// ============================================================================
// ETrackpadMode
// Defines how trackpad input is interpreted
// ============================================================================
UENUM(BlueprintType)
enum class ETrackpadMode : uint8
{
    MouseLike           UMETA(DisplayName = "Mouse-Like Camera Control"),
    JoystickEmu         UMETA(DisplayName = "Joystick Emulation"),
    DPadEmulation       UMETA(DisplayName = "D-Pad Emulation"),
    RadialMenu          UMETA(DisplayName = "Radial Menu Selection"),
    ScrollWheel         UMETA(DisplayName = "Scroll Wheel"),
    FlickStick          UMETA(DisplayName = "Flick Stick Aiming"),
    Disabled            UMETA(DisplayName = "Disabled")
};

// ============================================================================
// USteamDeckTrackpadProcessor
// Processes trackpad input based on configured modes
// ============================================================================
UCLASS(BlueprintType)
class STEAMDECKINPUT_API USteamDeckTrackpadProcessor : public UObject
{
    GENERATED_BODY()

public:
    // ========================================================================
    // Processing
    // ========================================================================

    // Process left trackpad input (typically D-pad or radial menu)
    UFUNCTION(BlueprintCallable, Category = "SteamDeck|Trackpad")
    FVector2D ProcessLeftTrackpad(FVector2D RawInput, bool bIsTouching, float DeltaTime);

    // Process right trackpad input (typically mouse-like camera)
    UFUNCTION(BlueprintCallable, Category = "SteamDeck|Trackpad")
    FVector2D ProcessRightTrackpad(FVector2D RawInput, bool bIsTouching, float DeltaTime);

    // ========================================================================
    // Configuration
    // ========================================================================

    UFUNCTION(BlueprintCallable, Category = "SteamDeck|Trackpad")
    void SetLeftTrackpadMode(ETrackpadMode Mode);

    UFUNCTION(BlueprintCallable, Category = "SteamDeck|Trackpad")
    void SetRightTrackpadMode(ETrackpadMode Mode);

    // Get discrete D-pad direction from trackpad position
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SteamDeck|Trackpad")
    FName GetDPadDirection(FVector2D TrackpadPosition) const;

    // ========================================================================
    // Properties
    // ========================================================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trackpad Settings")
    ETrackpadMode LeftMode = ETrackpadMode::DPadEmulation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trackpad Settings")
    ETrackpadMode RightMode = ETrackpadMode::MouseLike;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trackpad Settings", meta = (ClampMin = "0.1", ClampMax = "3.0"))
    float TrackpadSensitivity = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trackpad Settings", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float HapticStrength = 0.5f;

private:
    FVector2D LeftPreviousPosition;
    FVector2D RightPreviousPosition;
    bool bLeftWasTouching = false;
    bool bRightWasTouching = false;

    // Internal mode handlers
    FVector2D ProcessMouseLike(FVector2D RawInput, FVector2D& PreviousPosition, bool bIsTouching, bool& bWasTouching, float DeltaTime);
    FVector2D ProcessDPadEmulation(FVector2D RawInput, bool bIsTouching);
    FVector2D ProcessRadialMenu(FVector2D RawInput, bool bIsTouching);
    FVector2D ProcessJoystickEmu(FVector2D RawInput, bool bIsTouching);
    FVector2D ProcessScrollWheel(FVector2D RawInput, FVector2D& PreviousPosition, bool bIsTouching, bool& bWasTouching, float DeltaTime);
    FVector2D ProcessFlickStick(FVector2D RawInput, FVector2D& PreviousPosition, bool bIsTouching, bool& bWasTouching, float DeltaTime);
};
