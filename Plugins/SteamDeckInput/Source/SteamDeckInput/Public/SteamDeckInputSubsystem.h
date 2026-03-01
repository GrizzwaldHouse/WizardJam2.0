// ============================================================================
// SteamDeckInputSubsystem.h
// Developer: Marcus Daley
// Date: February 11, 2026
// Project: SteamDeckInput Plugin
// ============================================================================
// Purpose:
// Game instance subsystem that manages Steam Deck input mappings and mode detection.
// Provides Blueprint-accessible methods to apply/remove IMCs, push/pop context layers,
// and access gyro and trackpad processors. Implements ISteamDeckInputProvider.
//
// Designer Usage:
// 1. Access via UGameInstance::GetSubsystem<USteamDeckInputSubsystem>()
// 2. Call ApplySteamDeckMappings() on BeginPlay to auto-detect and apply
// 3. Use PushContextLayer() to add menu/vehicle/flight layers
// 4. Use PopContextLayer() to remove layers when exiting those modes
// 5. Access GetGyroProcessor() and GetTrackpadProcessor() for runtime tuning
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ISteamDeckInputProvider.h"
#include "SteamDeckInputSubsystem.generated.h"

// Forward declarations
class UInputMappingContext;
class APlayerController;
class USteamDeckGyroProcessor;
class USteamDeckTrackpadProcessor;
class UEnhancedInputLocalPlayerSubsystem;

// ============================================================================
// FOnInputModeChanged
// Delegate broadcast when input mode changes
// ============================================================================
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInputModeChanged, ESteamDeckInputMode, NewMode);

// ============================================================================
// USteamDeckInputSubsystem
// Core subsystem for Steam Deck input management
// ============================================================================
UCLASS()
class STEAMDECKINPUT_API USteamDeckInputSubsystem : public UGameInstanceSubsystem, public IISteamDeckInputProvider
{
    GENERATED_BODY()

public:
    // USubsystem interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ========================================================================
    // IISteamDeckInputProvider Interface Implementation
    // ========================================================================
    virtual ESteamDeckInputMode DetectInputMode() const override;
    virtual void SetInputMode(ESteamDeckInputMode NewMode) override;
    virtual ESteamDeckInputMode GetCurrentInputMode() const override;
    virtual bool IsGyroAvailable() const override;
    virtual bool AreTrackpadsAvailable() const override;

    // ========================================================================
    // Blueprint API - Input Mapping Context Management
    // ========================================================================

    // Applies Steam Deck-specific input mappings to the player controller
    UFUNCTION(BlueprintCallable, Category = "SteamDeck|Input")
    void ApplySteamDeckMappings(APlayerController* PC, int32 Priority = 0);

    // Removes Steam Deck input mappings from the player controller
    UFUNCTION(BlueprintCallable, Category = "SteamDeck|Input")
    void RemoveSteamDeckMappings(APlayerController* PC);

    // Pushes an additional context layer (menu, vehicle, flight, etc.)
    UFUNCTION(BlueprintCallable, Category = "SteamDeck|Input")
    void PushContextLayer(APlayerController* PC, UInputMappingContext* Context, int32 Priority);

    // Pops a context layer
    UFUNCTION(BlueprintCallable, Category = "SteamDeck|Input")
    void PopContextLayer(APlayerController* PC, UInputMappingContext* Context);

    // ========================================================================
    // Blueprint API - Processor Access
    // ========================================================================

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SteamDeck|Gyro")
    USteamDeckGyroProcessor* GetGyroProcessor() const { return GyroProcessor; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SteamDeck|Trackpad")
    USteamDeckTrackpadProcessor* GetTrackpadProcessor() const { return TrackpadProcessor; }

    // ========================================================================
    // Events
    // ========================================================================

    UPROPERTY(BlueprintAssignable, Category = "SteamDeck|Events")
    FOnInputModeChanged OnInputModeChanged;

protected:
    // ========================================================================
    // Input Mapping Context References (soft references for lazy loading)
    // ========================================================================

    UPROPERTY(EditDefaultsOnly, Category = "Input Mapping Contexts")
    TSoftObjectPtr<UInputMappingContext> SteamDeckDefaultIMC;

    UPROPERTY(EditDefaultsOnly, Category = "Input Mapping Contexts")
    TSoftObjectPtr<UInputMappingContext> SteamDeckFlightIMC;

    UPROPERTY(EditDefaultsOnly, Category = "Input Mapping Contexts")
    TSoftObjectPtr<UInputMappingContext> SteamDeckMenuIMC;

private:
    // Detects Steam Deck hardware via CPU brand or environment variables
    bool DetectSteamDeckHardware() const;

    // Safe accessor for Enhanced Input subsystem
    UEnhancedInputLocalPlayerSubsystem* GetInputSubsystem(APlayerController* PC) const;

    // Current input mode
    ESteamDeckInputMode CurrentInputMode;

    // Processor instances
    UPROPERTY()
    USteamDeckGyroProcessor* GyroProcessor;

    UPROPERTY()
    USteamDeckTrackpadProcessor* TrackpadProcessor;

    // Tracks whether Steam Deck mappings are currently active
    bool bMappingsApplied = false;

    // Tracks context layers pushed through this subsystem for cleanup
    // Key: the IMC pointer, Value: the priority it was added at
    TMap<UInputMappingContext*, int32> TrackedContextLayers;
};
