// ============================================================================
// SteamDeckTrackpadProcessor.cpp
// Developer: Marcus Daley
// Date: February 11, 2026
// Project: SteamDeckInput Plugin
// ============================================================================
// Purpose:
// Implementation of trackpad input processing.
// Dispatches to mode-specific handlers and provides D-pad direction detection.
// ============================================================================

#include "SteamDeckTrackpadProcessor.h"
#include "SteamDeckInputModule.h"

// ============================================================================
// Processing Methods
// ============================================================================

FVector2D USteamDeckTrackpadProcessor::ProcessLeftTrackpad(FVector2D RawInput, bool bIsTouching, float DeltaTime)
{
    switch (LeftMode)
    {
        case ETrackpadMode::MouseLike:
            return ProcessMouseLike(RawInput, LeftPreviousPosition, bIsTouching, bLeftWasTouching, DeltaTime);

        case ETrackpadMode::DPadEmulation:
            return ProcessDPadEmulation(RawInput, bIsTouching);

        case ETrackpadMode::RadialMenu:
            return ProcessRadialMenu(RawInput, bIsTouching);

        case ETrackpadMode::JoystickEmu:
            return ProcessJoystickEmu(RawInput, bIsTouching);

        case ETrackpadMode::ScrollWheel:
            return ProcessScrollWheel(RawInput, LeftPreviousPosition, bIsTouching, bLeftWasTouching, DeltaTime);

        case ETrackpadMode::FlickStick:
            return ProcessFlickStick(RawInput, LeftPreviousPosition, bIsTouching, bLeftWasTouching, DeltaTime);

        case ETrackpadMode::Disabled:
        default:
            return FVector2D::ZeroVector;
    }
}

FVector2D USteamDeckTrackpadProcessor::ProcessRightTrackpad(FVector2D RawInput, bool bIsTouching, float DeltaTime)
{
    switch (RightMode)
    {
        case ETrackpadMode::MouseLike:
            return ProcessMouseLike(RawInput, RightPreviousPosition, bIsTouching, bRightWasTouching, DeltaTime);

        case ETrackpadMode::DPadEmulation:
            return ProcessDPadEmulation(RawInput, bIsTouching);

        case ETrackpadMode::RadialMenu:
            return ProcessRadialMenu(RawInput, bIsTouching);

        case ETrackpadMode::JoystickEmu:
            return ProcessJoystickEmu(RawInput, bIsTouching);

        case ETrackpadMode::ScrollWheel:
            return ProcessScrollWheel(RawInput, RightPreviousPosition, bIsTouching, bRightWasTouching, DeltaTime);

        case ETrackpadMode::FlickStick:
            return ProcessFlickStick(RawInput, RightPreviousPosition, bIsTouching, bRightWasTouching, DeltaTime);

        case ETrackpadMode::Disabled:
        default:
            return FVector2D::ZeroVector;
    }
}

// ============================================================================
// Configuration Methods
// ============================================================================

void USteamDeckTrackpadProcessor::SetLeftTrackpadMode(ETrackpadMode Mode)
{
    LeftMode = Mode;
    LeftPreviousPosition = FVector2D::ZeroVector;
    bLeftWasTouching = false;
    UE_LOG(LogSteamDeckInput, Log, TEXT("Left trackpad mode set to: %d"), static_cast<int32>(Mode));
}

void USteamDeckTrackpadProcessor::SetRightTrackpadMode(ETrackpadMode Mode)
{
    RightMode = Mode;
    RightPreviousPosition = FVector2D::ZeroVector;
    bRightWasTouching = false;
    UE_LOG(LogSteamDeckInput, Log, TEXT("Right trackpad mode set to: %d"), static_cast<int32>(Mode));
}

FName USteamDeckTrackpadProcessor::GetDPadDirection(FVector2D TrackpadPosition) const
{
    if (TrackpadPosition.IsNearlyZero())
    {
        return NAME_None;
    }

    // Calculate angle in degrees (-180 to 180)
    float Angle = FMath::Atan2(TrackpadPosition.Y, TrackpadPosition.X) * (180.0f / PI);

    // Convert to 0-360 range
    if (Angle < 0.0f)
    {
        Angle += 360.0f;
    }

    // Map to 4 directions (45 degree deadzone around each axis)
    if (Angle >= 315.0f || Angle < 45.0f)
    {
        return FName(TEXT("Right"));
    }
    else if (Angle >= 45.0f && Angle < 135.0f)
    {
        return FName(TEXT("Up"));
    }
    else if (Angle >= 135.0f && Angle < 225.0f)
    {
        return FName(TEXT("Left"));
    }
    else // 225.0f to 315.0f
    {
        return FName(TEXT("Down"));
    }
}

// ============================================================================
// Internal Mode Handlers
// ============================================================================

