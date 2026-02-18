// ============================================================================
// SteamDeckInputSubsystem.cpp
// Developer: Marcus Daley
// Date: February 11, 2026
// Project: SteamDeckInput Plugin
// ============================================================================
// Purpose:
// Implementation of the Steam Deck input subsystem.
// Handles hardware detection, IMC application, and processor lifecycle.
// ============================================================================

#include "SteamDeckInputSubsystem.h"
#include "SteamDeckInputModule.h"
#include "SteamDeckGyroProcessor.h"
#include "SteamDeckTrackpadProcessor.h"
#include "SteamDeckInputSettings.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CoreDelegates.h"
#include "HAL/PlatformMisc.h"

// ============================================================================
// Initialization
// ============================================================================

void USteamDeckInputSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Create processor instances
    GyroProcessor = NewObject<USteamDeckGyroProcessor>(this);
    TrackpadProcessor = NewObject<USteamDeckTrackpadProcessor>(this);

    // Auto-detect input mode if settings allow
    const USteamDeckInputSettings* Settings = GetDefault<USteamDeckInputSettings>();
    if (Settings && Settings->bAutoDetectSteamDeck)
    {
        CurrentInputMode = DetectInputMode();
        UE_LOG(LogSteamDeckInput, Log, TEXT("Auto-detected input mode: %d"), static_cast<int32>(CurrentInputMode));
    }
    else
    {
        CurrentInputMode = ESteamDeckInputMode::Desktop;
    }
}

void USteamDeckInputSubsystem::Deinitialize()
{
    GyroProcessor = nullptr;
    TrackpadProcessor = nullptr;

    Super::Deinitialize();
}

// ============================================================================
// IISteamDeckInputProvider Interface Implementation
// ============================================================================

ESteamDeckInputMode USteamDeckInputSubsystem::DetectInputMode() const
{
    if (DetectSteamDeckHardware())
    {
        return ESteamDeckInputMode::SteamDeck;
    }

    return ESteamDeckInputMode::Desktop;
}

void USteamDeckInputSubsystem::SetInputMode(ESteamDeckInputMode NewMode)
{
    if (CurrentInputMode != NewMode)
    {
        CurrentInputMode = NewMode;
        OnInputModeChanged.Broadcast(NewMode);
        UE_LOG(LogSteamDeckInput, Log, TEXT("Input mode changed to: %d"), static_cast<int32>(NewMode));
    }
}

ESteamDeckInputMode USteamDeckInputSubsystem::GetCurrentInputMode() const
{
    return CurrentInputMode;
}

bool USteamDeckInputSubsystem::IsGyroAvailable() const
{
    return CurrentInputMode == ESteamDeckInputMode::SteamDeck;
}

bool USteamDeckInputSubsystem::AreTrackpadsAvailable() const
{
    return CurrentInputMode == ESteamDeckInputMode::SteamDeck;
}

// ============================================================================
// Input Mapping Context Management
// ============================================================================

void USteamDeckInputSubsystem::ApplySteamDeckMappings(APlayerController* PC, int32 Priority)
{
    if (!PC)
    {
        UE_LOG(LogSteamDeckInput, Warning, TEXT("ApplySteamDeckMappings called with null PlayerController"));
        return;
    }

    if (bMappingsApplied)
    {
        UE_LOG(LogSteamDeckInput, Log, TEXT("Steam Deck mappings already applied, skipping"));
        return;
    }

    UEnhancedInputLocalPlayerSubsystem* InputSubsystem = GetInputSubsystem(PC);
    if (!InputSubsystem)
    {
        UE_LOG(LogSteamDeckInput, Warning, TEXT("Failed to get Enhanced Input subsystem for PlayerController"));
        return;
    }

    // Load and apply the default Steam Deck IMC
    if (!SteamDeckDefaultIMC.IsNull())
    {
        UInputMappingContext* IMC = SteamDeckDefaultIMC.LoadSynchronous();
        if (IMC)
        {
            InputSubsystem->AddMappingContext(IMC, Priority);
            UE_LOG(LogSteamDeckInput, Log, TEXT("Applied Steam Deck default IMC at priority %d"), Priority);
        }
        else
        {
            UE_LOG(LogSteamDeckInput, Warning, TEXT("Failed to load Steam Deck default IMC"));
        }
    }

    bMappingsApplied = true;
}

