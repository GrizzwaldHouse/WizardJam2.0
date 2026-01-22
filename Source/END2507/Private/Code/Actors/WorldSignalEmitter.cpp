// ============================================================================
// WorldSignalEmitter.cpp
// Implementation of modular signal emitter with AI perception integration
//
// Developer: Marcus Daley
// Date: January 19, 2026
// Project: Game Architecture (END2507)
// ============================================================================

#include "Code/Actors/WorldSignalEmitter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogSignalEmitter);

// Initialize static delegate
FOnAnySignalEmittedGlobal AWorldSignalEmitter::OnAnySignalEmittedGlobal;

// ============================================================================
// CONSTRUCTOR
// ============================================================================

AWorldSignalEmitter::AWorldSignalEmitter()
    : SignalType(SignalTypeNames::ArenaMatchStart)
    , TriggerCondition(ESignalTriggerCondition::Manual)
    , bCanEmitMultipleTimes(false)
    , EmissionCooldown(5.0f)
    , SignalTeamID(-1)
    , RequiredReadyPlayerCount(1)
    , SignalLoudness(1.0f)
    , MaxHearingRange(0.0f)
    , PerceptionTag(SignalPerceptionTags::SignalStimulus)
    , BeginPlayDelay(3.0f)
    , bHasEmitted(false)
    , bPendingConfirmation(false)
    , bOnCooldown(false)
{
    PrimaryActorTick.bCanEverTick = false;

    // Create root component
    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    SetRootComponent(RootSceneComponent);

    // Create trigger volume for overlap detection
    TriggerVolume = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerVolume"));
    TriggerVolume->SetupAttachment(RootSceneComponent);
    TriggerVolume->SetSphereRadius(200.0f);
    TriggerVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    TriggerVolume->SetGenerateOverlapEvents(true);

    // Create visual mesh (optional, can be set to invisible)
    EmitterMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EmitterMesh"));
    EmitterMesh->SetupAttachment(RootSceneComponent);
    EmitterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Create audio component for signal sound
    SignalAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("SignalAudio"));
    SignalAudio->SetupAttachment(RootSceneComponent);
    SignalAudio->bAutoActivate = false;

    // Default confirmation text
    ConfirmationPromptText = FText::FromString(TEXT("Ready to begin?"));
}

// ============================================================================
// BEGIN PLAY
// ============================================================================

void AWorldSignalEmitter::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogSignalEmitter, Display,
        TEXT("[%s] WorldSignalEmitter initialized - Type: %s, TriggerCondition: %d"),
        *GetName(),
        *SignalType.ToString(),
        static_cast<int32>(TriggerCondition));

    // Bind overlap event if using overlap trigger
    if (TriggerCondition == ESignalTriggerCondition::OnPlayerOverlap)
    {
        TriggerVolume->OnComponentBeginOverlap.AddDynamic(
            this, &AWorldSignalEmitter::OnTriggerOverlap);
    }

    // Handle automatic trigger conditions
    switch (TriggerCondition)
    {
    case ESignalTriggerCondition::OnBeginPlay:
        EmitSignal();
        break;

    case ESignalTriggerCondition::OnBeginPlayDelayed:
        GetWorld()->GetTimerManager().SetTimer(
            DelayTimerHandle,
            this,
            &AWorldSignalEmitter::ExecuteEmission,
            BeginPlayDelay,
            false
        );
        UE_LOG(LogSignalEmitter, Log,
            TEXT("[%s] Signal will emit in %.1f seconds"),
            *GetName(), BeginPlayDelay);
        break;

    default:
        // Manual, overlap, channel, and other modes wait for external trigger
        break;
    }
}

// ============================================================================
// PUBLIC API
// ============================================================================

bool AWorldSignalEmitter::EmitSignal()
{
    if (!CanEmit())
    {
        UE_LOG(LogSignalEmitter, Warning,
            TEXT("[%s] EmitSignal blocked - CanEmit() returned false"),
            *GetName());
        return false;
    }

    // For UI confirmation mode, show popup first
    if (TriggerCondition == ESignalTriggerCondition::OnUIConfirm && !bPendingConfirmation)
    {
        RequestUIConfirmation();
        return false; // Signal will emit after UI confirms
    }

    ExecuteEmission();
    return true;
}

