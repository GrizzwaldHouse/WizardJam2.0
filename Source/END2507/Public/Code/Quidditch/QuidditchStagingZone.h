// ============================================================================
// QuidditchStagingZone.h
// Developer: Marcus Daley
// Date: January 23, 2026 (Refactored January 28, 2026)
// Project: END2507
// ============================================================================
// Purpose:
// A simple "landing zone" actor that broadcasts its presence via AI Perception.
// Following the "Bee and Flower" pattern - the flower (zone) knows nothing
// about the bee (agent). It just broadcasts "I exist here" and the bee
// decides whether to land on it.
//
// Bee and Flower Pattern:
// - FLOWER (this class): Just broadcasts location via perception. Knows nothing
//   about teams, roles, or who should land here.
// - BEE (agent): Perceives flowers, decides which one to fly to based on its
//   own team/role logic, and broadcasts "I landed" when it arrives.
// - HIVE (GameMode): Listens for "I landed" broadcasts from agents.
//
// This is modular - agents can use this same pattern to fly to ANY perceptible
// target (staging zones, health pickups, objectives, etc.)
//
// Usage:
// 1. Place QuidditchStagingZone actors in level
// 2. Optionally set ZoneIdentifier tag for agent-side filtering
// 3. Agents perceive zones while flying
// 4. Agent decides which zone matches their team/role
// 5. When agent lands (overlaps), AGENT broadcasts the landing event
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "QuidditchStagingZone.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UBillboardComponent;
class UAIPerceptionStimuliSourceComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogQuidditchStagingZone, Log, All);

// Delegate for when ANY pawn enters this zone - zone doesn't filter, just reports
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnZoneEntered, AActor*, Zone, APawn*, EnteredPawn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnZoneExited, AActor*, Zone, APawn*, ExitedPawn);

UCLASS()
class END2507_API AQuidditchStagingZone : public AActor
{
    GENERATED_BODY()

public:
    AQuidditchStagingZone();

    // ========================================================================
    // ZONE IDENTIFICATION (for agent-side filtering)
    // The zone doesn't use these - agents read them to decide if this is
    // the right zone for them.
    // ========================================================================

    // Simple identifier tag - agents can filter by this
    // Examples: "TeamA_Seeker", "TeamB_Keeper", "Neutral_Spawn"
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Zone Identity")
    FName ZoneIdentifier;

    // Optional team hint (agents read this, zone doesn't use it)
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Zone Identity")
    int32 TeamHint;

    // Optional role hint (agents read this, zone doesn't use it)
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Zone Identity")
    int32 RoleHint;

    // ========================================================================
    // EVENTS - Observer Pattern (zone broadcasts, others listen)
    // ========================================================================

    // Broadcast when ANY pawn enters - zone doesn't filter
    UPROPERTY(BlueprintAssignable, Category = "Zone Events")
    FOnZoneEntered OnZoneEntered;

    // Broadcast when ANY pawn exits
    UPROPERTY(BlueprintAssignable, Category = "Zone Events")
    FOnZoneExited OnZoneExited;

    // ========================================================================
    // QUERIES
    // ========================================================================

    UFUNCTION(BlueprintPure, Category = "Zone")
    bool IsOccupied() const { return OccupyingPawn.IsValid(); }

    UFUNCTION(BlueprintPure, Category = "Zone")
    APawn* GetOccupyingPawn() const { return OccupyingPawn.Get(); }

    UFUNCTION(BlueprintPure, Category = "Zone")
    FName GetZoneIdentifier() const { return ZoneIdentifier; }

protected:
    virtual void BeginPlay() override;

    // ========================================================================
    // COMPONENTS
    // ========================================================================

    // Trigger volume for detecting arrival
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* TriggerVolume;

    // Visual representation (optional)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* VisualMesh;

    // Editor billboard for visibility
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBillboardComponent* EditorBillboard;

    // AI Perception source - so agents can perceive this zone
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UAIPerceptionStimuliSourceComponent* PerceptionSource;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    // Radius of the trigger volume
    UPROPERTY(EditDefaultsOnly, Category = "Zone")
    float TriggerRadius;

private:
    // Currently occupying pawn (if any) - just tracking, no logic
    TWeakObjectPtr<APawn> OccupyingPawn;

    // Overlap handlers - just broadcast events, no filtering
    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp,
        AActor* OtherActor, UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnOverlapEnd(UPrimitiveComponent* OverlappedComp,
        AActor* OtherActor, UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex);
};
