// ============================================================================
// BTService_FindBludger.cpp
// Developer: Marcus Daley
// Date: January 24, 2026
// Project: END2507
// ============================================================================
// Algorithm: MsPacManAgent.java INVERTED safety scoring
//
// Original MsPacMan: Find safest path AWAY from ghosts by scoring paths
// Inverted for Beater: Find Bludgers TO intercept by scoring threat levels
//
// Threat Score = 1 / Distance(Bludger, Teammate)
// Higher score = more urgent interception needed
// ============================================================================

#include "Code/AI/Quidditch/BTService_FindBludger.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "Perception/AIPerceptionComponent.h"
#include "GameFramework/Actor.h"
#include "EngineUtils.h"
#include "Code/GameModes/QuidditchGameMode.h"

UBTService_FindBludger::UBTService_FindBludger()
    : MaxBludgerRange(6000.0f)
    , ThreatRadius(1500.0f)
    , EnemyPriorityRadius(2000.0f)
{
    NodeName = TEXT("Find Bludger");
    bNotifyTick = true;

    // Medium-high frequency - Bludgers move fast
    Interval = 0.12f;
    RandomDeviation = 0.02f;

    // Setup key filters
    NearestBludgerKey.AddObjectFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTService_FindBludger, NearestBludgerKey),
        AActor::StaticClass());

    BludgerLocationKey.AddVectorFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTService_FindBludger, BludgerLocationKey));

    BludgerVelocityKey.AddVectorFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTService_FindBludger, BludgerVelocityKey));

    ThreateningBludgerKey.AddObjectFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTService_FindBludger, ThreateningBludgerKey),
        AActor::StaticClass());

    ThreatenedTeammateKey.AddObjectFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTService_FindBludger, ThreatenedTeammateKey),
        APawn::StaticClass());

    BestEnemyTargetKey.AddObjectFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTService_FindBludger, BestEnemyTargetKey),
        APawn::StaticClass());
}

void UBTService_FindBludger::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);

    if (UBlackboardData* BBAsset = GetBlackboardAsset())
    {
        NearestBludgerKey.ResolveSelectedKey(*BBAsset);
        BludgerLocationKey.ResolveSelectedKey(*BBAsset);
        BludgerVelocityKey.ResolveSelectedKey(*BBAsset);
        ThreateningBludgerKey.ResolveSelectedKey(*BBAsset);
        ThreatenedTeammateKey.ResolveSelectedKey(*BBAsset);
        BestEnemyTargetKey.ResolveSelectedKey(*BBAsset);
    }
}

