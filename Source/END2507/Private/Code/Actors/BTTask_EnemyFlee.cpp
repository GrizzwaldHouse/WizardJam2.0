// Fill out your copyright notice in the Description page of Project Settings.


#include "Code/Actors/BTTask_EnemyFlee.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "GameFramework/Character.h"
#include "NavigationSystem.h"
#include "EngineUtils.h" 
#include "Code/Actors/HideWall.h"
#include "../END2507.h"

DEFINE_LOG_CATEGORY(LogFleeBehavior);
UBTTask_EnemyFlee::UBTTask_EnemyFlee()
{
	NodeName = "Flee From Player";
	FleeLocationKey = TEXT("Location");
	FleeRadius = 1500.0f;
	bNotifyTick = false;
}

EBTNodeResult::Type UBTTask_EnemyFlee::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        UE_LOG(LogFleeBehavior, Error, TEXT("No AIController"));
        return EBTNodeResult::Failed;
    }
    APawn* ControlledPawn = AIController->GetPawn();
    if (!ControlledPawn)
    {
        UE_LOG(LogFleeBehavior, Error, TEXT("No controlled pawn"));
        return EBTNodeResult::Failed;
    }

    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        UE_LOG(LogFleeBehavior, Error, TEXT("No Blackboard"));
        return EBTNodeResult::Failed;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return EBTNodeResult::Failed;
    }

    int32 WallCount = 0;
    for (TActorIterator<AHideWall> Counter(World); Counter; ++Counter) { WallCount++; }
    UE_LOG(LogFleeBehavior, Error, TEXT("Total HideWall actors in level: %d"), WallCount);
    //Cover Finding Logic

    AHideWall* BestCoverActor = nullptr;
    float ClosestSafeWallDistance = TNumericLimits<float>::Max();

    // Iterate through all AHideWall actors in the level
    for (TActorIterator<AHideWall> It(World); It; ++It)
    {
        AHideWall* Wall = *It;
        // Check if the wall is valid and is NOT spinning 
        if (Wall && Wall->IsSafeForCover())
        {
            float Distance = FVector::Dist(ControlledPawn->GetActorLocation(), Wall->GetActorLocation());
            if (Distance < ClosestSafeWallDistance)
            {
                ClosestSafeWallDistance = Distance;
                BestCoverActor = Wall;
            }
        }
    }

    // Cover Finding Logic

    if (BestCoverActor)
    {
        // found a safe wall, set its location as the FleeLocation for the Move To task
        BlackboardComp->SetValueAsVector(TEXT("FleeLocation"), BestCoverActor->GetActorLocation());
        UE_LOG(LogFleeBehavior, Log, TEXT("Flee location found: %s"),
            *BestCoverActor->GetActorLocation().ToString());
        return EBTNodeResult::Succeeded;
    }

    // No safe cover found, let's use  fallback
    AActor* PlayerActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TEXT("Player")));
    if (!PlayerActor)
    {
        UE_LOG(LogFleeBehavior, Warning, TEXT("No player target to flee from and no cover!"));
        return EBTNodeResult::Failed;
    }

    FVector MyLocation = ControlledPawn->GetActorLocation();
    FVector AwayDirection = (MyLocation - PlayerActor->GetActorLocation()).GetSafeNormal();
    FVector FleeTarget = MyLocation + (AwayDirection * FleeRadius);


    BlackboardComp->SetValueAsVector(TEXT("FleeLocation"), FleeTarget);
    UE_LOG(LogFleeBehavior, Log, TEXT("Flee location found: %s"), *FleeTarget.ToString());
    return EBTNodeResult::Succeeded;

}