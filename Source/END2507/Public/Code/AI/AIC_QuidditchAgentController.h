// ============================================================================
// AIC_QuidditchAgentController.h
// Extended AI Controller for Quidditch agents - inherits combat + adds flight
//
// Developer: Marcus Daley
// Date: January 22, 2026
// Project: WizardJam
//
// PURPOSE:
// Extends AIC_CodeBaseAgentController to add Quidditch-specific functionality
// while KEEPING all the combat behaviors (shoot, reload, flee, perception).
//
// WHAT WE INHERIT FROM BASE:
// - AI Perception (Sight + Hearing)
// - Team/Faction system (IGenericTeamAgentInterface)
// - Health tracking and death handling
// - Combat blackboard keys (Player, bHasTarget, HealthRatio, bCanAttack, bShouldFlee)
// - Signal/Distraction response
//
// WHAT WE ADD:
// - Flight-specific blackboard keys (TargetLocation, IsFlying, etc.)
// - Staging zone awareness
// - Quidditch role tracking
// - Flight target API (SetFlightTarget, ClearFlightTarget)
//
// USAGE:
// 1. Create Blueprint child: BP_QuidditchAgentController
// 2. Set BehaviorTreeAsset to BT_QuidditchAI
// 3. Assign to your Quidditch agent pawn's AIControllerClass
//
// DESIGNER CONFIGURATION:
// - All base perception settings (SightRadius, HearingRange, etc.)
// - Flight blackboard key names (configurable)
// - Quidditch role (Seeker, Chaser, Beater, Keeper)
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Code/Actors/AIC_CodeBaseAgentController.h"
#include "Code/Quidditch/QuidditchTypes.h"
#include "AIC_QuidditchAgentController.generated.h"

class AQuidditchStagingZone;
class UAC_BroomComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogQuidditchAgentAI, Log, All);

UCLASS()
class END2507_API AAIC_QuidditchAgentController : public AAIC_CodeBaseAgentController
{
    GENERATED_BODY()

public:
    AAIC_QuidditchAgentController();

    // ========================================================================
    // LIFECYCLE OVERRIDES
    // ========================================================================

    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;

    // ========================================================================
    // FLIGHT TARGET API
    // Used by BT services/tasks and gameplay code
    // ========================================================================

    // Set flight destination as world location
    UFUNCTION(BlueprintCallable, Category = "Quidditch|Flight")
    void SetFlightTarget(const FVector& TargetLocation);

    // Set flight destination as actor to follow (ball, player, etc.)
    UFUNCTION(BlueprintCallable, Category = "Quidditch|Flight")
    void SetFlightTargetActor(AActor* TargetActor);

    // Clear flight target (hover in place)
    UFUNCTION(BlueprintCallable, Category = "Quidditch|Flight")
    void ClearFlightTarget();

    // Get current flight target location
    // Returns false if no target set
    UFUNCTION(BlueprintPure, Category = "Quidditch|Flight")
    bool GetFlightTarget(FVector& OutLocation) const;

    // ========================================================================
    // FLIGHT STATE API
    // ========================================================================

    // Update blackboard IsFlying state
    UFUNCTION(BlueprintCallable, Category = "Quidditch|Flight")
    void SetIsFlying(bool bIsFlying);

    // Check if AI is currently flying
    UFUNCTION(BlueprintPure, Category = "Quidditch|Flight")
    bool GetIsFlying() const;

    // ========================================================================
    // QUIDDITCH ROLE API
    // ========================================================================

    // Set this agent's Quidditch role (Seeker, Chaser, Beater, Keeper)
    UFUNCTION(BlueprintCallable, Category = "Quidditch|Role")
    void SetQuidditchRole(EQuidditchRole NewRole);

    // Get assigned role
    UFUNCTION(BlueprintPure, Category = "Quidditch|Role")
    EQuidditchRole GetQuidditchRole() const;

    // ========================================================================
    // STAGING ZONE API
    // ========================================================================

    // Find and cache staging zone for this agent's team
    UFUNCTION(BlueprintCallable, Category = "Quidditch|Staging")
    AQuidditchStagingZone* FindMyStagingZone();

    // Get cached staging zone (may be null)
    UFUNCTION(BlueprintPure, Category = "Quidditch|Staging")
    AQuidditchStagingZone* GetStagingZone() const { return CachedStagingZone; }

protected:
    // ========================================================================
    // CONFIGURATION - Flight Blackboard Keys
    // ========================================================================

    // Blackboard key for flight target location (Vector)
    UPROPERTY(EditDefaultsOnly, Category = "Quidditch|Blackboard")
    FName FlightTargetLocationKeyName;

    // Blackboard key for flight target actor (Object)
    UPROPERTY(EditDefaultsOnly, Category = "Quidditch|Blackboard")
    FName FlightTargetActorKeyName;

    // Blackboard key for flying state (Bool)
    UPROPERTY(EditDefaultsOnly, Category = "Quidditch|Blackboard")
    FName IsFlyingKeyName;

    // Blackboard key for has broom channel (Bool)
    UPROPERTY(EditDefaultsOnly, Category = "Quidditch|Blackboard")
    FName HasBroomKeyName;

    // Blackboard key for Quidditch role (Int - cast from EQuidditchRole)
    UPROPERTY(EditDefaultsOnly, Category = "Quidditch|Blackboard")
    FName QuidditchRoleKeyName;

    // ========================================================================
    // CONFIGURATION - Quidditch Settings
    // ========================================================================

    // Default role if not assigned by GameMode
    UPROPERTY(EditDefaultsOnly, Category = "Quidditch|Settings")
    EQuidditchRole DefaultRole;

    // ========================================================================
    // RUNTIME STATE
    // ========================================================================

    // Cached reference to staging zone (found at possess time)
    UPROPERTY()
    AQuidditchStagingZone* CachedStagingZone;

    // Current Quidditch role
    UPROPERTY()
    EQuidditchRole CurrentRole;

    // ========================================================================
    // INTERNAL HELPERS
    // ========================================================================

    void SetupQuidditchBlackboard(APawn* InPawn);

    // Called when broom component flight state changes
    UFUNCTION()
    void HandleFlightStateChanged(bool bNowFlying);
};
