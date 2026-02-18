// ============================================================================
// IQuidditchAgent.h
// Interface contract for all Quidditch participants
//
// Developer: Marcus Daley
// Date: January 20, 2026
// Project: WizardJam / GAR (END2507)
//
// PURPOSE:
// Defines the contract that all Quidditch participants must implement.
// Both AI agents AND the player character can implement this interface.
//
// WHY INTERFACE INSTEAD OF INHERITANCE:
// The player character might be a Seeker while AI controls the Chasers.
// Using an interface means we can query ANY actor's Quidditch state
// without knowing its class hierarchy. This follows the principle of
// interface-driven design over tight coupling.
//
// USAGE:
// 1. Any class that participates in Quidditch implements UIQuidditchAgent
// 2. Systems query capabilities via Execute_ functions
// 3. TeamAIManager and GameMode use interface for role coordination
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Code/Quidditch/QuidditchTypes.h"
#include "IQuidditchAgent.generated.h"

// This class does not need to be modified
UINTERFACE(MinimalAPI, BlueprintType)
class UIQuidditchAgent : public UInterface
{
    GENERATED_BODY()
};

// Interface for Quidditch participants (Players and AI)
// Implement this on any Character/Pawn that plays Quidditch
class END2507_API IIQuidditchAgent
{
    GENERATED_BODY()

public:
    // ========================================================================
    // ROLE QUERIES - What is this agent?
    // ========================================================================

    // Returns the agent's assigned Quidditch role
    // Used by behavior trees and team manager for role-specific logic
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Quidditch|Role")
    EQuidditchRole GetQuidditchRole() const;

    // Sets the agent's Quidditch role
    // Called by TeamAIManager during match setup
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Quidditch|Role")
    void SetQuidditchRole(EQuidditchRole NewRole);

    // Returns the agent's team ID (0 or 1 in standard match)
    // Uses IGenericTeamAgentInterface under the hood
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Quidditch|Team")
    int32 GetQuidditchTeamID() const;

    // ========================================================================
    // STATE QUERIES - What is this agent doing?
    // ========================================================================

    // Is the agent currently mounted on a broom and flying?
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Quidditch|State")
    bool IsOnBroom() const;

    // Is the agent currently holding a ball?
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Quidditch|Ball")
    bool HasBall() const;

    // What type of ball is the agent holding (None if not holding)
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Quidditch|Ball")
    EQuidditchBall GetHeldBallType() const;

    // Get the agent's current location (for team coordination)
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Quidditch|State")
    FVector GetAgentLocation() const;

    // Get the agent's current velocity (for intercept calculations)
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Quidditch|State")
    FVector GetAgentVelocity() const;

    // ========================================================================
    // ACTIONS - What can this agent do?
    // ========================================================================

    // Called by AI or input to mount a broom
    // BroomActor can be nullptr for default
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Quidditch|Actions")
    bool TryMountBroom(AActor* BroomActor);

    // Called by AI or input to dismount from broom
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Quidditch|Actions")
    void DismountBroom();

    // Called by AI or input to pick up a nearby ball
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Quidditch|Actions")
    bool TryPickUpBall(AActor* Ball);

    // Called by AI or input to throw/release held ball toward target
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Quidditch|Actions")
    bool ThrowBallAtTarget(FVector TargetLocation);

    // Called by AI or input to pass ball to teammate
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Quidditch|Actions")
    bool PassBallToTeammate(AActor* Teammate);

    // ========================================================================
    // FLOCKING - Reynolds steering integration
    // Used by UFlyingSteeringComponent for team coordination
    // ========================================================================

    // Get agents that should be considered flock members for steering
    // Typically returns teammates within a certain radius
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Quidditch|Flocking")
    void GetFlockMembers(TArray<AActor*>& OutFlockMembers, float SearchRadius);

    // Get the tag used for flock identification
    // Agents with matching tags are considered part of the same flock
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Quidditch|Flocking")
    FName GetFlockTag() const;
};
