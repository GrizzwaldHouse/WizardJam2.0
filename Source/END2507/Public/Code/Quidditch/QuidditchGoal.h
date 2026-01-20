// ============================================================================
// QuidditchGoal.h (REFACTORED)
// Developer: Marcus Daley
// Date: January 7, 2026
// Project: WizardJam
// ============================================================================
// Purpose:
// Elemental goal post for Quidditch-style scoring system. Goals accept
// projectiles of matching elements and award points to the shooting team.
//
// REFACTORING CHANGES:
// - REMOVED: GetColorForElement() declaration
// - Color now queried from UElementDatabaseSubsystem in ApplyElementColor()
// - This eliminates duplicate color definitions across the codebase
//
// Designer Usage:
//   1. Place BP_QuidditchGoal in level
//   2. Set GoalElement from dropdown (Flame, Ice, Lightning, Arcane)
//   3. Set TeamID: 0 = Player goals (AI targets), 1 = AI goals (Player targets)
//   4. Color is auto-applied from DA_Elements Data Asset!
//
// How Element Matching Works:
//   1. Goal stores GoalElement as FName (designer-friendly)
//   2. Projectile stores SpellElement as FName (via GetSpellElement accessor)
//   3. When projectile hits goal:
//      a. Compare Goal's GoalElement to Projectile's SpellElement
//      b. Match = points, no match = 0 points
//      c. Broadcast scoring event to GameMode via delegate
//
// Observer Pattern:
//   Goal broadcasts -> GameMode receives -> GameMode broadcasts to HUD
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GenericTeamAgentInterface.h"
#include "QuidditchGoal.generated.h"

// Forward declarations
class UBoxComponent;
class UStaticMeshComponent;
class AProjectile;

DECLARE_LOG_CATEGORY_EXTERN(LogQuidditchGoal, Log, All);

// ============================================================================
// DELEGATE DEFINITION
// Delegate broadcast when goal is scored
// Parameters: ScoringActor (who scored), Element (spell type), PointsAwarded, bWasCorrectElement
// ============================================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
    FOnGoalScored,
    AActor*, ScoringActor,
    FName, Element,
    int32, PointsAwarded,
    bool, bWasCorrectElement
);
// Broadcast when ANY goal is scored
// GameMode subscribes to this ONE delegate, receives events from ALL goals
// Parameters: Goal (which goal), ScoringActor (who scored), Element, Points, bCorrect
DECLARE_MULTICAST_DELEGATE_FiveParams(
    FOnAnyGoalScored,
    AQuidditchGoal*,    // Goal - which goal was scored on
    AActor*,            // ScoringActor - who fired the projectile
    FName,              // ProjectileElement - what element was used
    int32,              // PointsAwarded - how many points
    bool                // bCorrectElement - was it the right element
);

// Broadcast when a goal spawns (optional - for tracking active goals)
DECLARE_MULTICAST_DELEGATE_OneParam(
    FOnGoalRegistered,
    AQuidditchGoal*     // The goal that just spawned
);

// Broadcast when a goal despawns (optional - for cleanup)
DECLARE_MULTICAST_DELEGATE_OneParam(
    FOnGoalUnregistered,
    AQuidditchGoal*     // The goal that's being destroyed
);

// ============================================================================
// INSTANCE DELEGATE DEFINITIONS
// These are per-goal delegates for Blueprint/local systems
// ============================================================================

// Per-instance delegate (for Blueprint binding on specific goals)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
    FOnGoalScoredInstance,
    AActor*, ScoringActor,
    FName, Element,
    int32, PointsAwarded,
    bool, bWasCorrectElement
);

// Per-instance wrong element event (for UI/audio on specific goals)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FOnWrongElementHit,
    AActor*, ShootingActor,
    FName, ProjectileElement,
    FName, RequiredElement
);
UCLASS()
class END2507_API AQuidditchGoal : public AActor, public IGenericTeamAgentInterface
{
    GENERATED_BODY()

public:
    AQuidditchGoal();

    // GameMode subscribes to this in its constructor
     // When ANY goal scores, this fires - no per-goal subscription needed
    static FOnAnyGoalScored OnAnyGoalScored;

    // Optional: Track when goals spawn/despawn
    static FOnGoalRegistered OnGoalRegistered;
    static FOnGoalUnregistered OnGoalUnregistered;

    // ========================================================================
    // DESIGNER CONFIGURATION
    // ========================================================================

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Goal|Element")
    FName GoalElement;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Goal|Team")
    int32 TeamID;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goal|Scoring")
    int32 CorrectElementPoints;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goal|Scoring")
    int32 WrongElementPoints;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Goal|Feedback")
    float HitFlashDuration;

    // ========================================================================
    // COMPONENTS
    // ========================================================================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* GoalMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBoxComponent* ScoringZone;

    // ========================================================================
    // INSTANCE DELEGATES 
    // ========================================================================

    // Per-instance scoring event (Blueprint can bind to specific goal)
    UPROPERTY(BlueprintAssignable, Category = "Goal|Events")
    FOnGoalScoredInstance OnGoalScored;

    // Per-instance wrong element event
    UPROPERTY(BlueprintAssignable, Category = "Goal|Events")
    FOnWrongElementHit OnWrongElementHit;

    // ========================================================================
    // RUNTIME STATE
    // ========================================================================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Goal|Runtime")
    FLinearColor CurrentColor;

    // Local flag set when match ends (no polling GameMode)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Goal|Runtime")
    bool bMatchEnded;

    // ========================================================================
    // MATCH END NOTIFICATION
    // Call this from GameMode when match ends - sets local flag on all goals
    // ========================================================================

    // Static function to notify ALL goals that match ended
    // GameMode calls this ONCE, all goals receive it
    static void NotifyAllGoalsMatchEnded();

    // ========================================================================
    // IGenericTeamAgentInterface
    // ========================================================================

    virtual FGenericTeamId GetGenericTeamId() const override;
    virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void PostInitializeComponents() override;

private:
    // ========================================================================
    // OVERLAP HANDLER
    // ========================================================================

    UFUNCTION()
    void OnScoringZoneBeginOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult
    );

    // ========================================================================
    // INTERNAL HELPERS
    // ========================================================================

    bool IsCorrectElement(AProjectile* InProjectile) const;
    int32 CalculatePoints(bool bCorrectElement) const;
    void ApplyElementColor();
    void PlayHitFeedback(bool bCorrectElement);

    // ========================================================================
    // INTERNAL STATE
    // ========================================================================

    FGenericTeamId TeamId;

    UPROPERTY()
    UMaterialInstanceDynamic* DynamicMaterial;

    FTimerHandle HitFlashTimer;

    // Static collection of all active goals (for NotifyAllGoalsMatchEnded)
    static TArray<AQuidditchGoal*> ActiveGoals;
};