bool AWorldSignalEmitter::CanEmit() const
{
    // Check if already emitted (and not allowed to emit again)
    if (bHasEmitted && !bCanEmitMultipleTimes)
    {
        UE_LOG(LogSignalEmitter, Verbose,
            TEXT("[%s] Cannot emit - already emitted and bCanEmitMultipleTimes is false"),
            *GetName());
        return false;
    }

    // Check cooldown
    if (bOnCooldown)
    {
        UE_LOG(LogSignalEmitter, Verbose,
            TEXT("[%s] Cannot emit - on cooldown"),
            *GetName());
        return false;
    }

    // Check channel requirements
    if (!AreRequiredChannelsMet())
    {
        UE_LOG(LogSignalEmitter, Verbose,
            TEXT("[%s] Cannot emit - required channels not met"),
            *GetName());
        return false;
    }

    // Check player ready requirements (for OnAllPlayersReady mode)
    if (TriggerCondition == ESignalTriggerCondition::OnAllPlayersReady)
    {
        if (!AreRequiredPlayersReady())
        {
            UE_LOG(LogSignalEmitter, Verbose,
                TEXT("[%s] Cannot emit - not all players ready"),
                *GetName());
            return false;
        }
    }

    return true;
}

void AWorldSignalEmitter::NotifyChannelAcquired(AActor* AcquiringActor, FName ChannelName)
{
    // Add to acquired channels
    AcquiredChannels.Add(ChannelName);

    UE_LOG(LogSignalEmitter, Log,
        TEXT("[%s] Channel acquired: %s (by %s)"),
        *GetName(),
        *ChannelName.ToString(),
        *AcquiringActor->GetName());

    // Broadcast channel met event
    OnChannelRequirementMet.Broadcast(ChannelName);

    // Check if this was the last required channel
    if (TriggerCondition == ESignalTriggerCondition::OnChannelAcquired)
    {
        if (AreRequiredChannelsMet())
        {
            UE_LOG(LogSignalEmitter, Log,
                TEXT("[%s] All required channels acquired - emitting signal"),
                *GetName());
            EmitSignal();
        }
    }
}

void AWorldSignalEmitter::ConfirmPendingSignal()
{
    if (bPendingConfirmation)
    {
        bPendingConfirmation = false;
        ExecuteEmission();
    }
}

void AWorldSignalEmitter::CancelPendingSignal()
{
    if (bPendingConfirmation)
    {
        bPendingConfirmation = false;
        UE_LOG(LogSignalEmitter, Log,
            TEXT("[%s] Signal cancelled by user"),
            *GetName());
    }
}

void AWorldSignalEmitter::MarkActorReady(AActor* ReadyActor)
{
    if (!ReadyActor)
    {
        return;
    }

    // Add to ready list (using weak pointer for safety)
    ReadyPlayers.Add(ReadyActor);

    UE_LOG(LogSignalEmitter, Log,
        TEXT("[%s] Actor marked ready: %s (%d/%d ready)"),
        *GetName(),
        *ReadyActor->GetName(),
        ReadyPlayers.Num(),
        RequiredReadyPlayerCount);

    // Check if all players are now ready
    if (TriggerCondition == ESignalTriggerCondition::OnAllPlayersReady)
    {
        if (AreRequiredPlayersReady())
        {
            UE_LOG(LogSignalEmitter, Log,
                TEXT("[%s] All players ready - emitting signal"),
                *GetName());
            EmitSignal();
        }
    }
}

// ============================================================================
// INTERNAL FUNCTIONS
// ============================================================================

