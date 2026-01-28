// ============================================================================
// BTService_FindQuaffle.cpp
// Developer: Marcus Daley
// Date: January 24, 2026
// Project: END2507
// ============================================================================

#include "Code/AI/Quidditch/BTService_FindQuaffle.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "Perception/AIPerceptionComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Actor.h"
#include "Code/GameModes/QuidditchGameMode.h"

UBTService_FindQuaffle::UBTService_FindQuaffle()
    : MaxQuaffleRange(8000.0f)
{
    NodeName = TEXT("Find Quaffle");
    bNotifyTick = true;

    // Medium frequency - Quaffle moves slower than Snitch
    Interval = 0.15f;
    RandomDeviation = 0.03f;

    // Setup key filters
    QuaffleActorKey.AddObjectFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTService_FindQuaffle, QuaffleActorKey),
        AActor::StaticClass());

    QuaffleLocationKey.AddVectorFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTService_FindQuaffle, QuaffleLocationKey));

    QuaffleVelocityKey.AddVectorFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTService_FindQuaffle, QuaffleVelocityKey));

    IsQuaffleFreeKey.AddBoolFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTService_FindQuaffle, IsQuaffleFreeKey));

    TeammateHasQuaffleKey.AddBoolFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTService_FindQuaffle, TeammateHasQuaffleKey));
}

void UBTService_FindQuaffle::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);

    if (UBlackboardData* BBAsset = GetBlackboardAsset())
    {
        QuaffleActorKey.ResolveSelectedKey(*BBAsset);
        QuaffleLocationKey.ResolveSelectedKey(*BBAsset);
        QuaffleVelocityKey.ResolveSelectedKey(*BBAsset);
        IsQuaffleFreeKey.ResolveSelectedKey(*BBAsset);
        TeammateHasQuaffleKey.ResolveSelectedKey(*BBAsset);
    }
}

void UBTService_FindQuaffle::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    AAIController* AIController = OwnerComp.GetAIOwner();
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();

    if (!AIController || !Blackboard)
    {
        return;
    }

    APawn* OwnerPawn = AIController->GetPawn();
    if (!OwnerPawn)
    {
        return;
    }

    // Find Quaffle via perception or world search
    AActor* Quaffle = FindQuaffleInPerception(AIController);

    if (!Quaffle && AIController->GetWorld())
    {
        Quaffle = FindQuaffleInWorld(AIController->GetWorld());
    }

    if (Quaffle)
    {
        // Write Quaffle actor
        if (QuaffleActorKey.IsSet())
        {
            Blackboard->SetValueAsObject(QuaffleActorKey.SelectedKeyName, Quaffle);
        }

        // Write Quaffle location
        if (QuaffleLocationKey.IsSet())
        {
            Blackboard->SetValueAsVector(QuaffleLocationKey.SelectedKeyName, Quaffle->GetActorLocation());
        }

        // Write Quaffle velocity
        if (QuaffleVelocityKey.IsSet())
        {
            Blackboard->SetValueAsVector(QuaffleVelocityKey.SelectedKeyName, Quaffle->GetVelocity());
        }

        // Check possession state
        AActor* Holder = nullptr;
        bool bIsHeld = IsQuaffleHeld(Quaffle, Holder);

        if (IsQuaffleFreeKey.IsSet())
        {
            Blackboard->SetValueAsBool(IsQuaffleFreeKey.SelectedKeyName, !bIsHeld);
        }

        if (TeammateHasQuaffleKey.IsSet())
        {
            bool bTeammateHas = bIsHeld && IsHolderTeammate(Holder, OwnerPawn);
            Blackboard->SetValueAsBool(TeammateHasQuaffleKey.SelectedKeyName, bTeammateHas);
        }
    }
    else
    {
        // Clear keys if Quaffle not found
        if (QuaffleActorKey.IsSet())
        {
            Blackboard->ClearValue(QuaffleActorKey.SelectedKeyName);
        }

        if (IsQuaffleFreeKey.IsSet())
        {
            Blackboard->SetValueAsBool(IsQuaffleFreeKey.SelectedKeyName, false);
        }

        if (TeammateHasQuaffleKey.IsSet())
        {
            Blackboard->SetValueAsBool(TeammateHasQuaffleKey.SelectedKeyName, false);
        }
    }
}

