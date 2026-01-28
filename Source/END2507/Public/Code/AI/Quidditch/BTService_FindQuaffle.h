// ============================================================================
// BTService_FindQuaffle.h
// Developer: Marcus Daley
// Date: January 24, 2026
// Project: END2507
// ============================================================================
// Purpose:
// BT Service that locates the Quaffle ball for Chaser AI agents.
// Uses AI perception with fallback to world search.
// Also tracks if Quaffle is held by teammate or opponent.
//
// Usage:
// 1. Attach to Chaser behavior branch
// 2. Configure SnitchClass to match BP_Quaffle
// 3. Outputs Quaffle actor, location, velocity, and possession state
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTService_FindQuaffle.generated.h"

// Forward declarations
class AAIController;
class AActor;

UCLASS(meta = (DisplayName = "Find Quaffle"))
class END2507_API UBTService_FindQuaffle : public UBTService
{
    GENERATED_BODY()

public:
    UBTService_FindQuaffle();

    // ========================================================================
    // BTService Interface
    // ========================================================================

    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

    virtual FString GetStaticDescription() const override;

    // ========================================================================
    // MANDATORY: FBlackboardKeySelector Initialization
    // ========================================================================

    virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

protected:
    // ========================================================================
    // OUTPUT BLACKBOARD KEYS
    // ========================================================================

    // Quaffle actor reference (Object)
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector QuaffleActorKey;

    // Quaffle world location (Vector)
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector QuaffleLocationKey;

    // Quaffle velocity for prediction (Vector)
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector QuaffleVelocityKey;

    // True if Quaffle is free (not held) (Bool)
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector IsQuaffleFreeKey;

    // True if teammate has Quaffle (Bool)
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector TeammateHasQuaffleKey;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    // Class of Quaffle actor to search for
    UPROPERTY(EditAnywhere, Category = "Perception")
    TSubclassOf<AActor> QuaffleClass;

    // Maximum range to perceive Quaffle
    UPROPERTY(EditAnywhere, Category = "Perception")
    float MaxQuaffleRange;

private:
    // Search perception for Quaffle
    AActor* FindQuaffleInPerception(AAIController* AIController) const;

    // Fallback world search
    AActor* FindQuaffleInWorld(UWorld* World) const;

    // Check if Quaffle is attached to another actor (held)
    bool IsQuaffleHeld(AActor* Quaffle, AActor*& OutHolder) const;

    // Check if holder is on same team
    bool IsHolderTeammate(AActor* Holder, APawn* OwnerPawn) const;
};

