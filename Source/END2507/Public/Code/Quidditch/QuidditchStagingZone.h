// ============================================================================
// QuidditchStagingZone.h
// Team formation and launch area for Quidditch matches
//
// Developer: Marcus Daley
// Date: January 22, 2026
// Project: WizardJam
// ============================================================================
// PURPOSE:
// Defines the staging area where AI agents fly to and hover while waiting
// for the match to begin. Each team has a formation zone with spawn points
// for the 7 positions (1 Seeker, 3 Chasers, 2 Beaters, 1 Keeper).
//
// DESIGNER WORKFLOW:
// 1. Place BP_QuidditchStagingZone in level
// 2. Set TeamID (0 or 1)
// 3. Position the actor at desired staging location
// 4. Optional: Adjust FormationRadius and HoverAltitude
// 5. AI agents will automatically navigate here when they acquire brooms
//
// AI INTEGRATION:
// BTService_UpdateFlightTarget queries the world for staging zones
// and writes the appropriate team's zone location to the blackboard.
// BTTask_ControlFlight then navigates to that location.
//
// MATCH FLOW:
// 1. AI spawns, acquires broom channel, mounts broom
// 2. AI queries staging zone for its team
// 3. AI flies to staging zone and hovers
// 4. When all positions filled, staging zone broadcasts OnTeamReady
// 5. GameMode receives OnTeamReady from both zones, starts match
// 6. Match start triggers LaunchTeam() which tells AI to fly to play area
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Code/Quidditch/QuidditchTypes.h"
#include "QuidditchStagingZone.generated.h"

// Forward declarations
class UBoxComponent;
class USphereComponent;
class UBillboardComponent;
class UArrowComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogQuidditchStaging, Log, All);

// ============================================================================
// DELEGATES
// ============================================================================

// Broadcast when enough agents have arrived to fill all roles
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnTeamReadyAtStaging,
    AQuidditchStagingZone*, StagingZone,
    int32, TeamID
);

// Broadcast when team is launched to start match
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnTeamLaunched,
    AQuidditchStagingZone*, StagingZone,
    int32, TeamID
);

// Broadcast when an agent arrives at staging zone
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FOnAgentArrivedAtStaging,
    AQuidditchStagingZone*, StagingZone,
    AActor*, Agent,
    int32, TeamID
);

// Static delegate for GameMode to receive events from ANY staging zone
DECLARE_MULTICAST_DELEGATE_TwoParams(
    FOnAnyStagingZoneReady,
    AQuidditchStagingZone*,  // StagingZone
    int32                     // TeamID
);

UCLASS()
class END2507_API AQuidditchStagingZone : public AActor
{
    GENERATED_BODY()

public:
    AQuidditchStagingZone();

    // ========================================================================
    // STATIC DELEGATES - GameMode subscribes once
    // ========================================================================

    // GameMode binds to this in constructor
    static FOnAnyStagingZoneReady OnAnyStagingZoneReady;

    // ========================================================================
    // DESIGNER CONFIGURATION
    // ========================================================================

    // Which team uses this staging zone
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Staging|Team")
    int32 TeamID;

    // Display color for editor visualization
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Staging|Visuals")
    FLinearColor TeamColor;

    // Radius of the formation area - agents spread within this
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Staging|Formation")
    float FormationRadius;

    // Altitude above staging zone origin for hovering
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Staging|Formation")
    float HoverAltitude;

    // Distance within which an agent is considered "arrived"
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Staging|Detection")
    float ArrivalRadius;

    // Number of agents required before team is considered ready
    // Set to 7 for full Quidditch team, lower for testing
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Staging|Requirements")
    int32 RequiredAgentCount;

    // ========================================================================
    // COMPONENTS
    // ========================================================================

    // Root scene component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* RootSceneComponent;

    // Detection volume for arriving agents
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* ArrivalZone;

    // Visual indicator in editor
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBillboardComponent* EditorBillboard;

    // Arrow showing launch direction
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UArrowComponent* LaunchDirectionArrow;

    // ========================================================================
    // INSTANCE DELEGATES - Blueprint binding
    // ========================================================================

    UPROPERTY(BlueprintAssignable, Category = "Staging|Events")
    FOnTeamReadyAtStaging OnTeamReady;

    UPROPERTY(BlueprintAssignable, Category = "Staging|Events")
    FOnTeamLaunched OnTeamLaunchedEvent;

    UPROPERTY(BlueprintAssignable, Category = "Staging|Events")
    FOnAgentArrivedAtStaging OnAgentArrived;

    // ========================================================================
    // PUBLIC API - Used by AI and GameMode
    // ========================================================================

    // Get the target location for AI to fly to (center + hover altitude)
    UFUNCTION(BlueprintCallable, Category = "Staging")
    FVector GetStagingTargetLocation() const;

    // Get formation position for a specific role
    // Returns offset from center based on role
    UFUNCTION(BlueprintCallable, Category = "Staging")
    FVector GetFormationPositionForRole(EQuidditchRole Role) const;

    // Check if team has enough agents to be ready
    UFUNCTION(BlueprintCallable, Category = "Staging")
    bool IsTeamReady() const;

    // Get number of agents currently staged
    UFUNCTION(BlueprintCallable, Category = "Staging")
    int32 GetStagedAgentCount() const;

    // Manually register an agent (called by AI when it enters zone)
    UFUNCTION(BlueprintCallable, Category = "Staging")
    void RegisterStagedAgent(AActor* Agent);

    // Remove agent from staging (if they leave or die)
    UFUNCTION(BlueprintCallable, Category = "Staging")
    void UnregisterStagedAgent(AActor* Agent);

    // Called by GameMode to launch team into match
    UFUNCTION(BlueprintCallable, Category = "Staging")
    void LaunchTeam();

    // Get the launch direction (arrow direction in world space)
    UFUNCTION(BlueprintCallable, Category = "Staging")
    FVector GetLaunchDirection() const;

    // ========================================================================
    // STATIC HELPERS - Find staging zones in world
    // ========================================================================

    // Find staging zone for a specific team
    UFUNCTION(BlueprintCallable, Category = "Staging", meta = (WorldContext = "WorldContextObject"))
    static AQuidditchStagingZone* FindStagingZoneForTeam(const UObject* WorldContextObject, int32 InTeamID);

    // Get all staging zones in level
    UFUNCTION(BlueprintCallable, Category = "Staging", meta = (WorldContext = "WorldContextObject"))
    static TArray<AQuidditchStagingZone*> GetAllStagingZones(const UObject* WorldContextObject);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Editor visualization
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    // ========================================================================
    // OVERLAP HANDLERS
    // ========================================================================

    UFUNCTION()
    void OnArrivalZoneBeginOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult
    );

    UFUNCTION()
    void OnArrivalZoneEndOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex
    );

    // ========================================================================
    // INTERNAL STATE
    // ========================================================================

    // Agents currently in staging zone
    UPROPERTY()
    TArray<AActor*> StagedAgents;

    // Has team been launched?
    bool bTeamLaunched;

    // Static collection of all staging zones
    static TArray<AQuidditchStagingZone*> AllStagingZones;

    // ========================================================================
    // INTERNAL HELPERS
    // ========================================================================

    void UpdateEditorVisualization();
    void CheckTeamReadyState();
    bool IsValidQuidditchAgent(AActor* Actor) const;
};
