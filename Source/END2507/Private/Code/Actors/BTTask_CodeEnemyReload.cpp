// BTTask_CodeEnemyReload.cpp
// Implementation of AI reload task with message-based completion
// Author: Marcus Daley
// Date: 10/08/2025


#include "Code/Actors/BTTask_CodeEnemyReload.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Code/Actors/BaseAgent.h"
#include "Code/Actors/BaseRifle.h"
#include "Code/IEnemyInterface.h"
#include "../END2507.h"
DEFINE_LOG_CATEGORY_STATIC(LogEnemyReload, Log, All);
UBTTask_CodeEnemyReload::UBTTask_CodeEnemyReload()
{
	NodeName = "Enemy Reload";
	bNotifyTick = false;
	bNotifyTaskFinished = true;

	// Default blackboard key for task completion message
	FinishedMessageKey = FName(TEXT("ActionFinished"));
}
EBTNodeResult::Type UBTTask_CodeEnemyReload::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		UE_LOG(LogEnemyReload, Error, TEXT("ExecuteTask failed — AIController is null"));
		return EBTNodeResult::Failed;
	}
	ABaseAgent* Agent = Cast<ABaseAgent>(AIController->GetPawn());
	if (!Agent)
	{
		UE_LOG(LogEnemyReload, Error, TEXT("ExecuteTask failed — Pawn is not BaseAgent"));
		return EBTNodeResult::Failed;
	}
	ABaseRifle* Rifle = Agent->GetEquippedRifle();
	if (!Rifle)
	{
		UE_LOG(LogEnemyReload, Error, TEXT("[%s] ExecuteTask failed — No rifle equipped"), *Agent->GetName());
		return EBTNodeResult::Failed;
	}
	// Check if reload is actually needed
	if (Rifle->GetCurrentAmmo() >= Rifle->GetMaxAmmo())
	{
		UE_LOG(LogEnemyReload, Warning, TEXT("[%s] Ammo already full (%d/%d) — Skipping reload"),
			*Agent->GetName(), Rifle->GetCurrentAmmo(), Rifle->GetMaxAmmo());
		return EBTNodeResult::Succeeded;
	}
	// Cache owner component for callback
	CachedOwnerComp = &OwnerComp;
	// Bind to rifle's action stopped delegate (OnRifleAttack fires when action completes)
	Rifle->OnRifleAttack.AddDynamic(this, &UBTTask_CodeEnemyReload::OnReloadFinished);
	UE_LOG(LogEnemyReload, Warning, TEXT("[%s] Bound OnRifleAttack delegate for reload completion"), *Agent->GetName());

	// Request reload through rifle
	Rifle->RequestReload();

	UE_LOG(LogEnemyReload, Warning, TEXT("[%s] Submarine torpedo reload initiated — Waiting for animation complete signal"),
		*Agent->GetName());
	Agent->EnemyReload(nullptr);
	UE_LOG(LogEnemyReload, Warning, TEXT("[%s] Submarine torpedo reload initiated — Waiting for animation complete signal"),
		*Agent->GetName());

	// Return InProgress to keep task active until message received
	return EBTNodeResult::InProgress;

}

EBTNodeResult::Type UBTTask_CodeEnemyReload::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UE_LOG(LogEnemyReload, Warning, TEXT("Reload task aborted — Cleaning up delegate bindings"));
	// Clean up delegate if task is aborted
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		if (ABaseAgent* Agent = Cast<ABaseAgent>(AIController->GetPawn()))
		{
			if (ABaseRifle* Rifle = Agent->GetEquippedRifle())
			{
				Rifle->OnRifleAttack.RemoveDynamic(this, &UBTTask_CodeEnemyReload::OnReloadFinished);
			}
		}
	}

	CachedOwnerComp = nullptr;
	return EBTNodeResult::Aborted;
}

void UBTTask_CodeEnemyReload::OnReloadFinished()
{
	if (!CachedOwnerComp)
	{
		UE_LOG(LogEnemyReload, Error, TEXT("OnReloadFinished — CachedOwnerComp is null, cannot finish task"));
		return;
	}

	UE_LOG(LogEnemyReload, Warning, TEXT("Harry Potter says: Reload spell complete — Task finishing with Success"));

	// Unbind delegate
	if (AAIController* AIController = CachedOwnerComp->GetAIOwner())
	{
		if (ABaseAgent* Agent = Cast<ABaseAgent>(AIController->GetPawn()))
		{
			if (ABaseRifle* Rifle = Agent->GetEquippedRifle())
			{
				Rifle->OnRifleAttack.RemoveDynamic(this, &UBTTask_CodeEnemyReload::OnReloadFinished);
			}
		}
	}
	UBlackboardComponent* BlackboardComp = CachedOwnerComp->GetBlackboardComponent();
	//if (BlackboardComp)
	//{
	//	BlackboardComp->SetValueAsBool(FinishedMessageKey, true);
	//	UE_LOG(LogEnemyReload, Warning, TEXT("[%s] ⚡ ActionFinished set to TRUE in Blackboard"), *Agent->GetName());
	//}
	// Finish task with success
	FinishLatentTask(*CachedOwnerComp, EBTNodeResult::Succeeded);
	CachedOwnerComp = nullptr;
}
