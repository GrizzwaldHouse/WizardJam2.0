// ============================================================================
// BTDecorator_HasChannel.cpp
// Implementation of channel check decorator for Behavior Trees
//
// Developer: Marcus Daley
// Date: January 20, 2026
// Project: END2507
// ============================================================================

#include "Code/AI/BTDecorator_HasChannel.h"
#include "Code/Utilities/AC_SpellCollectionComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

// ============================================================================
// CONSTRUCTOR
// ============================================================================

UBTDecorator_HasChannel::UBTDecorator_HasChannel()
    : ChannelName(NAME_None)
    , bInvertResult(false)
{
    NodeName = "Has Channel";

    // This decorator doesn't need to tick - it's a simple condition check
    bNotifyTick = false;
}

// ============================================================================
// CONDITION CHECK
// ============================================================================

bool UBTDecorator_HasChannel::CalculateRawConditionValue(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory) const
{
    // Get the AI controller
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[BTDecorator_HasChannel] No AI Controller found"));
        return bInvertResult; // No controller = no channels
    }

    // Get the controlled pawn
    APawn* AIPawn = AIController->GetPawn();
    if (!AIPawn)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[BTDecorator_HasChannel] AI Controller has no pawn"));
        return bInvertResult; // No pawn = no channels
    }

    // Find the spell collection component
    UAC_SpellCollectionComponent* SpellComp =
        AIPawn->FindComponentByClass<UAC_SpellCollectionComponent>();

    if (!SpellComp)
    {
        UE_LOG(LogTemp, Log,
            TEXT("[BTDecorator_HasChannel] Pawn '%s' has no SpellCollectionComponent - treating as no channels"),
            *AIPawn->GetName());
        return bInvertResult; // No component = no channels
    }

    // Check if the channel exists
    bool bHasChannel = SpellComp->HasChannel(ChannelName);

    // Apply inverse if needed
    bool bResult = bInvertResult ? !bHasChannel : bHasChannel;

    UE_LOG(LogTemp, Verbose,
        TEXT("[BTDecorator_HasChannel] '%s' checking channel '%s': Has=%s, Inverse=%s, Result=%s"),
        *AIPawn->GetName(),
        *ChannelName.ToString(),
        bHasChannel ? TEXT("YES") : TEXT("NO"),
        bInvertResult ? TEXT("YES") : TEXT("NO"),
        bResult ? TEXT("PASS") : TEXT("FAIL"));

    return bResult;
}

// ============================================================================
// DESCRIPTION
// ============================================================================

FString UBTDecorator_HasChannel::GetStaticDescription() const
{
    if (ChannelName.IsNone())
    {
        return TEXT("Channel: (not set)");
    }

    if (bInvertResult)
    {
        return FString::Printf(TEXT("Missing Channel: %s"), *ChannelName.ToString());
    }
    else
    {
        return FString::Printf(TEXT("Has Channel: %s"), *ChannelName.ToString());
    }
}
