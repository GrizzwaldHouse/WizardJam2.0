// ============================================================================
// BTTask_WaitForMatchStart.cpp
// Developer: Marcus Daley
// Date: January 23, 2026
// Project: END2507
// ============================================================================

#include "Code/AI/BTTask_WaitForMatchStart.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"

DEFINE_LOG_CATEGORY(LogBTTask_WaitForMatchStart);

UBTTask_WaitForMatchStart::UBTTask_WaitForMatchStart()
    : bPlayHoverAnimation(true)
    , CheckInterval(0.2f)
    , TimeSinceLastCheck(0.0f)
{
    NodeName = TEXT("Wait For Match Start");
    bNotifyTick = true;

    // MANDATORY: Add filter for Bool key type
    MatchStartedKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_WaitForMatchStart, MatchStartedKey));
}

void UBTTask_WaitForMatchStart::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);

    // MANDATORY: Resolve blackboard key
    if (UBlackboardData* BBAsset = GetBlackboardAsset())
    {
        MatchStartedKey.ResolveSelectedKey(*BBAsset);
    }
}

EBTNodeResult::Type UBTTask_WaitForMatchStart::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB)
    {
        UE_LOG(LogBTTask_WaitForMatchStart, Warning, TEXT("No Blackboard"));
        return EBTNodeResult::Failed;
    }

    // Check if key is properly set up
    if (!MatchStartedKey.IsSet())
    {
        UE_LOG(LogBTTask_WaitForMatchStart, Warning, TEXT("MatchStartedKey is not set!"));
        return EBTNodeResult::Failed;
    }

    // Check if match already started (late-joining agent scenario)
    bool bMatchStarted = BB->GetValueAsBool(MatchStartedKey.SelectedKeyName);
    if (bMatchStarted)
    {
        UE_LOG(LogBTTask_WaitForMatchStart, Display, TEXT("Match already started - immediate success"));
        return EBTNodeResult::Succeeded;
    }

    // Reset check timer
    TimeSinceLastCheck = 0.0f;

    UE_LOG(LogBTTask_WaitForMatchStart, Display,
        TEXT("Waiting for match start (checking BB.%s)"),
        *MatchStartedKey.SelectedKeyName.ToString());

    // Gas Station Pattern: Wait for starting gun
    return EBTNodeResult::InProgress;
}

void UBTTask_WaitForMatchStart::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    // Throttle checks (not really necessary since we're just reading BB, but good practice)
    TimeSinceLastCheck += DeltaSeconds;
    if (TimeSinceLastCheck < CheckInterval)
    {
        return;
    }
    TimeSinceLastCheck = 0.0f;

    // Check Blackboard - NOT GameMode!
    // The controller's HandleMatchStarted() delegate handler sets this when OnMatchStarted fires
    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    bool bMatchStarted = BB->GetValueAsBool(MatchStartedKey.SelectedKeyName);
    if (bMatchStarted)
    {
        UE_LOG(LogBTTask_WaitForMatchStart, Display,
            TEXT("Match started! (BB.%s = true) - proceeding to gameplay"),
            *MatchStartedKey.SelectedKeyName.ToString());

        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
        return;
    }

    // Still waiting - controller will set BB when OnMatchStarted fires
}

FString UBTTask_WaitForMatchStart::GetStaticDescription() const
{
    if (MatchStartedKey.IsSet())
    {
        return FString::Printf(TEXT("Wait for BB.%s = true"), *MatchStartedKey.SelectedKeyName.ToString());
    }
    return TEXT("Wait For Match Start (key not set)");
}
