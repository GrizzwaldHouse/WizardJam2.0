// ============================================================================
// SpellCollectible.h
// Developer: Marcus Daley
// Date: December 25, 2025
// ============================================================================
// Purpose:
// Spell pickup actor that grants spells to actors implementing ISpellCollector.
//
// Designer Workflow :
// 1. Create Blueprint child of SpellCollectible
// 2. Set SpellTypeName and SpellColor
// 3. Configure collector filters and channel requirements
// 4. Place in level
//
// Actor Requirements:
// - Implement ISpellCollector interface
// - Have UAC_SpellCollectionComponent attached
// - Return component from GetSpellCollectionComponent()
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Code/Actors/CollectiblePickup.h"
#include "SpellCollectible.generated.h"

// Forward declarations
class AActor;
class ASpellCollectible;
class UAC_SpellCollectionComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMeshComponent;
class USoundBase;

DECLARE_LOG_CATEGORY_EXTERN(LogSpellCollectible, Log, All);

// ============================================================================
// STATIC DELEGATE
// GameMode binds once at startup, receives events from ALL spell collectibles
// ============================================================================

DECLARE_MULTICAST_DELEGATE_ThreeParams(
    FOnSpellPickedUpGlobal,
    FName,                  // SpellTypeName
    AActor*,                // Collecting actor
    ASpellCollectible*      // This collectible instance
);

// ============================================================================
// INSTANCE DELEGATES
// Per-collectible events for Blueprint VFX/SFX
// ============================================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnSpellPickedUpInstance,
    FName, SpellTypeName,
    AActor*, CollectingActor
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FOnSpellPickupDenied,
    AActor*, AttemptingActor,
    FName, MissingRequirement,
    FString, DenialReason
);

// ============================================================================
// SPELL COLLECTIBLE CLASS
// ============================================================================

UCLASS()
class END2507_API ASpellCollectible : public ACollectiblePickup
{
    GENERATED_BODY()

public:
    ASpellCollectible();

    // Static delegate - GameMode binds to this
    static FOnSpellPickedUpGlobal OnAnySpellPickedUp;

    // ========================================================================
    // ACCESSORS
    // ========================================================================

    UFUNCTION(BlueprintPure, Category = "Spell")
    FName GetSpellTypeName() const { return SpellTypeName; }

    UFUNCTION(BlueprintPure, Category = "Spell")
    FLinearColor GetSpellColor() const { return SpellColor; }

    // ========================================================================
    // REQUIREMENT CHECKING
    // Use for UI to show locked state before player touches
    // ========================================================================

    // Full validation - all requirements must pass
    UFUNCTION(BlueprintPure, Category = "Spell|Requirements")
    bool CanActorCollect(AActor* Actor) const;

    // Check just the collector type filter (team-based)
    // Uses inherited bPlayerCanCollect, bEnemyCanCollect, bCompanionCanCollect
    UFUNCTION(BlueprintPure, Category = "Spell|Requirements")
    bool IsAllowedCollectorType(AActor* Actor) const;

    // Check just channel requirements
    UFUNCTION(BlueprintPure, Category = "Spell|Requirements")
    bool MeetsChannelRequirements(AActor* Actor) const;

    // Get list of missing channels for UI feedback
    UFUNCTION(BlueprintPure, Category = "Spell|Requirements")
    TArray<FName> GetMissingChannels(AActor* Actor) const;

    // ========================================================================
    // INSTANCE DELEGATES
    // ========================================================================

    UPROPERTY(BlueprintAssignable, Category = "Spell|Events")
    FOnSpellPickedUpInstance OnSpellPickedUp;

    UPROPERTY(BlueprintAssignable, Category = "Spell|Events")
    FOnSpellPickupDenied OnPickupDenied;

protected:
    virtual void BeginPlay() override;
    virtual bool CanPickup(AActor* OtherActor) override;
    virtual void HandlePickup(AActor* OtherActor) override;

    // ========================================================================
    // SPELL IDENTITY
    // ========================================================================

    // Spell type name - designer types any string
    // Examples: "Flame", "Ice", "Lightning", "Arcane", "Void", "Storm"
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spell",
        meta = (DisplayName = "Spell Type Name"))
    FName SpellTypeName;

    // Spell color - applied to mesh materials automatically
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spell|Visuals",
        meta = (DisplayName = "Spell Color"))
    FLinearColor SpellColor;

    // ========================================================================
    // COLLECTOR TYPE FILTERS
    // NOTE: bPlayerCanCollect, bEnemyCanCollect, bCompanionCanCollect
    // are INHERITED from CollectiblePickup base class!
    // Configure them in Blueprint Class Defaults - no need to redeclare here.
    // ========================================================================

    // ========================================================================
    // CHANNEL REQUIREMENTS
    // ========================================================================

    // Channels collector must have BEFORE pickup is allowed
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Spell|Requirements",
        meta = (DisplayName = "Required Channels"))
    TArray<FName> RequiredChannels;

    // If true, must have ALL required channels (AND)
    // If false, need only ONE channel (OR)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spell|Requirements",
        meta = (DisplayName = "Require All Channels"))
    bool bRequireAllChannels;

    // Channels granted on successful collection
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Spell|Requirements",
        meta = (DisplayName = "Grants Channels"))
    TArray<FName> GrantsChannels;

    // ========================================================================
    // FEEDBACK CONFIGURATION
    // ========================================================================

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spell|Feedback")
    USoundBase* DeniedSound;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spell|Feedback")
    FString DeniedMessage;

private:
    // ========================================================================
    // MATERIALS
    // ========================================================================

    UPROPERTY()
    UMaterialInterface* ProjectColorableMaterial;

    UPROPERTY()
    UMaterialInterface* EngineColorableMaterial;

    UPROPERTY()
    TArray<UMaterialInstanceDynamic*> DynamicMaterials;

    TArray<FName> ColorParameterNames;

    // ========================================================================
    // INTERNAL FUNCTIONS
    // ========================================================================

    void SetupSpellAppearance();

    bool TryApplyColorToMaterial(UMaterialInterface* BaseMaterial,
        UStaticMeshComponent* MeshComp, int32 SlotIndex, FName& OutWorkingParam);

    FName FindWorkingColorParameter(UMaterialInterface* Material) const;

    void GrantChannelsToCollector(UAC_SpellCollectionComponent* SpellComponent);

    void HandleDenied(AActor* Actor, const FString& Reason, FName MissingRequirement);

    // Helper to get SpellCollectionComponent from actor via interface
    UAC_SpellCollectionComponent* GetCollectorComponent(AActor* Actor) const;

    // Helper to check team filter using INHERITED properties from CollectiblePickup
    bool CheckTeamFilter(int32 TeamID) const;
};