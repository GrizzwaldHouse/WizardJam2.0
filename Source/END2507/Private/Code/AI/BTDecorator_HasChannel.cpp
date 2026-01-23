// ============================================================================
// BTDecorator_HasChannel.cpp
// Implementation of channel check decorator using Blackboard (Observer Pattern)
//
// Developer: Marcus Daley
// Date: January 20, 2026 (Refactored January 23, 2026)
// Project: END2507
//
// REFACTOR NOTES (January 23, 2026):
// - Removed direct component query (FindComponentByClass)
// - Now reads channel state from Blackboard key
// - Controller maintains Blackboard via OnChannelAdded delegate
// - Supports Observer Aborts for reactive BT re-evaluation
// ============================================================================

#include "Code/AI/BTDecorator_HasChannel.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"

// ============================================================================
// CONSTRUCTOR
// ============================================================================

UBTDecorator_HasChannel::UBTDecorator_HasChannel()
    : bInvertResult(false)
{
    NodeName = "Has Channel (Blackboard)";

    // This decorator doesn't need to tick - condition is evaluated on demand
    // and can be re-evaluated via Observer Aborts when Blackboard key changes
    bNotifyTick = false;

    // CRITICAL: Add bool filter so the editor knows what key types are valid
    // This is required for FBlackboardKeySelector to work correctly
    ChannelKey.AddBoolFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTDecorator_HasChannel, ChannelKey));

    // Enable flow control - allows Observer Aborts to work
    // Designer can configure FlowAbortMode in BT editor:
    // - None: No abort, just condition check
    // - Self: Abort this decorator's branch when condition changes
    // - LowerPriority: Abort lower priority branches
    // - Both: Abort both this branch and lower priority branches
    FlowAbortMode = EBTFlowAbortMode::Self;
}

// ============================================================================
// INITIALIZE FROM ASSET - CRITICAL for Blackboard key resolution
// ============================================================================

void UBTDecorator_HasChannel::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);

    // Resolve the key selector against the blackboard asset
    // Without this, ChannelKey.IsSet() returns false at runtime!
    if (UBlackboardData* BBAsset = GetBlackboardAsset())
    {
        ChannelKey.ResolveSelectedKey(*BBAsset);
    }
}

// ============================================================================
// CONDITION CHECK - Now reads from Blackboard (Observer Pattern)
// ============================================================================

bool UBTDecorator_HasChannel::CalculateRawConditionValue(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory) const
{
    // Get the blackboard component
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    if (!Blackboard)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[BTDecorator_HasChannel] No Blackboard component!"));
        return bInvertResult; // No blackboard = no channels
    }

    // Validate the key is set
    if (!ChannelKey.IsSet())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[BTDecorator_HasChannel] ChannelKey is not set! Configure in BT editor."));
        return bInvertResult;
    }

    // Read channel state from Blackboard
    // The AI Controller maintains this key via OnChannelAdded/OnChannelRemoved delegates
    bool bHasChannel = Blackboard->GetValueAsBool(ChannelKey.SelectedKeyName);

    // Apply inverse if needed
    bool bResult = bInvertResult ? !bHasChannel : bHasChannel;

    UE_LOG(LogTemp, Verbose,
        TEXT("[BTDecorator_HasChannel] Checking key '%s': Value=%s, Inverse=%s, Result=%s"),
        *ChannelKey.SelectedKeyName.ToString(),
        bHasChannel ? TEXT("TRUE") : TEXT("FALSE"),
        bInvertResult ? TEXT("YES") : TEXT("NO"),
        bResult ? TEXT("PASS") : TEXT("FAIL"));

    return bResult;
}

// ============================================================================
// DESCRIPTION
// ============================================================================

FString UBTDecorator_HasChannel::GetStaticDescription() const
{
    if (!ChannelKey.IsSet())
    {
        return TEXT("Channel Key: (not set)");
    }

    FString KeyName = ChannelKey.SelectedKeyName.ToString();

    if (bInvertResult)
    {
        return FString::Printf(TEXT("Missing: %s"), *KeyName);
    }
    else
    {
        return FString::Printf(TEXT("Has: %s"), *KeyName);
    }
}
