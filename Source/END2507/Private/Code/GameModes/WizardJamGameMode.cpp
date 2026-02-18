// WizardJamGameMode.cpp
// Developer: Marcus Daley
// Date: December 21, 2025
// Implementation of core game mode functionality

#include "Code/GameModes/WizardJamGameMode.h"
#include "Code/Actors/BasePlayer.h"
#include "Code/Actors/BaseAgent.h"
#include "Code/Spells/SpellCollectible.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogWizardJamGameMode, Log, All);

AWizardJamGameMode::AWizardJamGameMode()
    : RequiredSpellsToWin(4)
    , bRequireBossDefeat(true)
    , RequiredWavesToWin(0)
    , TotalWaves(3)
    , WaveDelaySeconds(3.0f)
    , CurrentEnemyCount(0)
    , TotalEnemiesKilled(0)
    , CurrentWave(0)
    , CollectedSpellCount(0)
    , bBossDefeated(false)
    , bIsGameOver(false)
    , CurrentBossPhase(0)
    , PlayerScore(0)
    , AIScore(0)
    , WinningScore(100)
{

    AQuidditchGoal::OnAnyGoalScored.AddUObject(this, &AWizardJamGameMode::HandleGoalScored);

    UE_LOG(LogWizardJamGameMode, Log,
        TEXT("[GameMode] Subscribed to AQuidditchGoal::OnAnyGoalScored (static delegate)"));
}

void AWizardJamGameMode::HandleGoalScored(AQuidditchGoal* Goal, AActor* ScoringActor, FName ProjectileElement, int32 PointsAwarded, bool bCorrectElement)
{  // Skip if match already ended
    if (bIsGameOver)
    {
        return;
    }
    // Null checks
    if (!Goal || !ScoringActor)
    {
        UE_LOG(LogWizardJamGameMode, Warning,
            TEXT("[GameMode] HandleGoalScored received null Goal or ScoringActor"));
        return;
    }
    // Determine which team scored based on shooter's team
    // Player (Team 0) scoring on AI goal (Team 1) = Player gets points
    // AI (Team 1) scoring on Player goal (Team 0) = AI gets points

    int32 ScorerTeamID = 0;  // Default to player
    if (ScoringActor->Implements<UGenericTeamAgentInterface>())
    {
        IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(ScoringActor);
        if (TeamAgent)
        {
            ScorerTeamID = TeamAgent->GetGenericTeamId().GetId();
        }
    }

    if (ScorerTeamID == 0)
    {
        PlayerScore += PointsAwarded;

        UE_LOG(LogWizardJamGameMode, Display,
            TEXT("[GameMode] PLAYER scored %d pts (Total: %d) | Element: '%s' | Goal: '%s'"),
            PointsAwarded, PlayerScore, *ProjectileElement.ToString(), *Goal->GetName());
    }
    else
    {
        AIScore += PointsAwarded;

        UE_LOG(LogWizardJamGameMode, Display,
            TEXT("[GameMode] AI scored %d pts (Total: %d) | Element: '%s' | Goal: '%s'"),
            PointsAwarded, AIScore, *ProjectileElement.ToString(), *Goal->GetName());
    }
    // Broadcast score change for HUD updates
    OnScoreChanged.Broadcast(PlayerScore, AIScore, ScoringActor);

    // Check win conditions using class member WinningScore (set in constructor/Blueprint)
    if (PlayerScore >= WinningScore)
    {
        EndMatch(true, FString::Printf(TEXT("Player reached %d points!"), WinningScore));
    }
    else if (AIScore >= WinningScore)
    {
        EndMatch(false, FString::Printf(TEXT("AI reached %d points!"), WinningScore));
    }
}