void USteamDeckInputSubsystem::RemoveSteamDeckMappings(APlayerController* PC)
{
    if (!PC)
    {
        UE_LOG(LogSteamDeckInput, Warning, TEXT("RemoveSteamDeckMappings called with null PlayerController"));
        return;
    }

    if (!bMappingsApplied)
    {
        return;
    }

    UEnhancedInputLocalPlayerSubsystem* InputSubsystem = GetInputSubsystem(PC);
    if (!InputSubsystem)
    {
        return;
    }

    // Remove all tracked context layers first
    for (const auto& Pair : TrackedContextLayers)
    {
        if (Pair.Key)
        {
            InputSubsystem->RemoveMappingContext(Pair.Key);
            UE_LOG(LogSteamDeckInput, Log, TEXT("Removed tracked context layer: %s"), *Pair.Key->GetName());
        }
    }
    TrackedContextLayers.Empty();

    // Remove the default Steam Deck IMC
    if (!SteamDeckDefaultIMC.IsNull())
    {
        UInputMappingContext* IMC = SteamDeckDefaultIMC.LoadSynchronous();
        if (IMC)
        {
            InputSubsystem->RemoveMappingContext(IMC);
            UE_LOG(LogSteamDeckInput, Log, TEXT("Removed Steam Deck default IMC"));
        }
    }

    bMappingsApplied = false;
}

void USteamDeckInputSubsystem::PushContextLayer(APlayerController* PC, UInputMappingContext* Context, int32 Priority)
{
    if (!PC || !Context)
    {
        UE_LOG(LogSteamDeckInput, Warning, TEXT("PushContextLayer called with null parameter"));
        return;
    }

    UEnhancedInputLocalPlayerSubsystem* InputSubsystem = GetInputSubsystem(PC);
    if (!InputSubsystem)
    {
        return;
    }

    // Remove existing entry if this context was already pushed
    if (TrackedContextLayers.Contains(Context))
    {
        InputSubsystem->RemoveMappingContext(Context);
    }

    InputSubsystem->AddMappingContext(Context, Priority);
    TrackedContextLayers.Add(Context, Priority);
    UE_LOG(LogSteamDeckInput, Log, TEXT("Pushed context layer: %s at priority %d"), *Context->GetName(), Priority);
}

void USteamDeckInputSubsystem::PopContextLayer(APlayerController* PC, UInputMappingContext* Context)
{
    if (!PC || !Context)
    {
        UE_LOG(LogSteamDeckInput, Warning, TEXT("PopContextLayer called with null parameter"));
        return;
    }

    UEnhancedInputLocalPlayerSubsystem* InputSubsystem = GetInputSubsystem(PC);
    if (!InputSubsystem)
    {
        return;
    }

    InputSubsystem->RemoveMappingContext(Context);
    TrackedContextLayers.Remove(Context);
    UE_LOG(LogSteamDeckInput, Log, TEXT("Popped context layer: %s"), *Context->GetName());
}

// ============================================================================
// Private Helper Methods
// ============================================================================

bool USteamDeckInputSubsystem::DetectSteamDeckHardware() const
{
    // Check CPU brand for AMD Custom APU 0405 (Steam Deck's APU)
    FString CPUBrand = FPlatformMisc::GetCPUBrand();
    if (CPUBrand.Contains(TEXT("AMD Custom APU 0405")))
    {
        return true;
    }

    // Check environment variable as fallback
    FString SteamDeckEnvVar = FPlatformMisc::GetEnvironmentVariable(TEXT("SteamDeck"));
    if (SteamDeckEnvVar == TEXT("1"))
    {
        return true;
    }

    return false;
}

UEnhancedInputLocalPlayerSubsystem* USteamDeckInputSubsystem::GetInputSubsystem(APlayerController* PC) const
{
    if (!PC)
    {
        return nullptr;
    }

    ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
    if (!LocalPlayer)
    {
        UE_LOG(LogSteamDeckInput, Warning, TEXT("PlayerController has no LocalPlayer"));
        return nullptr;
    }

    return LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
}
