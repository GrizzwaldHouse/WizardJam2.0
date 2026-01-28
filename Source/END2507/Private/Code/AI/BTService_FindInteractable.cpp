// ============================================================================
// BTService_FindInteractable.cpp
// Developer: Marcus Daley
// Date: January 22, 2026
// Project: END2507
//
// KEY IMPLEMENTATION NOTES:
// 1. Added AddObjectFilter() in constructor for FBlackboardKeySelector
// 2. Added InitializeFromAsset() override with ResolveSelectedKey() call
// Without these, OutputKey.IsSet() returns false even when configured!
//
// This service scans for IInteractable interface, not collectible class hierarchy.
// Used for finding BroomActor (BP_Broom_C) which is an interactable, not a collectible.
// ============================================================================

#include "Code/AI/BTService_FindInteractable.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "Perception/AIPerceptionComponent.h"
#include "Code/Interfaces/Interactable.h"
#include "Code/Flight/BroomActor.h"
#include "Code/Flight/AC_BroomComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogFindInteractable, Log, All);

UBTService_FindInteractable::UBTService_FindInteractable()
    : InteractableClass(nullptr)
    , MaxSearchDistance(0.0f)
    , bRequireCanInteract(false)
{
    NodeName = "Find Interactable";
    Interval = 0.5f;
    RandomDeviation = 0.1f;

    // CRITICAL FIX #1: Add object filter so the editor knows what key types are valid
    // Without this, the key dropdown shows options but IsSet() returns false at runtime
    OutputKey.AddObjectFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTService_FindInteractable, OutputKey),
        AActor::StaticClass());
}

void UBTService_FindInteractable::InitializeFromAsset(UBehaviorTree& Asset)
{
    // Always call parent first
    Super::InitializeFromAsset(Asset);

    // CRITICAL FIX #2: Resolve the key selector against the blackboard asset
    // The editor stores a string key name, but at runtime we need to bind it
    // to the actual blackboard slot. Without this, IsSet() returns false!
    if (UBlackboardData* BBAsset = GetBlackboardAsset())
    {
        OutputKey.ResolveSelectedKey(*BBAsset);

        UE_LOG(LogFindInteractable, Log,
            TEXT("[FindInteractable] Resolved OutputKey '%s' against blackboard '%s'"),
            *OutputKey.SelectedKeyName.ToString(),
            *BBAsset->GetName());
    }
}

void UBTService_FindInteractable::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    AAIController* AIC = OwnerComp.GetAIOwner();
    if (!AIC)
    {
        UE_LOG(LogFindInteractable, Warning, TEXT("[FindInteractable] No AIController!"));
        return;
    }

    APawn* Pawn = AIC->GetPawn();
    if (!Pawn)
    {
        UE_LOG(LogFindInteractable, Warning, TEXT("[FindInteractable] No Pawn!"));
        return;
    }

    // Skip searching for interactables (like brooms) if the agent is already flying
    // This prevents the AI from looking for brooms when it already has one
    UAC_BroomComponent* BroomComp = Pawn->FindComponentByClass<UAC_BroomComponent>();
    if (BroomComp && BroomComp->IsFlying())
    {
        // Agent is flying - clear the output key since we don't need another interactable
        UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
        if (BB && OutputKey.IsSet())
        {
            BB->SetValueAsObject(OutputKey.SelectedKeyName, nullptr);
        }
        return;
    }

    UAIPerceptionComponent* Perception = AIC->GetPerceptionComponent();
    if (!Perception)
    {
        UE_LOG(LogFindInteractable, Warning, TEXT("[FindInteractable] No PerceptionComponent!"));
        return;
    }

    // Get all actors currently perceived by this AI
    TArray<AActor*> Perceived;
    Perception->GetCurrentlyPerceivedActors(nullptr, Perceived);

    // Find the nearest valid interactable
    AActor* Nearest = nullptr;
    float NearestDist = FLT_MAX;
    FVector PawnLocation = Pawn->GetActorLocation();

    for (AActor* Actor : Perceived)
    {
        if (!Actor) continue;

        // Check if actor implements IInteractable interface
        if (!Actor->Implements<UInteractable>())
        {
            continue;
        }

        // Class type filter (if specified)
        if (InteractableClass && !Actor->IsA(InteractableClass))
        {
            continue;
        }

        // CanInteract filter (if enabled)
        if (bRequireCanInteract)
        {
            bool bCanInteract = IInteractable::Execute_CanInteract(Actor);
            if (!bCanInteract)
            {
                continue;
            }
        }

        // Skip brooms that are already being ridden (prevents spam logging mounted brooms)
        if (ABroomActor* Broom = Cast<ABroomActor>(Actor))
        {
            if (Broom->IsBeingRidden())
            {
                continue;
            }
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
        UE_LOG(LogFindInteractable, Warning, TEXT("[FindInteractable] No BlackboardComponent!"));
        return;
    }

    // Validate the output key is properly configured
    if (!OutputKey.IsSet())
    {
        UE_LOG(LogFindInteractable, Error,
            TEXT("[FindInteractable] OutputKey not set! Full rebuild required after code changes."));
        return;
    }

    // Write the result to blackboard
    BB->SetValueAsObject(OutputKey.SelectedKeyName, Nearest);

    // Log the result
    if (Nearest)
    {
        UE_LOG(LogFindInteractable, Display,
            TEXT("[FindInteractable] %s -> Found %s at %.0f units, wrote to '%s'"),
            *Pawn->GetName(),
            *Nearest->GetName(),
            NearestDist,
            *OutputKey.SelectedKeyName.ToString());
    }
    else
    {
        UE_LOG(LogFindInteractable, Verbose,
            TEXT("[FindInteractable] %s -> No interactable found (perceived: %d)"),
            *Pawn->GetName(),
            Perceived.Num());
    }
}

FString UBTService_FindInteractable::GetStaticDescription() const
{
    FString ClassFilter = InteractableClass ? InteractableClass->GetName() : TEXT("Any IInteractable");
    FString KeyTarget = OutputKey.IsSet() ? OutputKey.SelectedKeyName.ToString() : TEXT("NOT SET!");

    FString CanInteractFilter = bRequireCanInteract ? TEXT(" (CanInteract=true)") : TEXT("");

    return FString::Printf(TEXT("Find %s%s -> %s"), *ClassFilter, *CanInteractFilter, *KeyTarget);
}
