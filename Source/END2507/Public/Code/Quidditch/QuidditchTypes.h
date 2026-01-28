// ============================================================================
// QuidditchTypes.h
// Shared type definitions for Quidditch AI and gameplay systems
//
// Developer: Marcus Daley
// Date: January 20, 2026
// Project: WizardJam / GAR (END2507)
//
// PURPOSE:
// Defines Quidditch-specific enums, delegates, and structs.
// Used by AI agents, game mode, and HUD systems.
//
// DESIGN DECISION:
// We use enum class for roles because Quidditch has exactly 4 official positions.
// This is different from spell types which use FName for designer expansion.
// Roles are game RULES (fixed), not game CONTENT (expandable).
//
// ALGORITHM MAPPING (from coursework):
// - Seeker: PathSearch.cpp intercept prediction
// - Chaser: Flock.cs full flocking behaviors
// - Beater: MsPacManAgent.java INVERTED safety scoring
// - Keeper: Flock.cs Cohesion to fixed goal point
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "QuidditchTypes.generated.h"

// ============================================================================
// QUIDDITCH ROLE ENUM
// The four official Quidditch positions
// Each position has dedicated behavior tree and component configuration
// ============================================================================

UENUM(BlueprintType)
enum class EQuidditchRole : uint8
{
    None        UMETA(DisplayName = "None - Unassigned"),
    Seeker      UMETA(DisplayName = "Seeker - Catches Snitch"),
    Chaser      UMETA(DisplayName = "Chaser - Scores Goals"),
    Beater      UMETA(DisplayName = "Beater - Protects Team"),
    Keeper      UMETA(DisplayName = "Keeper - Defends Goal")
};

// ============================================================================
// QUIDDITCH BALL ENUM
// Ball types in the game - each has different physics and AI interaction
// ============================================================================

UENUM(BlueprintType)
enum class EQuidditchBall : uint8
{
    None        UMETA(DisplayName = "None"),
    Quaffle     UMETA(DisplayName = "Quaffle - Scoring Ball"),
    Bludger     UMETA(DisplayName = "Bludger - Attack Ball"),
    Snitch      UMETA(DisplayName = "Golden Snitch - Game Ender")
};

// ============================================================================
// MATCH STATE ENUM
// Current state of the Quidditch match
// Gas Station Pattern: States map to synchronization phases
// ============================================================================

UENUM(BlueprintType)
enum class EQuidditchMatchState : uint8
{
    // Pre-match phases (Gas Station: cars arriving at starting line)
    Initializing    UMETA(DisplayName = "Initializing"),           // Agents spawning, acquiring brooms
    FlyingToStart   UMETA(DisplayName = "Flying to Start"),        // Agents flying to staging zones
    WaitingForReady UMETA(DisplayName = "Waiting for Ready"),      // Gas Station: waiting at starting line
    Countdown       UMETA(DisplayName = "Countdown"),              // Brief visual countdown before start

    // Match phases (Gas Station: gun fired, fillTank loop)
    InProgress      UMETA(DisplayName = "Match In Progress"),      // Gas Station: gun fired, match running
    PlayerJoining   UMETA(DisplayName = "Player Joining"),         // Player joining, AI swapping teams

    // End phases (Gas Station: testOver = true)
    SnitchCaught    UMETA(DisplayName = "Snitch Caught - Ending"),
    Ended           UMETA(DisplayName = "Match Ended")
};

// ============================================================================
// DELEGATES - Observer Pattern Communication
// ============================================================================

// Broadcast when any agent's role is assigned or changed
// Listeners: ATeamAIManager, GameMode, HUD
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnQuidditchRoleAssigned,
    AActor*, Agent,
    EQuidditchRole, AssignedRole
);

// Broadcast when ball possession changes hands
// Listeners: ATeamAIManager, all AI agents for strategy updates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FOnBallPossessionChanged,
    AActor*, NewHolder,
    EQuidditchBall, BallType,
    int32, TeamID
);

