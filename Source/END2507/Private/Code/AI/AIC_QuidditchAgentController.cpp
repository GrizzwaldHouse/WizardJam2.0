// ============================================================================
// AIC_QuidditchAgentController.cpp
// Extended AI Controller for Quidditch - combat + flight
//
// Developer: Marcus Daley
// Date: January 22, 2026
// Project: WizardJam
//
// This controller inherits all combat behaviors from AIC_CodeBaseAgentController
// and adds Quidditch-specific flight control and role tracking.
// ============================================================================

#include "Code/AI/AIC_QuidditchAgentController.h"
#include "Code/Quidditch/QuidditchStagingZone.h"
#include "Code/Flight/AC_BroomComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

DEFINE_LOG_CATEGORY(LogQuidditchAgentAI);

AAIC_QuidditchAgentController::AAIC_QuidditchAgentController()
    : FlightTargetLocationKeyName(TEXT("TargetLocation"))
    , FlightTargetActorKeyName(TEXT("TargetActor"))
    , IsFlyingKeyName(TEXT("IsFlying"))
    , HasBroomKeyName(TEXT("HasBroom"))
    , QuidditchRoleKeyName(TEXT("QuidditchRole"))
    , DefaultRole(EQuidditchRole::Chaser)
    , CachedStagingZone(nullptr)
    , CurrentRole(EQuidditchRole::None)
{
    // Base class already sets up perception
    UE_LOG(LogQuidditchAgentAI, Log, TEXT("QuidditchAgentController constructed"));
}

void AAIC_QuidditchAgentController::OnPossess(APawn* InPawn)
{
    // Call parent - this runs BehaviorTree and sets up base blackboard
    Super::OnPossess(InPawn);

    if (!InPawn)
    {
        return;
    }

    UE_LOG(LogQuidditchAgentAI, Display, TEXT("[%s] Possessed Quidditch pawn: %s"),
        *GetName(), *InPawn->GetName());

    // Setup Quidditch-specific blackboard keys
    SetupQuidditchBlackboard(InPawn);

    // Find and cache staging zone for our team
    CachedStagingZone = FindMyStagingZone();
    if (CachedStagingZone)
    {
        UE_LOG(LogQuidditchAgentAI, Display, TEXT("[%s] Found staging zone: %s"),
            *GetName(), *CachedStagingZone->GetName());
    }

    // Bind to broom component if pawn has one
    UAC_BroomComponent* BroomComp = InPawn->FindComponentByClass<UAC_BroomComponent>();
    if (BroomComp)
    {
        BroomComp->OnFlightStateChanged.AddDynamic(this, &AAIC_QuidditchAgentController::HandleFlightStateChanged);
        UE_LOG(LogQuidditchAgentAI, Log, TEXT("[%s] Bound to BroomComponent"), *GetName());
    }

    // Set default role if not already assigned
    if (CurrentRole == EQuidditchRole::None)
    {
        SetQuidditchRole(DefaultRole);
    }
}

void AAIC_QuidditchAgentController::OnUnPossess()
{
    // Unbind from broom component
    APawn* CurrentPawn = GetPawn();
    if (CurrentPawn)
    {
        UAC_BroomComponent* BroomComp = CurrentPawn->FindComponentByClass<UAC_BroomComponent>();
        if (BroomComp)
        {
            BroomComp->OnFlightStateChanged.RemoveDynamic(this, &AAIC_QuidditchAgentController::HandleFlightStateChanged);
        }
    }

    Super::OnUnPossess();
}

void AAIC_QuidditchAgentController::SetupQuidditchBlackboard(APawn* InPawn)
{
    UBlackboardComponent* BB = GetBlackboardComponent();
    if (!BB)
    {
        UE_LOG(LogQuidditchAgentAI, Warning, TEXT("[%s] No BlackboardComponent for Quidditch setup"),
            *GetName());
        return;
    }

    // Initialize flight state keys
    BB->SetValueAsBool(IsFlyingKeyName, false);
    BB->SetValueAsBool(HasBroomKeyName, false);
    BB->SetValueAsInt(QuidditchRoleKeyName, static_cast<int32>(EQuidditchRole::None));

    // Set SelfActor for BT tasks that need pawn reference
    BB->SetValueAsObject(TEXT("SelfActor"), InPawn);

    // Set team ID from parent class (inherited)
    BB->SetValueAsInt(TEXT("TeamID"), GetGenericTeamId().GetId());

    UE_LOG(LogQuidditchAgentAI, Log, TEXT("[%s] Quidditch blackboard initialized"), *GetName());
}