void UBTService_FindBludger::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    AAIController* AIController = OwnerComp.GetAIOwner();
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();

    if (!AIController || !Blackboard)
    {
        return;
    }

    APawn* OwnerPawn = AIController->GetPawn();
    UWorld* World = AIController->GetWorld();
    if (!OwnerPawn || !World)
    {
        return;
    }

    // Find all Bludgers
    TArray<AActor*> Bludgers = FindBludgersInPerception(AIController);
    if (Bludgers.Num() == 0)
    {
        Bludgers = FindBludgersInWorld(World);
    }

    FVector OwnerLocation = OwnerPawn->GetActorLocation();

    if (Bludgers.Num() > 0)
    {
        // Find nearest Bludger
        AActor* NearestBludger = nullptr;
        float NearestDistance = MaxBludgerRange;

        for (AActor* Bludger : Bludgers)
        {
            float Distance = FVector::Dist(OwnerLocation, Bludger->GetActorLocation());
            if (Distance < NearestDistance)
            {
                NearestDistance = Distance;
                NearestBludger = Bludger;
            }
        }

        if (NearestBludger)
        {
            if (NearestBludgerKey.IsSet())
            {
                Blackboard->SetValueAsObject(NearestBludgerKey.SelectedKeyName, NearestBludger);
            }

            if (BludgerLocationKey.IsSet())
            {
                Blackboard->SetValueAsVector(BludgerLocationKey.SelectedKeyName,
                    NearestBludger->GetActorLocation());
            }

            if (BludgerVelocityKey.IsSet())
            {
                Blackboard->SetValueAsVector(BludgerVelocityKey.SelectedKeyName,
                    NearestBludger->GetVelocity());
            }
        }

        // Find most threatening Bludger (MsPacMan INVERTED algorithm)
        APawn* ThreatenedTeammate = nullptr;
        AActor* ThreateningBludger = FindMostThreateningBludger(Bludgers, OwnerPawn, World, ThreatenedTeammate);

        if (ThreateningBludgerKey.IsSet())
        {
            if (ThreateningBludger)
            {
                Blackboard->SetValueAsObject(ThreateningBludgerKey.SelectedKeyName, ThreateningBludger);
            }
            else
            {
                Blackboard->ClearValue(ThreateningBludgerKey.SelectedKeyName);
            }
        }

        if (ThreatenedTeammateKey.IsSet())
        {
            if (ThreatenedTeammate)
            {
                Blackboard->SetValueAsObject(ThreatenedTeammateKey.SelectedKeyName, ThreatenedTeammate);
            }
            else
            {
                Blackboard->ClearValue(ThreatenedTeammateKey.SelectedKeyName);
            }
        }
    }
    else
    {
        // Clear Bludger keys
        if (NearestBludgerKey.IsSet())
        {
            Blackboard->ClearValue(NearestBludgerKey.SelectedKeyName);
        }
        if (ThreateningBludgerKey.IsSet())
        {
            Blackboard->ClearValue(ThreateningBludgerKey.SelectedKeyName);
        }
    }

    // Always update best enemy target for offensive play
    if (BestEnemyTargetKey.IsSet())
    {
        APawn* BestEnemy = FindBestEnemyTarget(OwnerPawn, World);
        if (BestEnemy)
        {
            Blackboard->SetValueAsObject(BestEnemyTargetKey.SelectedKeyName, BestEnemy);
        }
        else
        {
            Blackboard->ClearValue(BestEnemyTargetKey.SelectedKeyName);
        }
    }
}

TArray<AActor*> UBTService_FindBludger::FindBludgersInPerception(AAIController* AIController) const
{
    TArray<AActor*> Bludgers;

    if (!AIController)
    {
        return Bludgers;
    }

    UAIPerceptionComponent* PerceptionComp = AIController->GetPerceptionComponent();
    if (!PerceptionComp)
    {
        return Bludgers;
    }

    TArray<AActor*> PerceivedActors;
    PerceptionComp->GetCurrentlyPerceivedActors(nullptr, PerceivedActors);

    for (AActor* Actor : PerceivedActors)
    {
        if (!Actor)
        {
            continue;
        }

        bool bIsBludger = false;

        if (BludgerClass && Actor->IsA(BludgerClass))
        {
            bIsBludger = true;
        }
        else if (Actor->ActorHasTag(TEXT("Bludger")))
        {
            bIsBludger = true;
        }

        if (bIsBludger)
        {
            Bludgers.Add(Actor);
        }
    }

    return Bludgers;
}

TArray<AActor*> UBTService_FindBludger::FindBludgersInWorld(UWorld* World) const
{
    TArray<AActor*> Bludgers;

    if (!World)
    {
        return Bludgers;
    }

    if (BludgerClass)
    {
        for (TActorIterator<AActor> It(World, BludgerClass); It; ++It)
        {
            Bludgers.Add(*It);
        }
    }

    if (Bludgers.Num() == 0)
    {
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            if (It->ActorHasTag(TEXT("Bludger")))
            {
                Bludgers.Add(*It);
            }
        }
    }

    return Bludgers;
}

AQuidditchGameMode* UBTService_FindBludger::GetGameMode(UWorld* World)
{
    if (!CachedGameMode.IsValid() && World)
    {
        CachedGameMode = Cast<AQuidditchGameMode>(World->GetAuthGameMode());
    }
    return CachedGameMode.Get();
}

