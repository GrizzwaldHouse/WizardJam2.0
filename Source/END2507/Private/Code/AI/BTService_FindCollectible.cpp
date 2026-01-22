// ============================================================================
// BTService_FindCollectible.cpp
// Developer: Marcus Daley
// Date: January 21, 2026
// Project: END2507
//
// KEY FIXES APPLIED:
// 1. Added AddObjectFilter() in constructor for FBlackboardKeySelector
// 2. Added InitializeFromAsset() override with ResolveSelectedKey() call
// Without these, OutputKey.IsSet() returns false even when configured!
// ============================================================================

#include "Code/AI/BTService_FindCollectible.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "Perception/AIPerceptionComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogFindCollectible, Log, All);

UBTService_FindCollectible::UBTService_FindCollectible()
    : CollectibleClass(nullptr)
    , RequiredGrantChannel(NAME_None)
    , MaxSearchDistance(0.0f)
{
    NodeName = "Find Collectible";
    Interval = 0.5f;
    RandomDeviation = 0.1f;

    // CRITICAL FIX #1: Add object filter so the editor knows what key types are valid
    // Without this, the key dropdown shows options but IsSet() returns false at runtime
    OutputKey.AddObjectFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTService_FindCollectible, OutputKey),
        AActor::StaticClass());
}

void UBTService_FindCollectible::InitializeFromAsset(UBehaviorTree& Asset)
{
    // Always call parent first
    Super::InitializeFromAsset(Asset);

    // CRITICAL FIX #2: Resolve the key selector against the blackboard asset
    // The editor stores a string key name, but at runtime we need to bind it
    // to the actual blackboard slot. Without this, IsSet() returns false!
    if (UBlackboardData* BBAsset = GetBlackboardAsset())
    {
        OutputKey.ResolveSelectedKey(*BBAsset);

        UE_LOG(LogFindCollectible, Log,
            TEXT("[FindCollectible] Resolved OutputKey '%s' against blackboard '%s'"),
            *OutputKey.SelectedKeyName.ToString(),
            *BBAsset->GetName());
    }
}

void UBTService_FindCollectible::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    AAIController* AIC = OwnerComp.GetAIOwner();
    if (!AIC)
    {
        UE_LOG(LogFindCollectible, Warning, TEXT("[FindCollectible] No AIController!"));
        return;
    }

    APawn* Pawn = AIC->GetPawn();
    if (!Pawn)
    {
        UE_LOG(LogFindCollectible, Warning, TEXT("[FindCollectible] No Pawn!"));
        return;
    }

    UAIPerceptionComponent* Perception = AIC->GetPerceptionComponent();
    if (!Perception)
    {
        UE_LOG(LogFindCollectible, Warning, TEXT("[FindCollectible] No PerceptionComponent!"));
        return;
    }

    // Get all actors currently perceived by this AI
    TArray<AActor*> Perceived;
    Perception->GetCurrentlyPerceivedActors(nullptr, Perceived);

    // Find the nearest valid collectible
    AActor* Nearest = nullptr;
    float NearestDist = FLT_MAX;
    FVector PawnLocation = Pawn->GetActorLocation();

    for (AActor* Actor : Perceived)
    {
        if (!Actor) continue;

        // Class type filter
        if (CollectibleClass && !Actor->IsA(CollectibleClass))
        {
            continue;
        }

        // Distance filter
        float Distance = FVector::Dist(PawnLocation, Actor->GetActorLocation());
        if (MaxSearchDistance > 0.0f && Distance > MaxSearchDistance)
        {
            continue;
        }

        // Track nearest
        if (Distance < NearestDist)
        {
            NearestDist = Distance;
            Nearest = Actor;
        }
    }

    // Get blackboard to write result
    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB)
    {
        UE_LOG(LogFindCollectible, Warning, TEXT("[FindCollectible] No BlackboardComponent!"));
        return;
    }

    // Validate the output key is properly configured
    if (!OutputKey.IsSet())
    {
        UE_LOG(LogFindCollectible, Error,
            TEXT("[FindCollectible] OutputKey not set! Full rebuild required after code changes."));
        return;
    }

    // Write the result to blackboard
    BB->SetValueAsObject(OutputKey.SelectedKeyName, Nearest);

    // Log the result
    if (Nearest)
    {
        UE_LOG(LogFindCollectible, Display,
            TEXT("[FindCollectible] %s -> Found %s at %.0f units, wrote to '%s'"),
            *Pawn->GetName(),
            *Nearest->GetName(),
            NearestDist,
            *OutputKey.SelectedKeyName.ToString());
    }
    else
    {
        UE_LOG(LogFindCollectible, Verbose,
            TEXT("[FindCollectible] %s -> No collectible found (perceived: %d)"),
            *Pawn->GetName(),
            Perceived.Num());
    }
}

FString UBTService_FindCollectible::GetStaticDescription() const
{
    FString ClassFilter = CollectibleClass ? CollectibleClass->GetName() : TEXT("Any");
    FString KeyTarget = OutputKey.IsSet() ? OutputKey.SelectedKeyName.ToString() : TEXT("NOT SET!");

    return FString::Printf(TEXT("Find %s -> %s"), *ClassFilter, *KeyTarget);
}
