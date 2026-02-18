
// BTTask_CodeEnemyReload.h
// Behavior tree task for AI agent reload functionality
// Waits for reload animation and ammo replenishment via message system
// Author: Marcus Daley
// Date: 10/08/2025

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_CodeEnemyReload.generated.h"

UCLASS()
class END2507_API UBTTask_CodeEnemyReload : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_CodeEnemyReload();

protected:
	// Executes the reload task on the AI agent
	
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	// Handles abort request during task execution

	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
	// Blackboard key name for reload finished message

	UPROPERTY(EditDefaultsOnly, Category = "Blackboard", meta = (AllowPrivateAccess = "true"))
	FName FinishedMessageKey;
	// Cached reference to behavior tree component for message handling
	UBehaviorTreeComponent* CachedOwnerComp;
	// Delegate handler for reload completion
	// Called when rifle broadcasts OnReloadNow or animation completes
	UFUNCTION()
	void OnReloadFinished();
};