// Broadcast when Snitch is caught (ends match)
// Listeners: GameMode to end match
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnSnitchCaught,
    AActor*, Catcher,
    int32, CatchingTeamID
);

// Broadcast when an agent requests to switch roles
// Listeners: TeamAIManager decides if switch is allowed
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FOnRoleSwitchRequested,
    AActor*, RequestingAgent,
    EQuidditchRole, CurrentRole,
    EQuidditchRole, RequestedRole
);

// ============================================================================
// QUIDDITCH TEAM DATA STRUCT
// Configuration for one team in a Quidditch match
// ============================================================================

USTRUCT(BlueprintType)
struct END2507_API FQuidditchTeamData
{
    GENERATED_BODY()

    // Team identifier (0 or 1)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
    int32 TeamID;

    // Team display name
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
    FString TeamName;

    // Team color for visuals
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
    FLinearColor TeamColor;

    // Current score
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Team")
    int32 Score;

    // Number of active players on this team
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Team")
    int32 ActivePlayerCount;

    FQuidditchTeamData()
        : TeamID(0)
        , TeamName(TEXT("Unassigned"))
        , TeamColor(FLinearColor::White)
        , Score(0)
        , ActivePlayerCount(0)
    {}

    FQuidditchTeamData(int32 InTeamID, const FString& InName, const FLinearColor& InColor)
        : TeamID(InTeamID)
        , TeamName(InName)
        , TeamColor(InColor)
        , Score(0)
        , ActivePlayerCount(0)
    {}
};

// ============================================================================
// QUIDDITCH AGENT STATE STRUCT
// Runtime state of a single Quidditch participant
// ============================================================================

USTRUCT(BlueprintType)
struct END2507_API FQuidditchAgentState
{
    GENERATED_BODY()

    // Reference to the agent actor
    UPROPERTY(BlueprintReadOnly, Category = "Agent")
    TWeakObjectPtr<AActor> Agent;

    // Current assigned role
    UPROPERTY(BlueprintReadOnly, Category = "Agent")
    EQuidditchRole Role;

    // Team this agent belongs to
    UPROPERTY(BlueprintReadOnly, Category = "Agent")
    int32 TeamID;

    // Is the agent currently on a broom?
    UPROPERTY(BlueprintReadOnly, Category = "Agent")
    bool bIsOnBroom;

    // What ball (if any) is the agent holding?
    UPROPERTY(BlueprintReadOnly, Category = "Agent")
    EQuidditchBall HeldBall;

    // Personal score contribution this match
    UPROPERTY(BlueprintReadOnly, Category = "Agent")
    int32 PersonalScore;

    FQuidditchAgentState()
        : Role(EQuidditchRole::None)
        , TeamID(-1)
        , bIsOnBroom(false)
        , HeldBall(EQuidditchBall::None)
        , PersonalScore(0)
    {}
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

namespace QuidditchHelpers
{
    // Convert role enum to display string
    inline FString RoleToString(EQuidditchRole Role)
    {
        switch (Role)
        {
        case EQuidditchRole::Seeker:  return TEXT("Seeker");
        case EQuidditchRole::Chaser:  return TEXT("Chaser");
        case EQuidditchRole::Beater:  return TEXT("Beater");
        case EQuidditchRole::Keeper:  return TEXT("Keeper");
        default:                      return TEXT("None");
        }
    }

    // Convert ball enum to display string
    inline FString BallToString(EQuidditchBall Ball)
    {
        switch (Ball)
        {
        case EQuidditchBall::Quaffle: return TEXT("Quaffle");
        case EQuidditchBall::Bludger: return TEXT("Bludger");
        case EQuidditchBall::Snitch:  return TEXT("Snitch");
        default:                       return TEXT("None");
        }
    }

    // Get points awarded for catching Snitch (game rule constant)
    inline int32 GetSnitchCatchPoints() { return 150; }

    // Get points awarded for goal (game rule constant)
    inline int32 GetGoalPoints() { return 10; }
}