AActor* UBTService_FindQuaffle::FindQuaffleInPerception(AAIController* AIController) const
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

    AActor* ClosestQuaffle = nullptr;
    float ClosestDistance = MaxQuaffleRange;

    for (AActor* Actor : PerceivedActors)
    {
        if (!Actor)
        {
            continue;
        }

        // Check if actor is Quaffle
        bool bIsQuaffle = false;

        if (QuaffleClass && Actor->IsA(QuaffleClass))
        {
            bIsQuaffle = true;
        }
        else if (Actor->ActorHasTag(TEXT("Quaffle")))
        {
            bIsQuaffle = true;
        }

        if (bIsQuaffle)
        {
            float Distance = FVector::Dist(OwnerLocation, Actor->GetActorLocation());
            if (Distance < ClosestDistance)
            {
                ClosestDistance = Distance;
                ClosestQuaffle = Actor;
            }
        }
    }

    return ClosestQuaffle;
}

AActor* UBTService_FindQuaffle::FindQuaffleInWorld(UWorld* World) const
{
    if (!World)
    {
        return nullptr;
    }

    // Find by class
    if (QuaffleClass)
    {
        TArray<AActor*> FoundActors;
        UGameplayStatics::GetAllActorsOfClass(World, QuaffleClass, FoundActors);

        if (FoundActors.Num() > 0)
        {
            return FoundActors[0];
        }
    }

    // Find by tag
    TArray<AActor*> TaggedActors;
    UGameplayStatics::GetAllActorsWithTag(World, TEXT("Quaffle"), TaggedActors);

    if (TaggedActors.Num() > 0)
    {
        return TaggedActors[0];
    }

    return nullptr;
}

bool UBTService_FindQuaffle::IsQuaffleHeld(AActor* Quaffle, AActor*& OutHolder) const
{
    if (!Quaffle)
    {
        return false;
    }

    // Check if Quaffle is attached to another actor
    AActor* Parent = Quaffle->GetAttachParentActor();
    if (Parent)
    {
        OutHolder = Parent;
        return true;
    }

    // Alternative: Check for "Held" tag
    if (Quaffle->ActorHasTag(TEXT("Held")))
    {
        // Try to get holder from Quaffle's owner
        OutHolder = Quaffle->GetOwner();
        return true;
    }

    OutHolder = nullptr;
    return false;
}

bool UBTService_FindQuaffle::IsHolderTeammate(AActor* Holder, APawn* OwnerPawn) const
{
    if (!Holder || !OwnerPawn)
    {
        return false;
    }

    APawn* HolderPawn = Cast<APawn>(Holder);
    if (!HolderPawn)
    {
        return false;
    }

    UWorld* World = OwnerPawn->GetWorld();
    if (!World)
    {
        return false;
    }

    AQuidditchGameMode* GameMode = Cast<AQuidditchGameMode>(World->GetAuthGameMode());
    if (!GameMode)
    {
        return false;
    }

    EQuidditchTeam OwnerTeam = GameMode->GetAgentTeam(OwnerPawn);
    EQuidditchTeam HolderTeam = GameMode->GetAgentTeam(HolderPawn);

    return (OwnerTeam != EQuidditchTeam::None && OwnerTeam == HolderTeam);
}

FString UBTService_FindQuaffle::GetStaticDescription() const
{
    FString Description = TEXT("Finds Quaffle via perception\n");

    if (QuaffleActorKey.IsSet())
    {
        Description += FString::Printf(TEXT("Actor → %s\n"), *QuaffleActorKey.SelectedKeyName.ToString());
    }

    if (QuaffleLocationKey.IsSet())
    {
        Description += FString::Printf(TEXT("Location → %s\n"), *QuaffleLocationKey.SelectedKeyName.ToString());
    }

    if (IsQuaffleFreeKey.IsSet())
    {
        Description += FString::Printf(TEXT("IsFree → %s"), *IsQuaffleFreeKey.SelectedKeyName.ToString());
    }

    return Description;
}

