// ============================================================================
// SignalTypes.h
// Signal type definitions for World Signal Emitter system
//
// Developer: Marcus Daley
// Date: January 19, 2026
// Project: Game Architecture (END2507)
//
// PURPOSE:
// Defines signal types, trigger conditions, and related enums.
// Kept separate from the emitter for clean includes and modularity.
//
// USAGE:
// Include this header in any file that needs signal type definitions.
// The emitter actor includes this automatically.
//
// EXPANSION:
// To add new signal types, simply add new FName constants to the
// SignalTypeNames namespace. No enum modification required.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "SignalTypes.generated.h"

// ============================================================================
// SIGNAL TYPE NAMES (FName-based for designer expansion)
// Using FName instead of enum allows designers to create new signal types
// without modifying C++ code. Just use any FName string.
// ============================================================================

namespace SignalTypeNames
{
    // Quidditch/Match signals
    const FName QuidditchMatchStart = TEXT("Signal.Quidditch.MatchStart");
    const FName QuidditchMatchEnd = TEXT("Signal.Quidditch.MatchEnd");
    const FName QuidditchGoalScored = TEXT("Signal.Quidditch.GoalScored");
    
    // Arena/Combat signals
    const FName ArenaMatchStart = TEXT("Signal.Arena.MatchStart");
    const FName ArenaWaveStart = TEXT("Signal.Arena.WaveStart");
    const FName ArenaWaveComplete = TEXT("Signal.Arena.WaveComplete");
    const FName ArenaBossSpawn = TEXT("Signal.Arena.BossSpawn");
    
    // General game signals
    const FName GameStart = TEXT("Signal.Game.Start");
    const FName GamePause = TEXT("Signal.Game.Pause");
    const FName GameResume = TEXT("Signal.Game.Resume");
    
    // Race signals (future use)
    const FName RaceCountdown = TEXT("Signal.Race.Countdown");
    const FName RaceStart = TEXT("Signal.Race.Start");
    const FName RaceFinish = TEXT("Signal.Race.Finish");
}

// ============================================================================
// AI PERCEPTION TAG
// Used to identify signal stimuli in AI perception callbacks
// AI Controllers check for this tag to filter signal-based stimuli
// ============================================================================

namespace SignalPerceptionTags
{
    const FName SignalStimulus = TEXT("SignalStimulus");
}

// ============================================================================
// TRIGGER CONDITIONS
// Defines HOW the signal emitter can be activated
// Designer selects one of these in the Blueprint details panel
// ============================================================================

UENUM(BlueprintType)
enum class ESignalTriggerCondition : uint8
{
    // Manual only - must be activated via code or Blueprint
    Manual              UMETA(DisplayName = "Manual Only"),
    
    // Automatic on BeginPlay
    OnBeginPlay         UMETA(DisplayName = "On Begin Play"),
    
    // Automatic on BeginPlay with delay
    OnBeginPlayDelayed  UMETA(DisplayName = "On Begin Play (Delayed)"),
    
    // When player overlaps trigger volume
    OnPlayerOverlap     UMETA(DisplayName = "On Player Overlap"),
    
    // When player confirms via UI popup
    OnUIConfirm         UMETA(DisplayName = "On UI Confirmation"),
    
    // When specific projectile/damage hits the emitter
    OnProjectileHit     UMETA(DisplayName = "On Projectile Hit"),
    
    // When player acquires specific channel (item collection)
    OnChannelAcquired   UMETA(DisplayName = "On Channel Acquired"),
    
    // When all required players are in position
    OnAllPlayersReady   UMETA(DisplayName = "On All Players Ready")
};

// ============================================================================
// SIGNAL DATA STRUCT
// Passed to receivers when signal is emitted
// Contains all context about the signal event
// ============================================================================

USTRUCT(BlueprintType)
struct END2507_API FSignalData
{
    GENERATED_BODY()

    // Type of signal (e.g., "Signal.Quidditch.MatchStart")
    UPROPERTY(BlueprintReadOnly, Category = "Signal")
    FName SignalType;

    // Actor that emitted the signal
    UPROPERTY(BlueprintReadOnly, Category = "Signal")
    TWeakObjectPtr<AActor> Emitter;

    // World location of the signal source
    UPROPERTY(BlueprintReadOnly, Category = "Signal")
    FVector SignalLocation;

    // World time when signal was emitted
    UPROPERTY(BlueprintReadOnly, Category = "Signal")
    float EmitTime;

    // Optional: Team ID associated with signal (-1 = all teams)
    UPROPERTY(BlueprintReadOnly, Category = "Signal")
    int32 TeamID;

    // Optional: Custom data payload for game-specific needs
    UPROPERTY(BlueprintReadOnly, Category = "Signal")
    FString CustomData;

    // Default constructor
    FSignalData()
        : SignalType(NAME_None)
        , SignalLocation(FVector::ZeroVector)
        , EmitTime(0.0f)
        , TeamID(-1)
    {}

    // Convenience constructor
    FSignalData(FName InType, AActor* InEmitter, FVector InLocation, float InTime)
        : SignalType(InType)
        , Emitter(InEmitter)
        , SignalLocation(InLocation)
        , EmitTime(InTime)
        , TeamID(-1)
    {}
};
