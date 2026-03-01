 // ============================================================================
// WizardPlayer.h
// Wizard player character for Quidditch gameplay
//
// Developer: Marcus Daley
// Date: January 20, 2026
// Project: WizardJam / GAR (END2507)
//
// PURPOSE:
// Main player character class for Quidditch mode. Inherits from InputCharacter
// for Enhanced Input System integration and implements IQuidditchAgent interface
// for Quidditch-specific functionality.
//
// ARCHITECTURE:
// - InputCharacter: Provides camera, movement, look, jump, sprint, spell input
// - IQuidditchAgent: Provides Quidditch role, ball handling, broom mounting
// - IGenericTeamAgentInterface: Provides team system integration
//
// COMPONENTS:
// - UAC_BroomComponent: Flight mechanics (from existing flight system)
// - UAC_StaminaComponent: Stamina for flight/sprint (from existing system)
// - UAC_HealthComponent: Health (from existing system)
// - UInteractionComponent: Object interaction (for broom mounting)
//
// INPUT ACTIONS REQUIRED (set in Blueprint):
// - IA_Move, IA_Look, IA_Jump, IA_Sprint (from InputCharacter)
// - IA_Fire (spell casting)
// - IA_Interact (broom mount/dismount, ball pickup)
// - IA_ToggleFlight (manual flight toggle)
// - IA_ThrowBall (throw held ball)
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Code/Actors/InputCharacter.h"
#include "GenericTeamAgentInterface.h"
#include "Code/Quidditch/IQuidditchAgent.h"
#include "Code/Quidditch/QuidditchTypes.h"
#include "WizardPlayer.generated.h"

// Forward declarations
class UAC_BroomComponent;
class UAC_StaminaComponent;
class UAC_HealthComponent;
class UInteractionComponent;
class UInputAction;
class UUserWidget;

DECLARE_LOG_CATEGORY_EXTERN(LogWizardPlayer, Log, All);

// ============================================================================
// DELEGATES
// ============================================================================

// Broadcast when player's Quidditch role changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerRoleChanged, EQuidditchRole, NewRole);

// Broadcast when player picks up or drops a ball
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerBallChanged, EQuidditchBall, NewBall);

