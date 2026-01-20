// WizardJamGameMode.h
// Developer: Marcus Daley
// Date: December 21, 2025
// Purpose: Core game mode for WizardJam - tracks win conditions, waves, and spell progress
// Usage: Set as the default GameMode in Project Settings or per-level World Settings

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Code/Spells/SpellChannelTypes.h"
#include "Code/Quidditch/QuidditchGoal.h"
#include "WizardJamGameMode.generated.h"

// Forward declarations for pointer types 
class ABasePlayer;
class ABaseAgent;

// ============================================================================
// DELEGATE DECLARATIONS
// Observer pattern: Other systems subscribe to these events
// ============================================================================

// Broadcast when player wins the game 
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameWon, FString, WinReason);

// Broadcast when player loses the game 
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameLost, FString, LoseReason);

// Broadcast when a wave is cleared and next wave is starting 
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWaveComplete, int32, CompletedWave, int32, NextWave);

// Broadcast when enemy count changes (spawn/death) 
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEnemyCountChanged, int32, CurrentCount, int32, TotalKilled);

//Broadcast when player collects a spell 
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSpellProgressChanged, int32, CollectedCount, int32, RequiredCount);

// Broadcast when boss enters a new phase 
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBossPhaseChanged, int32, NewPhase, float, BossHealthPercent);
// Broadcast when match ends - for any system that needs to know
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnMatchEnded,
    bool, bPlayerWon,
    const FString&, Reason
);

// Broadcast when score changes (HUD subscribes for display updates)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FOnScoreChanged,
    int32, PlayerScore,
    int32, AIScore,
    AActor*, ScoringActor
);
// ============================================================================
// GAME MODE CLASS
// ============================================================================

//
// AWizardJamGameMode
// 
//  Central game coordinator that tracks:
//  - Spell collection progress (4 elemental spells)
//  - Wave-based enemy spawning (3 waves + boss)
//  - Boss phase transitions
//
// All communication uses delegates (observer pattern) for loose coupling.
// Configure win conditions in Blueprint child class.
 //
UCLASS()
class END2507_API AWizardJamGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AWizardJamGameMode();

    // ========================================================================
    // GAME STATE QUERIES
    // ========================================================================

    // Get current count of living enemies 
    UFUNCTION(BlueprintPure, Category = "Game State")
    int32 GetCurrentEnemyCount() const { return CurrentEnemyCount; }

    // Get total enemies killed this session 
    UFUNCTION(BlueprintPure, Category = "Game State")
    int32 GetTotalEnemiesKilled() const { return TotalEnemiesKilled; }

    // Get current wave number (1-indexed) 
    UFUNCTION(BlueprintPure, Category = "Game State")
    int32 GetCurrentWave() const { return CurrentWave; }

    // Get count of spells player has collected 
    UFUNCTION(BlueprintPure, Category = "Game State")
    int32 GetCollectedSpellCount() const { return CollectedSpellCount; }

    // Check if player has collected a specific spell type 
    UFUNCTION(BlueprintPure, Category = "Game State")
    bool HasCollectedSpell(ESpellChannel Spell) const;

    //Check if game has ended (win or loss) 
    UFUNCTION(BlueprintPure, Category = "Game State")
    bool IsGameOver() const { return bIsGameOver; }

    // Broadcast when match ends - QuidditchGoal subscribes
    // Goals use this to know when to stop processing projectiles
    FOnMatchEnded OnMatchEnded;

    // Broadcast when score changes - HUD subscribes
    UPROPERTY(BlueprintAssignable, Category = "GameMode|Events")
    FOnScoreChanged OnScoreChanged;

    UFUNCTION(BlueprintPure, Category = "GameMode|Score")
    int32 GetPlayerScore() const { return PlayerScore; }

    UFUNCTION(BlueprintPure, Category = "GameMode|Score")
    int32 GetAIScore() const { return AIScore; }


    // ========================================================================
    // GAME CONTROL FUNCTIONS (BlueprintCallable - can modify state)
    // ========================================================================

    //Called by spawners when an enemy is spawned - adds to tracking 
    UFUNCTION(BlueprintCallable, Category = "Game Control")
    void RegisterEnemy(ABaseAgent* Enemy);

    // Called when enemy dies or is removed - updates counts 
    UFUNCTION(BlueprintCallable, Category = "Game Control")
    void UnregisterEnemy(ABaseAgent* Enemy, bool bWasKilled = true);

    //Called by SpellCollectible when player picks up a spell 
    UFUNCTION(BlueprintCallable, Category = "Game Control")
    void RegisterSpellCollected(ESpellChannel Spell, ABasePlayer* Player);

    // Called by spawners to check if wave should advance
    UFUNCTION(BlueprintCallable, Category = "Game Control")
    void CheckWaveCompletion();

    // Called by boss when entering new phase (for HUD updates) 
    UFUNCTION(BlueprintCallable, Category = "Game Control")
    void NotifyBossPhaseChange(int32 NewPhase, float HealthPercent);

    //Called when boss is killed - triggers win check 
    UFUNCTION(BlueprintCallable, Category = "Game Control")
    void NotifyBossDefeated();

    // Force a win with specified reason (e.g., "All spells collected!") 
    UFUNCTION(BlueprintCallable, Category = "Game Control")
    void TriggerWin(const FString& Reason);

    //Force a loss with specified reason (e.g., "Player died") 
    UFUNCTION(BlueprintCallable, Category = "Game Control")
    void TriggerLoss(const FString& Reason);

    // ========================================================================
    // DELEGATE EVENTS (Subscribe in Blueprint or C++)
    // ========================================================================

    //Fired when player wins - bind to show results screen 
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnGameWon OnGameWon;

    // Fired when player loses - bind to show game over screen 
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnGameLost OnGameLost;

    // Fired when wave completed - bind to update HUD wave counter 
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnWaveComplete OnWaveComplete;

    //Fired when enemy count changes - bind to update HUD enemy counter 
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnEnemyCountChanged OnEnemyCountChanged;

    //Fired when spell collected - bind to update HUD spell icons 
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnSpellProgressChanged OnSpellProgressChanged;

    // Fired when boss phase changes - bind to update boss health bar 
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnBossPhaseChanged OnBossPhaseChanged;
    // ========================================================================
