// ============================================================================
// BTTask_Interact.cpp
// Implementation of modular AI interaction task
//
// Developer: Marcus Daley
// Date: January 22, 2026
// Project: Game Architecture (END2507)
// ============================================================================

#include "Code/AI/BTTask_Interact.h"
#include "Code/Interfaces/Interactable.h"
#include "Code/Utilities/AC_SpellCollectionComponent.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "GameFramework/Character.h"

// ============================================================================
// CONSTRUCTOR
// ============================================================================

UBTTask_Interact::UBTTask_Interact()
    : InteractionRange(200.0f)
    , RequiredChannel(NAME_None)
    , bClearTargetOnSuccess(true)
{
    NodeName = "Interact With Target";
    bNotifyTick = true;

    // Key filter for target - must be an object
    TargetKey.AddObjectFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTTask_Interact, TargetKey),
        AActor::StaticClass());

    // Success state key filter - boolean
    SuccessStateKey.AddBoolFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTTask_Interact, SuccessStateKey));
}

// ============================================================================
// INITIALIZE FROM ASSET
// ============================================================================

void UBTTask_Interact::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);

    if (UBlackboardData* BBAsset = GetBlackboardAsset())
    {
        TargetKey.ResolveSelectedKey(*BBAsset);
        SuccessStateKey.ResolveSelectedKey(*BBAsset);
    }
}

// ============================================================================
// EXECUTE TASK
// ============================================================================

EBTNodeResult::Type UBTTask_Interact::ExecuteTask(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    APawn* AIPawn = AIController ? AIController->GetPawn() : nullptr;

    if (!AIPawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("[BTTask_Interact] No AI Controller or Pawn"));
        return EBTNodeResult::Failed;
    }

    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    if (!Blackboard || !TargetKey.IsSet())
    {
        UE_LOG(LogTemp, Warning, TEXT("[BTTask_Interact] No Blackboard or TargetKey not set"));
        return EBTNodeResult::Failed;
    }

    // Get target actor from blackboard
    UObject* TargetObject = Blackboard->GetValueAsObject(TargetKey.SelectedKeyName);
    AActor* TargetActor = Cast<AActor>(TargetObject);

    if (!TargetActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("[BTTask_Interact] No target actor in Blackboard"));
        return EBTNodeResult::Failed;
    }

    // Check if target implements IInteractable
    if (!TargetActor->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
    {
        UE_LOG(LogTemp, Warning, TEXT("[BTTask_Interact] Target %s doesn't implement IInteractable"),
            *TargetActor->GetName());
        return EBTNodeResult::Failed;
    }

    // Check required channel
    if (!HasRequiredChannel(AIPawn))
    {
        UE_LOG(LogTemp, Warning, TEXT("[BTTask_Interact] AI lacks required channel: %s"),
            *RequiredChannel.ToString());
        return EBTNodeResult::Failed;
    }

    UE_LOG(LogTemp, Display, TEXT("[BTTask_Interact] Starting interaction with %s"),
        *TargetActor->GetName());

    // Continue ticking until we're in range and can interact
    return EBTNodeResult::InProgress;
}

// ============================================================================
// TICK TASK
// ============================================================================

void UBTTask_Interact::TickTask(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory,
    float DeltaSeconds)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    APawn* AIPawn = AIController ? AIController->GetPawn() : nullptr;

    if (!AIPawn)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    UObject* TargetObject = Blackboard ? Blackboard->GetValueAsObject(TargetKey.SelectedKeyName) : nullptr;
    AActor* TargetActor = Cast<AActor>(TargetObject);

    if (!TargetActor)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    // Check distance to target
    float Distance = FVector::Dist(AIPawn->GetActorLocation(), TargetActor->GetActorLocation());

    if (Distance <= InteractionRange)
    {
        // In range - try to interact
        if (TryInteract(OwnerComp, TargetActor))
        {
            // Success - optionally clear target
            if (bClearTargetOnSuccess && Blackboard)
            {
                Blackboard->ClearValue(TargetKey.SelectedKeyName);
            }

            // Optionally set success state key
            if (SuccessStateKey.IsSet() && Blackboard)
            {
                Blackboard->SetValueAsBool(SuccessStateKey.SelectedKeyName, true);
            }

            UE_LOG(LogTemp, Display, TEXT("[BTTask_Interact] Successfully interacted with %s"),
                *TargetActor->GetName());

            FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        }
        else
        {
            // Can't interact yet - keep trying
        }
    }
    else
    {
        // Move toward target
        FVector Direction = (TargetActor->GetActorLocation() - AIPawn->GetActorLocation()).GetSafeNormal();
        AIPawn->AddMovementInput(Direction, 1.0f);
    }
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

bool UBTTask_Interact::TryInteract(UBehaviorTreeComponent& OwnerComp, AActor* Target)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    APawn* AIPawn = AIController ? AIController->GetPawn() : nullptr;

    if (!AIPawn || !Target)
    {
        return false;
    }

    // Check if target can be interacted with
    bool bCanInteract = IInteractable::Execute_CanInteract(Target);
    if (!bCanInteract)
    {
        return false;
    }

    // Perform interaction
    IInteractable::Execute_OnInteract(Target, AIPawn);

    return true;
}

bool UBTTask_Interact::HasRequiredChannel(APawn* Pawn) const
{
    // If no channel required, always pass
    if (RequiredChannel == NAME_None)
    {
        return true;
    }

    // Check if pawn has the required channel via SpellCollectionComponent
    // (Channel system stores collected abilities/items like Broom access)
    UAC_SpellCollectionComponent* SpellComp = Pawn->FindComponentByClass<UAC_SpellCollectionComponent>();
    if (SpellComp)
    {
        return SpellComp->HasChannel(RequiredChannel);
    }

    return false;
}

// ============================================================================
// DESCRIPTION
// ============================================================================

FString UBTTask_Interact::GetStaticDescription() const
{
    FString Description = FString::Printf(
        TEXT("Interact with: %s\nRange: %.0f"),
        TargetKey.IsSet() ? *TargetKey.SelectedKeyName.ToString() : TEXT("(not set)"),
        InteractionRange
    );

    if (RequiredChannel != NAME_None)
    {
        Description += FString::Printf(TEXT("\nRequires channel: %s"), *RequiredChannel.ToString());
    }

    if (bClearTargetOnSuccess)
    {
        Description += TEXT("\nClears target on success");
    }

    return Description;
}
