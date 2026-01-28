// ============================================================================
// BTService_SyncFlightState.cpp
// Implementation of flight state synchronization service
//
// Developer: Marcus Daley
// Date: January 23, 2026
// Project: WizardJam (END2507)
// ============================================================================

#include "Code/AI/BTService_SyncFlightState.h"
#include "Code/Flight/AC_BroomComponent.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"

DEFINE_LOG_CATEGORY_STATIC(LogSyncFlightState, Log, All);

// ============================================================================
// CONSTRUCTOR
// ============================================================================

UBTService_SyncFlightState::UBTService_SyncFlightState()
{
	NodeName = "Sync Flight State";

	// Default tick interval - 0.25s is responsive without being excessive
	Interval = 0.25f;
	RandomDeviation = 0.0f;

	// Add bool filter for the key selector
	IsFlyingKey.AddBoolFilter(this,
		GET_MEMBER_NAME_CHECKED(UBTService_SyncFlightState, IsFlyingKey));
}

// ============================================================================
// INITIALIZE FROM ASSET
// ============================================================================

void UBTService_SyncFlightState::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		IsFlyingKey.ResolveSelectedKey(*BBAsset);

		UE_LOG(LogSyncFlightState, Log,
			TEXT("[SyncFlightState] Resolved IsFlyingKey '%s' against blackboard '%s'"),
			*IsFlyingKey.SelectedKeyName.ToString(),
			*BBAsset->GetName());
	}
}

// ============================================================================
// TICK NODE
// ============================================================================

void UBTService_SyncFlightState::TickNode(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory,
	float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC)
	{
		return;
	}

	APawn* Pawn = AIC->GetPawn();
	if (!Pawn)
	{
		return;
	}

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB || !IsFlyingKey.IsSet())
	{
		return;
	}

	// Get the BroomComponent from the pawn
	// This may be nullptr if agent hasn't mounted a broom yet
	UAC_BroomComponent* BroomComp = Pawn->FindComponentByClass<UAC_BroomComponent>();

	// Determine actual flight state
	bool bIsActuallyFlying = BroomComp ? BroomComp->IsFlying() : false;

	// Get current blackboard value to check if we need to update
	bool bCurrentBBValue = BB->GetValueAsBool(IsFlyingKey.SelectedKeyName);

	// Only update if the value has changed to reduce unnecessary broadcasts
	if (bCurrentBBValue != bIsActuallyFlying)
	{
		BB->SetValueAsBool(IsFlyingKey.SelectedKeyName, bIsActuallyFlying);

		UE_LOG(LogSyncFlightState, Display,
			TEXT("[SyncFlightState] %s | IsFlying changed: %s -> %s"),
			*Pawn->GetName(),
			bCurrentBBValue ? TEXT("true") : TEXT("false"),
			bIsActuallyFlying ? TEXT("true") : TEXT("false"));
	}
}

// ============================================================================
// DESCRIPTION
// ============================================================================

FString UBTService_SyncFlightState::GetStaticDescription() const
{
	FString KeyName = IsFlyingKey.IsSet() ? IsFlyingKey.SelectedKeyName.ToString() : TEXT("NOT SET!");
	return FString::Printf(TEXT("Sync BroomComponent.IsFlying() -> '%s'\nInterval: %.2fs"),
		*KeyName, Interval);
}
