// ============================================================================
// QuidditchGameMode.cpp
// Developer: Marcus Daley
// Date: January 20, 2026
// Project: END2507
// ============================================================================

#include "Code/GameModes/QuidditchGameMode.h"
#include "GameFramework/Pawn.h"

DEFINE_LOG_CATEGORY(LogQuidditchGameMode);

AQuidditchGameMode::AQuidditchGameMode()
    : SnitchCatchPoints(150)
    , QuaffleGoalPoints(10)
    , MaxSeekersPerTeam(1)
    , MaxChasersPerTeam(3)
    , MaxBeatersPerTeam(2)
    , MaxKeepersPerTeam(1)
    , TeamAScore(0)
    , TeamBScore(0)
    , bSnitchCaught(false)
{
}

void AQuidditchGameMode::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogQuidditchGameMode, Display,
        TEXT("[QuidditchGameMode] Match started | Snitch=%d pts | Goal=%d pts"),
        SnitchCatchPoints, QuaffleGoalPoints);
}

// ============================================================================
// AGENT REGISTRATION
// ============================================================================

EQuidditchRole AQuidditchGameMode::RegisterQuidditchAgent(APawn* Agent, EQuidditchRole PreferredRole, EQuidditchTeam Team)
{
    if (!Agent)
    {
        UE_LOG(LogQuidditchGameMode, Warning, TEXT("[QuidditchGameMode] RegisterQuidditchAgent called with null agent"));
        return EQuidditchRole::None;
    }

    if (Team == EQuidditchTeam::None)
    {
        UE_LOG(LogQuidditchGameMode, Warning, TEXT("[QuidditchGameMode] Agent '%s' has no team assigned"), *Agent->GetName());
        return EQuidditchRole::None;
    }

    // Check if already registered
    if (FQuidditchAgentInfo* Existing = FindAgentInfo(Agent))
    {
        UE_LOG(LogQuidditchGameMode, Log, TEXT("[QuidditchGameMode] Agent '%s' already registered as %d"),
            *Agent->GetName(), (int32)Existing->AssignedRole);
        return Existing->AssignedRole;
    }

    // Find available role
    EQuidditchRole AssignedRole = FindAvailableRole(Team, PreferredRole);

    if (AssignedRole == EQuidditchRole::None)
    {
        UE_LOG(LogQuidditchGameMode, Warning, TEXT("[QuidditchGameMode] No roles available for agent '%s' on team %d"),
            *Agent->GetName(), (int32)Team);
        return EQuidditchRole::None;
    }

    // Register agent
    FQuidditchAgentInfo Info;
    Info.Agent = Agent;
    Info.Team = Team;
    Info.PreferredRole = PreferredRole;
    Info.AssignedRole = AssignedRole;
    RegisteredAgents.Add(Info);

    UE_LOG(LogQuidditchGameMode, Display,
        TEXT("[QuidditchGameMode] Registered '%s' | Team: %s | Role: %s (Preferred: %s)"),
        *Agent->GetName(),
        Team == EQuidditchTeam::TeamA ? TEXT("A") : TEXT("B"),
        *UEnum::GetValueAsString(AssignedRole),
        *UEnum::GetValueAsString(PreferredRole));

    // Broadcast assignment
    OnQuidditchRoleAssigned.Broadcast(Agent, Team, AssignedRole);

    return AssignedRole;
}

void AQuidditchGameMode::UnregisterQuidditchAgent(APawn* Agent)
{
    if (!Agent) return;

    for (int32 i = RegisteredAgents.Num() - 1; i >= 0; --i)
    {
        if (RegisteredAgents[i].Agent.Get() == Agent)
        {
            UE_LOG(LogQuidditchGameMode, Log, TEXT("[QuidditchGameMode] Unregistered '%s'"), *Agent->GetName());
            RegisteredAgents.RemoveAt(i);
            return;
        }
    }
}

// ============================================================================
// TEAM QUERIES
// ============================================================================

int32 AQuidditchGameMode::GetTeamScore(EQuidditchTeam Team) const
{
    switch (Team)
    {
    case EQuidditchTeam::TeamA: return TeamAScore;
    case EQuidditchTeam::TeamB: return TeamBScore;
    default: return 0;
    }
}

