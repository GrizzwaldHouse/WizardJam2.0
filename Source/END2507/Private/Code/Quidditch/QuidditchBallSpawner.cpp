// ============================================================================
// QuidditchBallSpawner.cpp
// Spawns Quidditch balls when match starts (responds to signals)
//
// Developer: Marcus Daley
// Date: January 25, 2026
// Project: WizardJam (END2507)
//
// PURPOSE:
// Listens to WorldSignalEmitter for match start, then spawns balls
// Designer controls how many, when, and auto-cleanup
// ============================================================================

#include "Code/Quidditch/QuidditchBallSpawner.h"
#include "Code/Actors/WorldSignalEmitter.h"
#include "Code/Utilities/SignalTypes.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"

AQuidditchBallSpawner::AQuidditchBallSpawner()
    : BallType(EQuidditchBallType::Snitch)
    , SpawnDelayAfterMatchStart(3.0f)
    , MatchStartEmitter(nullptr)
    , MatchStartSignalType(TEXT("Signal.Quidditch.MatchStart"))
    , bAutoCleanupOnMatchEnd(true)
    , MatchEndSignalType(TEXT("Signal.Quidditch.MatchEnd"))
    , CurrentSpawnCount(0)
{
    // Use parent class properties for spawning control
    MaxSpawnCount = 1;        // Default: 1 ball (Snitch)
    SpawnInterval = 2.0f;     // 2 sec between spawns if multiple
    SpawnRadius = 200.0f;     // Random offset radius
    bInfiniteSpawn = false;   // Finite spawn count
}

void AQuidditchBallSpawner::BeginPlay()
{
    Super::BeginPlay();

    if (!MatchStartEmitter)
    {
        FindMatchStartEmitter();
    }

    if (MatchStartEmitter)
    {
        MatchStartEmitter->OnSignalEmitted.AddDynamic(
            this, 
            &AQuidditchBallSpawner::OnMatchStartSignalReceived
        );
    }

    if (bAutoCleanupOnMatchEnd)
    {
        AWorldSignalEmitter::OnAnySignalEmittedGlobal.AddUObject(
            this,
            &AQuidditchBallSpawner::OnMatchEndSignalReceived
        );
    }
}

void AQuidditchBallSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Unbind from match start emitter delegate
    if (MatchStartEmitter)
    {
        MatchStartEmitter->OnSignalEmitted.RemoveDynamic(
            this,
            &AQuidditchBallSpawner::OnMatchStartSignalReceived
        );
    }

    // Unbind from global signal delegate
    if (bAutoCleanupOnMatchEnd)
    {
        AWorldSignalEmitter::OnAnySignalEmittedGlobal.RemoveAll(this);
    }

    Super::EndPlay(EndPlayReason);
}

void AQuidditchBallSpawner::FindMatchStartEmitter()
{
    for (TActorIterator<AWorldSignalEmitter> It(GetWorld()); It; ++It)
    {
        AWorldSignalEmitter* Emitter = *It;
        
        if (Emitter->GetSignalType() == MatchStartSignalType)
        {
            MatchStartEmitter = Emitter;
            break;
        }
    }
}

void AQuidditchBallSpawner::OnMatchStartSignalReceived(const FSignalData& SignalData)
{
    if (SignalData.SignalType != MatchStartSignalType)
    {
        return;
    }

    GetWorld()->GetTimerManager().SetTimer(
        SpawnDelayTimer,
        this,
        &AQuidditchBallSpawner::BeginBallSpawning,
        SpawnDelayAfterMatchStart,
        false
    );
}

void AQuidditchBallSpawner::OnMatchEndSignalReceived(const FSignalData& SignalData)
{
    if (SignalData.SignalType != MatchEndSignalType)
    {
        return;
    }

    CleanupSpawnedBalls();
}

void AQuidditchBallSpawner::BeginBallSpawning()
{
    CurrentSpawnCount = 0;

    if (MaxSpawnCount == 1)
    {
        SpawnBall();
    }
    else
    {
        SpawnBall();
        
        if (MaxSpawnCount > 1)
        {
            GetWorld()->GetTimerManager().SetTimer(
                SpawnIntervalTimer,
                this,
                &AQuidditchBallSpawner::SpawnBall,
                SpawnInterval,
                true
            );
        }
    }
}

void AQuidditchBallSpawner::SpawnBall()
{
    if (CurrentSpawnCount >= MaxSpawnCount)
    {
        GetWorld()->GetTimerManager().ClearTimer(SpawnIntervalTimer);
        return;
    }

    if (!BallClassToSpawn)
    {
        return;
    }

    FVector RandomOffset = FMath::VRand() * FMath::FRandRange(0.0f, SpawnRadius);
    RandomOffset.Z = FMath::Abs(RandomOffset.Z);
    FVector SpawnLocation = GetActorLocation() + RandomOffset + SpawnOffset;

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = 
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AActor* SpawnedBall = GetWorld()->SpawnActor<AActor>(
        BallClassToSpawn,
        SpawnLocation,
        FRotator::ZeroRotator,
        SpawnParams
    );

    if (SpawnedBall)
    {
        SpawnedBalls.Add(SpawnedBall);
        CurrentSpawnCount++;
    }
}

void AQuidditchBallSpawner::CleanupSpawnedBalls()
{
    for (TWeakObjectPtr<AActor>& Ball : SpawnedBalls)
    {
        if (Ball.IsValid())
        {
            Ball->Destroy();
        }
    }

    SpawnedBalls.Empty();
    CurrentSpawnCount = 0;

    GetWorld()->GetTimerManager().ClearTimer(SpawnDelayTimer);
    GetWorld()->GetTimerManager().ClearTimer(SpawnIntervalTimer);
}
