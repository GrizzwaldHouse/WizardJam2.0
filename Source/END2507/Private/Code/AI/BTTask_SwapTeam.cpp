// ============================================================================
// BTTask_SwapTeam.cpp
// Developer: Marcus Daley
// Date: January 23, 2026
// Project: END2507
// ============================================================================

#include "Code/AI/BTTask_SwapTeam.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "GameFramework/Pawn.h"
#include "Code/Actors/BaseAgent.h"

DEFINE_LOG_CATEGORY(LogBTTask_SwapTeam);

UBTTask_SwapTeam::UBTTask_SwapTeam()
{
    NodeName = TEXT("Swap Team");

    // MANDATORY: Add filter for Bool key type
    ShouldSwapTeamKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_SwapTeam, ShouldSwapTeamKey));
}

void UBTTask_SwapTeam::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);

    // MANDATORY: Resolve blackboard key
    if (UBlackboardData* BBAsset = GetBlackboardAsset())
    {
        ShouldSwapTeamKey.ResolveSelectedKey(*BBAsset);
    }
}

EBTNodeResult::Type UBTTask_SwapTeam::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        UE_LOG(LogBTTask_SwapTeam, Warning, TEXT("No AIController"));
        return EBTNodeResult::Failed;
    }

    APawn* Pawn = AIController->GetPawn();
    if (!Pawn)
    {
        UE_LOG(LogBTTask_SwapTeam, Warning, TEXT("No Pawn"));
        return EBTNodeResult::Failed;
    }

    // Get GameMode
    AQuidditchGameMode* GameMode = Cast<AQuidditchGameMode>(Pawn->GetWorld()->GetAuthGameMode());
    if (!GameMode)
    {
        UE_LOG(LogBTTask_SwapTeam, Warning, TEXT("No QuidditchGameMode"));
        return EBTNodeResult::Failed;
    }

    // Get current team
    EQuidditchTeam CurrentTeam = GameMode->GetAgentTeam(Pawn);
    if (CurrentTeam == EQuidditchTeam::None)
    {
        UE_LOG(LogBTTask_SwapTeam, Warning,
            TEXT("[%s] Agent has no team"), *Pawn->GetName());
        return EBTNodeResult::Failed;
    }

    // Calculate new team (swap to opposite)
    EQuidditchTeam NewTeam = (CurrentTeam == EQuidditchTeam::TeamA)
        ? EQuidditchTeam::TeamB
        : EQuidditchTeam::TeamA;

    UE_LOG(LogBTTask_SwapTeam, Display,
        TEXT("[%s] Swapping team: %d -> %d"),
        *Pawn->GetName(),
        static_cast<int32>(CurrentTeam),
        static_cast<int32>(NewTeam));

    // Update visual appearance if BaseAgent
    if (ABaseAgent* Agent = Cast<ABaseAgent>(Pawn))
    {
        // Query GameMode for team color (designer-configurable)
        FLinearColor NewColor = GameMode->GetTeamColor(NewTeam);

        Agent->OnFactionAssigned_Implementation(static_cast<int32>(NewTeam), NewColor);

        UE_LOG(LogBTTask_SwapTeam, Display,
            TEXT("[%s] Updated appearance to team color: R=%.2f G=%.2f B=%.2f"),
            *Pawn->GetName(), NewColor.R, NewColor.G, NewColor.B);
    }

    // Clear the swap flag in Blackboard
    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (BB && ShouldSwapTeamKey.IsSet())
    {
        BB->SetValueAsBool(ShouldSwapTeamKey.SelectedKeyName, false);
    }

    // Notify GameMode to complete the swap
    // This updates internal registration and broadcasts OnTeamSwapComplete
    GameMode->ExecuteTeamSwap(Pawn, NewTeam);

    UE_LOG(LogBTTask_SwapTeam, Display,
        TEXT("[%s] Team swap complete!"), *Pawn->GetName());

    return EBTNodeResult::Succeeded;
}

FString UBTTask_SwapTeam::GetStaticDescription() const
{
    return TEXT("Swap to opposite team");
}
