// ============================================================================
// BTService_FindSnitch.cpp
// Developer: Marcus Daley
// Date: January 24, 2026
// Project: END2507
// ============================================================================

#include "Code/AI/Quidditch/BTService_FindSnitch.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "Perception/AIPerceptionComponent.h"
#include "GameFramework/Actor.h"
#include "EngineUtils.h"

UBTService_FindSnitch::UBTService_FindSnitch()
    : MaxSnitchRange(5000.0f)
{
    NodeName = TEXT("Find Snitch");
    bNotifyTick = true;

    // High frequency for fast-moving target
    Interval = 0.1f;
    RandomDeviation = 0.02f;

    // Setup key filters
    SnitchActorKey.AddObjectFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTService_FindSnitch, SnitchActorKey),
        AActor::StaticClass());

    SnitchLocationKey.AddVectorFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTService_FindSnitch, SnitchLocationKey));

    SnitchVelocityKey.AddVectorFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTService_FindSnitch, SnitchVelocityKey));
}

void UBTService_FindSnitch::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);

    if (UBlackboardData* BBAsset = GetBlackboardAsset())
    {
        SnitchActorKey.ResolveSelectedKey(*BBAsset);
        SnitchLocationKey.ResolveSelectedKey(*BBAsset);
        SnitchVelocityKey.ResolveSelectedKey(*BBAsset);
    }
}

void UBTService_FindSnitch::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    AAIController* AIController = OwnerComp.GetAIOwner();
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();

    if (!AIController || !Blackboard)
    {
        return;
    }

    // Try perception first (preferred method)
    AActor* Snitch = FindSnitchInPerception(AIController);

    // Fallback to world search if perception fails
    if (!Snitch && AIController->GetWorld())
    {
        Snitch = FindSnitchInWorld(AIController->GetWorld());
    }

    if (Snitch)
    {
        // Write Snitch actor to Blackboard
        if (SnitchActorKey.IsSet())
        {
            Blackboard->SetValueAsObject(SnitchActorKey.SelectedKeyName, Snitch);
        }

        // Write Snitch location
        if (SnitchLocationKey.IsSet())
        {
            Blackboard->SetValueAsVector(SnitchLocationKey.SelectedKeyName, Snitch->GetActorLocation());
        }

        // Write Snitch velocity for prediction
        if (SnitchVelocityKey.IsSet())
        {
            FVector Velocity = Snitch->GetVelocity();
            Blackboard->SetValueAsVector(SnitchVelocityKey.SelectedKeyName, Velocity);
        }
    }
    else
    {
        // Clear keys if Snitch not found
        if (SnitchActorKey.IsSet())
        {
            Blackboard->ClearValue(SnitchActorKey.SelectedKeyName);
        }
    }
}

AActor* UBTService_FindSnitch::FindSnitchInPerception(AAIController* AIController) const
{
    if (!AIController)
    {
        return nullptr;
    }

    UAIPerceptionComponent* PerceptionComp = AIController->GetPerceptionComponent();
    if (!PerceptionComp)
    {
        return nullptr;
    }

    TArray<AActor*> PerceivedActors;
    PerceptionComp->GetCurrentlyPerceivedActors(nullptr, PerceivedActors);

    APawn* OwnerPawn = AIController->GetPawn();
    FVector OwnerLocation = OwnerPawn ? OwnerPawn->GetActorLocation() : FVector::ZeroVector;

    AActor* ClosestSnitch = nullptr;
    float ClosestDistance = MaxSnitchRange;

    for (AActor* Actor : PerceivedActors)
    {
        if (!Actor)
        {
            continue;
        }

        // Check if actor is Snitch class or has Snitch tag
        bool bIsSnitch = false;

        if (SnitchClass && Actor->IsA(SnitchClass))
        {
            bIsSnitch = true;
        }
        else if (Actor->ActorHasTag(TEXT("Snitch")) || Actor->ActorHasTag(TEXT("GoldenSnitch")))
        {
            bIsSnitch = true;
        }

        if (bIsSnitch)
        {
            float Distance = FVector::Dist(OwnerLocation, Actor->GetActorLocation());
            if (Distance < ClosestDistance)
            {
                ClosestDistance = Distance;
                ClosestSnitch = Actor;
            }
        }
    }

    return ClosestSnitch;
}

AActor* UBTService_FindSnitch::FindSnitchInWorld(UWorld* World) const
{
    if (!World)
    {
        return nullptr;
    }

    // Fallback: Find by class using TActorIterator
    if (SnitchClass)
    {
        for (TActorIterator<AActor> It(World, SnitchClass); It; ++It)
        {
            return *It;
        }
    }

    // Fallback: Find by tag
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (It->ActorHasTag(TEXT("Snitch")))
        {
            return *It;
        }
    }

    return nullptr;
}

FString UBTService_FindSnitch::GetStaticDescription() const
{
    FString Description = TEXT("Finds Golden Snitch via perception\n");

    if (SnitchActorKey.IsSet())
    {
        Description += FString::Printf(TEXT("Actor → %s\n"), *SnitchActorKey.SelectedKeyName.ToString());
    }

    if (SnitchLocationKey.IsSet())
    {
        Description += FString::Printf(TEXT("Location → %s"), *SnitchLocationKey.SelectedKeyName.ToString());
    }

    return Description;
}
