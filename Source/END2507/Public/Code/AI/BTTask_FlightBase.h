// ============================================================================
// BTTask_FlightBase.h
// Base class for all flight-related Behavior Tree tasks
//
// Developer: Marcus Daley
// Date: March 1, 2026
// Project: WizardJam / GAR (END2507)
//
// PURPOSE:
// Extracts common flight helper functions used across BTTask_ControlFlight,
// BTTask_FlyToStagingZone, and future flight tasks. Eliminates duplicated
// BroomComponent acquisition, velocity control, rotation, boost logic, and
// stamina checks.
//
// USAGE:
// Inherit from this instead of UBTTaskNode for any flight-related BT task.
// Call protected helpers in your ExecuteTask/TickTask implementations.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FlightBase.generated.h"

class UAC_BroomComponent;
class UAC_StaminaComponent;
class UCharacterMovementComponent;

UCLASS(Abstract)
class END2507_API UBTTask_FlightBase : public UBTTaskNode
{
    GENERATED_BODY()

protected:
    // ========================================================================
    // COMPONENT ACQUISITION HELPERS
    // ========================================================================

    // Get the BroomComponent from the AI's pawn, with null and flight-state validation
    // Returns nullptr if pawn has no BroomComponent
    UAC_BroomComponent* GetBroomComponent(UBehaviorTreeComponent& OwnerComp) const;

    // Get the StaminaComponent from a pawn
    // Returns nullptr if pawn has no StaminaComponent
    UAC_StaminaComponent* GetStaminaComponent(APawn* Pawn) const;

    // Get stamina percentage from pawn (0.0-1.0), returns -1.0f if no StaminaComponent
    float GetStaminaPercent(APawn* Pawn) const;

    // Get CharacterMovementComponent from pawn (casts to ACharacter internally)
    // Returns nullptr if pawn is not a Character or has no movement component
    UCharacterMovementComponent* GetFlightMovementComponent(APawn* Pawn) const;

    // ========================================================================
    // FLIGHT MOVEMENT HELPERS
    // ========================================================================

    // Apply direct velocity to AI pawn for flight movement
    // Direction should be normalized, SpeedMultiplier scales MaxFlySpeed
    // bIncludeVertical: if false, preserves existing Z velocity (BroomComponent manages altitude)
    void ApplyFlightVelocity(APawn* Pawn, const FVector& Direction, float SpeedMultiplier, bool bIncludeVertical = false) const;

    // Apply smooth rotation toward target location (horizontal plane only)
    // InterpSpeed controls rotation smoothness (higher = faster snap)
    void ApplyFlightRotation(APawn* Pawn, const FVector& TargetLocation, float DeltaSeconds, float InterpSpeed = 5.0f) const;

    // ========================================================================
    // BOOST HELPERS
    // ========================================================================

    // Simple boost control: enable when distance > threshold AND stamina sufficient
    void UpdateBoostSimple(UAC_BroomComponent* BroomComp, float Distance,
        float DistanceThreshold, float StaminaPercent, float StaminaThreshold) const;

    // Hysteresis boost control: prevents oscillation near threshold boundary
    // bCurrentlyBoosting is updated in-place to track state between ticks
    void UpdateBoostWithHysteresis(UAC_BroomComponent* BroomComp, float Distance,
        float EnableThreshold, float DisableThreshold,
        float StaminaPercent, float MinStamina, bool& bCurrentlyBoosting) const;

    // ========================================================================
    // CLEANUP HELPERS
    // ========================================================================

    // Stop all flight outputs (zero vertical input, disable boost)
    void StopFlightOutputs(UAC_BroomComponent* BroomComp) const;

    // Stop flight outputs AND zero horizontal velocity
    void StopFlightCompletely(APawn* Pawn, UAC_BroomComponent* BroomComp) const;
};
