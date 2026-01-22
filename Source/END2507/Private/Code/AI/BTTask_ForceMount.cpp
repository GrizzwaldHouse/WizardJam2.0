// ============================================================================
// BTTask_ForceMount.cpp
// Testing utility implementation - Force mount for debugging flight
//
// Developer: Marcus Daley
// Date: January 22, 2026
// Project: Game Architecture (END2507)
// ============================================================================

#include "Code/AI/BTTask_ForceMount.h"
#include "Code/Flight/BroomActor.h"
#include "Code/Flight/AC_BroomComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"

// ============================================================================
// CONSTRUCTOR
// ============================================================================

UBTTask_ForceMount::UBTTask_ForceMount()
    : SpawnOffset(FVector(0.0f, 0.0f, 50.0f))
{
    NodeName = "Force Mount (DEBUG)";
    bNotifyTick = false;
}

// ============================================================================
// EXECUTE TASK
// ============================================================================

EBTNodeResult::Type UBTTask_ForceMount::ExecuteTask(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    APawn* AIPawn = AIController ? AIController->GetPawn() : nullptr;

    if (!AIPawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("[BTTask_ForceMount] No AI Controller or Pawn"));
        return EBTNodeResult::Failed;
    }

    // Check if AI already has broom component and is flying
    UAC_BroomComponent* ExistingBroom = AIPawn->FindComponentByClass<UAC_BroomComponent>();
    if (ExistingBroom && ExistingBroom->IsFlying())
    {
        UE_LOG(LogTemp, Display, TEXT("[BTTask_ForceMount] AI already flying!"));
        return EBTNodeResult::Succeeded;
    }

    // Need to spawn a broom and mount
    if (!BroomClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("[BTTask_ForceMount] No BroomClass set! Set in BT Details."));
        return EBTNodeResult::Failed;
    }

    UWorld* World = AIPawn->GetWorld();
    if (!World)
    {
        return EBTNodeResult::Failed;
    }

    // Spawn broom near AI
    FVector SpawnLocation = AIPawn->GetActorLocation() + SpawnOffset;
    FRotator SpawnRotation = AIPawn->GetActorRotation();

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    ABroomActor* SpawnedBroom = World->SpawnActor<ABroomActor>(BroomClass, SpawnLocation, SpawnRotation, SpawnParams);

    if (!SpawnedBroom)
    {
        UE_LOG(LogTemp, Warning, TEXT("[BTTask_ForceMount] Failed to spawn broom"));
        return EBTNodeResult::Failed;
    }

    // Force interact (mount) via the IInteractable interface
    // This triggers the same flow as player mounting
    IInteractable::Execute_OnInteract(SpawnedBroom, AIPawn);

    // Verify mount succeeded by checking if AI is now flying
    UAC_BroomComponent* BroomComp = AIPawn->FindComponentByClass<UAC_BroomComponent>();
    if (BroomComp && BroomComp->IsFlying())
    {
        UE_LOG(LogTemp, Display, TEXT("[BTTask_ForceMount] Successfully mounted AI on broom!"));

        // Update blackboard to indicate flying state
        UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
        if (Blackboard)
        {
            Blackboard->SetValueAsBool(TEXT("IsFlying"), true);
        }

        return EBTNodeResult::Succeeded;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[BTTask_ForceMount] Mount failed - AI not flying"));

        // Clean up spawned broom
        SpawnedBroom->Destroy();

        return EBTNodeResult::Failed;
    }
}

// ============================================================================
// DESCRIPTION
// ============================================================================

FString UBTTask_ForceMount::GetStaticDescription() const
{
    if (BroomClass)
    {
        return FString::Printf(TEXT("DEBUG: Force mount\nBroom: %s"), *BroomClass->GetName());
    }
    return TEXT("DEBUG: Force mount\n(No broom class set!)");
}
