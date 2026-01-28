// ============================================================================
// QuidditchGameMode.h
// Developer: Marcus Daley
// Date: January 20, 2026
// Project: END2507
// ============================================================================
// Purpose:
// Game mode for Quidditch matches. Handles team formation, role assignment,
// and match flow. Agents register with preferred roles and the game mode
// assigns them based on availability.
//
// Quidditch Positions:
// - Seeker (1 per team): Chases the Golden Snitch
// - Chaser (3 per team): Handles Quaffle, scores goals
// - Beater (2 per team): Hits Bludgers at opponents
// - Keeper (1 per team): Guards goal hoops
//
// Usage:
// 1. Set as GameMode in level World Settings
// 2. AI agents call RegisterQuidditchAgent() on BeginPlay
// 3. Game mode assigns roles and broadcasts FOnQuidditchRoleAssigned
// 4. Agents receive role and configure their behavior trees accordingly
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Code/GameModes/WizardJamGameMode.h"
#include "Code/Quidditch/QuidditchTypes.h"
#include "Code/Utilities/SignalTypes.h"
#include "QuidditchGameMode.generated.h"

class APawn;

// ============================================================================
// ENUMS (EQuidditchRole defined in QuidditchTypes.h)
// ============================================================================

UENUM(BlueprintType)
enum class EQuidditchTeam : uint8
{
    None        UMETA(DisplayName = "None"),
    TeamA       UMETA(DisplayName = "Team A"),
    TeamB       UMETA(DisplayName = "Team B")
};

// ============================================================================
// STRUCTS
// ============================================================================

USTRUCT(BlueprintType)
struct FQuidditchAgentInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    TWeakObjectPtr<APawn> Agent;

    UPROPERTY(BlueprintReadOnly)
    EQuidditchTeam Team;

    UPROPERTY(BlueprintReadOnly)
    EQuidditchRole AssignedRole;

    UPROPERTY(BlueprintReadOnly)
    EQuidditchRole PreferredRole;

    FQuidditchAgentInfo()
        : Team(EQuidditchTeam::None)
        , AssignedRole(EQuidditchRole::None)
        , PreferredRole(EQuidditchRole::None)
    {}
};

// ============================================================================
// DELEGATES
// Note: FOnQuidditchRoleAssigned and FOnSnitchCaught are declared in QuidditchTypes.h
// We define game-mode-specific versions here with different signatures
// ============================================================================

// Broadcast when a team scores
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FOnQuidditchTeamScored,
    EQuidditchTeam, ScoringTeam,
    int32, Points,
    int32, TotalScore
);

// Game mode specific delegate for role assignment (uses APawn* and EQuidditchTeam)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FOnGameModeRoleAssigned,
    APawn*, Agent,
    EQuidditchTeam, Team,
    EQuidditchRole, AssignedRole
);

// Game mode specific delegate for snitch caught (uses APawn* and EQuidditchTeam)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnGameModeSnitchCaught,
    APawn*, CatchingSeeker,
    EQuidditchTeam, WinningTeam
);

// ============================================================================
// SYNCHRONIZATION DELEGATES (Gas Station Pattern - Observer Pattern)
// These delegates enable multi-agent synchronization without polling
// Controllers bind at BeginPlay, update their Blackboards when events fire
// ============================================================================

// Broadcast when match state changes - controllers/HUD listen to update their state
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnQuidditchMatchStateChanged,
    EQuidditchMatchState, OldState,
    EQuidditchMatchState, NewState
);

// Broadcast when an agent reaches their staging zone - for HUD ready count
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnAgentReadyAtStart,
    APawn*, Agent,
    int32, TotalReadyCount
);

// Broadcast when ALL agents are ready at staging zones
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllAgentsReady);

// Broadcast when match officially starts (after countdown)
// Controllers listen and set BB.bMatchStarted = true
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnQuidditchMatchStarted,
    float, CountdownSeconds
);

// Broadcast when match ends
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnQuidditchMatchEnded);

// Broadcast when player requests to join mid-match
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnPlayerJoinRequested,
    APlayerController*, Player,
    EQuidditchTeam, RequestedTeam
);