void AWizardJamGameMode::BeginPlay()
{
    Super::BeginPlay();
    // Initialize scores
    PlayerScore = 0;
    AIScore = 0;

    // Broadcast initial score (HUD will update)
    OnScoreChanged.Broadcast(PlayerScore, AIScore, nullptr);

    UE_LOG(LogWizardJamGameMode, Display,
        TEXT("[GameMode] Match started | Winning Score: %d | Goals will self-register"),
        WinningScore);
    UE_LOG(LogWizardJamGameMode, Display, TEXT("WizardJam Game Mode initialized"));
    UE_LOG(LogWizardJamGameMode, Display, TEXT("  Required Spells: %d"), RequiredSpellsToWin);
    UE_LOG(LogWizardJamGameMode, Display, TEXT("  Boss Required: %s"), bRequireBossDefeat ? TEXT("Yes") : TEXT("No"));
    UE_LOG(LogWizardJamGameMode, Display, TEXT("  Total Waves: %d"), TotalWaves);

    CurrentWave = 1;
}
void AWizardJamGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Unsubscribe from static delegate to prevent dangling pointer
    AQuidditchGoal::OnAnyGoalScored.RemoveAll(this);

    UE_LOG(LogWizardJamGameMode, Log,
        TEXT("[GameMode] Unsubscribed from QuidditchGoal static delegate"));

    Super::EndPlay(EndPlayReason);
}

void AWizardJamGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
    Super::HandleStartingNewPlayer_Implementation(NewPlayer);

    if (NewPlayer)
    {
        if (ABasePlayer* Player = Cast<ABasePlayer>(NewPlayer->GetPawn()))
        {
            CachedPlayer = Player;
            UE_LOG(LogWizardJamGameMode, Display, TEXT("Player registered: %s"), *Player->GetName());
        }
    }
}

bool AWizardJamGameMode::HasCollectedSpell(ESpellChannel Spell) const
{
    return CollectedSpells.Contains(Spell);
}

void AWizardJamGameMode::RegisterEnemy(ABaseAgent* Enemy)
{
    if (!Enemy) return;

    ActiveEnemies.AddUnique(Enemy);
    CurrentEnemyCount = ActiveEnemies.Num();

    UE_LOG(LogWizardJamGameMode, Verbose, TEXT("Enemy registered: %s (Total: %d)"),
        *Enemy->GetName(), CurrentEnemyCount);

    OnEnemyCountChanged.Broadcast(CurrentEnemyCount, TotalEnemiesKilled);
}

void AWizardJamGameMode::UnregisterEnemy(ABaseAgent* Enemy, bool bWasKilled)
{
    if (!Enemy) return;

    ActiveEnemies.Remove(Enemy);
    CurrentEnemyCount = ActiveEnemies.Num();

    if (bWasKilled)
    {
        TotalEnemiesKilled++;
        UE_LOG(LogWizardJamGameMode, Display, TEXT("Enemy killed: %s (Remaining: %d)"),
            *Enemy->GetName(), CurrentEnemyCount);
    }

    OnEnemyCountChanged.Broadcast(CurrentEnemyCount, TotalEnemiesKilled);
    CheckWaveCompletion();
}

void AWizardJamGameMode::RegisterSpellCollected(ESpellChannel Spell, ABasePlayer* Player)
{
    if (CollectedSpells.Contains(Spell))
    {
        UE_LOG(LogWizardJamGameMode, Warning, TEXT("Spell already collected: %d"), (int32)Spell);
        return;
    }

    CollectedSpells.Add(Spell);
    CollectedSpellCount = CollectedSpells.Num();

    UE_LOG(LogWizardJamGameMode, Display, TEXT("Spell collected! Total: %d/%d"),
        CollectedSpellCount, RequiredSpellsToWin);

    OnSpellProgressChanged.Broadcast(CollectedSpellCount, RequiredSpellsToWin);
    CheckWinConditions();
}

