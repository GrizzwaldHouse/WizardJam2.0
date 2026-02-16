// ============================================================================
// QuidditchGameMode.cpp
// Developer: Marcus Daley
// Date: January 20, 2026
// Project: END2507
// ============================================================================

#include "Code/GameModes/QuidditchGameMode.h"
#include "Code/Actors/WizardPlayer.h"
#include "Code/Actors/WorldSignalEmitter.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/HUD.h"
#include "StructuredLoggingMacros.h"
#include "StructuredLoggingSubsystem.h"

DEFINE_LOG_CATEGORY(LogQuidditchGameMode);

AQuidditchGameMode::AQuidditchGameMode()
    : SnitchCatchPoints(150)
    , QuaffleGoalPoints(10)
    , RequiredAgentOverride(0)
    , MaxSeekersPerTeam(1)
    , MaxChasersPerTeam(3)
    , MaxBeatersPerTeam(2)
    , MaxKeepersPerTeam(1)
    , TeamAColor(FLinearColor::Red)      // Default Team A color
    , TeamBColor(FLinearColor::Blue)     // Default Team B color
    , TeamAScore(0)
    , TeamBScore(0)
    , bSnitchCaught(false)
    , MatchState(EQuidditchMatchState::Initializing)
    , AgentsReadyCount(0)
    , RequiredAgentCount(0)
    , MatchStartCountdown(3.0f)
    , CountdownSecondsRemaining(0)
{
    // DefaultPawnClass intentionally NOT set here - let Blueprint configure it
    // BP_QuidditchGameMode should set DefaultPawnClass = BP_CodeWizardPlayer
    // The Blueprint child has all input, mesh, HUD, and broom configuration
    // Setting AWizardPlayer::StaticClass() here spawns the raw C++ class which
    // has no input bindings, no mesh, and no HUD widgets configured
}

