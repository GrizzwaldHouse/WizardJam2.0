// ============================================================================
// BTTask_MountBroom.cpp
// Implementation of Behavior Tree task for AI broom mounting
//
// Developer: Marcus Daley
// Date: January 19, 2026
// Project: Game Architecture (END2507)
//
// KEY INSIGHT:
// This task calls the EXACT SAME FUNCTION that player input calls!
// SetFlightEnabled(true) works identically whether called from:
// - Player pressing Space key (via HandleToggleInput)
// - AI Behavior Tree task (this function)
// - Blueprint event
// - Console command
// The Core API is truly controller-agnostic.
// ============================================================================

#include "Code/AI/BTTask_MountBroom.h"
#include "Code/Flight/AC_BroomComponent.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "GameFramework/Character.h"

// ============================================================================
// CONSTRUCTOR
// ============================================================================

UBTTask_MountBroom::UBTTask_MountBroom()
    : bMountBroom(true)
{
    NodeName = "Mount/Dismount Broom";

    // This task completes immediately (instant action)
    bNotifyTick = false;

    // CRITICAL FIX (January 23, 2026): Add bool filter for FlightStateKey
    // Without this, the key dropdown shows options but IsSet() returns false at runtime
    FlightStateKey.AddBoolFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTTask_MountBroom, FlightStateKey));
}

// ============================================================================
// EXECUTE TASK
// ============================================================================

EBTNodeResult::Type UBTTask_MountBroom::ExecuteTask(
    UBehaviorTreeComponent& OwnerComp, 
    uint8* NodeMemory)
{
    // Get the AI controller that owns this behavior tree
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        UE_LOG(LogTemp, Warning, TEXT("[BTTask_MountBroom] No AI Controller found"));
        return EBTNodeResult::Failed;
    }

    // Get the pawn this AI controls
    APawn* AIPawn = AIController->GetPawn();
    if (!AIPawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("[BTTask_MountBroom] AI Controller has no pawn"));
        return EBTNodeResult::Failed;
    }

    // Find the broom component on this pawn
    // This is the SAME component that players use!
    UAC_BroomComponent* BroomComp = AIPawn->FindComponentByClass<UAC_BroomComponent>();
    if (!BroomComp)
    {
        UE_LOG(LogTemp, Warning, 
            TEXT("[BTTask_MountBroom] Pawn '%s' has no AC_BroomComponent"),
            *AIPawn->GetName());
        return EBTNodeResult::Failed;
    }

    // ========================================================================
    // HERE IS THE KEY INSIGHT:
    // We call the EXACT SAME FUNCTION that player input calls!
    // The flight mechanics are identical because both paths use the Core API.
    // ========================================================================
    
    BroomComp->SetFlightEnabled(bMountBroom);

    // Check if the action succeeded
    bool bIsNowFlying = BroomComp->IsFlying();
    bool bSuccess = (bIsNowFlying == bMountBroom);

    if (bSuccess)
    {
        UE_LOG(LogTemp, Display, 
            TEXT("[BTTask_MountBroom] '%s' successfully %s broom"),
            *AIPawn->GetName(),
            bMountBroom ? TEXT("mounted") : TEXT("dismounted"));

        // Optionally update blackboard with flight state
        if (FlightStateKey.IsSet())
        {
            UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
            if (Blackboard)
            {
                Blackboard->SetValueAsBool(FlightStateKey.SelectedKeyName, bIsNowFlying);
            }
        }

        return EBTNodeResult::Succeeded;
    }
    else
    {
        // This might happen if stamina is too low to mount
        UE_LOG(LogTemp, Warning, 
            TEXT("[BTTask_MountBroom] '%s' failed to %s broom (insufficient stamina?)"),
            *AIPawn->GetName(),
            bMountBroom ? TEXT("mount") : TEXT("dismount"));
        
        return EBTNodeResult::Failed;
    }
}

// ============================================================================
// DESCRIPTION
// ============================================================================

FString UBTTask_MountBroom::GetStaticDescription() const
{
    FString ActionText = bMountBroom ? TEXT("Mount") : TEXT("Dismount");

    if (FlightStateKey.IsSet())
    {
        return FString::Printf(TEXT("%s Broom\n(Store result in: %s)"),
            *ActionText,
            *FlightStateKey.SelectedKeyName.ToString());
    }

    return FString::Printf(TEXT("%s Broom"), *ActionText);
}

// ============================================================================
// INITIALIZE FROM ASSET (CRITICAL for Blackboard key resolution)
// Added January 23, 2026 - fixes silent failure where IsSet() returns false
// ============================================================================

void UBTTask_MountBroom::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);

    // Resolve blackboard key against the behavior tree's blackboard asset
    // Without this, FlightStateKey.SelectedKeyType and SelectedKeyName won't be set
    if (UBlackboardData* BBAsset = GetBlackboardAsset())
    {
        FlightStateKey.ResolveSelectedKey(*BBAsset);
    }
}