UCLASS()
class END2507_API AWizardPlayer : public AInputCharacter,
    public IGenericTeamAgentInterface,
    public IIQuidditchAgent
{
    GENERATED_BODY()

public:
    AWizardPlayer();

    // ========================================================================
    // IGenericTeamAgentInterface Implementation
    // ========================================================================

    virtual FGenericTeamId GetGenericTeamId() const override;
    virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;

    // ========================================================================
    // IQuidditchAgent Implementation
    // ========================================================================

    virtual EQuidditchRole GetQuidditchRole_Implementation() const override;
    virtual void SetQuidditchRole_Implementation(EQuidditchRole NewRole) override;
    virtual int32 GetQuidditchTeamID_Implementation() const override;
    virtual bool IsOnBroom_Implementation() const override;
    virtual bool HasBall_Implementation() const override;
    virtual EQuidditchBall GetHeldBallType_Implementation() const override;
    virtual FVector GetAgentLocation_Implementation() const override;
    virtual FVector GetAgentVelocity_Implementation() const override;
    virtual bool TryMountBroom_Implementation(AActor* BroomActor) override;
    virtual void DismountBroom_Implementation() override;
    virtual bool TryPickUpBall_Implementation(AActor* Ball) override;
    virtual bool ThrowBallAtTarget_Implementation(FVector TargetLocation) override;
    virtual bool PassBallToTeammate_Implementation(AActor* Teammate) override;
    virtual void GetFlockMembers_Implementation(TArray<AActor*>& OutFlockMembers, float SearchRadius) override;
    virtual FName GetFlockTag_Implementation() const override;

    // ========================================================================
    // COMPONENTS - Public for Blueprint access
    // ========================================================================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UAC_BroomComponent* BroomComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UAC_StaminaComponent* StaminaComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UAC_HealthComponent* HealthComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UInteractionComponent* InteractionComponent;

    // ========================================================================
    // DELEGATES - For UI and game systems
    // ========================================================================

    UPROPERTY(BlueprintAssignable, Category = "Wizard|Events")
    FOnPlayerRoleChanged OnRoleChanged;

    UPROPERTY(BlueprintAssignable, Category = "Wizard|Events")
    FOnPlayerBallChanged OnBallChanged;

    // ========================================================================
    // PUBLIC API
    // ========================================================================

    /** Get the broom component */
    UFUNCTION(BlueprintPure, Category = "Wizard|Flight")
    UAC_BroomComponent* GetBroomComponent() const { return BroomComponent; }

    /** Get stamina component */
    UFUNCTION(BlueprintPure, Category = "Wizard|Stamina")
    UAC_StaminaComponent* GetStaminaComponent() const { return StaminaComponent; }

    /** Get health component */
    UFUNCTION(BlueprintPure, Category = "Wizard|Health")
    UAC_HealthComponent* GetHealthComponent() const { return HealthComponent; }

    /** Check if player is currently flying */
    UFUNCTION(BlueprintPure, Category = "Wizard|Flight")
    bool IsFlying() const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    // ========================================================================
    // INPUT HANDLERS - Override from InputCharacter
    // ========================================================================

    virtual void HandleFireInput() override;
    virtual void OnSprintStarted() override;
    virtual void OnSprintStopped() override;

    // ========================================================================
    // QUIDDITCH INPUT HANDLERS
    // ========================================================================

    /** Handle throw ball input */
    void HandleThrowBallInput();

    /** Handle toggle flight input */
    void HandleToggleFlightInput();

    // ========================================================================
    // COMPONENT EVENT HANDLERS
    // ========================================================================

    UFUNCTION()
    void OnHealthChanged(float HealthRatio);

    UFUNCTION()
    void OnDeath(AActor* DeadActor);

    UFUNCTION()
    void OnFlightStateChanged(bool bIsFlying);

    UFUNCTION()
    void OnStaminaDepleted(AActor* DepletedActor);

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    /** Player's team ID (0 = player team, 1 = AI team typically) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wizard|Team")
    int32 PlayerTeamID;

    /** Player's assigned Quidditch role */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wizard|Quidditch")
    EQuidditchRole QuidditchRole;

    /** Throw force when throwing balls */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wizard|Combat")
    float BallThrowForce;

    /** HUD widget classes to spawn - designer adds multiple widgets to this array
     *  All widgets are created and added to viewport at BeginPlay
     *  Example: Add WBP_WizardJamHUD for spell/stamina, WBP_PlayerHUD for ammo */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wizard|UI")
    TArray<TSubclassOf<UUserWidget>> HUDWidgetClasses;

    // ========================================================================
    // ADDITIONAL INPUT ACTIONS
    // ========================================================================

    /** Input action for throwing held ball */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Quidditch")
    UInputAction* ThrowBallAction;

    /** Input action for toggling flight (alternative to interact with broom) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Quidditch")
    UInputAction* ToggleFlightAction;

private:
    // ========================================================================
    // RUNTIME STATE
    // ========================================================================

    /** Team ID stored for interface */
    FGenericTeamId TeamId;

    /** Currently held ball type */
    EQuidditchBall HeldBall;

    /** Reference to held ball actor */
    UPROPERTY()
    AActor* HeldBallActor;

    /** HUD widget instances - matches HUDWidgetClasses array */
    UPROPERTY()
    TArray<UUserWidget*> HUDWidgetInstances;

    // ========================================================================
    // INTERNAL HELPERS
    // ========================================================================

    void SetupHUD();
    void BindComponentDelegates();
    void UpdateHUDStamina(float StaminaPercent);
    void UpdateHUDHealth(float HealthPercent);
};
