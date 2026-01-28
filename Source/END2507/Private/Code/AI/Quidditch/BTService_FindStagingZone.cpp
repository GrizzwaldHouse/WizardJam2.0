// ============================================================================
// BTService_FindStagingZone.cpp
// Developer: Marcus Daley
// Date: January 28, 2026
// Project: END2507
// ============================================================================
// Bee and Flower Pattern - Agent-Side Filtering:
// The agent (bee) perceives staging zones (flowers) and decides which one
// to fly to based on its own team/role. The zone just broadcasts "I exist".
// ============================================================================

#include "Code/AI/Quidditch/BTService_FindStagingZone.h"
#include "Code/Quidditch/QuidditchStagingZone.h"
#include "Code/GameModes/QuidditchGameMode.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "Perception/AIPerceptionComponent.h"
#include "EngineUtils.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBTService_FindStagingZone, Log, All);
DEFINE_LOG_CATEGORY(LogBTService_FindStagingZone);

UBTService_FindStagingZone::UBTService_FindStagingZone()
    : MaxStagingZoneRange(10000.0f)
{
    NodeName = TEXT("Find Staging Zone");
    bNotifyTick = true;

    // Medium frequency - staging zone doesn't move
    Interval = 0.5f;
    RandomDeviation = 0.1f;

    // Setup key filters - MANDATORY per CLAUDE.md
    StagingZoneActorKey.AddObjectFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTService_FindStagingZone, StagingZoneActorKey),
        AActor::StaticClass());

    StagingZoneLocationKey.AddVectorFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTService_FindStagingZone, StagingZoneLocationKey));
}

void UBTService_FindStagingZone::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);

    // MANDATORY: Resolve keys at runtime - without this, IsSet() returns false
    if (UBlackboardData* BBAsset = GetBlackboardAsset())
    {
        StagingZoneActorKey.ResolveSelectedKey(*BBAsset);
        StagingZoneLocationKey.ResolveSelectedKey(*BBAsset);
    }
}

void UBTService_FindStagingZone::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    AAIController* AIController = OwnerComp.GetAIOwner();
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    APawn* OwnerPawn = AIController ? AIController->GetPawn() : nullptr;

    if (!AIController || !Blackboard || !OwnerPawn)
    {
        return;
    }

    // Try perception first (preferred - bee sees flower)
    AQuidditchStagingZone* StagingZone = FindStagingZoneInPerception(AIController, OwnerPawn);

    // Fallback to world search if perception fails
    if (!StagingZone && AIController->GetWorld())
    {
        StagingZone = FindStagingZoneInWorld(AIController->GetWorld(), OwnerPawn);
    }

    if (StagingZone)
    {
        // Write staging zone actor to Blackboard
        if (StagingZoneActorKey.IsSet())
        {
            Blackboard->SetValueAsObject(StagingZoneActorKey.SelectedKeyName, StagingZone);
        }

        // Write staging zone location (the flight destination)
        if (StagingZoneLocationKey.IsSet())
        {
            Blackboard->SetValueAsVector(StagingZoneLocationKey.SelectedKeyName, StagingZone->GetActorLocation());
        }

        UE_LOG(LogBTService_FindStagingZone, Verbose,
            TEXT("[%s] Found staging zone '%s' at %s | Identifier=%s"),
            *OwnerPawn->GetName(),
            *StagingZone->GetName(),
            *StagingZone->GetActorLocation().ToString(),
            *StagingZone->GetZoneIdentifier().ToString());
    }
    else
    {
        // Log at lower frequency when not found (reduce spam)
        UE_LOG(LogBTService_FindStagingZone, Verbose,
            TEXT("[%s] No matching staging zone in perception or world"),
            *OwnerPawn->GetName());
    }
}

