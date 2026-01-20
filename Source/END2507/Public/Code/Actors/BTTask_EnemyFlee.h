// BTTask_EnemyFlee.h
// Behavior Tree task for fleeing from player when health is low
// Author: [Marcus Daley]
// Date: 9/30/2025
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_EnemyFlee.generated.h"
// Forward declarations
class UBlackboardComponent;
class AAIController;
class UEnvQuery;
DECLARE_LOG_CATEGORY_EXTERN(LogFleeBehavior, Log, All);
/**
 *  Task that makes the AI flee away from the player when health is low
 */

UCLASS()
class END2507_API UBTTask_EnemyFlee : public UBTTaskNode
{
	GENERATED_BODY()
public:
 UBTTask_EnemyFlee();

protected:
	virtual EBTNodeResult::Type  ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	UEnvQuery* FleeLocationQuery;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	FName FleeLocationKey;

	// Search radius for flee points
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	float FleeRadius;
private:
	//()
	//void OnEQSQueryFinished(TSharedPtr<struct FEnvQueryResult> Result);
	
	UPROPERTY()
	UBehaviorTreeComponent* CachedOwnerComp;
};