// Broadcast to notify a specific agent they should swap teams
// Controller checks if Agent == GetPawn(), then sets BB.bShouldSwapTeam = true
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnAgentSelectedForSwap,
    APawn*, SelectedAgent
);

// Broadcast when team swap completes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FOnTeamSwapComplete,
    APawn*, SwappedAgent,
    EQuidditchTeam, OldTeam,
    EQuidditchTeam, NewTeam
);

DECLARE_LOG_CATEGORY_EXTERN(LogQuidditchGameMode, Log, All);

// ============================================================================
// GAME MODE CLASS
// ============================================================================

UCLASS()
class END2507_API AQuidditchGameMode : public AWizardJamGameMode
{
    GENERATED_BODY()

public:
    AQuidditchGameMode();

    // ========================================================================
    // AGENT REGISTRATION
    // ========================================================================

    // Register an agent for Quidditch play
    // Returns the assigned role (may differ from preferred if slot is full)
    UFUNCTION(BlueprintCallable, Category = "Quidditch|Registration")
    EQuidditchRole RegisterQuidditchAgent(APawn* Agent, EQuidditchRole PreferredRole, EQuidditchTeam Team);

    // Unregister an agent (when destroyed or leaving match)
    UFUNCTION(BlueprintCallable, Category = "Quidditch|Registration")
    void UnregisterQuidditchAgent(APawn* Agent);

    // ========================================================================
    // TEAM QUERIES
    // ========================================================================

    UFUNCTION(BlueprintPure, Category = "Quidditch|Teams")
    int32 GetTeamScore(EQuidditchTeam Team) const;

    UFUNCTION(BlueprintPure, Category = "Quidditch|Teams")
    FLinearColor GetTeamColor(EQuidditchTeam Team) const;

    UFUNCTION(BlueprintPure, Category = "Quidditch|Teams")
    TArray<APawn*> GetTeamMembers(EQuidditchTeam Team) const;

    UFUNCTION(BlueprintPure, Category = "Quidditch|Teams")
    APawn* GetTeamSeeker(EQuidditchTeam Team) const;

    UFUNCTION(BlueprintPure, Category = "Quidditch|Teams")
    APawn* GetTeamKeeper(EQuidditchTeam Team) const;

    UFUNCTION(BlueprintPure, Category = "Quidditch|Teams")
    TArray<APawn*> GetTeamChasers(EQuidditchTeam Team) const;

    UFUNCTION(BlueprintPure, Category = "Quidditch|Teams")
    TArray<APawn*> GetTeamBeaters(EQuidditchTeam Team) const;

    // ========================================================================
    // ROLE QUERIES
    // ========================================================================

    UFUNCTION(BlueprintPure, Category = "Quidditch|Roles")
    EQuidditchRole GetAgentRole(APawn* Agent) const;

    UFUNCTION(BlueprintPure, Category = "Quidditch|Roles")
    EQuidditchTeam GetAgentTeam(APawn* Agent) const;

    UFUNCTION(BlueprintPure, Category = "Quidditch|Roles")
    bool IsRoleAvailable(EQuidditchTeam Team, EQuidditchRole QuidditchRole) const;

    // ========================================================================
    // SCORING
    // ========================================================================

