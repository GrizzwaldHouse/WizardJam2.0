// ============================================================================
// SteamDeckGyroProcessor.h
// Developer: Marcus Daley
// Date: February 11, 2026
// Project: SteamDeckInput Plugin
// ============================================================================
// Purpose:
// Processes gyroscope input from the Steam Deck for camera aiming.
// Provides calibration, sensitivity adjustment, smoothing, and activation modes.
//
// Designer Usage:
// 1. Access via USteamDeckInputSubsystem::GetGyroProcessor()
// 2. Call SetSensitivity() to adjust aim speed
// 3. Call SetActivationMode() to change when gyro is active (AlwaysOn, OnADS, etc.)
// 4. Call Calibrate() when player presses a "reset gyro" button
// 5. Call ProcessGyroInput() each frame with raw gyro data
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SteamDeckGyroProcessor.generated.h"

// ============================================================================
// EGyroActivation
// Defines when the gyroscope should be active for aiming
// ============================================================================
UENUM(BlueprintType)
enum class EGyroActivation : uint8
{
    AlwaysOn            UMETA(DisplayName = "Always On"),
    OnADS               UMETA(DisplayName = "Only When Aiming Down Sights"),
    OnTrackpadTouch     UMETA(DisplayName = "When Touching Right Trackpad"),
    OnButtonHold        UMETA(DisplayName = "When Holding Activation Button"),
    Off                 UMETA(DisplayName = "Disabled")
};

// ============================================================================
// USteamDeckGyroProcessor
// Processes and filters gyroscope input for camera control
// ============================================================================
UCLASS(BlueprintType)
class STEAMDECKINPUT_API USteamDeckGyroProcessor : public UObject
{
    GENERATED_BODY()

public:
    // ========================================================================
    // Processing
    // ========================================================================

    // Process raw gyro input and return camera delta
    UFUNCTION(BlueprintCallable, Category = "SteamDeck|Gyro")
    FVector2D ProcessGyroInput(FVector GyroRaw, float DeltaTime);

    // ========================================================================
    // Configuration
    // ========================================================================

    UFUNCTION(BlueprintCallable, Category = "SteamDeck|Gyro")
    void SetActivationMode(EGyroActivation Mode);

    UFUNCTION(BlueprintCallable, Category = "SteamDeck|Gyro")
    void SetSensitivity(float NewSensitivity);

    UFUNCTION(BlueprintCallable, Category = "SteamDeck|Gyro")
    void SetSmoothing(float NewSmoothing);

    UFUNCTION(BlueprintCallable, Category = "SteamDeck|Gyro")
    void SetActivationState(bool bIsActive);

    // Resets calibration offset
    UFUNCTION(BlueprintCallable, Category = "SteamDeck|Gyro")
    void Calibrate();

    // ========================================================================
    // Properties
    // ========================================================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gyro Settings", meta = (ClampMin = "0.1", ClampMax = "3.0"))
    float Sensitivity = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gyro Settings", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Smoothing = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gyro Settings", meta = (ClampMin = "0.0", ClampMax = "0.1"))
    float DeadZone = 0.01f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gyro Settings")
    EGyroActivation ActivationMode = EGyroActivation::OnTrackpadTouch;

private:
    FVector2D PreviousOutput;
    FVector CalibrationOffset;
    bool bIsCurrentlyActive = false;
};
