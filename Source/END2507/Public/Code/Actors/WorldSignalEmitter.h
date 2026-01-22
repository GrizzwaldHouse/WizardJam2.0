// ============================================================================
// WorldSignalEmitter.h
// Modular signal emitter for game events with AI perception integration
//
// Developer: Marcus Daley
// Date: January 19, 2026
// Project: Game Architecture (END2507)
//
// PURPOSE:
// Emits signals that can be received by:
// 1. AI agents via UAISense_Hearing (for behavior tree reactions)
// 2. Game systems via delegates (for GameMode, UI, etc.)
// 3. Blueprint event bindings
//
// EXAMPLE USE CASE (Quidditch/Arena):
// Place in arena, configure SignalType as "Signal.Arena.MatchStart"
// When emitted, AI agents hear the signal and enter combat/flight mode
// GameMode receives delegate and starts match countdown
//
// TRIGGER OPTIONS (Designer configurable):
// - Manual: Call EmitSignal() from code or Blueprint
// - OnBeginPlay: Automatic on level start
// - OnPlayerOverlap: When player enters trigger volume
// - OnUIConfirm: After player confirms UI popup
// - OnProjectileHit: When shot by player
// - OnChannelAcquired: When player gets required channel
// - OnAllPlayersReady: When all participants are in position
//
// CHANNEL GATING:
// Can require specific channels before activation
// Example: Require "BroomKey" channel before whistle can blow
//
// DESIGNER WORKFLOW:
// 1. Place BP_WorldSignalEmitter in level
// 2. Set SignalType (e.g., Signal.Arena.MatchStart)
// 3. Set TriggerCondition (how to activate)
// 4. Set RequiredChannels (if any)
// 5. Configure sound/VFX
// 6. AI and GameMode automatically respond
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Code/Utilities/SignalTypes.h"
#include "WorldSignalEmitter.generated.h"

// Forward declarations
class USphereComponent;
class UAudioComponent;
class UNiagaraComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogSignalEmitter, Log, All);

// ============================================================================
// DELEGATES
// ============================================================================

// Broadcast when signal is emitted - GameMode, UI, and other systems bind to this
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnSignalEmitted,
    const FSignalData&, SignalData
);

// Static delegate for global signal listening (any system can subscribe)
DECLARE_MULTICAST_DELEGATE_OneParam(
    FOnAnySignalEmittedGlobal,
    const FSignalData& // SignalData
);

// Broadcast when signal is pending UI confirmation
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnSignalPendingConfirmation,
    AWorldSignalEmitter*, Emitter,
    FName, SignalType
);

// Broadcast when channel requirement is met
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnChannelRequirementMet,
    FName, ChannelName
);

// ============================================================================
// WORLD SIGNAL EMITTER
// ============================================================================

UCLASS()
class END2507_API AWorldSignalEmitter : public AActor
{
    GENERATED_BODY()

public:
    AWorldSignalEmitter();

    // ========================================================================
    // PUBLIC API
    // ========================================================================

    // Manually emit the signal (can be called from code or Blueprint)
    // Returns true if signal was emitted, false if blocked (cooldown, requirements)
    UFUNCTION(BlueprintCallable, Category = "Signal")
    bool EmitSignal();

    // Check if signal can be emitted right now
    UFUNCTION(BlueprintPure, Category = "Signal")
    bool CanEmit() const;

    // Notify that a channel was acquired by an actor
    // Called by collectibles when player picks up channel-granting items
    UFUNCTION(BlueprintCallable, Category = "Signal|Channels")
    void NotifyChannelAcquired(AActor* AcquiringActor, FName ChannelName);

    // For UI confirmation flow
    UFUNCTION(BlueprintCallable, Category = "Signal|UI")
    void ConfirmPendingSignal();

    UFUNCTION(BlueprintCallable, Category = "Signal|UI")
    void CancelPendingSignal();

    // Mark an actor as "ready" (for OnAllPlayersReady mode)
    UFUNCTION(BlueprintCallable, Category = "Signal|Ready")
    void MarkActorReady(AActor* ReadyActor);

    // Get the signal type this emitter broadcasts
    UFUNCTION(BlueprintPure, Category = "Signal")
    FName GetSignalType() const { return SignalType; }

    // ========================================================================
    // DELEGATES
    // ========================================================================

    // Instance delegate - bind to specific emitter
    UPROPERTY(BlueprintAssignable, Category = "Signal|Events")
    FOnSignalEmitted OnSignalEmitted;

