// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "UBTTask_CodeFindLocation.generated.h"

/**
 * 
 */
UCLASS()
class END2507_API UUBTTask_CodeFindLocation : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UUBTTask_CodeFindLocation();
protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	// Search radius for finding random locations
	UPROPERTY()
	float SearchRadius = 1000.0f;
	// Blackboard key to store the found location
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Blackboard")
	FName LocationKeyName = TEXT("Location");
	// Helper function to find a valid navigation point
	FVector FindRandomNavigableLocation(const FVector& Origin, float Radius) const;

};