// SCORING (Called by goal delegates)
// ========================================================================

// GameMode subscribes to goal's delegate in BeginPlay
    UFUNCTION()
    void HandleGoalScored(AQuidditchGoal* Goal, AActor* ScoringActor,
        FName ProjectileElement, int32 PointsAwarded, bool bCorrectElement);
protected:
    virtual void BeginPlay() override;
    virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

    // ========================================================================
    // WIN CONDITION SETTINGS (Configure in Blueprint child class)
    // ========================================================================

    // How many spells must be collected to win (0 = not required) 
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Win Conditions")
    int32 RequiredSpellsToWin;

    //Must the boss be defeated to win? 
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Win Conditions")
    bool bRequireBossDefeat;

    // How many waves must be cleared to win (0 = not required) 
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Win Conditions")
    int32 RequiredWavesToWin;

    // ========================================================================
    // WAVE SYSTEM SETTINGS
    // ========================================================================

    //Total number of waves (excluding boss wave) 
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wave System")
    int32 TotalWaves;

    // Delay in seconds between waves 
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wave System")
    float WaveDelaySeconds;

    // ========================================================================
    // RUNTIME STATE (Read-only in editor, modified at runtime)
    // ========================================================================

    //Current count of living enemies 
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
    int32 CurrentEnemyCount;

    // Total enemies killed this session 
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
    int32 TotalEnemiesKilled;

    // Current wave number (1-indexed) 
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
    int32 CurrentWave;

    // Number of unique spells collected 
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
    int32 CollectedSpellCount;

    //Array of collected spell types 
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
    TArray<ESpellChannel> CollectedSpells;

    // Has the boss been defeated?
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
    bool bBossDefeated;

    // Is the game over (win or loss)? 
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
    bool bIsGameOver;

    //Current boss phase (0 = not fighting boss yet) 
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Runtime")
    int32 CurrentBossPhase;
    // ========================================================================
// SCORE TRACKING
// ========================================================================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameMode|Score")
    int32 PlayerScore;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GameMode|Score")
    int32 AIScore;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameMode|Score")
    int32 WinningScore;
    // ========================================================================
    // INTERNAL HELPERS
    // ========================================================================

    // Subscribe to all QuidditchGoal delegates in the level
    void BindToGoalEvents();

    // End the match and broadcast OnMatchEnded
    void EndMatch(bool bPlayerWon, const FString& Reason);
    void EndPlay(const EEndPlayReason::Type EndPlayReason);
private:
    // Internal: Check all win conditions and trigger win if met 
    void CheckWinConditions();

    // Internal: Start the next wave after delay 
    void StartNextWave();

    //Timer handle for wave delay 
    FTimerHandle WaveDelayTimerHandle;

    // Weak references to active enemies (auto-clears if destroyed) 
    UPROPERTY()
    TArray<TWeakObjectPtr<ABaseAgent>> ActiveEnemies;

    // Cached reference to player for quick access 
    UPROPERTY()
    TWeakObjectPtr<ABasePlayer> CachedPlayer;
};