void AWorldSignalEmitter::ExecuteEmission()
{
    // Mark as emitted
    bHasEmitted = true;

    // Create signal data
    FSignalData SignalData;
    SignalData.SignalType = SignalType;
    SignalData.Emitter = this;
    SignalData.SignalLocation = GetActorLocation();
    SignalData.EmitTime = GetWorld()->GetTimeSeconds();
    SignalData.TeamID = SignalTeamID;

    UE_LOG(LogSignalEmitter, Display,
        TEXT("========================================"));
    UE_LOG(LogSignalEmitter, Display,
        TEXT("[%s] SIGNAL EMITTED: %s"),
        *GetName(),
        *SignalType.ToString());
    UE_LOG(LogSignalEmitter, Display,
        TEXT("  Location: %s"),
        *SignalData.SignalLocation.ToString());
    UE_LOG(LogSignalEmitter, Display,
        TEXT("  Time: %.2f"),
        SignalData.EmitTime);
    UE_LOG(LogSignalEmitter, Display,
        TEXT("========================================"));

    // Broadcast to AI perception system
    BroadcastToAIPerception();

    // Broadcast instance delegate (specific listeners)
    OnSignalEmitted.Broadcast(SignalData);

    // Broadcast global static delegate (all listeners)
    OnAnySignalEmittedGlobal.Broadcast(SignalData);

    // Play feedback effects
    PlayFeedbackEffects();

    // Start cooldown if repeatable
    if (bCanEmitMultipleTimes && EmissionCooldown > 0.0f)
    {
        bOnCooldown = true;
        GetWorld()->GetTimerManager().SetTimer(
            CooldownTimerHandle,
            this,
            &AWorldSignalEmitter::OnCooldownComplete,
            EmissionCooldown,
            false
        );
    }
}

void AWorldSignalEmitter::BroadcastToAIPerception()
{
    // Report noise event to AI perception system
    // All AI controllers with hearing sense configured will receive this
    UAISense_Hearing::ReportNoiseEvent(
        GetWorld(),
        GetActorLocation(),
        SignalLoudness,
        this,                    // Instigator (this emitter)
        MaxHearingRange,         // 0 = infinite range
        PerceptionTag            // Tag for filtering in AI controller
    );

    UE_LOG(LogSignalEmitter, Log,
        TEXT("[%s] AI Perception noise reported - Loudness: %.1f, Range: %.0f, Tag: %s"),
        *GetName(),
        SignalLoudness,
        MaxHearingRange,
        *PerceptionTag.ToString());
}

void AWorldSignalEmitter::PlayFeedbackEffects()
{
    // Play sound
    if (SignalSound)
    {
        SignalAudio->SetSound(SignalSound);
        SignalAudio->Play();
    }

    // Spawn particle effect
    if (SignalParticle)
    {
        UGameplayStatics::SpawnEmitterAtLocation(
            GetWorld(),
            SignalParticle,
            GetActorLocation(),
            FRotator::ZeroRotator,
            true
        );
    }
}

bool AWorldSignalEmitter::AreRequiredChannelsMet() const
{
    // If no channels required, always met
    if (RequiredChannels.Num() == 0)
    {
        return true;
    }

    // Check if all required channels are acquired
    for (const FName& RequiredChannel : RequiredChannels)
    {
        if (!AcquiredChannels.Contains(RequiredChannel))
        {
            return false;
        }
    }

    return true;
}

bool AWorldSignalEmitter::AreRequiredPlayersReady() const
{
    // Clean up invalid weak pointers and count valid ones
    int32 ValidReadyCount = 0;
    for (const TWeakObjectPtr<AActor>& Player : ReadyPlayers)
    {
        if (Player.IsValid())
        {
            ValidReadyCount++;
        }
    }

    return ValidReadyCount >= RequiredReadyPlayerCount;
}

void AWorldSignalEmitter::OnCooldownComplete()
{
    bOnCooldown = false;

    UE_LOG(LogSignalEmitter, Log,
        TEXT("[%s] Signal cooldown complete - ready to emit again"),
        *GetName());
}

void AWorldSignalEmitter::RequestUIConfirmation()
{
    bPendingConfirmation = true;

    // Broadcast for UI to show confirmation popup
    OnPendingConfirmation.Broadcast(this, SignalType);

    UE_LOG(LogSignalEmitter, Log,
        TEXT("[%s] Awaiting UI confirmation for signal: %s"),
        *GetName(),
        *SignalType.ToString());
}

void AWorldSignalEmitter::OnTriggerOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    // Only trigger on player overlap
    APawn* Pawn = Cast<APawn>(OtherActor);
    if (!Pawn || !Pawn->IsPlayerControlled())
    {
        return;
    }

    UE_LOG(LogSignalEmitter, Log,
        TEXT("[%s] Player overlap detected: %s"),
        *GetName(),
        *OtherActor->GetName());

    // Try to emit (will check requirements)
    EmitSignal();
}