void AQuidditchGameMode::BeginPlay()
{
    Super::BeginPlay();

    // ========================================================================
    // SPAWN DEBUG LOGGING - Diagnose player spawn issue
    // ========================================================================
    UE_LOG(LogQuidditchGameMode, Warning, TEXT(""));
    UE_LOG(LogQuidditchGameMode, Warning, TEXT("=== SPAWN DEBUG START ==="));
    UE_LOG(LogQuidditchGameMode, Warning, TEXT("GameMode Class: %s"), *GetClass()->GetName());
    UE_LOG(LogQuidditchGameMode, Warning, TEXT("DefaultPawnClass: %s"),
        DefaultPawnClass ? *DefaultPawnClass->GetName() : TEXT("NULL! <- THIS IS THE BUG"));
    UE_LOG(LogQuidditchGameMode, Warning, TEXT("HUDClass: %s"),
        HUDClass ? *HUDClass->GetName() : TEXT("NULL (expected)"));
    UE_LOG(LogQuidditchGameMode, Warning, TEXT("PlayerControllerClass: %s"),
        PlayerControllerClass ? *PlayerControllerClass->GetName() : TEXT("Default APlayerController"));
    UE_LOG(LogQuidditchGameMode, Warning, TEXT("=== SPAWN DEBUG END ==="));
    UE_LOG(LogQuidditchGameMode, Warning, TEXT(""));

    // Calculate required agents based on team composition
    // Each team: 1 Seeker + 3 Chasers + 2 Beaters + 1 Keeper = 7 per team
    if (RequiredAgentOverride > 0)
    {
        RequiredAgentCount = RequiredAgentOverride;
        UE_LOG(LogQuidditchGameMode, Warning,
            TEXT("[QuidditchGameMode] RequiredAgentOverride=%d (testing mode)"),
            RequiredAgentCount);
    }
    else
    {
        RequiredAgentCount = 2 * (MaxSeekersPerTeam + MaxChasersPerTeam + MaxBeatersPerTeam + MaxKeepersPerTeam);
    }

    UE_LOG(LogQuidditchGameMode, Display,
        TEXT("[QuidditchGameMode] Match initialized | Snitch=%d pts | Goal=%d pts | RequiredAgents=%d"),
        SnitchCatchPoints, QuaffleGoalPoints, RequiredAgentCount);

    // Start in Initializing state
    TransitionToState(EQuidditchMatchState::Initializing);
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

FLinearColor AQuidditchGameMode::GetTeamColor(EQuidditchTeam Team) const
{
    switch (Team)
    {
    case EQuidditchTeam::TeamA: return TeamAColor;
    case EQuidditchTeam::TeamB: return TeamBColor;
    default: return FLinearColor::White;
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

    // Emit WorldSignal for match end (ball cleanup, etc.)
    EmitWorldSignal(SignalTypeNames::QuidditchMatchEnd);

    // Notify all controllers that match has ended (resets BB.MatchStarted -> false)
    OnMatchEnded.Broadcast();

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

// ============================================================================
// SYNCHRONIZATION - Gas Station Pattern Implementation
// ============================================================================

void AQuidditchGameMode::TransitionToState(EQuidditchMatchState NewState)
{
    if (MatchState == NewState)
    {
        return;
    }

    EQuidditchMatchState OldState = MatchState;
    MatchState = NewState;

    UE_LOG(LogQuidditchGameMode, Display,
        TEXT("[QuidditchGameMode] State: %s -> %s"),
        *UEnum::GetValueAsString(OldState),
        *UEnum::GetValueAsString(NewState));

    // Broadcast state change - controllers and HUD listen
    OnMatchStateChanged.Broadcast(OldState, NewState);
}

void AQuidditchGameMode::HandleAgentReachedStagingZone(APawn* Agent)
{
    if (!Agent)
    {
        return;
    }

    // Prevent double-counting
    TWeakObjectPtr<APawn> WeakAgent(Agent);
    if (ReadyAgents.Contains(WeakAgent))
    {
        return;
    }

    // Verify agent is registered
    const FQuidditchAgentInfo* Info = FindAgentInfo(Agent);
    if (!Info)
    {
        UE_LOG(LogQuidditchGameMode, Warning,
            TEXT("[QuidditchGameMode] Unregistered agent '%s' reached staging zone"),
            *Agent->GetName());
        return;
    }

    // Gas Station Pattern: incNumberWaitingInLine()
    ReadyAgents.Add(WeakAgent);
    AgentsReadyCount = ReadyAgents.Num();

    UE_LOG(LogQuidditchGameMode, Display,
        TEXT("[QuidditchGameMode] Agent '%s' ready at staging zone | %d/%d ready"),
        *Agent->GetName(), AgentsReadyCount, RequiredAgentCount);

    // Broadcast for HUD feedback
    OnAgentReadyAtStart.Broadcast(Agent, AgentsReadyCount);

    // Transition to WaitingForReady if this is the first ready agent
    if (MatchState == EQuidditchMatchState::Initializing || MatchState == EQuidditchMatchState::FlyingToStart)
    {
        TransitionToState(EQuidditchMatchState::WaitingForReady);
    }

    // Check if all agents are ready
    CheckAllAgentsReady();
}

void AQuidditchGameMode::HandleAgentLeftStagingZone(APawn* Agent)
{
    if (!Agent)
    {
        return;
    }

    // Only allow leaving during pre-match phases (not during active match)
    if (MatchState == EQuidditchMatchState::InProgress || MatchState == EQuidditchMatchState::Ended)
    {
        return;
    }

    TWeakObjectPtr<APawn> WeakAgent(Agent);
    if (!ReadyAgents.Contains(WeakAgent))
    {
        return;
    }

    // Remove from ready set
    ReadyAgents.Remove(WeakAgent);
    AgentsReadyCount = ReadyAgents.Num();

    UE_LOG(LogQuidditchGameMode, Display,
        TEXT("[QuidditchGameMode] Agent '%s' LEFT staging zone | %d/%d ready"),
        *Agent->GetName(), AgentsReadyCount, RequiredAgentCount);

    // If countdown was running and we lost a ready agent, cancel it
    if (MatchState == EQuidditchMatchState::Countdown)
    {
        GetWorld()->GetTimerManager().ClearTimer(CountdownTimerHandle);
        TransitionToState(EQuidditchMatchState::WaitingForReady);

        UE_LOG(LogQuidditchGameMode, Warning,
            TEXT("[QuidditchGameMode] Countdown cancelled - agent left staging zone"));
    }
}

void AQuidditchGameMode::CheckAllAgentsReady()
{
    // Gas Station Pattern: while(numberStandingInLine != maxCars) wait
    // Here we check if all registered agents are ready
    if (AgentsReadyCount >= RequiredAgentCount && RequiredAgentCount > 0)
    {
        UE_LOG(LogQuidditchGameMode, Display,
            TEXT("[QuidditchGameMode] All %d agents ready! Starting countdown..."),
            AgentsReadyCount);

        // Broadcast all-ready event
        OnAllAgentsReady.Broadcast();

        // Start countdown
        StartCountdown();
    }
}

void AQuidditchGameMode::StartCountdown()
{
    TransitionToState(EQuidditchMatchState::Countdown);

    CountdownSecondsRemaining = FMath::CeilToInt(MatchStartCountdown);

    UE_LOG(LogQuidditchGameMode, Display,
        TEXT("[QuidditchGameMode] Countdown: %d seconds"),
        CountdownSecondsRemaining);

    // Set timer for countdown ticks
    GetWorld()->GetTimerManager().SetTimer(
        CountdownTimerHandle,
        this,
        &AQuidditchGameMode::OnCountdownTick,
        1.0f,
        true  // Looping
    );
}

void AQuidditchGameMode::OnCountdownTick()
{
    CountdownSecondsRemaining--;

    UE_LOG(LogQuidditchGameMode, Display,
        TEXT("[QuidditchGameMode] Countdown: %d..."),
        CountdownSecondsRemaining);

    if (CountdownSecondsRemaining <= 0)
    {
        OnCountdownComplete();
    }
}

void AQuidditchGameMode::OnCountdownComplete()
{
    // Clear countdown timer
    GetWorld()->GetTimerManager().ClearTimer(CountdownTimerHandle);

    // Gas Station Pattern: gunCondition.notify_all() - fire the starting gun!
    TransitionToState(EQuidditchMatchState::InProgress);

    UE_LOG(LogQuidditchGameMode, Display,
        TEXT("[QuidditchGameMode] MATCH STARTED! All agents begin playing."));

    // Structured logging - match started (critical event)
    SLOG_EVENT(this, "Quidditch.Match", "MatchStarted",
        Metadata.Add(TEXT("registered_agents"), FString::FromInt(RegisteredAgents.Num()));
        Metadata.Add(TEXT("team_a_score"), FString::FromInt(TeamAScore));
        Metadata.Add(TEXT("team_b_score"), FString::FromInt(TeamBScore));
    );

    // Force immediate flush for critical events (don't wait for async thread)
    if (UStructuredLoggingSubsystem* LogSystem = UStructuredLoggingSubsystem::Get(this))
    {
        LogSystem->FlushLogs();
    }

    // This is the key broadcast - all controllers listen and set BB.bMatchStarted = true
    OnMatchStarted.Broadcast(0.0f);

    // Emit WorldSignal for orchestration systems (QuidditchBallSpawner listens for this)
    EmitWorldSignal(SignalTypeNames::QuidditchMatchStart);
}

FVector AQuidditchGameMode::GetStagingZoneLocation(EQuidditchTeam InTeam, EQuidditchRole InRole, FName InSlotName) const
{
    // Look up by FName key
    FName Key = EncodeStagingKey(InTeam, InRole, InSlotName);

    if (const TWeakObjectPtr<AActor>* ZonePtr = StagingZoneRegistry.Find(Key))
    {
        if (ZonePtr->IsValid())
        {
            return ZonePtr->Get()->GetActorLocation();
        }
    }

    // Fallback: If SlotName matches Role name, try finding any zone for this Team+Role
    // This helps when designer hasn't set a specific slot name yet
    for (const auto& Pair : StagingZoneRegistry)
    {
        if (Pair.Value.IsValid())
        {
            // Check if key contains our team and role
            FString KeyStr = Pair.Key.ToString();
            FString TeamStr = UEnum::GetValueAsString(InTeam);
            FString RoleStr = UEnum::GetValueAsString(InRole);

            if (KeyStr.Contains(TeamStr) && KeyStr.Contains(RoleStr))
            {
                UE_LOG(LogQuidditchGameMode, Warning,
                    TEXT("[QuidditchGameMode] Staging zone slot name mismatch - requested '%s', found key '%s'. Update BTTask_FlyToStagingZone.SlotName or staging zone StagingSlotName."),
                    *InSlotName.ToString(), *KeyStr);
                return Pair.Value->GetActorLocation();
            }
        }
    }

    // Final fallback: return origin with warning
    UE_LOG(LogQuidditchGameMode, Warning,
        TEXT("[QuidditchGameMode] No staging zone found for Team=%s Role=%s SlotName='%s' - place BP_QuidditchStagingZone in level with matching configuration"),
        *UEnum::GetValueAsString(InTeam), *UEnum::GetValueAsString(InRole), *InSlotName.ToString());

    return FVector::ZeroVector;
}

void AQuidditchGameMode::RegisterStagingZone(AActor* Zone, EQuidditchTeam InTeam, EQuidditchRole InRole, FName InSlotName)
{
    if (!Zone)
    {
        return;
    }

    FName Key = EncodeStagingKey(InTeam, InRole, InSlotName);
    StagingZoneRegistry.Add(Key, Zone);

    UE_LOG(LogQuidditchGameMode, Log,
        TEXT("[QuidditchGameMode] Registered staging zone '%s' | Team=%s Role=%s SlotName='%s' | Key='%s'"),
        *Zone->GetName(),
        *UEnum::GetValueAsString(InTeam),
        *UEnum::GetValueAsString(InRole),
        *InSlotName.ToString(),
        *Key.ToString());
}

FName AQuidditchGameMode::EncodeStagingKey(EQuidditchTeam InTeam, EQuidditchRole InRole, FName InSlotName)
{
    // Create readable FName key: "TeamA_Seeker_Seeker" or "TeamB_Chaser_Chaser_Left"
    FString TeamStr = UEnum::GetValueAsString(InTeam);
    FString RoleStr = UEnum::GetValueAsString(InRole);
    FString SlotStr = InSlotName.IsNone() ? RoleStr : InSlotName.ToString();

    return FName(*FString::Printf(TEXT("%s_%s_%s"), *TeamStr, *RoleStr, *SlotStr));
}

void AQuidditchGameMode::RequestPlayerJoin(APlayerController* Player, EQuidditchTeam PreferredTeam)
{
    if (!Player)
    {
        return;
    }

    if (MatchState != EQuidditchMatchState::InProgress)
    {
        UE_LOG(LogQuidditchGameMode, Warning,
            TEXT("[QuidditchGameMode] Player join rejected - match not in progress (state=%s)"),
            *UEnum::GetValueAsString(MatchState));
        return;
    }

    UE_LOG(LogQuidditchGameMode, Display,
        TEXT("[QuidditchGameMode] Player requesting to join team %d"),
        (int32)PreferredTeam);

    // Transition to joining state
    TransitionToState(EQuidditchMatchState::PlayerJoining);

    // Broadcast join request
    OnPlayerJoinRequested.Broadcast(Player, PreferredTeam);

    // Select AI to swap - opposite team from player
    EQuidditchTeam SwapFromTeam = (PreferredTeam == EQuidditchTeam::TeamA)
        ? EQuidditchTeam::TeamB
        : EQuidditchTeam::TeamA;

    APawn* AgentToSwap = SelectAgentForSwap(SwapFromTeam);

    if (AgentToSwap)
    {
        UE_LOG(LogQuidditchGameMode, Display,
            TEXT("[QuidditchGameMode] Selected '%s' for team swap"),
            *AgentToSwap->GetName());

        // Broadcast - the selected agent's controller listens and sets BB.bShouldSwapTeam = true
        OnAgentSelectedForSwap.Broadcast(AgentToSwap);
    }
    else
    {
        UE_LOG(LogQuidditchGameMode, Warning,
            TEXT("[QuidditchGameMode] No agent available for team swap"));

        // Resume match without swap
        TransitionToState(EQuidditchMatchState::InProgress);
    }
}

APawn* AQuidditchGameMode::SelectAgentForSwap(EQuidditchTeam FromTeam)
{
    // Priority: Chaser > Beater > Keeper > Seeker
    // We want to swap the least important role to minimize disruption
    static const EQuidditchRole SwapPriority[] = {
        EQuidditchRole::Chaser,
        EQuidditchRole::Beater,
        EQuidditchRole::Keeper,
        EQuidditchRole::Seeker
    };

    for (EQuidditchRole CurrentRole : SwapPriority)
    {
        for (const FQuidditchAgentInfo& Info : RegisteredAgents)
        {
            if (Info.Team == FromTeam && Info.AssignedRole == CurrentRole && Info.Agent.IsValid())
            {
                return Info.Agent.Get();
            }
        }
    }

    return nullptr;
}

void AQuidditchGameMode::ExecuteTeamSwap(APawn* Agent, EQuidditchTeam NewTeam)
{
    if (!Agent)
    {
        return;
    }

    FQuidditchAgentInfo* Info = FindAgentInfo(Agent);
    if (!Info)
    {
        return;
    }

    EQuidditchTeam OldTeam = Info->Team;
    Info->Team = NewTeam;

    UE_LOG(LogQuidditchGameMode, Display,
        TEXT("[QuidditchGameMode] Agent '%s' swapped from team %d to team %d"),
        *Agent->GetName(), (int32)OldTeam, (int32)NewTeam);

    // Broadcast swap complete
    OnTeamSwapComplete.Broadcast(Agent, OldTeam, NewTeam);

    // Resume match
    TransitionToState(EQuidditchMatchState::InProgress);
}

// ============================================================================
// DEBUG FUNCTIONS
// ============================================================================

void AQuidditchGameMode::DEBUG_ForceStartMatch()
{
    UE_LOG(LogQuidditchGameMode, Warning,
        TEXT("[DEBUG] Force starting match | Registered: %d | Ready: %d | Required: %d"),
        RegisteredAgents.Num(), AgentsReadyCount, RequiredAgentCount);

    // Clear any pending countdown timer
    GetWorld()->GetTimerManager().ClearTimer(CountdownTimerHandle);

    // Force transition to InProgress regardless of ready count
    TransitionToState(EQuidditchMatchState::InProgress);

    // Broadcast match start - all controllers listening will set BB.bMatchStarted = true
    OnMatchStarted.Broadcast(0.0f);

    // Emit WorldSignal for orchestration systems (QuidditchBallSpawner listens for this)
    EmitWorldSignal(SignalTypeNames::QuidditchMatchStart);

    UE_LOG(LogQuidditchGameMode, Warning, TEXT("[DEBUG] Match force-started! OnMatchStarted broadcast sent."));
}

// ============================================================================
// WORLD SIGNAL EMISSION
// Emits signals via WorldSignalEmitter static delegate for orchestration
// ============================================================================

void AQuidditchGameMode::EmitWorldSignal(FName InSignalType)
{
    // Create signal data for broadcast
    FSignalData SignalData;
    SignalData.SignalType = InSignalType;
    SignalData.Emitter = nullptr;  // GameMode is the source, not an emitter actor
    SignalData.SignalLocation = FVector::ZeroVector;
    SignalData.EmitTime = GetWorld()->GetTimeSeconds();
    SignalData.TeamID = -1;  // Applies to all teams

    // Broadcast via static delegate - QuidditchBallSpawner and other systems listen here
    AWorldSignalEmitter::OnAnySignalEmittedGlobal.Broadcast(SignalData);

    UE_LOG(LogQuidditchGameMode, Display,
        TEXT("[QuidditchGameMode] Emitted WorldSignal: %s"),
        *InSignalType.ToString());
}

// ============================================================================
// SPAWN DEBUG - Override to diagnose player spawn issue
// ============================================================================

APawn* AQuidditchGameMode::SpawnDefaultPawnAtTransform_Implementation(
    AController* NewPlayer, const FTransform& SpawnTransform)
{
    UE_LOG(LogQuidditchGameMode, Warning, TEXT(""));
    UE_LOG(LogQuidditchGameMode, Warning, TEXT("=== SPAWNING PLAYER PAWN ==="));
    UE_LOG(LogQuidditchGameMode, Warning, TEXT("Controller: %s"),
        NewPlayer ? *NewPlayer->GetClass()->GetName() : TEXT("NULL"));
    UE_LOG(LogQuidditchGameMode, Warning, TEXT("DefaultPawnClass: %s"),
        DefaultPawnClass ? *DefaultPawnClass->GetName() : TEXT("NULL <- CRITICAL BUG!"));
    UE_LOG(LogQuidditchGameMode, Warning, TEXT("SpawnLocation: %s"), *SpawnTransform.GetLocation().ToString());
    UE_LOG(LogQuidditchGameMode, Warning, TEXT("SpawnRotation: %s"), *SpawnTransform.GetRotation().Rotator().ToString());

    // Call parent to do actual spawn
    APawn* SpawnedPawn = Super::SpawnDefaultPawnAtTransform_Implementation(NewPlayer, SpawnTransform);

    if (SpawnedPawn)
    {
        UE_LOG(LogQuidditchGameMode, Warning, TEXT("Spawned Pawn: %s"), *SpawnedPawn->GetClass()->GetName());
        UE_LOG(LogQuidditchGameMode, Warning, TEXT("Spawned Location: %s"), *SpawnedPawn->GetActorLocation().ToString());

        // Verify it's the expected type
        if (AWizardPlayer* WP = Cast<AWizardPlayer>(SpawnedPawn))
        {
            UE_LOG(LogQuidditchGameMode, Display, TEXT("SUCCESS: AWizardPlayer spawned correctly!"));
        }
        else
        {
            UE_LOG(LogQuidditchGameMode, Error,
                TEXT("WRONG PAWN TYPE! Expected AWizardPlayer (or child), got %s"),
                *SpawnedPawn->GetClass()->GetName());
            UE_LOG(LogQuidditchGameMode, Error,
                TEXT("FIX: Open BP_QuidditchGameMode -> Set Default Pawn Class = BP_CodeWizardPlayer"));
        }
    }
    else
    {
        UE_LOG(LogQuidditchGameMode, Error, TEXT("SPAWN FAILED - returned nullptr!"));
        UE_LOG(LogQuidditchGameMode, Error, TEXT("Check: Is there a PlayerStart in the level?"));
    }

    UE_LOG(LogQuidditchGameMode, Warning, TEXT("=== SPAWN COMPLETE ==="));
    UE_LOG(LogQuidditchGameMode, Warning, TEXT(""));

    return SpawnedPawn;
}
