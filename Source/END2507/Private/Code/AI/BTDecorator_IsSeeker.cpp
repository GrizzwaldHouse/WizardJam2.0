// ============================================================================
// BTDecorator_IsSeeker.cpp
// Implementation of Seeker role decorator
//
// Developer: Marcus Daley
// Date: January 25, 2026
// Project: WizardJam (END2507)
// ============================================================================

#include "Code/AI/BTDecorator_IsSeeker.h"
#include "Code/GameModes/QuidditchGameMode.h"
#include "Code/Quidditch/QuidditchTypes.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

UBTDecorator_IsSeeker::UBTDecorator_IsSeeker()
{
    NodeName = "Is Seeker";
}

bool UBTDecorator_IsSeeker::CalculateRawConditionValue(
    UBehaviorTreeComponent& OwnerComp, 
    uint8* NodeMemory) const
{
    // Get AI controller and pawn
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController || !AIController->GetPawn())
    {
        return false;
    }

    APawn* AIPawn = AIController->GetPawn();

    // Get QuidditchGameMode
    AQuidditchGameMode* GameMode = Cast<AQuidditchGameMode>(
        UGameplayStatics::GetGameMode(AIController)
    );

    if (!GameMode)
    {
        return false;
    }

    // Query agent's assigned role from GameMode
    EQuidditchRole AssignedRole = GameMode->GetAgentRole(AIPawn);

    // Return true if Seeker, false otherwise
    return (AssignedRole == EQuidditchRole::Seeker);
}

FString UBTDecorator_IsSeeker::GetStaticDescription() const
{
    return TEXT("Passes if agent role is Seeker");
}
