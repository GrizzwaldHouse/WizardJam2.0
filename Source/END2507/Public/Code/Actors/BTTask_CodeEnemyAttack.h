// UBTTask_CodeEnemyAttack - Behavior tree task for enemy attack
// Author: Marcus Daley
// Date: 9/29/2025
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_CodeEnemyAttack.generated.h"
class AAIController;

UCLASS()
class END2507_API UBTTask_CodeEnemyAttack : public UBTTaskNode
{
	GENERATED_BODY()
public:
    UBTTask_CodeEnemyAttack();

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
   

protected:
   
    virtual void  TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)override;


    
};
