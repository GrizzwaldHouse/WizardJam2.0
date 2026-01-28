// ============================================================================
// BroomActor.h
// World-placeable broom that grants flight with designer-configurable behavior
//
// Developer: Marcus Daley
// Date: January 20, 2026
// Project: WizardJam
// ============================================================================
// PURPOSE:
// This is the configurable entity that designers create child Blueprints from.
// Each child Blueprint (BP_Broom_FreeFlight, BP_Broom_Quidditch, etc.) has
// different settings in the BroomConfiguration struct.
//
// The AC_BroomComponent reads its behavior FROM this actor's configuration.
// This means zero C++ changes are needed for new broom types.
//
// ARCHITECTURE (Following ElementDatabase pattern):
// - BroomTypes.h defines FBroomConfiguration struct
// - BroomActor.h exposes the struct for designer editing
// - AC_BroomComponent reads from GetBroomConfiguration()
// - Designer creates BP children with different configurations
//
// DESIGNER WORKFLOW:
// 1. Create child Blueprint of BroomActor (e.g., BP_Broom_Quidditch)
// 2. In Details panel, expand "Broom Configuration"
// 3. Configure all flight behavior settings
// 4. Place broom in level or reference in spawner
// 5. Player interacts -> Component reads this actor's configuration
//
// EXAMPLE BROOM VARIANTS:
// - BP_Broom_FreeFlight: Timed, constant drain, dismount on depletion
// - BP_Broom_Quidditch: Infinite, movement drain, regen when idle
// - BP_Broom_Racing: Fast, no deceleration, high drain
// - BP_Broom_Stealth: Slow, minimal stamina, quiet audio
//
// INTEGRATION WITH CHANNEL SYSTEM:
// Each broom can require a different channel to unlock:
// - "BroomFlight" for basic broom
// - "QuidditchBroom" for Quidditch-specific broom
// - "RacingLicense" for racing broom
// Player collects unlock item -> gains channel -> can interact with that broom type
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Code/Interfaces/Interactable.h"
#include "Code/Flight/BroomTypes.h"
#include "BroomActor.generated.h"

// Forward declarations
class UStaticMeshComponent;
class UAC_BroomComponent;
class UAIPerceptionStimuliSourceComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogBroomActor, Log, All);

// ============================================================================
// DELEGATES
// ============================================================================

// Broadcast when a player mounts this broom
// GameMode can listen to track active flyers
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBroomMounted, ABroomActor*, Broom, AActor*, Rider);

// Broadcast when a player dismounts this broom
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBroomDismounted, ABroomActor*, Broom, AActor*, Rider);

// ============================================================================
// BROOM ACTOR CLASS
// ============================================================================

UCLASS()
class END2507_API ABroomActor : public AActor, public IInteractable
{
    GENERATED_BODY()

public:
    ABroomActor();

    // ========================================================================
    // PUBLIC API - Used by AC_BroomComponent and other systems
    // ========================================================================

    // Returns the configuration struct for this broom
    // AC_BroomComponent calls this to get all flight behavior settings
    UFUNCTION(BlueprintPure, Category = "Broom")
    const FBroomConfiguration& GetBroomConfiguration() const { return BroomConfiguration; }

    // Returns the broom's visual mesh component
    // Used for socket attachment when mounting
    UFUNCTION(BlueprintPure, Category = "Broom")
    UStaticMeshComponent* GetBroomMesh() const { return BroomMesh; }

    // Check if this broom is currently being ridden
    UFUNCTION(BlueprintPure, Category = "Broom")
    bool IsBeingRidden() const { return CurrentRider != nullptr; }

    // Get the current rider (nullptr if not being ridden)
    UFUNCTION(BlueprintPure, Category = "Broom")
    AActor* GetCurrentRider() const { return CurrentRider; }

    // Called by AC_BroomComponent when flight ends
    // Resets broom state so it can be mounted again
    UFUNCTION(BlueprintCallable, Category = "Broom")
    void OnRiderDismounted();

    // ========================================================================
    // DELEGATES
    // ========================================================================

    UPROPERTY(BlueprintAssignable, Category = "Broom|Events")
    FOnBroomMounted OnBroomMounted;

    UPROPERTY(BlueprintAssignable, Category = "Broom|Events")
    FOnBroomDismounted OnBroomDismounted;

    // ========================================================================
    // IINTERACTABLE INTERFACE IMPLEMENTATION
    // ========================================================================

    // Returns broom display name for hover tooltip
    virtual FText GetTooltipText_Implementation() const override;

    // Returns interaction prompt or locked message based on channel requirements
    virtual FText GetInteractionPrompt_Implementation() const override;

    // Returns detailed description of broom capabilities
    virtual FText GetDetailedInfo_Implementation() const override;

    // Checks if player has required channel to use this broom
    virtual bool CanInteract_Implementation() const override;

    // Mounts player to broom, passes configuration to AC_BroomComponent
    virtual void OnInteract_Implementation(AActor* Interactor) override;

    // Returns interaction range
    virtual float GetInteractionRange_Implementation() const override;

protected:
    virtual void BeginPlay() override;

    // ========================================================================
    // COMPONENTS
    // ========================================================================

    // Root component - collision/attachment anchor (leave mesh empty)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* BroomMesh;

    // Visual mesh component - designers assign static mesh and adjust transform here
    // Separated from root to allow independent transform adjustment
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* BroomVisual;

    // AI Perception component so AI agents can detect this broom
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UAIPerceptionStimuliSourceComponent* PerceptionSource;

    // ========================================================================
    // DESIGNER CONFIGURATION
    // This is THE configuration struct - all broom behavior is defined here
    // Designer creates child Blueprints with different configurations
    // ========================================================================

    // All broom flight settings - designer configures in Blueprint child
    // See BroomTypes.h for full list of configurable properties
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Broom Configuration",
        meta = (ShowOnlyInnerProperties))
    FBroomConfiguration BroomConfiguration;

    // ========================================================================
    // AI PERCEPTION SETTINGS
    // ========================================================================

    // Whether to automatically register for AI sight perception
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Broom|AI")
    bool bAutoRegisterForSight;

    // ========================================================================
    // INTERACTION SETTINGS
    // ========================================================================

    // Maximum distance from which player can interact with this broom
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    float InteractionRange;

    // Prompt text shown when player can mount ("Press E to mount")
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FText MountPromptText;

    // Prompt text shown when player lacks required channel
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FText LockedPromptText;

    // Text shown when another player is riding
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FText InUseText;

    // ========================================================================
    // RUNTIME STATE
    // ========================================================================

    // Track who is currently riding this broom
    UPROPERTY()
    AActor* CurrentRider;

    // Cache the rider's broom component for dismount notification
    UPROPERTY()
    UAC_BroomComponent* RiderBroomComponent;

    // ========================================================================
    // HELPER FUNCTIONS
    // ========================================================================

    // Checks if interacting actor has the required channel
    bool HasRequiredChannel(AActor* Interactor) const;

    // Gets existing or creates new AC_BroomComponent on the interactor
    UAC_BroomComponent* GetOrCreateBroomComponent(AActor* Interactor);

    // Applies this broom's configuration to the component
    void ConfigureBroomComponent(UAC_BroomComponent* BroomComp);

    // Attaches broom mesh to rider's socket
    void AttachBroomToRider(AActor* Rider);

    // Detaches broom mesh from rider
    void DetachBroomFromRider();
};