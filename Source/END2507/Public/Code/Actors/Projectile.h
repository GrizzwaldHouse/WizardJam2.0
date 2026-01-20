// ============================================================================
// Projectile.h
// Developer: Marcus Daley
// Date: September 29, 2025 (Updated January 19, 2026)
// ============================================================================
// Purpose:
// Projectile actor with color, damage, combat faction, and spell element support.
// Used for both basic combat projectiles and elemental spell projectiles.
//
// Update Notes (Jan 2026):
// Added SpellElement property for QuidditchGoal elemental matching system.
// Goals check GetSpellElement() to determine if projectile matches goal element.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UMaterialInstanceDynamic;

// Combat faction for friendly fire prevention
UENUM(BlueprintType)
enum class ECombatFaction : uint8
{
    Unknown     UMETA(DisplayName = "Unknown"),
    Player      UMETA(DisplayName = "Player"),
    Agent       UMETA(DisplayName = "Agent")
};

UCLASS()
class END2507_API AProjectile : public AActor
{
    GENERATED_BODY()

public:
    AProjectile();

    // Initialize projectile with damage, direction, and faction
    UFUNCTION(BlueprintCallable, Category = "Projectile")
    void InitializeProjectile(float InDamage, const FVector& Direction, ECombatFaction Faction);

    // Pool system callback
    void SetPoolReturnCallback(TFunction<void(AProjectile*)> Callback);

    // Set the projectile color 
    UFUNCTION(BlueprintCallable, Category = "Projectile")
    void SetColor(const FLinearColor& NewColor);

    // Activate from object pool
    void ActivateProjectile(const FVector& SpawnLocation, const FRotator& SpawnRotation,
        const FVector& Direction, float InDamage, AActor* NewOwner, ECombatFaction Faction);

    // Get combat faction for friendly fire checks
    ECombatFaction GetCombatFaction() const;

    // Get the actor that fired this projectile
    AActor* GetOwnerPawn() const;

    // Check if projectile is active in pool
    UFUNCTION(BlueprintPure, Category = "Projectile")
    bool IsActiveInPool() const;

    // ========================================================================
    // SPELL ELEMENT SYSTEM (Added for QuidditchGoal matching)
    // ========================================================================

    // Get the spell element type (Flame, Ice, Lightning, Arcane, etc.)
    // QuidditchGoal checks this to determine if projectile matches goal element
    UFUNCTION(BlueprintPure, Category = "Projectile|Spell")
    FName GetSpellElement() const { return SpellElement; }

    // Set the spell element type
    // Called when spawning elemental projectiles
    UFUNCTION(BlueprintCallable, Category = "Projectile|Spell")
    void SetSpellElement(FName NewElement) { SpellElement = NewElement; }

protected:
    virtual void BeginPlay() override;
    virtual void PostInitializeComponents() override;

    // ========================================================================
    // COMPONENTS
    // ========================================================================

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* SphereCollision;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* SphereMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UProjectileMovementComponent* ProjectileMovement;

    UPROPERTY()
    UMaterialInstanceDynamic* DynamicMaterial;

    // ========================================================================
    // PROJECTILE PROPERTIES
    // ========================================================================

    UPROPERTY(BlueprintReadOnly, Category = "Projectile")
    FVector Size;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
    float Damage;

    UPROPERTY(BlueprintReadOnly, Category = "Projectile")
    FLinearColor ProjectileColor;

    // Combat faction of shooter (for friendly fire prevention)
    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    ECombatFaction CombatFaction;

    // ========================================================================
    // SPELL ELEMENT (Added for elemental system)
    // ========================================================================

    // Spell element type for elemental matching
    // Designer sets this in Blueprint child classes (BP_Projectile_Flame, etc.)
    // QuidditchGoal uses this to check if projectile matches goal element
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Spell",
        meta = (DisplayName = "Spell Element"))
    FName SpellElement;

    // ========================================================================
    // INTERNAL STATE
    // ========================================================================

    UPROPERTY()
    TWeakObjectPtr<AActor> OwnerPawn;

    bool bMaterialInitialized;

    UFUNCTION()
    void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int OtherBodyIndex, bool fromSweep,
        const FHitResult& SweepResult);

public:
    virtual void Tick(float DeltaTime) override;

private:
    FTimerHandle ExpirationTimer;
    TFunction<void(AProjectile*)> PoolReturnCallback;
    bool bActiveInPool;

    void ApplyDamageToActor(AActor* HitActor);
    bool IsFriendly(AActor* OtherActor) const;

    static const FName ColorParamName;
};