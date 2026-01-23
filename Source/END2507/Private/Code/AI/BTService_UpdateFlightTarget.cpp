// ============================================================================
// BTService_UpdateFlightTarget.cpp
// Implementation of flight target update service
//
// Developer: Marcus Daley
// Date: January 22, 2026
// Project: WizardJam
//
// KEY IMPLEMENTATION NOTES:
// 1. Uses static FindStagingZoneForTeam() to locate staging zone
// 2. Proper FBlackboardKeySelector initialization (AddVectorFilter + InitializeFromAsset)
// 3. Supports both static staging zone targets and dynamic follow targets
// ============================================================================

#include "Code/AI/BTService_UpdateFlightTarget.h"
#include "Code/Quidditch/QuidditchStagingZone.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Int.h"
#include "GenericTeamAgentInterface.h"

DEFINE_LOG_CATEGORY_STATIC(LogUpdateFlightTarget, Log, All);

UBTService_UpdateFlightTarget::UBTService_UpdateFlightTarget()
    : DefaultTeamID(0)
    , bFollowMovingTarget(false)
{
    NodeName = "Update Flight Target";
    Interval = 0.25f;  // Update 4 times per second - frequent enough for smooth flight
    RandomDeviation = 0.05f;

    // CRITICAL: Add filters so editor knows what key types are valid
    // OutputLocationKey must be a Vector
    OutputLocationKey.AddVectorFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTService_UpdateFlightTarget, OutputLocationKey));

    // TeamIDKey must be an Int
    TeamIDKey.AddIntFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTService_UpdateFlightTarget, TeamIDKey));

    // FollowActorKey must be an Object (Actor)
    FollowActorKey.AddObjectFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTService_UpdateFlightTarget, FollowActorKey),
        AActor::StaticClass());
}

void UBTService_UpdateFlightTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);

    // CRITICAL: Resolve all key selectors against the blackboard asset
    if (UBlackboardData* BBAsset = GetBlackboardAsset())
    {
        OutputLocationKey.ResolveSelectedKey(*BBAsset);
        TeamIDKey.ResolveSelectedKey(*BBAsset);
        FollowActorKey.ResolveSelectedKey(*BBAsset);

        UE_LOG(LogUpdateFlightTarget, Log,
            TEXT("[UpdateFlightTarget] Resolved keys - Output: '%s', TeamID: '%s', FollowActor: '%s'"),
            *OutputLocationKey.SelectedKeyName.ToString(),
            TeamIDKey.IsSet() ? *TeamIDKey.SelectedKeyName.ToString() : TEXT("(not set)"),
            FollowActorKey.IsSet() ? *FollowActorKey.SelectedKeyName.ToString() : TEXT("(not set)"));
    }
}

void UBTService_UpdateFlightTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    // Validate output key is configured
    if (!OutputLocationKey.IsSet())
    {
        UE_LOG(LogUpdateFlightTarget, Error,
            TEXT("[UpdateFlightTarget] OutputLocationKey not set!"));
        return;
    }

    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB)
    {
        UE_LOG(LogUpdateFlightTarget, Warning,
            TEXT("[UpdateFlightTarget] No BlackboardComponent!"));
        return;
    }

    FVector TargetLocation = FVector::ZeroVector;
    bool bFoundTarget = false;

    // Determine target based on mode
    if (bFollowMovingTarget)
    {
        // Dynamic target: follow an actor
        bFoundTarget = GetFollowTargetLocation(OwnerComp, TargetLocation);
    }
    else
    {
        // Static target: staging zone
        bFoundTarget = GetStagingZoneTarget(OwnerComp, TargetLocation);
    }

    if (bFoundTarget)
    {
        BB->SetValueAsVector(OutputLocationKey.SelectedKeyName, TargetLocation);

        UE_LOG(LogUpdateFlightTarget, Verbose,
            TEXT("[UpdateFlightTarget] Set target to %s"),
            *TargetLocation.ToString());
    }
    else
    {
        UE_LOG(LogUpdateFlightTarget, Warning,
            TEXT("[UpdateFlightTarget] Could not find flight target!"));
    }
}

int32 UBTService_UpdateFlightTarget::GetAgentTeamID(UBehaviorTreeComponent& OwnerComp) const
{
    // First try to get from blackboard
    if (TeamIDKey.IsSet())
    {
        UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
        if (BB)
        {
            return BB->GetValueAsInt(TeamIDKey.SelectedKeyName);
        }
    }

    // Try to get from pawn's team interface
    AAIController* AIC = OwnerComp.GetAIOwner();
    if (AIC)
    {
        APawn* Pawn = AIC->GetPawn();
        if (Pawn)
        {
            // Check if pawn implements IGenericTeamAgentInterface
            IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(Pawn);
            if (TeamAgent)
            {
                return TeamAgent->GetGenericTeamId().GetId();
            }
        }
    }

    // Fallback to default
    return DefaultTeamID;
}

bool UBTService_UpdateFlightTarget::GetStagingZoneTarget(UBehaviorTreeComponent& OwnerComp, FVector& OutLocation) const
{
    AAIController* AIC = OwnerComp.GetAIOwner();
    if (!AIC)
    {
        return false;
    }

    int32 TeamID = GetAgentTeamID(OwnerComp);

    // Find staging zone for this team
    AQuidditchStagingZone* StagingZone = AQuidditchStagingZone::FindStagingZoneForTeam(AIC, TeamID);

    if (StagingZone)
    {
        OutLocation = StagingZone->GetStagingTargetLocation();
        return true;
    }

    UE_LOG(LogUpdateFlightTarget, Warning,
        TEXT("[UpdateFlightTarget] No staging zone found for Team %d"),
        TeamID);

    return false;
}

bool UBTService_UpdateFlightTarget::GetFollowTargetLocation(UBehaviorTreeComponent& OwnerComp, FVector& OutLocation) const
{
    if (!FollowActorKey.IsSet())
    {
        return false;
    }

    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB)
    {
        return false;
    }

    UObject* TargetObject = BB->GetValueAsObject(FollowActorKey.SelectedKeyName);
    AActor* TargetActor = Cast<AActor>(TargetObject);

    if (TargetActor)
    {
        OutLocation = TargetActor->GetActorLocation();
        return true;
    }

    return false;
}

FString UBTService_UpdateFlightTarget::GetStaticDescription() const
{
    FString Description;

    if (bFollowMovingTarget)
    {
        Description = FString::Printf(
            TEXT("Follow actor from: %s\nWrite to: %s"),
            FollowActorKey.IsSet() ? *FollowActorKey.SelectedKeyName.ToString() : TEXT("(not set)"),
            OutputLocationKey.IsSet() ? *OutputLocationKey.SelectedKeyName.ToString() : TEXT("(not set)")
        );
    }
    else
    {
        Description = FString::Printf(
            TEXT("Find staging zone for Team: %s\nWrite to: %s"),
            TeamIDKey.IsSet() ? *TeamIDKey.SelectedKeyName.ToString() : FString::Printf(TEXT("%d (default)"), DefaultTeamID),
            OutputLocationKey.IsSet() ? *OutputLocationKey.SelectedKeyName.ToString() : TEXT("(not set)")
        );
    }

    return Description;
}
