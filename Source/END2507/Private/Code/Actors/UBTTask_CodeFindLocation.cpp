// Fill out your copyright notice in the Description page of Project Settings.


#include "Code/Actors/UBTTask_CodeFindLocation.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Engine/World.h"
#include "../END2507.h"
UUBTTask_CodeFindLocation::UUBTTask_CodeFindLocation()
{	// Set the task name for the behavior tree editor
	NodeName = TEXT("Find Location");
	bNotifyTick = false;
	bNotifyTaskFinished = false;
}

EBTNodeResult::Type UUBTTask_CodeFindLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // Get the AI controller that owns this behavior tree
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        UE_LOG(Game, Error, TEXT("FindLocation: No valid AI Controller found"));
        return EBTNodeResult::Failed;
    }

    // Get the AI's pawn to use as origin point
    APawn* AIPawn = AIController->GetPawn();
    if (!AIPawn)
    {
        UE_LOG(Game, Error, TEXT("FindLocation: No valid AI Pawn found"));
        return EBTNodeResult::Failed;
    }

    // Get the blackboard component to store our result
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return EBTNodeResult::Failed;
    }

    // Find a random navigable location around the AI's current position
    FVector CurrentLocation = AIPawn->GetActorLocation();
    FVector RandomLocation = FindRandomNavigableLocation(CurrentLocation, SearchRadius);

    // Check if we found a valid location
    if (RandomLocation != FVector::ZeroVector)
    {
        // Store the location in the blackboard for other tasks to use
        BlackboardComp->SetValueAsVector(LocationKeyName, RandomLocation);
        UE_LOG(Game, Log, TEXT("FindLocation: Found valid location at %s"), *RandomLocation.ToString());
        return EBTNodeResult::Succeeded;
    }
    else
    {
        UE_LOG(Game, Warning, TEXT("FindLocation: Could not find valid navigation point"));
        return EBTNodeResult::Failed;
    }
}

FVector UUBTTask_CodeFindLocation::FindRandomNavigableLocation(const FVector& Origin, float Radius) const
{
    // Get the navigation system from the world
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(Game, Error, TEXT("FindLocation: World is null"));
        return FVector::ZeroVector;
    }

    UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(World);
    if (!NavSystem)
    {
        UE_LOG(Game, Error, TEXT("FindLocation: Navigation System not found"));
        return FVector::ZeroVector;
    }

    // Try to find a random reachable point within the radius
    FNavLocation RandomPoint;
    bool bFoundLocation = NavSystem->GetRandomReachablePointInRadius(
        Origin,     // Starting point
        Radius,     // Search radius
        RandomPoint // Output location
    );

    if (bFoundLocation)
    {
        return RandomPoint.Location;
    }

    // If that failed, try a simpler random point approach
    bFoundLocation = NavSystem->GetRandomPointInNavigableRadius(
        Origin,
        Radius,
        RandomPoint
    );

    return bFoundLocation ? RandomPoint.Location : FVector::ZeroVector;
}