    UFUNCTION(BlueprintCallable, Category = "Quidditch|Scoring")
    void AddTeamScore(EQuidditchTeam Team, int32 Points, APawn* ScoringAgent = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Quidditch|Scoring")
    void NotifySnitchCaught(APawn* CatchingSeeker, EQuidditchTeam Team);

    // ========================================================================
    // DELEGATES
    // ========================================================================

    UPROPERTY(BlueprintAssignable, Category = "Quidditch|Events")
    FOnGameModeRoleAssigned OnQuidditchRoleAssigned;

    UPROPERTY(BlueprintAssignable, Category = "Quidditch|Events")
    FOnQuidditchTeamScored OnQuidditchTeamScored;

    UPROPERTY(BlueprintAssignable, Category = "Quidditch|Events")
    FOnGameModeSnitchCaught OnSnitchCaught;

    // ========================================================================
    // SYNCHRONIZATION DELEGATES (Gas Station Pattern)
    // Controllers bind at BeginPlay and update their Blackboards
    // ========================================================================

    UPROPERTY(BlueprintAssignable, Category = "Quidditch|Sync")
    FOnQuidditchMatchStateChanged OnMatchStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Quidditch|Sync")
    FOnAgentReadyAtStart OnAgentReadyAtStart;

    UPROPERTY(BlueprintAssignable, Category = "Quidditch|Sync")
    FOnAllAgentsReady OnAllAgentsReady;

    UPROPERTY(BlueprintAssignable, Category = "Quidditch|Sync")
    FOnQuidditchMatchStarted OnMatchStarted;

    UPROPERTY(BlueprintAssignable, Category = "Quidditch|Sync")
    FOnQuidditchMatchEnded OnMatchEnded;

    UPROPERTY(BlueprintAssignable, Category = "Quidditch|Sync")
    FOnPlayerJoinRequested OnPlayerJoinRequested;

    UPROPERTY(BlueprintAssignable, Category = "Quidditch|Sync")
    FOnAgentSelectedForSwap OnAgentSelectedForSwap;

    UPROPERTY(BlueprintAssignable, Category = "Quidditch|Sync")
    FOnTeamSwapComplete OnTeamSwapComplete;

    // ========================================================================
    // SYNCHRONIZATION API
    // Called by staging zones via overlap, not directly by agents
    // ========================================================================

    // Called by QuidditchStagingZone when an agent enters
    void HandleAgentReachedStagingZone(APawn* Agent);

    // Get staging zone world location for agent's team/role/slot
    // SlotName examples: "Seeker", "Chaser_Left", "Chaser_Center", "Chaser_Right", "Beater_A", "Beater_B", "Keeper"
    UFUNCTION(BlueprintPure, Category = "Quidditch|Staging")
    FVector GetStagingZoneLocation(EQuidditchTeam InTeam, EQuidditchRole InRole, FName InSlotName) const;

    // Player requests to join ongoing AI match
    UFUNCTION(BlueprintCallable, Category = "Quidditch|Sync")
    void RequestPlayerJoin(APlayerController* Player, EQuidditchTeam PreferredTeam);

    // Execute the team swap for selected agent (called by BTTask_SwapTeam)
    UFUNCTION(BlueprintCallable, Category = "Quidditch|Sync")
    void ExecuteTeamSwap(APawn* Agent, EQuidditchTeam NewTeam);

    // Query current match state
    UFUNCTION(BlueprintPure, Category = "Quidditch|Sync")
    EQuidditchMatchState GetMatchState() const { return MatchState; }

    // ========================================================================
    // DEBUG FUNCTIONS
    // ========================================================================

    // Force-start match regardless of agent count (for demo/testing)
    // Call via Blueprint, console: ke * DEBUG_ForceStartMatch, or bind to key
    UFUNCTION(BlueprintCallable, Category = "Quidditch|Debug", meta = (DevelopmentOnly))
    void DEBUG_ForceStartMatch();

protected:
    virtual void BeginPlay() override;

    // Debug spawn override - logs DefaultPawnClass and spawn result
    virtual APawn* SpawnDefaultPawnAtTransform_Implementation(
        AController* NewPlayer, const FTransform& SpawnTransform) override;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    // Points awarded for catching the snitch
    UPROPERTY(EditDefaultsOnly, Category = "Quidditch|Scoring")
    int32 SnitchCatchPoints;

    // Points awarded per Quaffle goal
    UPROPERTY(EditDefaultsOnly, Category = "Quidditch|Scoring")
    int32 QuaffleGoalPoints;

    // Maximum players per role per team
    UPROPERTY(EditDefaultsOnly, Category = "Quidditch|Teams")
    int32 MaxSeekersPerTeam;

    UPROPERTY(EditDefaultsOnly, Category = "Quidditch|Teams")
    int32 MaxChasersPerTeam;

    UPROPERTY(EditDefaultsOnly, Category = "Quidditch|Teams")
    int32 MaxBeatersPerTeam;

    UPROPERTY(EditDefaultsOnly, Category = "Quidditch|Teams")
    int32 MaxKeepersPerTeam;

    // Team colors (designer-configurable)
    UPROPERTY(EditDefaultsOnly, Category = "Quidditch|Teams")
    FLinearColor TeamAColor;

    UPROPERTY(EditDefaultsOnly, Category = "Quidditch|Teams")
    FLinearColor TeamBColor;

    // ========================================================================
    // RUNTIME STATE
    // ========================================================================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quidditch|Runtime")
    int32 TeamAScore;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quidditch|Runtime")
    int32 TeamBScore;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quidditch|Runtime")
    bool bSnitchCaught;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quidditch|Runtime")
    TArray<FQuidditchAgentInfo> RegisteredAgents;

    // ========================================================================
    // SYNCHRONIZATION STATE (Gas Station Pattern)
    // ========================================================================

    // Current match state - transitions trigger broadcasts
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quidditch|Sync")
    EQuidditchMatchState MatchState;

    // Number of agents that have reached their staging zones
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quidditch|Sync")
    int32 AgentsReadyCount;

    // Total agents required before match can start
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quidditch|Sync")
    int32 RequiredAgentCount;

    // Track which agents have signaled ready (prevent double-counting)
    UPROPERTY()
    TSet<TWeakObjectPtr<APawn>> ReadyAgents;

    // ========================================================================
    // SYNCHRONIZATION CONFIGURATION
    // ========================================================================

    // Countdown duration before match starts (seconds)
    UPROPERTY(EditDefaultsOnly, Category = "Quidditch|Sync")
    float MatchStartCountdown;

    // Timer for countdown
    FTimerHandle CountdownTimerHandle;

    // Current countdown value (for HUD display)
    int32 CountdownSecondsRemaining;

private:
    // Get count of agents with specific role on team
    int32 GetRoleCount(EQuidditchTeam Team, EQuidditchRole QuidditchRole) const;

    // Get max allowed for role
    int32 GetMaxForRole(EQuidditchRole QuidditchRole) const;

    // Find an available role for team (fallback if preferred is full)
    EQuidditchRole FindAvailableRole(EQuidditchTeam Team, EQuidditchRole Preferred) const;

    // Find agent info by pawn
    FQuidditchAgentInfo* FindAgentInfo(APawn* Agent);
    const FQuidditchAgentInfo* FindAgentInfo(APawn* Agent) const;

    // ========================================================================
    // SYNCHRONIZATION INTERNALS (Gas Station Pattern)
    // ========================================================================

    // State machine transition - broadcasts OnMatchStateChanged
    void TransitionToState(EQuidditchMatchState NewState);

    // Called after each agent ready - checks if all ready
    void CheckAllAgentsReady();

    // Countdown management
    void StartCountdown();
    void OnCountdownTick();
    void OnCountdownComplete();

    // Select which AI to swap when player joins
    // Priority: Chaser > Beater > Keeper > Seeker (keeps key roles stable)
    APawn* SelectAgentForSwap(EQuidditchTeam ToTeam);

    // Registry of staging zones placed in level (populated by zones at BeginPlay)
    // Key: FName combining Team_Role_SlotName (e.g., "TeamA_Seeker_Seeker")
    UPROPERTY()
    TMap<FName, TWeakObjectPtr<AActor>> StagingZoneRegistry;

    // Encode staging zone lookup key as FName
    static FName EncodeStagingKey(EQuidditchTeam InTeam, EQuidditchRole InRole, FName InSlotName);

    // Friend class for staging zone registration
    friend class AQuidditchStagingZone;

    // Called by staging zones to register themselves
    void RegisterStagingZone(AActor* Zone, EQuidditchTeam InTeam, EQuidditchRole InRole, FName InSlotName);

    // ========================================================================
    // WORLD SIGNAL EMISSION
    // Emits signals via WorldSignalEmitter static delegate for orchestration
    // QuidditchBallSpawner and other systems listen for these signals
    // ========================================================================
    void EmitWorldSignal(FName InSignalType);
};
