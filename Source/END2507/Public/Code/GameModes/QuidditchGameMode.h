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

protected:
    virtual void BeginPlay() override;

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
};
