// ============================================================================
// BTTask_CheckBroomChannel.cpp
// Developer: Marcus Daley
// Date: January 22, 2026
// Project: END2507
// ============================================================================

#include "Code/AI/BTTask_CheckBroomChannel.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "Code/Utilities/AC_SpellCollectionComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogCheckBroomChannel, Log, All);

UBTTask_CheckBroomChannel::UBTTask_CheckBroomChannel()
    : ChannelToCheck(TEXT("Broom"))
{
    NodeName = "Check Broom Channel";

    // This task completes instantly
    bNotifyTick = false;

    // Add bool filter for the key selector
    HasBroomKey.AddBoolFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTTask_CheckBroomChannel, HasBroomKey));
}

void UBTTask_CheckBroomChannel::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);

    if (UBlackboardData* BBAsset = GetBlackboardAsset())
    {
        HasBroomKey.ResolveSelectedKey(*BBAsset);

        UE_LOG(LogCheckBroomChannel, Log,
            TEXT("[CheckBroomChannel] Resolved HasBroomKey '%s' against blackboard '%s'"),
            *HasBroomKey.SelectedKeyName.ToString(),
            *BBAsset->GetName());
    }
}

EBTNodeResult::Type UBTTask_CheckBroomChannel::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIC = OwnerComp.GetAIOwner();
    if (!AIC)
    {
        UE_LOG(LogCheckBroomChannel, Warning, TEXT("[CheckBroomChannel] No AIController!"));
        return EBTNodeResult::Failed;
    }

    APawn* Pawn = AIC->GetPawn();
    if (!Pawn)
    {
        UE_LOG(LogCheckBroomChannel, Warning, TEXT("[CheckBroomChannel] No Pawn!"));
        return EBTNodeResult::Failed;
    }

    // Get SpellCollectionComponent from pawn
    UAC_SpellCollectionComponent* SpellComp = Pawn->FindComponentByClass<UAC_SpellCollectionComponent>();
    if (!SpellComp)
    {
        UE_LOG(LogCheckBroomChannel, Warning,
            TEXT("[CheckBroomChannel] %s has no SpellCollectionComponent!"),
            *Pawn->GetName());
        return EBTNodeResult::Failed;
    }

    // Check if pawn has the required channel
    bool bHasChannel = SpellComp->HasChannel(ChannelToCheck);

    UE_LOG(LogCheckBroomChannel, Display,
        TEXT("[CheckBroomChannel] %s | Channel '%s' = %s"),
        *Pawn->GetName(),
        *ChannelToCheck.ToString(),
        bHasChannel ? TEXT("YES") : TEXT("NO"));

    // Get blackboard and set the key
    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB)
    {
        UE_LOG(LogCheckBroomChannel, Warning, TEXT("[CheckBroomChannel] No BlackboardComponent!"));
        return EBTNodeResult::Failed;
    }

    if (!HasBroomKey.IsSet())
    {
        UE_LOG(LogCheckBroomChannel, Error, TEXT("[CheckBroomChannel] HasBroomKey not configured!"));
        return EBTNodeResult::Failed;
    }

    // Set the blackboard key
    BB->SetValueAsBool(HasBroomKey.SelectedKeyName, bHasChannel);

    UE_LOG(LogCheckBroomChannel, Display,
        TEXT("[CheckBroomChannel] Set '%s' = %s on blackboard"),
        *HasBroomKey.SelectedKeyName.ToString(),
        bHasChannel ? TEXT("true") : TEXT("false"));

    // Return success if channel was found, failed if not
    return bHasChannel ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}

FString UBTTask_CheckBroomChannel::GetStaticDescription() const
{
    FString KeyName = HasBroomKey.IsSet() ? HasBroomKey.SelectedKeyName.ToString() : TEXT("NOT SET!");
    return FString::Printf(TEXT("Check channel '%s' -> Set '%s'"), *ChannelToCheck.ToString(), *KeyName);
}
