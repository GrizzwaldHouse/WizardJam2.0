// ============================================================================
// BTService_SyncFlightState.h
// Continuously syncs actual flight state from BroomComponent to Blackboard
//
// Developer: Marcus Daley
// Date: January 23, 2026
// Project: WizardJam (END2507)
// ============================================================================
// PURPOSE:
// The IsFlying blackboard key must reflect the ACTUAL flight state from
// AC_BroomComponent, not just whether the agent has a channel. This service
// reads BroomComponent->IsFlying() every tick and updates the blackboard.
//
// USAGE:
// 1. Add this service to the Mount Broom branch (or Root for global sync)
// 2. Set IsFlyingKey to the "IsFlying" blackboard key
// 3. Service will update the key at the configured interval (default 0.25s)
//
// REQUIREMENTS:
// - Pawn must have UAC_BroomComponent (can be added dynamically by BroomActor)
// - Blackboard must have a Bool key for IsFlying
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_SyncFlightState.generated.h"

UCLASS()
class END2507_API UBTService_SyncFlightState : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_SyncFlightState();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual FString GetStaticDescription() const override;

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	// Blackboard key to store the IsFlying state
	// Must be a Bool key type
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector IsFlyingKey;
};