TArray<APawn*> AQuidditchGameMode::GetTeamMembers(EQuidditchTeam Team) const
{
    TArray<APawn*> Members;
    for (const FQuidditchAgentInfo& Info : RegisteredAgents)
    {
        if (Info.Team == Team && Info.Agent.IsValid())
        {
            Members.Add(Info.Agent.Get());
        }
    }
    return Members;
}

APawn* AQuidditchGameMode::GetTeamSeeker(EQuidditchTeam Team) const
{
    for (const FQuidditchAgentInfo& Info : RegisteredAgents)
    {
        if (Info.Team == Team && Info.AssignedRole == EQuidditchRole::Seeker && Info.Agent.IsValid())
        {
            return Info.Agent.Get();
        }
    }
    return nullptr;
}

APawn* AQuidditchGameMode::GetTeamKeeper(EQuidditchTeam Team) const
{
    for (const FQuidditchAgentInfo& Info : RegisteredAgents)
    {
        if (Info.Team == Team && Info.AssignedRole == EQuidditchRole::Keeper && Info.Agent.IsValid())
        {
            return Info.Agent.Get();
        }
    }
    return nullptr;
}

TArray<APawn*> AQuidditchGameMode::GetTeamChasers(EQuidditchTeam Team) const
{
    TArray<APawn*> Chasers;
    for (const FQuidditchAgentInfo& Info : RegisteredAgents)
    {
        if (Info.Team == Team && Info.AssignedRole == EQuidditchRole::Chaser && Info.Agent.IsValid())
        {
            Chasers.Add(Info.Agent.Get());
        }
    }
    return Chasers;
}

TArray<APawn*> AQuidditchGameMode::GetTeamBeaters(EQuidditchTeam Team) const
{
    TArray<APawn*> Beaters;
    for (const FQuidditchAgentInfo& Info : RegisteredAgents)
    {
        if (Info.Team == Team && Info.AssignedRole == EQuidditchRole::Beater && Info.Agent.IsValid())
        {
            Beaters.Add(Info.Agent.Get());
        }
    }
    return Beaters;
}

// ============================================================================
// ROLE QUERIES
// ============================================================================

EQuidditchRole AQuidditchGameMode::GetAgentRole(APawn* Agent) const
{
    if (const FQuidditchAgentInfo* Info = FindAgentInfo(Agent))
    {
        return Info->AssignedRole;
    }
    return EQuidditchRole::None;
}

EQuidditchTeam AQuidditchGameMode::GetAgentTeam(APawn* Agent) const
{
    if (const FQuidditchAgentInfo* Info = FindAgentInfo(Agent))
    {
        return Info->Team;
    }
    return EQuidditchTeam::None;
}

bool AQuidditchGameMode::IsRoleAvailable(EQuidditchTeam Team, EQuidditchRole QuidditchRole) const
{
    int32 Current = GetRoleCount(Team, QuidditchRole);
    int32 Max = GetMaxForRole(QuidditchRole);
    return Current < Max;
}

// ============================================================================
// SCORING
// ============================================================================

void AQuidditchGameMode::AddTeamScore(EQuidditchTeam Team, int32 Points, APawn* ScoringAgent)
{
    if (bSnitchCaught)
    {
        UE_LOG(LogQuidditchGameMode, Log, TEXT("[QuidditchGameMode] Match over - ignoring score"));
        return;
    }

    switch (Team)
    {
    case EQuidditchTeam::TeamA:
        TeamAScore += Points;
        UE_LOG(LogQuidditchGameMode, Display, TEXT("[QuidditchGameMode] Team A scored %d | Total: %d"),
            Points, TeamAScore);
        OnQuidditchTeamScored.Broadcast(Team, Points, TeamAScore);
        break;

    case EQuidditchTeam::TeamB:
        TeamBScore += Points;
        UE_LOG(LogQuidditchGameMode, Display, TEXT("[QuidditchGameMode] Team B scored %d | Total: %d"),
            Points, TeamBScore);
        OnQuidditchTeamScored.Broadcast(Team, Points, TeamBScore);
        break;

    default:
        break;
    }
}

