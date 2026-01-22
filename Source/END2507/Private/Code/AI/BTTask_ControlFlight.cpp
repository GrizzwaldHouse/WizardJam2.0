// ============================================================================
// BTTask_ControlFlight.cpp
// Implementation of continuous flight control for AI agents
//
// Developer: Marcus Daley
// Date: January 19, 2026
// Project: Game Architecture (END2507)
//
// This implementation shows how simple AI flight control becomes when using
// the Core API approach. The AI just calls the same functions the player uses.
//
// FLIGHT CONTROL LOGIC:
// 1. Read target from Blackboard
// 2. Calculate altitude difference
// 3. Call SetVerticalInput() with normalized value (-1 to +1)
// 4. Calculate horizontal distance
// 5. Call SetBoostEnabled() if far away
// 6. Add movement input toward target
// 7. Check if arrived, finish task if so
// ============================================================================

#include "Code/AI/BTTask_ControlFlight.h"
#include "Code/Flight/AC_BroomComponent.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

// ============================================================================
// CONSTRUCTOR
// ============================================================================

UBTTask_ControlFlight::UBTTask_ControlFlight()
    : ArrivalTolerance(100.0f)
    , AltitudeTolerance(50.0f)
    , bUseBoostWhenFar(true)
    , BoostDistanceThreshold(500.0f)
    , VerticalInputMultiplier(1.0f)
    , bDirectFlight(true)
{
    NodeName = "Control Flight";

    // This task ticks every frame while active
    bNotifyTick = true;

    // Key filters allow both Object (Actor) and Vector types in BT editor dropdown
    TargetKey.AddObjectFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTTask_ControlFlight, TargetKey),
        AActor::StaticClass());
    TargetKey.AddVectorFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTTask_ControlFlight, TargetKey));
}

// ============================================================================
// EXECUTE TASK (Called once when task starts)
// ============================================================================

EBTNodeResult::Type UBTTask_ControlFlight::ExecuteTask(
    UBehaviorTreeComponent& OwnerComp, 
    uint8* NodeMemory)
{
    // Validate we have an AI controller and pawn
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController || !AIController->GetPawn())
    {
        UE_LOG(LogTemp, Warning, 
            TEXT("[BTTask_ControlFlight] No AI Controller or Pawn"));
        return EBTNodeResult::Failed;
    }

    // Verify pawn has broom component and is flying
    UAC_BroomComponent* BroomComp = 
        AIController->GetPawn()->FindComponentByClass<UAC_BroomComponent>();
    
    if (!BroomComp)
    {
        UE_LOG(LogTemp, Warning, 
            TEXT("[BTTask_ControlFlight] Pawn has no AC_BroomComponent"));
        return EBTNodeResult::Failed;
    }

    if (!BroomComp->IsFlying())
    {
        UE_LOG(LogTemp, Warning, 
            TEXT("[BTTask_ControlFlight] Pawn is not flying - run BTTask_MountBroom first"));
        return EBTNodeResult::Failed;
    }

    // Validate target key is set
    FVector TargetLocation;
    if (!GetTargetLocation(OwnerComp, TargetLocation))
    {
        UE_LOG(LogTemp, Warning, 
            TEXT("[BTTask_ControlFlight] Could not get target location from Blackboard"));
        return EBTNodeResult::Failed;
    }

    UE_LOG(LogTemp, Display, 
        TEXT("[BTTask_ControlFlight] Started flying toward %s"),
        *TargetLocation.ToString());

    // Task runs continuously until we reach target
    return EBTNodeResult::InProgress;
}

// ============================================================================
// TICK TASK (Called every frame while task is active)
// ============================================================================

