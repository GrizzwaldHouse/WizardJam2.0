// UBTTask_CodeEnemyAttack - Behavior tree task for enemy attack
// Author: Marcus Daley
// Date: 9/29/2025


#include "Code/Actors/BTTask_CodeEnemyAttack.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Code/IEnemyInterface.h"
#include "../END2507.h"
DEFINE_LOG_CATEGORY_STATIC(LogAttackTask, Log, All);
UBTTask_CodeEnemyAttack::UBTTask_CodeEnemyAttack()
    : ActionFinishedKeyName(TEXT("ActionFinished"))
{
	NodeName = "CodeAttack";
    bNotifyTick = true;
}
void UBTTask_CodeEnemyAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }
    // Check if action finished
    // Check if rifle fired and agent signaled completion
    bool bActionFinished = BlackboardComp->GetValueAsBool(ActionFinishedKeyName);
    if (bActionFinished)
    {
        // Reset flag for next attack
        BlackboardComp->SetValueAsBool(ActionFinishedKeyName, false);

        UE_LOG(LogAttackTask, Log, TEXT("ActionFinished detected, completing attack task"));
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
    }
}
EBTNodeResult::Type UBTTask_CodeEnemyAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        UE_LOG(LogAttackTask, Error, TEXT("No AIController"));
        return EBTNodeResult::Failed;
    }

    APawn* ControlledPawn = AIController->GetPawn();
    if (!ControlledPawn || !ControlledPawn->Implements<UEnemyInterface>())
    {
        UE_LOG(LogAttackTask, Error, TEXT("Pawn does not implement IEnemyInterface"));
        return EBTNodeResult::Failed;
    }
    // Get target actor from blackboard
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    AActor* TargetActor = BlackboardComp ? Cast<AActor>(BlackboardComp->GetValueAsObject("Player")) : nullptr;

    if (!TargetActor)
    {
        UE_LOG(LogAttackTask, Warning, TEXT("BTTask_CodeAttack: No target in blackboard"));
        return EBTNodeResult::Failed;
    }
    UE_LOG(LogAttackTask, Warning, TEXT("Target found: %s"), *TargetActor->GetName());

 
    IEnemyInterface* EnemyInterface = Cast<IEnemyInterface>(ControlledPawn);
    if (EnemyInterface)
    {
        EnemyInterface->EnemyAttack(TargetActor);
        UE_LOG(LogAttackTask, Log, TEXT("Attack executed on %s targeting %s"),
            *ControlledPawn->GetName(), *TargetActor->GetName());

        // Return InProgress - task completes when action finishes
        return EBTNodeResult::InProgress;
    }

    UE_LOG(LogAttackTask, Error, TEXT("Failed to cast to IEnemyInterface"));
    return EBTNodeResult::Failed;
}