void AAIC_QuidditchAgentController::SetFlightTarget(const FVector& TargetLocation)
{
    UBlackboardComponent* BB = GetBlackboardComponent();
    if (BB)
    {
        BB->SetValueAsVector(FlightTargetLocationKeyName, TargetLocation);
        BB->ClearValue(FlightTargetActorKeyName);  // Clear actor target

        UE_LOG(LogQuidditchAgentAI, Verbose, TEXT("[%s] Flight target set: %s"),
            *GetName(), *TargetLocation.ToString());
    }
}

void AAIC_QuidditchAgentController::SetFlightTargetActor(AActor* TargetActor)
{
    UBlackboardComponent* BB = GetBlackboardComponent();
    if (BB)
    {
        BB->SetValueAsObject(FlightTargetActorKeyName, TargetActor);

        if (TargetActor)
        {
            // Also set location for tasks that read location instead of actor
            BB->SetValueAsVector(FlightTargetLocationKeyName, TargetActor->GetActorLocation());
        }

        UE_LOG(LogQuidditchAgentAI, Verbose, TEXT("[%s] Flight target actor: %s"),
            *GetName(), TargetActor ? *TargetActor->GetName() : TEXT("None"));
    }
}

void AAIC_QuidditchAgentController::ClearFlightTarget()
{
    UBlackboardComponent* BB = GetBlackboardComponent();
    if (BB)
    {
        BB->ClearValue(FlightTargetLocationKeyName);
        BB->ClearValue(FlightTargetActorKeyName);

        UE_LOG(LogQuidditchAgentAI, Verbose, TEXT("[%s] Flight target cleared"), *GetName());
    }
}

bool AAIC_QuidditchAgentController::GetFlightTarget(FVector& OutLocation) const
{
    UBlackboardComponent* BB = GetBlackboardComponent();
    if (!BB)
    {
        return false;
    }

    // First check for actor target
    UObject* TargetObj = BB->GetValueAsObject(FlightTargetActorKeyName);
    if (AActor* TargetActor = Cast<AActor>(TargetObj))
    {
        OutLocation = TargetActor->GetActorLocation();
        return true;
    }

    // Fall back to location target
    OutLocation = BB->GetValueAsVector(FlightTargetLocationKeyName);
    return !OutLocation.IsZero();
}

void AAIC_QuidditchAgentController::SetIsFlying(bool bIsFlying)
{
    UBlackboardComponent* BB = GetBlackboardComponent();
    if (BB)
    {
        BB->SetValueAsBool(IsFlyingKeyName, bIsFlying);

        UE_LOG(LogQuidditchAgentAI, Display, TEXT("[%s] IsFlying = %s"),
            *GetName(), bIsFlying ? TEXT("TRUE") : TEXT("FALSE"));
    }
}

bool AAIC_QuidditchAgentController::GetIsFlying() const
{
    UBlackboardComponent* BB = GetBlackboardComponent();
    if (BB)
    {
        return BB->GetValueAsBool(IsFlyingKeyName);
    }
    return false;
}

void AAIC_QuidditchAgentController::SetQuidditchRole(EQuidditchRole NewRole)
{
    CurrentRole = NewRole;

    UBlackboardComponent* BB = GetBlackboardComponent();
    if (BB)
    {
        BB->SetValueAsInt(QuidditchRoleKeyName, static_cast<int32>(NewRole));
    }

    UE_LOG(LogQuidditchAgentAI, Display, TEXT("[%s] Quidditch role set: %s"),
        *GetName(), *QuidditchHelpers::RoleToString(NewRole));
}

EQuidditchRole AAIC_QuidditchAgentController::GetQuidditchRole() const
{
    return CurrentRole;
}

AQuidditchStagingZone* AAIC_QuidditchAgentController::FindMyStagingZone()
{
    int32 MyTeamID = GetGenericTeamId().GetId();
    CachedStagingZone = AQuidditchStagingZone::FindStagingZoneForTeam(this, MyTeamID);
    return CachedStagingZone;
}

void AAIC_QuidditchAgentController::HandleFlightStateChanged(bool bNowFlying)
{
    SetIsFlying(bNowFlying);

    UE_LOG(LogQuidditchAgentAI, Display, TEXT("[%s] Flight state changed: %s"),
        *GetName(), bNowFlying ? TEXT("FLYING") : TEXT("GROUNDED"));
}