void UBTTask_ControlFlight::TickTask(
    UBehaviorTreeComponent& OwnerComp, 
    uint8* NodeMemory, 
    float DeltaSeconds)
{
    // Get references
    AAIController* AIController = OwnerComp.GetAIOwner();
    APawn* AIPawn = AIController ? AIController->GetPawn() : nullptr;
    
    if (!AIPawn)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    UAC_BroomComponent* BroomComp = AIPawn->FindComponentByClass<UAC_BroomComponent>();
    if (!BroomComp || !BroomComp->IsFlying())
    {
        // Lost flight capability - task fails
        UE_LOG(LogTemp, Warning, 
            TEXT("[BTTask_ControlFlight] Lost flight during task"));
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    // Get target location from blackboard
    FVector TargetLocation;
    if (!GetTargetLocation(OwnerComp, TargetLocation))
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    // Get current location
    FVector CurrentLocation = AIPawn->GetActorLocation();

    // ========================================================================
    // VERTICAL CONTROL - Using the Core API!
    // Same function the player's HandleAscendInput calls
    // ========================================================================

    float AltitudeDiff = TargetLocation.Z - CurrentLocation.Z;
    
    if (FMath::Abs(AltitudeDiff) > AltitudeTolerance)
    {
        // Need to ascend or descend
        // Normalize to -1.0 to 1.0 range, scale by multiplier
        float VerticalInput = FMath::Clamp(AltitudeDiff / 200.0f, -1.0f, 1.0f);
        VerticalInput *= VerticalInputMultiplier;
        
        // Call Core API - same function player input uses!
        BroomComp->SetVerticalInput(VerticalInput);
    }
    else
    {
        // At correct altitude - hover
        BroomComp->SetVerticalInput(0.0f);
    }

    // ========================================================================
    // BOOST CONTROL - Using the Core API!
    // Same function the player's HandleBoostInput calls
    // ========================================================================

    float HorizontalDist = FVector::Dist2D(CurrentLocation, TargetLocation);
    
    if (bUseBoostWhenFar)
    {
        bool bShouldBoost = HorizontalDist > BoostDistanceThreshold;
        BroomComp->SetBoostEnabled(bShouldBoost);
    }

    // ========================================================================
    // HORIZONTAL MOVEMENT
    // This uses standard movement input, not broom-specific
    // CharacterMovementComponent handles forward/strafe in MOVE_Flying mode
    // ========================================================================

    if (bDirectFlight)
    {
        // Move directly toward target in 3D
        FVector DirectionToTarget = (TargetLocation - CurrentLocation).GetSafeNormal();
        AIPawn->AddMovementInput(DirectionToTarget, 1.0f);
    }
    else
    {
        // Only adjust altitude, let navigation handle horizontal
        // This mode is for when you want AI to follow paths but fly at correct height
        FVector HorizontalDirection = (TargetLocation - CurrentLocation);
        HorizontalDirection.Z = 0.0f;
        HorizontalDirection.Normalize();
        AIPawn->AddMovementInput(HorizontalDirection, 1.0f);
    }

    // ========================================================================
    // CHECK ARRIVAL
    // ========================================================================

    float TotalDist = FVector::Dist(CurrentLocation, TargetLocation);
    
    if (TotalDist < ArrivalTolerance)
    {
        // Arrived at target!
        BroomComp->SetVerticalInput(0.0f);
        BroomComp->SetBoostEnabled(false);
        
        UE_LOG(LogTemp, Display, 
            TEXT("[BTTask_ControlFlight] Arrived at target!"));
        
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
    }
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

bool UBTTask_ControlFlight::GetTargetLocation(
    UBehaviorTreeComponent& OwnerComp, 
    FVector& OutLocation) const
{
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    if (!Blackboard || !TargetKey.IsSet())
    {
        return false;
    }

    // Check if key is an Object (actor) or Vector
    if (TargetKey.SelectedKeyType == UBlackboardKeyType_Object::StaticClass())
    {
        // Target is an actor - get its location
        UObject* TargetObject = Blackboard->GetValueAsObject(TargetKey.SelectedKeyName);
        AActor* TargetActor = Cast<AActor>(TargetObject);
        
        if (TargetActor)
        {
            OutLocation = TargetActor->GetActorLocation();
            return true;
        }
    }
    else if (TargetKey.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
    {
        // Target is a vector location
        OutLocation = Blackboard->GetValueAsVector(TargetKey.SelectedKeyName);
        return true;
    }

    return false;
}

// ============================================================================
// DESCRIPTION
// ============================================================================

FString UBTTask_ControlFlight::GetStaticDescription() const
{
    FString Description = FString::Printf(
        TEXT("Fly toward: %s\nArrival tolerance: %.0f"),
        TargetKey.IsSet() ? *TargetKey.SelectedKeyName.ToString() : TEXT("(not set)"),
        ArrivalTolerance
    );

    if (bUseBoostWhenFar)
    {
        Description += FString::Printf(TEXT("\nBoost when >%.0f away"), BoostDistanceThreshold);
    }

    if (bDirectFlight)
    {
        Description += TEXT("\nDirect 3D flight");
    }

    return Description;
}

// ============================================================================
// INITIALIZE FROM ASSET (CRITICAL for Blackboard key resolution)
// ============================================================================

void UBTTask_ControlFlight::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);

    // Resolve blackboard key against the behavior tree's blackboard asset
    // Without this, TargetKey.SelectedKeyType and SelectedKeyName won't be set
    if (UBlackboardData* BBAsset = GetBlackboardAsset())
    {
        TargetKey.ResolveSelectedKey(*BBAsset);
    }
}