AActor* UBTService_FindBludger::FindMostThreateningBludger(
    const TArray<AActor*>& Bludgers,
    APawn* OwnerPawn,
    UWorld* World,
    APawn*& OutThreatenedTeammate)
{
    // MsPacMan INVERTED: Instead of finding safest path, find highest threat
    // Threat Score = 1 / Distance(Bludger, Teammate)
    // We want the Bludger closest to any teammate

    OutThreatenedTeammate = nullptr;

    if (Bludgers.Num() == 0 || !OwnerPawn || !World)
    {
        return nullptr;
    }

    AQuidditchGameMode* GameMode = GetGameMode(World);
    if (!GameMode)
    {
        return nullptr;
    }

    EQuidditchTeam OwnerTeam = GameMode->GetAgentTeam(OwnerPawn);
    if (OwnerTeam == EQuidditchTeam::None)
    {
        return nullptr;
    }

    AActor* MostThreateningBludger = nullptr;
    float HighestThreatScore = 0.0f;

    // For each Bludger, check distance to all teammates
    for (AActor* Bludger : Bludgers)
    {
        if (!Bludger)
        {
            continue;
        }

        FVector BludgerPos = Bludger->GetActorLocation();

        // Check against all teammates
        for (TActorIterator<APawn> It(World); It; ++It)
        {
            APawn* Teammate = *It;
            if (Teammate == OwnerPawn)
            {
                continue;
            }

            if (GameMode->GetAgentTeam(Teammate) != OwnerTeam)
            {
                continue;
            }

            float Distance = FVector::Dist(BludgerPos, Teammate->GetActorLocation());

            // Only consider if within threat radius
            if (Distance < ThreatRadius)
            {
                // MsPacMan INVERTED: Threat score is inverse of distance
                float ThreatScore = 1.0f / FMath::Max(Distance, 1.0f);

                if (ThreatScore > HighestThreatScore)
                {
                    HighestThreatScore = ThreatScore;
                    MostThreateningBludger = Bludger;
                    OutThreatenedTeammate = Teammate;
                }
            }
        }
    }

    return MostThreateningBludger;
}

APawn* UBTService_FindBludger::FindBestEnemyTarget(APawn* OwnerPawn, UWorld* World)
{
    if (!OwnerPawn || !World)
    {
        return nullptr;
    }

    AQuidditchGameMode* GameMode = GetGameMode(World);
    if (!GameMode)
    {
        return nullptr;
    }

    EQuidditchTeam OwnerTeam = GameMode->GetAgentTeam(OwnerPawn);
    if (OwnerTeam == EQuidditchTeam::None)
    {
        return nullptr;
    }

    FVector OwnerLocation = OwnerPawn->GetActorLocation();
    APawn* BestEnemy = nullptr;
    float BestScore = 0.0f;

    for (TActorIterator<APawn> It(World); It; ++It)
    {
        APawn* Enemy = *It;
        EQuidditchTeam EnemyTeam = GameMode->GetAgentTeam(Enemy);

        // Must be enemy team
        if (EnemyTeam == EQuidditchTeam::None || EnemyTeam == OwnerTeam)
        {
            continue;
        }

        float Distance = FVector::Dist(OwnerLocation, Enemy->GetActorLocation());

        // Score based on proximity (closer = better target)
        // Also consider role priority (could extend to prioritize Seeker)
        float Score = 1.0f / FMath::Max(Distance, 1.0f);

        // Bonus for enemies within priority radius
        if (Distance < EnemyPriorityRadius)
        {
            Score *= 2.0f;
        }

        if (Score > BestScore)
        {
            BestScore = Score;
            BestEnemy = Enemy;
        }
    }

    return BestEnemy;
}

FString UBTService_FindBludger::GetStaticDescription() const
{
    FString Description = TEXT("Find Bludgers (MsPacMan INVERTED)\n");

    if (NearestBludgerKey.IsSet())
    {
        Description += FString::Printf(TEXT("Nearest → %s\n"), *NearestBludgerKey.SelectedKeyName.ToString());
    }

    if (ThreateningBludgerKey.IsSet())
    {
        Description += FString::Printf(TEXT("Threat → %s\n"), *ThreateningBludgerKey.SelectedKeyName.ToString());
    }

    Description += FString::Printf(TEXT("Threat Radius: %.0f"), ThreatRadius);

    return Description;
}

