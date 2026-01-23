// ============================================================================
// BTService_UpdateFlightTarget.h
// Behavior Tree service that updates flight target from staging zone
//
// Developer: Marcus Daley
// Date: January 22, 2026
// Project: WizardJam
//
// PURPOSE:
// Continuously updates the flight target location in the blackboard by
// querying the QuidditchStagingZone for the AI's team. This service works
// with BTTask_ControlFlight which reads the target and navigates to it.
//
// WORKFLOW:
// 1. Service starts when AI enters flight navigation branch
// 2. Each tick: Query world for QuidditchStagingZone matching AI's team
// 3. Write staging zone's GetStagingTargetLocation() to blackboard
// 4. BTTask_ControlFlight reads this location and flies toward it
//
// USAGE IN BEHAVIOR TREE:
// Sequence (with BTDecorator_IsFlying)
//   ├── BTService_UpdateFlightTarget (this service)
//   └── BTTask_ControlFlight (reads TargetLocation key)
//
// CONFIGURATION:
// - OutputLocationKey: Blackboard key to write target location (Vector)
// - TeamIDKey: Optional blackboard key holding agent's team ID
// - DefaultTeamID: Fallback team if TeamIDKey not set
//
// CRITICAL INITIALIZATION:
// - FBlackboardKeySelector requires AddVectorFilter() in constructor
// - FBlackboardKeySelector requires InitializeFromAsset() override
// - Without BOTH, IsSet() returns false even when configured in editor!
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTService_UpdateFlightTarget.generated.h"

class AQuidditchStagingZone;

UCLASS(meta = (DisplayName = "Update Flight Target"))
class END2507_API UBTService_UpdateFlightTarget : public UBTService
{
    GENERATED_BODY()

public:
    UBTService_UpdateFlightTarget();

    // Called periodically while service is active
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

    // Description shown in Behavior Tree editor
    virtual FString GetStaticDescription() const override;

    // CRITICAL: Resolves the blackboard key selector at runtime
    virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

protected:
    // ========================================================================
    // OUTPUT CONFIGURATION
    // ========================================================================

    // Blackboard key to write target location (Vector type)
    UPROPERTY(EditAnywhere, Category = "Flight Target")
    FBlackboardKeySelector OutputLocationKey;

    // ========================================================================
    // TEAM CONFIGURATION
    // ========================================================================

    // Optional: Blackboard key holding agent's team ID (Int type)
    // If not set, uses DefaultTeamID instead
    UPROPERTY(EditAnywhere, Category = "Flight Target")
    FBlackboardKeySelector TeamIDKey;

    // Fallback team ID if TeamIDKey is not set or invalid
    UPROPERTY(EditAnywhere, Category = "Flight Target")
    int32 DefaultTeamID;

    // ========================================================================
    // TARGET MODE
    // ========================================================================

    // If true, tracks a moving actor (like following another player)
    // If false, tracks the static staging zone location
    UPROPERTY(EditAnywhere, Category = "Flight Target")
    bool bFollowMovingTarget;

    // Optional: Blackboard key for actor to follow (Object type)
    // Only used if bFollowMovingTarget is true
    UPROPERTY(EditAnywhere, Category = "Flight Target",
        meta = (EditCondition = "bFollowMovingTarget"))
    FBlackboardKeySelector FollowActorKey;

private:
    // Helper to get team ID from blackboard or default
    int32 GetAgentTeamID(UBehaviorTreeComponent& OwnerComp) const;

    // Helper to find staging zone and get target location
    bool GetStagingZoneTarget(UBehaviorTreeComponent& OwnerComp, FVector& OutLocation) const;

    // Helper to get follow target location
    bool GetFollowTargetLocation(UBehaviorTreeComponent& OwnerComp, FVector& OutLocation) const;
};