    // Static delegate - receive ALL signal emissions globally
    static FOnAnySignalEmittedGlobal OnAnySignalEmittedGlobal;

    // UI confirmation delegate
    UPROPERTY(BlueprintAssignable, Category = "Signal|Events")
    FOnSignalPendingConfirmation OnPendingConfirmation;

    // Channel met delegate
    UPROPERTY(BlueprintAssignable, Category = "Signal|Events")
    FOnChannelRequirementMet OnChannelRequirementMet;

protected:
    virtual void BeginPlay() override;

    // ========================================================================
    // SIGNAL CONFIGURATION
    // ========================================================================

    // Type of signal to emit (use SignalTypeNames constants or custom FName)
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Signal|Configuration")
    FName SignalType;

    // How this signal gets triggered
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Signal|Configuration")
    ESignalTriggerCondition TriggerCondition;

    // Whether this signal can emit multiple times
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Signal|Configuration")
    bool bCanEmitMultipleTimes;

    // Cooldown between emissions (if bCanEmitMultipleTimes is true)
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Signal|Configuration",
        meta = (EditCondition = "bCanEmitMultipleTimes", ClampMin = "0.0"))
    float EmissionCooldown;

    // Optional team ID for team-specific signals (-1 = all teams)
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Signal|Configuration")
    int32 SignalTeamID;

    // ========================================================================
    // CHANNEL GATING
    // ========================================================================

    // Channels required before this signal can emit
    // Example: Require "BroomKey" before match can start
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Signal|Channels")
    TArray<FName> RequiredChannels;

    // Channels that have been acquired
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Signal|Channels")
    TSet<FName> AcquiredChannels;

    // ========================================================================
    // READY CHECK (for OnAllPlayersReady mode)
    // ========================================================================

    // Number of players/actors that must be ready
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Signal|Ready Check",
        meta = (EditCondition = "TriggerCondition == ESignalTriggerCondition::OnAllPlayersReady"))
    int32 RequiredReadyPlayerCount;

    // Actors currently marked as ready
    TArray<TWeakObjectPtr<AActor>> ReadyPlayers;

    // ========================================================================
    // AI PERCEPTION
    // ========================================================================

    // Loudness of the signal for AI perception (higher = heard from farther)
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Signal|AI Perception",
        meta = (ClampMin = "0.0", ClampMax = "10.0"))
    float SignalLoudness;

    // Maximum range AI can hear this signal (0 = infinite)
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Signal|AI Perception",
        meta = (ClampMin = "0.0"))
    float MaxHearingRange;

    // Tag applied to the AI stimulus (for perception filtering)
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Signal|AI Perception")
    FName PerceptionTag;

    // ========================================================================
    // UI CONFIGURATION
    // ========================================================================

    // Text shown in confirmation popup (for OnUIConfirm mode)
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Signal|UI",
        meta = (EditCondition = "TriggerCondition == ESignalTriggerCondition::OnUIConfirm"))
    FText ConfirmationPromptText;

    // ========================================================================
    // VISUAL/AUDIO FEEDBACK
    // ========================================================================

    // Sound to play when signal emits
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Signal|Feedback")
    USoundBase* SignalSound;

    // Particle effect to spawn when signal emits
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Signal|Feedback")
    UParticleSystem* SignalParticle;

    // ========================================================================
    // DELAY CONFIGURATION
    // ========================================================================

    // Delay before emission (for OnBeginPlayDelayed mode)
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Signal|Timing",
        meta = (EditCondition = "TriggerCondition == ESignalTriggerCondition::OnBeginPlayDelayed",
                ClampMin = "0.0"))
    float BeginPlayDelay;

    // ========================================================================
    // COMPONENTS
    // ========================================================================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* RootSceneComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* TriggerVolume;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* EmitterMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UAudioComponent* SignalAudio;

private:
    // ========================================================================
    // INTERNAL STATE
    // ========================================================================

    bool bHasEmitted;
    bool bPendingConfirmation;
    bool bOnCooldown;
    FTimerHandle CooldownTimerHandle;
    FTimerHandle DelayTimerHandle;

    // ========================================================================
    // INTERNAL FUNCTIONS
    // ========================================================================

    void ExecuteEmission();
    void BroadcastToAIPerception();
    void PlayFeedbackEffects();
    bool AreRequiredChannelsMet() const;
    bool AreRequiredPlayersReady() const;
    void OnCooldownComplete();
    void RequestUIConfirmation();

    // Overlap callback for OnPlayerOverlap mode
    UFUNCTION()
    void OnTriggerOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult
    );
};
