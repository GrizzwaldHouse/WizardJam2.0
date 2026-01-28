// ============================================================================
// BTTask_ChaseSnitch.cpp
// Implementation of Snitch chase behavior for Seeker AI
//
// Developer: Marcus Daley
// Date: January 25, 2026
// Project: WizardJam (END2507)
//
// CHASE ALGORITHM:
// 1. Read Snitch location from Blackboard (updated by BTService_FindSnitch)
// 2. Calculate 3D direction toward Snitch
// 3. Use SetVerticalInput() for altitude adjustments
// 4. Enable boost for burst speed when far away
// 5. Add movement input for horizontal pursuit
// 6. Loop continuously - Snitch is a moving target
// 7. Succeed only when within catch radius (overlap handles actual catch)
// ============================================================================

#include "Code/AI/Quidditch/BTTask_ChaseSnitch.h"
#include "Code/Flight/AC_BroomComponent.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UBTTask_ChaseSnitch::UBTTask_ChaseSnitch()
    : CatchRadius(200.0f)
    , AltitudeTolerance(100.0f)
    , bUseBoostForPursuit(true)
    , BoostDistanceThreshold(1000.0f)
    , VerticalInputMultiplier(1.0f)
    , MaxVerticalInputChangeRate(5.0f)
{
    NodeName = "Chase Snitch";
    bNotifyTick = true;

    SnitchLocationKey.AddVectorFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTTask_ChaseSnitch, SnitchLocationKey));
}

EBTNodeResult::Type UBTTask_ChaseSnitch::ExecuteTask(
    UBehaviorTreeComponent& OwnerComp, 
    uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController || !AIController->GetPawn())
    {
        return EBTNodeResult::Failed;
    }

    UAC_BroomComponent* BroomComp = 
        AIController->GetPawn()->FindComponentByClass<UAC_BroomComponent>();
    
    if (!BroomComp || !BroomComp->IsFlying())
    {
        return EBTNodeResult::Failed;
    }

    FVector SnitchLocation;
    if (!GetSnitchLocation(OwnerComp, SnitchLocation))
    {
        return EBTNodeResult::Failed;
    }

    return EBTNodeResult::InProgress;
}

void UBTTask_ChaseSnitch::TickTask(
    UBehaviorTreeComponent& OwnerComp, 
    uint8* NodeMemory, 
    float DeltaSeconds)
{
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
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    FVector SnitchLocation;
    if (!GetSnitchLocation(OwnerComp, SnitchLocation))
    {
        BroomComp->SetVerticalInput(0.0f);
        BroomComp->SetBoostEnabled(false);
        return;
    }

    FVector CurrentLocation = AIPawn->GetActorLocation();
    float AltitudeDiff = SnitchLocation.Z - CurrentLocation.Z;
    
    if (FMath::Abs(AltitudeDiff) > AltitudeTolerance)
    {
        float VerticalInput = FMath::Clamp(AltitudeDiff / 300.0f, -1.0f, 1.0f);
        VerticalInput *= VerticalInputMultiplier;
        BroomComp->SetVerticalInput(VerticalInput);
    }
    else
    {
        BroomComp->SetVerticalInput(0.0f);
    }

    float DistanceToSnitch = FVector::Dist(CurrentLocation, SnitchLocation);
    
    if (bUseBoostForPursuit)
    {
        bool bShouldBoost = DistanceToSnitch > BoostDistanceThreshold;
        BroomComp->SetBoostEnabled(bShouldBoost);
    }

    FVector DirectionToSnitch = (SnitchLocation - CurrentLocation).GetSafeNormal();

    // Direct velocity control for AI pawns - AddMovementInput requires PlayerController processing
    ACharacter* Character = Cast<ACharacter>(AIPawn);
    if (Character)
    {
        UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
        if (MoveComp && MoveComp->MovementMode == MOVE_Flying)
        {
            float TargetSpeed = MoveComp->MaxFlySpeed;
            FVector DesiredVelocity = DirectionToSnitch * TargetSpeed;

            // Preserve vertical velocity managed by BroomComponent
            DesiredVelocity.Z = MoveComp->Velocity.Z;

            // Direct velocity assignment for AI - instant direction change
            // VInterpTo causes circular orbits when direction changes rapidly at high speed
            MoveComp->Velocity = DesiredVelocity;
        }
    }

    // ========================================================================
    // ROTATION CONTROL - Prevents circular/spiral movement
    // Pawn should face the direction of movement for visual correctness
    // ========================================================================
    FVector RotationDirection = (SnitchLocation - CurrentLocation);
    RotationDirection.Z = 0.0f;  // Keep rotation on horizontal plane only

    if (!RotationDirection.IsNearlyZero())
    {
        RotationDirection.Normalize();
        FRotator TargetRotation = RotationDirection.Rotation();
        FRotator CurrentRotation = AIPawn->GetActorRotation();
        FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaSeconds, 5.0f);
        AIPawn->SetActorRotation(NewRotation);
    }

    if (DistanceToSnitch < CatchRadius)
    {
        BroomComp->SetVerticalInput(0.0f);
        BroomComp->SetBoostEnabled(false);
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
    }
}

bool UBTTask_ChaseSnitch::GetSnitchLocation(
    UBehaviorTreeComponent& OwnerComp, 
    FVector& OutLocation) const
{
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    if (!Blackboard || !SnitchLocationKey.IsSet())
    {
        return false;
    }

    OutLocation = Blackboard->GetValueAsVector(SnitchLocationKey.SelectedKeyName);
    
    if (OutLocation.IsNearlyZero())
    {
        return false;
    }

    return true;
}

FString UBTTask_ChaseSnitch::GetStaticDescription() const
{
    return FString::Printf(
        TEXT("Chase Snitch: %s\nCatch radius: %.0fcm\nContinuous pursuit"),
        SnitchLocationKey.IsSet() ? *SnitchLocationKey.SelectedKeyName.ToString() : TEXT("(not set)"),
        CatchRadius
    );
}

void UBTTask_ChaseSnitch::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);

    if (UBlackboardData* BBAsset = GetBlackboardAsset())
    {
        SnitchLocationKey.ResolveSelectedKey(*BBAsset);
    }
}
