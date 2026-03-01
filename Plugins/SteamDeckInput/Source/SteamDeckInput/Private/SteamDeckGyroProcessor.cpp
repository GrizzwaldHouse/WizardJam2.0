// ============================================================================
// SteamDeckGyroProcessor.cpp
// Developer: Marcus Daley
// Date: February 11, 2026
// Project: SteamDeckInput Plugin
// ============================================================================
// Purpose:
// Implementation of gyroscope input processing.
// Applies calibration, deadzone filtering, sensitivity scaling, and smoothing.
// ============================================================================

#include "SteamDeckGyroProcessor.h"
#include "SteamDeckInputModule.h"

FVector2D USteamDeckGyroProcessor::ProcessGyroInput(FVector GyroRaw, float DeltaTime)
{
    // If gyro is not active, return zero
    if (!bIsCurrentlyActive)
    {
        return FVector2D::ZeroVector;
    }

    // Apply calibration offset
    FVector Calibrated = GyroRaw - CalibrationOffset;

    // Apply deadzone filter
    if (Calibrated.Size() < DeadZone)
    {
        return FVector2D::ZeroVector;
    }

    // Convert to 2D camera delta (pitch and yaw)
    FVector2D CameraDelta;
    CameraDelta.X = Calibrated.Y; // Yaw (left/right rotation)
    CameraDelta.Y = Calibrated.X; // Pitch (up/down rotation)

    // Apply sensitivity
    CameraDelta *= Sensitivity;

    // Apply smoothing using exponential moving average
    if (Smoothing > 0.0f)
    {
        CameraDelta = FMath::Lerp(PreviousOutput, CameraDelta, 1.0f - Smoothing);
    }

    // Store for next frame
    PreviousOutput = CameraDelta;

    return CameraDelta;
}

void USteamDeckGyroProcessor::SetActivationMode(EGyroActivation Mode)
{
    ActivationMode = Mode;
    UE_LOG(LogSteamDeckInput, Log, TEXT("Gyro activation mode set to: %d"), static_cast<int32>(Mode));
}

void USteamDeckGyroProcessor::SetSensitivity(float NewSensitivity)
{
    Sensitivity = FMath::Clamp(NewSensitivity, 0.1f, 3.0f);
}

void USteamDeckGyroProcessor::SetSmoothing(float NewSmoothing)
{
    Smoothing = FMath::Clamp(NewSmoothing, 0.0f, 1.0f);
}

void USteamDeckGyroProcessor::SetActivationState(bool bIsActive)
{
    bIsCurrentlyActive = bIsActive;

    // Reset smoothing state when turning off
    if (!bIsActive)
    {
        PreviousOutput = FVector2D::ZeroVector;
    }
}

void USteamDeckGyroProcessor::Calibrate()
{
    // Reset calibration offset
    // Note: Full implementation would average N frames of gyro input
    CalibrationOffset = FVector::ZeroVector;
    PreviousOutput = FVector2D::ZeroVector;
    UE_LOG(LogSteamDeckInput, Log, TEXT("Gyro calibration reset"));
}
