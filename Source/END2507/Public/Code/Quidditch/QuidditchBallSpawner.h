// QuidditchBallSpawner.h
// Spawns Quidditch balls (Snitch/Bludgers) in response to match signals
// Designer-configurable spawn count, delay, and cleanup
// Author: Marcus Daley
// Date: January 25, 2026

#pragma once

#include "CoreMinimal.h"
#include "Code/Actors/Spawner.h"
#include "Code/Utilities/SignalTypes.h"
#include "QuidditchBallSpawner.generated.h"

class AWorldSignalEmitter;

UENUM(BlueprintType)
enum class EQuidditchBallType : uint8
{
    Snitch      UMETA(DisplayName = "Golden Snitch"),
    Bludger     UMETA(DisplayName = "Bludger"),
    Quaffle     UMETA(DisplayName = "Quaffle")
};

UCLASS()
class END2507_API AQuidditchBallSpawner : public ASpawner
{
    GENERATED_BODY()

public:
    AQuidditchBallSpawner();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // ========================================================================
    // BALL CONFIGURATION
    // ========================================================================

    // Type of ball to spawn
    UPROPERTY(EditDefaultsOnly, Category = "Quidditch|Ball")
    EQuidditchBallType BallType;

    // Actor class to spawn (BP_GoldenSnitch, BP_Bludger, BP_Quaffle)
    UPROPERTY(EditDefaultsOnly, Category = "Quidditch|Ball")
    TSubclassOf<AActor> BallClassToSpawn;

    // ========================================================================
    // SPAWN TIMING (Designer Control)
    // ========================================================================
    // NOTE: SpawnInterval, SpawnRadius, SpawnOffset inherited from ASpawner

    // Delay after match start signal before first spawn
    UPROPERTY(EditInstanceOnly, Category = "Quidditch|Timing",
        meta = (ClampMin = "0.0", ClampMax = "60.0"))
    float SpawnDelayAfterMatchStart;

    // ========================================================================
    // MATCH INTEGRATION
    // ========================================================================

    // Signal emitter to listen to (auto-finds if not set)
    UPROPERTY(EditInstanceOnly, Category = "Quidditch|Signals")
    AWorldSignalEmitter* MatchStartEmitter;

    // Signal type to respond to
    UPROPERTY(EditInstanceOnly, Category = "Quidditch|Signals")
    FName MatchStartSignalType;

    // Auto-delete spawned balls when match ends
    UPROPERTY(EditInstanceOnly, Category = "Quidditch|Cleanup")
    bool bAutoCleanupOnMatchEnd;

    // Signal for match end (for cleanup)
    UPROPERTY(EditInstanceOnly, Category = "Quidditch|Signals")
    FName MatchEndSignalType;

private:
    // Track spawned balls for cleanup
    TArray<TWeakObjectPtr<AActor>> SpawnedBalls;
    int32 CurrentSpawnCount;
    FTimerHandle SpawnDelayTimer;
    FTimerHandle SpawnIntervalTimer;

    // Signal response callbacks
    UFUNCTION()
    void OnMatchStartSignalReceived(const FSignalData& SignalData);

    UFUNCTION()
    void OnMatchEndSignalReceived(const FSignalData& SignalData);

    // Spawning logic
    void BeginBallSpawning();
    void SpawnBall();
    void CleanupSpawnedBalls();

    // Find signal emitter if not set
    void FindMatchStartEmitter();
};