void AQuidditchGameMode::NotifySnitchCaught(APawn* CatchingSeeker, EQuidditchTeam Team)
{
    if (bSnitchCaught)
    {
        UE_LOG(LogQuidditchGameMode, Warning, TEXT("[QuidditchGameMode] Snitch already caught!"));
        return;
    }

    bSnitchCaught = true;

    // Award snitch points
    AddTeamScore(Team, SnitchCatchPoints, CatchingSeeker);

    // Determine winner
    EQuidditchTeam Winner = (TeamAScore > TeamBScore) ? EQuidditchTeam::TeamA : EQuidditchTeam::TeamB;
    if (TeamAScore == TeamBScore)
    {
        Winner = Team;  // Snitch catcher wins ties
    }

    UE_LOG(LogQuidditchGameMode, Display,
        TEXT("[QuidditchGameMode] SNITCH CAUGHT by '%s' | Final: Team A=%d, Team B=%d | Winner: %s"),
        CatchingSeeker ? *CatchingSeeker->GetName() : TEXT("Unknown"),
        TeamAScore, TeamBScore,
        Winner == EQuidditchTeam::TeamA ? TEXT("Team A") : TEXT("Team B"));

    OnSnitchCaught.Broadcast(CatchingSeeker, Winner);

    // End match
    FString Reason = FString::Printf(TEXT("Snitch caught! Final: %d - %d"), TeamAScore, TeamBScore);
    EndMatch(Winner == EQuidditchTeam::TeamA, Reason);  // Assuming player is Team A
}

// ============================================================================
// PRIVATE HELPERS
// ============================================================================

int32 AQuidditchGameMode::GetRoleCount(EQuidditchTeam Team, EQuidditchRole QuidditchRole) const
{
    int32 Count = 0;
    for (const FQuidditchAgentInfo& Info : RegisteredAgents)
    {
        if (Info.Team == Team && Info.AssignedRole == QuidditchRole && Info.Agent.IsValid())
        {
            Count++;
        }
    }
    return Count;
}

int32 AQuidditchGameMode::GetMaxForRole(EQuidditchRole QuidditchRole) const
{
    switch (QuidditchRole)
    {
    case EQuidditchRole::Seeker: return MaxSeekersPerTeam;
    case EQuidditchRole::Chaser: return MaxChasersPerTeam;
    case EQuidditchRole::Beater: return MaxBeatersPerTeam;
    case EQuidditchRole::Keeper: return MaxKeepersPerTeam;
    default: return 0;
    }
}

EQuidditchRole AQuidditchGameMode::FindAvailableRole(EQuidditchTeam Team, EQuidditchRole Preferred) const
{
    // Try preferred role first
    if (Preferred != EQuidditchRole::None && IsRoleAvailable(Team, Preferred))
    {
        return Preferred;
    }

    // Fallback priority: Chaser > Beater > Keeper > Seeker
    static const EQuidditchRole FallbackOrder[] = {
        EQuidditchRole::Chaser,
        EQuidditchRole::Beater,
        EQuidditchRole::Keeper,
        EQuidditchRole::Seeker
    };

    for (EQuidditchRole FallbackRole : FallbackOrder)
    {
        if (IsRoleAvailable(Team, FallbackRole))
        {
            return FallbackRole;
        }
    }

    return EQuidditchRole::None;
}

FQuidditchAgentInfo* AQuidditchGameMode::FindAgentInfo(APawn* Agent)
{
    for (FQuidditchAgentInfo& Info : RegisteredAgents)
    {
        if (Info.Agent.Get() == Agent)
        {
            return &Info;
        }
    }
    return nullptr;
}

const FQuidditchAgentInfo* AQuidditchGameMode::FindAgentInfo(APawn* Agent) const
{
    for (const FQuidditchAgentInfo& Info : RegisteredAgents)
    {
        if (Info.Agent.Get() == Agent)
        {
            return &Info;
        }
    }
    return nullptr;
}
