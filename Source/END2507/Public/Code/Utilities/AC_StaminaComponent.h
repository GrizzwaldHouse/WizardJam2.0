// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AC_StaminaComponent.generated.h"

// ============================================================================
// DELEGATES
// ============================================================================

// Broadcast when stamina value changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnStaminaChanged,
	AActor*, Owner,
	float, NewStamina,
	float, Delta
);

// Broadcast when stamina is fully depleted
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnStaminaDepleted,
	AActor*, Owner
);

// Broadcast when stamina is fully restored
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnStaminaRestored,
	AActor*, Owner
);

// ============================================================================
// COMPONENT CLASS
// ============================================================================
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class END2507_API UAC_StaminaComponent : public UActorComponent
{
	GENERATED_BODY()

public:
    UAC_StaminaComponent();

    // ========== INITIALIZATION ==========

    // Initialize stamina to specified maximum value (also sets current to max)
    UFUNCTION(BlueprintCallable, Category = "Stamina")
    void Initialize(float InMaxStamina);

    // ========== SPRINT MANAGEMENT ==========

    // Set sprinting state - drains stamina when true, regenerates when false
    UFUNCTION(BlueprintCallable, Category = "Stamina")
    void SetSprinting(bool bNewSprinting);

    // Check if actor has enough stamina to sprint
    UFUNCTION(BlueprintPure, Category = "Stamina")
    bool CanSprint() const;

    // Check if actor is currently sprinting
    UFUNCTION(BlueprintPure, Category = "Stamina")
    bool IsSprinting() const { return bIsSprinting; }

    // ========== ABILITY USAGE ==========

    // Consume stamina for ability usage (spells, dash, etc.)
    // Returns true if enough stamina was available and consumed
    UFUNCTION(BlueprintCallable, Category = "Stamina")
    bool ConsumeStamina(float Amount);

    // Restore stamina from pickups or buffs
    // Returns actual amount restored (may be less if near max)
    UFUNCTION(BlueprintCallable, Category = "Stamina")
    float RestoreStamina(float Amount);

    // ========== GETTERS ==========

    UFUNCTION(BlueprintPure, Category = "Stamina")
    float GetStaminaPercent() const;

    UFUNCTION(BlueprintPure, Category = "Stamina")
    float GetCurrentStamina() const { return CurrentStamina; }

    UFUNCTION(BlueprintPure, Category = "Stamina")
    float GetMaxStamina() const { return MaxStamina; }

    // Check if stamina is full
    UFUNCTION(BlueprintPure, Category = "Stamina")
    bool IsStaminaFull() const { return CurrentStamina >= MaxStamina; }

    // Check if stamina is empty
    UFUNCTION(BlueprintPure, Category = "Stamina")
    bool IsStaminaDepleted() const { return CurrentStamina <= 0.0f; }

    // ========== DELEGATES ==========

    // Broadcast when stamina changes - bind HUD here
    UPROPERTY(BlueprintAssignable, Category = "Stamina|Events")
    FOnStaminaChanged OnStaminaChanged;

    // Broadcast when stamina hits zero
    UPROPERTY(BlueprintAssignable, Category = "Stamina|Events")
    FOnStaminaDepleted OnStaminaDepleted;

    // Broadcast when stamina reaches max
    UPROPERTY(BlueprintAssignable, Category = "Stamina|Events")
    FOnStaminaRestored OnStaminaRestored;

    // ========== PUBLIC CONFIGURATION ==========
    // EditDefaultsOnly per Nick Penney standards

    // Maximum stamina value
    UPROPERTY(EditDefaultsOnly, Category = "Stamina", meta = (ClampMin = "0.0"))
    float MaxStamina;

    // Current stamina - visible for debugging but not editable
    UPROPERTY(VisibleAnywhere, Category = "Stamina")
    float CurrentStamina;

protected:
    virtual void BeginPlay() override;

    // ========== CONFIGURATION ==========

    // How fast stamina regenerates per second when not sprinting
    UPROPERTY(EditDefaultsOnly, Category = "Stamina", meta = (ClampMin = "0.0"))
    float StaminaRegenRate;

    // How fast stamina drains per second while sprinting
    UPROPERTY(EditDefaultsOnly, Category = "Stamina", meta = (ClampMin = "0.0"))
    float StaminaDrainRate;

    // Delay before regeneration begins after sprinting stops (seconds)
    UPROPERTY(EditDefaultsOnly, Category = "Stamina", meta = (ClampMin = "0.0"))
    float RegenDelay;

    // ========== INTERNAL STATE ==========

    bool bIsSprinting;
    float RegenDelayTimer;

    UPROPERTY()
    AActor* OwnerActor;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

private:
    // Internal method to apply stamina change and broadcast
    void ApplyStaminaChange(float Delta);

    // Check if broadcast thresholds are crossed
    void CheckThresholds(float OldStamina, float NewStamina);
};
