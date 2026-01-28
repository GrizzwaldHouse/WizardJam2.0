// ============================================================================
// BTService_TrackNearestSeeker.h
// Developer: Marcus Daley
// Date: January 26, 2026
// Project: END2507 / WizardJam
// ============================================================================
// Purpose:
// BT Service that tracks the nearest actor with specified threat tags using
// AI perception. Writes the nearest threat to a blackboard key for use by
// evasion tasks. Generic implementation works with any AIController that has
// a perception component.
//
// USAGE:
// 1. Add as service to a BT composite node (Selector/Sequence)
// 2. Set NearestSeekerKey to an Object-type blackboard key
// 3. Configure ValidThreatTags to specify which actors are threats
// 4. Optionally set MaxTrackingDistance to limit detection range
//
// CRITICAL INITIALIZATION:
// - FBlackboardKeySelector requires AddObjectFilter() in constructor
// - FBlackboardKeySelector requires InitializeFromAsset() override
// - Without BOTH, IsSet() returns false even when configured in editor!
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTService_TrackNearestSeeker.generated.h"

class UAIPerceptionComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogTrackSeeker, Log, All);

UCLASS(meta = (DisplayName = "Track Nearest Seeker"))
class END2507_API UBTService_TrackNearestSeeker : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_TrackNearestSeeker();

	// Called periodically while service is active
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	// Description shown in Behavior Tree editor
	virtual FString GetStaticDescription() const override;

	// CRITICAL: Resolves the blackboard key selector at runtime
	// Without this override, NearestSeekerKey.IsSet() returns false!
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

protected:
	// ========================================================================
	// BLACKBOARD CONFIGURATION
	// ========================================================================

	// Blackboard key to store the nearest threat actor (Object type)
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector NearestSeekerKey;

	// ========================================================================
	// TRACKING CONFIGURATION
	// ========================================================================

	// Tags that identify valid threats - actor must have at least one of these
	// Default: "Seeker", "Player"
	UPROPERTY(EditAnywhere, Category = "Tracking")
	TArray<FName> ValidThreatTags;

	// Maximum distance to track threats (0 = unlimited)
	UPROPERTY(EditAnywhere, Category = "Tracking", meta = (ClampMin = "0.0"))
	float MaxTrackingDistance;

private:
	// Find nearest threat from perception component's currently perceived actors
	AActor* FindNearestThreatFromPerception(
		UAIPerceptionComponent* PerceptionComp,
		const FVector& OwnerLocation) const;

	// Check if actor has any of the configured threat tags
	bool HasValidThreatTag(const AActor* Actor) const;
};