AQuidditchStagingZone* UBTService_FindStagingZone::FindStagingZoneInPerception(AAIController* AIController, APawn* OwnerPawn) const
{
    if (!AIController || !OwnerPawn)
    {
        return nullptr;
    }

    UAIPerceptionComponent* PerceptionComp = AIController->GetPerceptionComponent();
    if (!PerceptionComp)
    {
        return nullptr;
    }

    // Get agent's team and role for filtering (agent decides, not zone)
    int32 AgentTeam = 0;
    int32 AgentRole = 0;
    GetAgentTeamAndRole(OwnerPawn, AgentTeam, AgentRole);

    TArray<AActor*> PerceivedActors;
    PerceptionComp->GetCurrentlyPerceivedActors(nullptr, PerceivedActors);

    FVector OwnerLocation = OwnerPawn->GetActorLocation();
    AQuidditchStagingZone* ClosestZone = nullptr;
    float ClosestDistance = MaxStagingZoneRange;

    for (AActor* Actor : PerceivedActors)
    {
        if (!Actor)
        {
            continue;
        }

        // Check if actor is a staging zone (by tag - the flower broadcasts this)
        if (!Actor->ActorHasTag(TEXT("StagingZone")) && !Actor->ActorHasTag(TEXT("LandingZone")))
        {
            continue;
        }

        AQuidditchStagingZone* Zone = Cast<AQuidditchStagingZone>(Actor);
        if (!Zone)
        {
            continue;
        }

        // AGENT-SIDE FILTERING: Read zone's hints and compare to our team/role
        // The zone doesn't know about us - we decide if it's the right one
        int32 ZoneTeam = Zone->TeamHint;
        int32 ZoneRole = Zone->RoleHint;

        if (ZoneTeam != AgentTeam || ZoneRole != AgentRole)
        {
            continue;
        }

        // Found a matching zone - check distance
        float Distance = FVector::Dist(OwnerLocation, Zone->GetActorLocation());
        if (Distance < ClosestDistance)
        {
            ClosestDistance = Distance;
            ClosestZone = Zone;
        }
    }

    if (ClosestZone)
    {
        UE_LOG(LogBTService_FindStagingZone, Display,
            TEXT("[%s] PERCEIVED staging zone '%s' | AgentTeam=%d AgentRole=%d | Identifier=%s | Dist=%.0f"),
            *OwnerPawn->GetName(),
            *ClosestZone->GetName(),
            AgentTeam, AgentRole,
            *ClosestZone->GetZoneIdentifier().ToString(),
            ClosestDistance);
    }

    return ClosestZone;
}

AQuidditchStagingZone* UBTService_FindStagingZone::FindStagingZoneInWorld(UWorld* World, APawn* OwnerPawn) const
{
    if (!World || !OwnerPawn)
    {
        return nullptr;
    }

    // Get agent's team and role for filtering
    int32 AgentTeam = 0;
    int32 AgentRole = 0;
    GetAgentTeamAndRole(OwnerPawn, AgentTeam, AgentRole);

    FVector OwnerLocation = OwnerPawn->GetActorLocation();
    AQuidditchStagingZone* ClosestZone = nullptr;
    float ClosestDistance = MaxStagingZoneRange;

    // Use TActorIterator (no Kismet dependency per CLAUDE.md rules)
    for (TActorIterator<AQuidditchStagingZone> It(World); It; ++It)
    {
        AQuidditchStagingZone* Zone = *It;
        if (!Zone)
        {
            continue;
        }

        // AGENT-SIDE FILTERING: Read zone's hints
        int32 ZoneTeam = Zone->TeamHint;
        int32 ZoneRole = Zone->RoleHint;

        if (ZoneTeam != AgentTeam || ZoneRole != AgentRole)
        {
            continue;
        }

        // Found a matching zone - check distance
        float Distance = FVector::Dist(OwnerLocation, Zone->GetActorLocation());
        if (Distance < ClosestDistance)
        {
            ClosestDistance = Distance;
            ClosestZone = Zone;
        }
    }

    if (ClosestZone)
    {
        UE_LOG(LogBTService_FindStagingZone, Display,
            TEXT("[%s] FOUND staging zone in world '%s' | AgentTeam=%d AgentRole=%d | Dist=%.0f"),
            *OwnerPawn->GetName(),
            *ClosestZone->GetName(),
            AgentTeam, AgentRole,
            ClosestDistance);
    }

    return ClosestZone;
}

bool UBTService_FindStagingZone::GetAgentTeamAndRole(APawn* Pawn, int32& OutTeam, int32& OutRole) const
{
    if (!Pawn)
    {
        return false;
    }

    // Cache GameMode on first call
    if (!CachedGameMode.IsValid())
    {
        CachedGameMode = Cast<AQuidditchGameMode>(Pawn->GetWorld()->GetAuthGameMode());
    }

    if (!CachedGameMode.IsValid())
    {
        UE_LOG(LogBTService_FindStagingZone, Warning,
            TEXT("[%s] No QuidditchGameMode - cannot get team/role"),
            *Pawn->GetName());
        return false;
    }

    // Get team and role from GameMode's registry
    EQuidditchTeam Team = CachedGameMode->GetAgentTeam(Pawn);
    EQuidditchRole Role = CachedGameMode->GetAgentRole(Pawn);

    OutTeam = static_cast<int32>(Team);
    OutRole = static_cast<int32>(Role);

    return true;
}

FString UBTService_FindStagingZone::GetStaticDescription() const
{
    FString Description = TEXT("Finds staging zone via perception\n");
    Description += TEXT("Agent-side filtering by TeamHint/RoleHint\n");

    if (StagingZoneActorKey.IsSet())
    {
        Description += FString::Printf(TEXT("Actor -> %s\n"), *StagingZoneActorKey.SelectedKeyName.ToString());
    }

    if (StagingZoneLocationKey.IsSet())
    {
        Description += FString::Printf(TEXT("Location -> %s"), *StagingZoneLocationKey.SelectedKeyName.ToString());
    }

    return Description;
}
