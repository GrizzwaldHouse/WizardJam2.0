// ============================================================================
// BTService_TrackNearestSeeker.cpp
// Developer: Marcus Daley
// Date: January 26, 2026
// Project: END2507 / WizardJam
// ============================================================================
// KEY FIXES APPLIED:
// 1. Added AddObjectFilter() in constructor for FBlackboardKeySelector
// 2. Added InitializeFromAsset() override with ResolveSelectedKey() call
// Without these, NearestSeekerKey.IsSet() returns false even when configured!
// ============================================================================

#include "Code/AI/BTService_TrackNearestSeeker.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "Perception/AIPerceptionComponent.h"

DEFINE_LOG_CATEGORY(LogTrackSeeker);

UBTService_TrackNearestSeeker::UBTService_TrackNearestSeeker()
	: MaxTrackingDistance(0.0f)
{
	NodeName = TEXT("Track Nearest Seeker");
	Interval = 0.2f;
	RandomDeviation = 0.05f;

	// Default threat tags - designer can modify in BT editor
	ValidThreatTags.Add(FName(TEXT("Seeker")));
	ValidThreatTags.Add(FName(TEXT("Player")));

	// CRITICAL FIX #1: Add object filter so the editor knows what key types are valid
	// Without this, the key dropdown shows options but IsSet() returns false at runtime
	NearestSeekerKey.AddObjectFilter(
		this,
		GET_MEMBER_NAME_CHECKED(UBTService_TrackNearestSeeker, NearestSeekerKey),
		AActor::StaticClass());
}

void UBTService_TrackNearestSeeker::InitializeFromAsset(UBehaviorTree& Asset)
{
	// Always call parent first
	Super::InitializeFromAsset(Asset);

	// CRITICAL FIX #2: Resolve the key selector against the blackboard asset
	// The editor stores a string key name, but at runtime we need to bind it
	// to the actual blackboard slot. Without this, IsSet() returns false!
	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		NearestSeekerKey.ResolveSelectedKey(*BBAsset);

		UE_LOG(LogTrackSeeker, Log,
			TEXT("[TrackSeeker] Resolved NearestSeekerKey '%s' against blackboard '%s'"),
			*NearestSeekerKey.SelectedKeyName.ToString(),
			*BBAsset->GetName());
	}
}

void UBTService_TrackNearestSeeker::TickNode(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC)
	{
		UE_LOG(LogTrackSeeker, Warning, TEXT("[TrackSeeker] No AIController!"));
		return;
	}

	APawn* OwnerPawn = AIC->GetPawn();
	if (!OwnerPawn)
	{
		UE_LOG(LogTrackSeeker, Warning, TEXT("[TrackSeeker] No Pawn!"));
		return;
	}

	UAIPerceptionComponent* PerceptionComp = AIC->GetPerceptionComponent();
	if (!PerceptionComp)
	{
		UE_LOG(LogTrackSeeker, Warning, TEXT("[TrackSeeker] No PerceptionComponent!"));
		return;
	}

	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	if (!BBComp)
	{
		UE_LOG(LogTrackSeeker, Warning, TEXT("[TrackSeeker] No BlackboardComponent!"));
		return;
	}

	// Validate the output key is properly configured
	if (!NearestSeekerKey.IsSet())
	{
		UE_LOG(LogTrackSeeker, Error,
			TEXT("[TrackSeeker] NearestSeekerKey not set! Full rebuild required after code changes."));
		return;
	}

	// Find the nearest threat actor
	AActor* NearestThreat = FindNearestThreatFromPerception(
		PerceptionComp,
		OwnerPawn->GetActorLocation());

	// Write to blackboard - clears key if no threat found
	BBComp->SetValueAsObject(NearestSeekerKey.SelectedKeyName, NearestThreat);

	// Log the result
	if (NearestThreat)
	{
		UE_LOG(LogTrackSeeker, Display,
			TEXT("[TrackSeeker] %s -> Found threat: %s"),
			*OwnerPawn->GetName(),
			*NearestThreat->GetName());
	}
	else
	{
		UE_LOG(LogTrackSeeker, Verbose,
			TEXT("[TrackSeeker] %s -> No threats detected"),
			*OwnerPawn->GetName());
	}
}

AActor* UBTService_TrackNearestSeeker::FindNearestThreatFromPerception(
	UAIPerceptionComponent* PerceptionComp,
	const FVector& OwnerLocation) const
{
	TArray<AActor*> PerceivedActors;
	PerceptionComp->GetCurrentlyPerceivedActors(nullptr, PerceivedActors);

	AActor* NearestThreat = nullptr;
	float NearestDistSq = (MaxTrackingDistance > 0.0f)
		? FMath::Square(MaxTrackingDistance)
		: TNumericLimits<float>::Max();

	for (AActor* Actor : PerceivedActors)
	{
		if (!Actor)
		{
			continue;
		}

		// Check if actor has valid threat tag
		if (!HasValidThreatTag(Actor))
		{
			continue;
		}

		// Check distance
		const float DistSq = FVector::DistSquared(OwnerLocation, Actor->GetActorLocation());
		if (DistSq < NearestDistSq)
		{
			NearestDistSq = DistSq;
			NearestThreat = Actor;
		}
	}

	return NearestThreat;
}

bool UBTService_TrackNearestSeeker::HasValidThreatTag(const AActor* Actor) const
{
	if (!Actor || ValidThreatTags.Num() == 0)
	{
		return false;
	}

	for (const FName& Tag : ValidThreatTags)
	{
		if (Actor->ActorHasTag(Tag))
		{
			return true;
		}
	}

	return false;
}

FString UBTService_TrackNearestSeeker::GetStaticDescription() const
{
	FString TagList;
	for (int32 i = 0; i < ValidThreatTags.Num(); i++)
	{
		if (i > 0)
		{
			TagList += TEXT(", ");
		}
		TagList += ValidThreatTags[i].ToString();
	}

	FString KeyTarget = NearestSeekerKey.IsSet()
		? NearestSeekerKey.SelectedKeyName.ToString()
		: TEXT("NOT SET!");

	return FString::Printf(TEXT("Track [%s] -> %s"), *TagList, *KeyTarget);
}
