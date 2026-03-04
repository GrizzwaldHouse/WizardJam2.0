// ============================================================================
// BTTask_FlightBase.cpp
// Shared flight helper implementations for all flight BT tasks
//
// Developer: Marcus Daley
// Date: March 1, 2026
// Project: WizardJam / GAR (END2507)
// ============================================================================

#include "Code/AI/BTTask_FlightBase.h"
#include "Code/Flight/AC_BroomComponent.h"
#include "Code/Utilities/AC_StaminaComponent.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

// ============================================================================
// COMPONENT ACQUISITION
// ============================================================================

UAC_BroomComponent* UBTTask_FlightBase::GetBroomComponent(UBehaviorTreeComponent& OwnerComp) const
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        return nullptr;
    }

    APawn* Pawn = AIController->GetPawn();
    if (!Pawn)
    {
        return nullptr;
    }

    return Pawn->FindComponentByClass<UAC_BroomComponent>();
}

UAC_StaminaComponent* UBTTask_FlightBase::GetStaminaComponent(APawn* Pawn) const
{
    if (!Pawn)
    {
        return nullptr;
    }

    return Pawn->FindComponentByClass<UAC_StaminaComponent>();
}

float UBTTask_FlightBase::GetStaminaPercent(APawn* Pawn) const
{
    UAC_StaminaComponent* StaminaComp = GetStaminaComponent(Pawn);
    if (!StaminaComp)
    {
        return -1.0f;
    }

    return StaminaComp->GetStaminaPercent();
}

UCharacterMovementComponent* UBTTask_FlightBase::GetFlightMovementComponent(APawn* Pawn) const
{
    ACharacter* Character = Cast<ACharacter>(Pawn);
    if (!Character)
    {
        return nullptr;
    }

    UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
    if (!MoveComp || MoveComp->MovementMode != MOVE_Flying)
    {
        return nullptr;
    }

    return MoveComp;
}

// ============================================================================
// FLIGHT MOVEMENT
// ============================================================================

void UBTTask_FlightBase::ApplyFlightVelocity(APawn* Pawn, const FVector& Direction,
    float SpeedMultiplier, bool bIncludeVertical) const
{
    UCharacterMovementComponent* MoveComp = GetFlightMovementComponent(Pawn);
    if (!MoveComp)
    {
        return;
    }

    float TargetSpeed = MoveComp->MaxFlySpeed * SpeedMultiplier;
    FVector DesiredVelocity = Direction * TargetSpeed;

    // Preserve BroomComponent's vertical velocity unless caller wants full 3D control
    if (!bIncludeVertical)
    {
        DesiredVelocity.Z = MoveComp->Velocity.Z;
    }

    // Direct velocity assignment - instant direction change
    // VInterpTo causes circular orbits when direction changes rapidly at high speed
    MoveComp->Velocity = DesiredVelocity;
}

void UBTTask_FlightBase::ApplyFlightRotation(APawn* Pawn, const FVector& TargetLocation,
    float DeltaSeconds, float InterpSpeed) const
{
    if (!Pawn)
    {
        return;
    }

    FVector RotationDirection = TargetLocation - Pawn->GetActorLocation();
    RotationDirection.Z = 0.0f;

    if (RotationDirection.IsNearlyZero())
    {
        return;
    }

    RotationDirection.Normalize();
    FRotator TargetRotation = RotationDirection.Rotation();
    FRotator CurrentRotation = Pawn->GetActorRotation();
    FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaSeconds, InterpSpeed);
    Pawn->SetActorRotation(NewRotation);
}

// ============================================================================
// BOOST CONTROL
// ============================================================================

void UBTTask_FlightBase::UpdateBoostSimple(UAC_BroomComponent* BroomComp, float Distance,
    float DistanceThreshold, float StaminaPercent, float StaminaThreshold) const
{
    if (!BroomComp)
    {
        return;
    }

    bool bShouldBoost = (Distance > DistanceThreshold) && (StaminaPercent >= StaminaThreshold);
    BroomComp->SetBoostEnabled(bShouldBoost);
}

void UBTTask_FlightBase::UpdateBoostWithHysteresis(UAC_BroomComponent* BroomComp, float Distance,
    float EnableThreshold, float DisableThreshold,
    float StaminaPercent, float MinStamina, bool& bCurrentlyBoosting) const
{
    if (!BroomComp)
    {
        return;
    }

    bool bShouldBoost = bCurrentlyBoosting;
    if (!bCurrentlyBoosting)
    {
        // Not boosting - check if we should turn ON
        bShouldBoost = (Distance > EnableThreshold) && (StaminaPercent >= MinStamina);
    }
    else
    {
        // Currently boosting - check if we should turn OFF (lower threshold prevents oscillation)
        bShouldBoost = (Distance > DisableThreshold) && (StaminaPercent >= MinStamina);
    }

    bCurrentlyBoosting = bShouldBoost;
    BroomComp->SetBoostEnabled(bShouldBoost);
}

// ============================================================================
// CLEANUP
// ============================================================================

void UBTTask_FlightBase::StopFlightOutputs(UAC_BroomComponent* BroomComp) const
{
    if (!BroomComp)
    {
        return;
    }

    BroomComp->SetBoostEnabled(false);
    BroomComp->SetVerticalInput(0.0f);
}

void UBTTask_FlightBase::StopFlightCompletely(APawn* Pawn, UAC_BroomComponent* BroomComp) const
{
    StopFlightOutputs(BroomComp);

    // Zero horizontal velocity to prevent drift
    UCharacterMovementComponent* MoveComp = GetFlightMovementComponent(Pawn);
    if (MoveComp)
    {
        FVector Velocity = MoveComp->Velocity;
        Velocity.X = 0.0f;
        Velocity.Y = 0.0f;
        MoveComp->Velocity = Velocity;
    }
}