void AWizardJamGameMode::CheckWaveCompletion()
{
    if (CurrentEnemyCount > 0 || bIsGameOver) return;

    UE_LOG(LogWizardJamGameMode, Display, TEXT("Wave %d complete!"), CurrentWave);

    if (CurrentWave >= TotalWaves)
    {
        UE_LOG(LogWizardJamGameMode, Display, TEXT("All waves complete! Boss incoming..."));
        OnWaveComplete.Broadcast(CurrentWave, -1);
        CheckWinConditions();
        return;
    }

    OnWaveComplete.Broadcast(CurrentWave, CurrentWave + 1);

    GetWorld()->GetTimerManager().SetTimer(
        WaveDelayTimerHandle,
        this,
        &AWizardJamGameMode::StartNextWave,
        WaveDelaySeconds,
        false
    );
}

void AWizardJamGameMode::StartNextWave()
{
    CurrentWave++;
    UE_LOG(LogWizardJamGameMode, Display, TEXT("Starting Wave %d"), CurrentWave);
}

void AWizardJamGameMode::NotifyBossPhaseChange(int32 NewPhase, float HealthPercent)
{
    CurrentBossPhase = NewPhase;
    UE_LOG(LogWizardJamGameMode, Display, TEXT("Boss Phase %d (%.0f%% HP)"),
        NewPhase, HealthPercent * 100.0f);
    OnBossPhaseChanged.Broadcast(NewPhase, HealthPercent);
}

void AWizardJamGameMode::NotifyBossDefeated()
{
    bBossDefeated = true;
    UE_LOG(LogWizardJamGameMode, Display, TEXT("Boss defeated!"));
    CheckWinConditions();
}

void AWizardJamGameMode::BindToGoalEvents()
{
}

void AWizardJamGameMode::EndMatch(bool bPlayerWon, const FString& Reason)
{ // Set game over flag
    bIsGameOver = true;

    // ========================================================================
    // NOTIFY ALL GOALS (Static function call)
    // This sets bMatchEnded = true on all active goals
    // Goals will stop processing projectiles
    // ========================================================================

    AQuidditchGoal::NotifyAllGoalsMatchEnded();

    // Broadcast match ended
    OnMatchEnded.Broadcast(bPlayerWon, Reason);

    // Also broadcast existing win/loss delegates if you have them
    if (bPlayerWon)
    {
        OnGameWon.Broadcast(Reason);
    }
    else
    {
        OnGameLost.Broadcast(Reason);
    }

    UE_LOG(LogWizardJamGameMode, Display,
        TEXT("[GameMode] === MATCH ENDED === Winner: %s | Reason: %s | Final: Player %d - AI %d"),
        bPlayerWon ? TEXT("PLAYER") : TEXT("AI"), *Reason, PlayerScore, AIScore);
}

void AWizardJamGameMode::CheckWinConditions()
{
    if (bIsGameOver) return;

    bool bSpellConditionMet = (RequiredSpellsToWin == 0) || (CollectedSpellCount >= RequiredSpellsToWin);
    bool bBossConditionMet = !bRequireBossDefeat || bBossDefeated;
    bool bWaveConditionMet = (RequiredWavesToWin == 0) || (CurrentWave >= RequiredWavesToWin);

    if (bSpellConditionMet && bBossConditionMet && bWaveConditionMet)
    {
        FString WinReason;
        if (bBossDefeated)
            WinReason = TEXT("Defeated the Wizard Boss!");
        else if (CollectedSpellCount >= RequiredSpellsToWin && RequiredSpellsToWin > 0)
            WinReason = FString::Printf(TEXT("Collected all %d spells!"), RequiredSpellsToWin);
        else
            WinReason = TEXT("Completed all objectives!");

        TriggerWin(WinReason);
    }
}

void AWizardJamGameMode::TriggerWin(const FString& Reason)
{
    if (bIsGameOver) return;

    bIsGameOver = true;
    UE_LOG(LogWizardJamGameMode, Display, TEXT("=== GAME WON === %s"), *Reason);
    OnGameWon.Broadcast(Reason);
}

void AWizardJamGameMode::TriggerLoss(const FString& Reason)
{
    if (bIsGameOver) return;

    bIsGameOver = true;
    UE_LOG(LogWizardJamGameMode, Display, TEXT("=== GAME LOST === %s"), *Reason);
    OnGameLost.Broadcast(Reason);
}