FVector2D USteamDeckTrackpadProcessor::ProcessMouseLike(FVector2D RawInput, FVector2D& PreviousPosition, bool bIsTouching, bool& bWasTouching, float DeltaTime)
{
    if (!bIsTouching)
    {
        bWasTouching = false;
        PreviousPosition = FVector2D::ZeroVector;
        return FVector2D::ZeroVector;
    }

    // Skip first touch (no delta available)
    if (!bWasTouching)
    {
        bWasTouching = true;
        PreviousPosition = RawInput;
        return FVector2D::ZeroVector;
    }

    // Calculate delta from previous position
    FVector2D Delta = RawInput - PreviousPosition;
    Delta *= TrackpadSensitivity;

    // Store for next frame
    PreviousPosition = RawInput;
    bWasTouching = true;

    return Delta;
}

FVector2D USteamDeckTrackpadProcessor::ProcessDPadEmulation(FVector2D RawInput, bool bIsTouching)
{
    if (!bIsTouching)
    {
        return FVector2D::ZeroVector;
    }

    // Get discrete direction
    FName Direction = GetDPadDirection(RawInput);

    // Convert to unit vector
    if (Direction == TEXT("Right"))
    {
        return FVector2D(1.0f, 0.0f);
    }
    else if (Direction == TEXT("Left"))
    {
        return FVector2D(-1.0f, 0.0f);
    }
    else if (Direction == TEXT("Up"))
    {
        return FVector2D(0.0f, 1.0f);
    }
    else if (Direction == TEXT("Down"))
    {
        return FVector2D(0.0f, -1.0f);
    }

    return FVector2D::ZeroVector;
}

FVector2D USteamDeckTrackpadProcessor::ProcessRadialMenu(FVector2D RawInput, bool bIsTouching)
{
    if (!bIsTouching)
    {
        return FVector2D::ZeroVector;
    }

    // Calculate angle for radial menu segment selection
    float Angle = FMath::Atan2(RawInput.Y, RawInput.X);

    // Convert to segment index (e.g., 8 segments for radial menu)
    const int32 NumSegments = 8;
    float SegmentAngle = (2.0f * PI) / NumSegments;
    int32 SegmentIndex = FMath::RoundToInt(Angle / SegmentAngle);

    // Wrap to 0-7 range
    if (SegmentIndex < 0)
    {
        SegmentIndex += NumSegments;
    }

    // Return segment index as X component
    return FVector2D(static_cast<float>(SegmentIndex), 0.0f);
}

FVector2D USteamDeckTrackpadProcessor::ProcessJoystickEmu(FVector2D RawInput, bool bIsTouching)
{
    if (!bIsTouching)
    {
        return FVector2D::ZeroVector;
    }

    // Raw input is already -1 to 1 range from Steam Input
    // Apply deadzone (15% center deadzone)
    const float DeadZone = 0.15f;
    float Magnitude = RawInput.Size();

    if (Magnitude < DeadZone)
    {
        return FVector2D::ZeroVector;
    }

    // Rescale outside deadzone to 0-1 range
    float ScaledMagnitude = (Magnitude - DeadZone) / (1.0f - DeadZone);
    ScaledMagnitude = FMath::Min(ScaledMagnitude, 1.0f);

    FVector2D Direction = RawInput.GetSafeNormal();
    return Direction * ScaledMagnitude * TrackpadSensitivity;
}

FVector2D USteamDeckTrackpadProcessor::ProcessScrollWheel(FVector2D RawInput, FVector2D& PreviousPosition, bool bIsTouching, bool& bWasTouching, float DeltaTime)
{
    if (!bIsTouching)
    {
        bWasTouching = false;
        PreviousPosition = FVector2D::ZeroVector;
        return FVector2D::ZeroVector;
    }

    // First touch - no delta yet
    if (!bWasTouching)
    {
        bWasTouching = true;
        PreviousPosition = RawInput;
        return FVector2D::ZeroVector;
    }

    // Only use vertical (Y) delta for scroll
    float ScrollDelta = (RawInput.Y - PreviousPosition.Y) * TrackpadSensitivity;
    PreviousPosition = RawInput;
    bWasTouching = true;

    // Return scroll delta in Y, zero in X
    return FVector2D(0.0f, ScrollDelta);
}

FVector2D USteamDeckTrackpadProcessor::ProcessFlickStick(FVector2D RawInput, FVector2D& PreviousPosition, bool bIsTouching, bool& bWasTouching, float DeltaTime)
{
    if (!bIsTouching)
    {
        bWasTouching = false;
        PreviousPosition = FVector2D::ZeroVector;
        return FVector2D::ZeroVector;
    }

    // First touch - calculate flick direction and snap
    if (!bWasTouching)
    {
        bWasTouching = true;
        PreviousPosition = RawInput;

        // Only flick if finger is far enough from center (avoid tiny accidental flicks)
        float Magnitude = RawInput.Size();
        if (Magnitude > 0.3f)
        {
            // Calculate angle and convert to a large yaw delta for instant camera snap
            float FlickAngle = FMath::Atan2(RawInput.Y, RawInput.X) * (180.0f / PI);
            return FVector2D(FlickAngle * TrackpadSensitivity, 0.0f);
        }

        return FVector2D::ZeroVector;
    }

    // Continued touch - fine mouse-like adjustment
    FVector2D Delta = RawInput - PreviousPosition;
    Delta *= TrackpadSensitivity;
    PreviousPosition = RawInput;
    bWasTouching = true;

    return Delta;
}
