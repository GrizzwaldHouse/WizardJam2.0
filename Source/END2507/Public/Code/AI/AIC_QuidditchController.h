// ============================================================================
// AIC_QuidditchController.h
// AI Controller for Quidditch flying agents
//
// Developer: Marcus Daley
// Date: January 20, 2026
// Project: END2507
//
// PURPOSE:
// Manages AI agents that fly on brooms. Runs behavior tree on possess
// and provides SetFlightTarget() to update the blackboard destination.
// Includes AI Perception for detecting collectibles and other actors.
//
// USAGE:
// 1. Create Blueprint child class (BP_QuidditchAIController)
// 2. Assign BT_QuidditchAI to BehaviorTreeAsset property
// 3. Set as AIControllerClass on your AI pawn
//
// BLACKBOARD KEYS USED:
// - TargetLocation (Vector): Destination for flight
// - TargetActor (Object): Actor to follow (optional, overrides location)
// - IsFlying (Bool): Current flight state
// - SelfActor (Object): Reference to controlled pawn
// - PerceivedCollectible (Object): Nearest collectible from perception
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "GenericTeamAgentInterface.h"
#include "AIC_QuidditchController.generated.h"

class UBehaviorTree;
class UBlackboardComponent;
class UBlackboardData;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;

DECLARE_LOG_CATEGORY_EXTERN(LogQuidditchAI, Log, All);

UCLASS()
class END2507_API AAIC_QuidditchController : public AAIController
{
    GENERATED_BODY()

public:
    AAIC_QuidditchController();

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;

    // ========================================================================
    // PUBLIC API - Flight Target Control
    // ========================================================================

    UFUNCTION(BlueprintCallable, Category = "Quidditch|Flight")
    void SetFlightTarget(const FVector& TargetLocation);

    UFUNCTION(BlueprintCallable, Category = "Quidditch|Flight")
    void SetFlightTargetActor(AActor* TargetActor);

    UFUNCTION(BlueprintCallable, Category = "Quidditch|Flight")
    void ClearFlightTarget();

    UFUNCTION(BlueprintPure, Category = "Quidditch|Flight")
    bool GetFlightTarget(FVector& OutLocation) const;

    // ========================================================================
    // BLACKBOARD ACCESS
    // ========================================================================

    UFUNCTION(BlueprintCallable, Category = "Quidditch|Blackboard")
    void SetIsFlying(bool bIsFlying);

    UFUNCTION(BlueprintPure, Category = "Quidditch|Blackboard")
    bool GetIsFlying() const;

    // ========================================================================
    // TEAM INTERFACE - Required for AI perception filtering and collectibles
    // ========================================================================

    // Sets this controller's team ID and updates perception system
    virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;

    // Returns this controller's current team ID
    virtual FGenericTeamId GetGenericTeamId() const override;

protected:
    // ========================================================================
    // COMPONENTS
    // ========================================================================

    // AI Perception for detecting collectibles and other actors
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quidditch|Perception")
    UAIPerceptionComponent* AIPerceptionComp;

    // Sight sense configuration
    UPROPERTY()
    UAISenseConfig_Sight* SightConfig;

    // ========================================================================
    // CONFIGURATION - Assign in Blueprint child class
    // ========================================================================

    // Behavior tree to run when possessing a pawn
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quidditch|AI")
    UBehaviorTree* BehaviorTreeAsset;

    // Blackboard data asset (optional - can be set on behavior tree instead)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quidditch|AI")
    UBlackboardData* BlackboardAsset;

    // ========================================================================
    // PERCEPTION CONFIGURATION
    // ========================================================================

    UPROPERTY(EditDefaultsOnly, Category = "Quidditch|Perception")
    float SightRadius;

    UPROPERTY(EditDefaultsOnly, Category = "Quidditch|Perception")
    float LoseSightRadius;

    UPROPERTY(EditDefaultsOnly, Category = "Quidditch|Perception")
    float PeripheralVisionAngle;

    // ========================================================================
    // BLACKBOARD KEY NAMES
    // ========================================================================

    UPROPERTY(EditDefaultsOnly, Category = "Quidditch|Blackboard")
    FName TargetLocationKeyName;

    UPROPERTY(EditDefaultsOnly, Category = "Quidditch|Blackboard")
    FName TargetActorKeyName;

    UPROPERTY(EditDefaultsOnly, Category = "Quidditch|Blackboard")
    FName IsFlyingKeyName;

    UPROPERTY(EditDefaultsOnly, Category = "Quidditch|Blackboard")
    FName SelfActorKeyName;

    // ========================================================================
    // PERCEPTION HANDLING - Nick Penney Pattern
    // ========================================================================

    virtual void BeginPlay() override;

    // Called by AIPerception when any actor is sensed or lost
    UFUNCTION()
    void HandlePerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

private:
    void SetupBlackboard(APawn* InPawn);

    // Blackboard key for perceived collectible
    UPROPERTY(EditDefaultsOnly, Category = "Quidditch|Blackboard")
    FName PerceivedCollectibleKeyName;
};
