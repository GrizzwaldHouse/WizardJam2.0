// ============================================================================
// ISpellProjectile.h
// Developer: Marcus Daley
// Date: January 19, 2026
// ============================================================================
// Purpose:
// Interface for projectiles that carry spell element information. This allows
// QuidditchGoal and other systems to query element type WITHOUT casting to a
// specific projectile class.
//
// Why Use Interface Instead of Cast:
// 1. Decouples goal from specific projectile implementation
// 2. Allows multiple projectile classes to work with goals
// 3. Follows Nick's coding standards (no tight coupling)
// 4. Makes testing easier (can mock projectiles)
//
// Usage:
// Any projectile class that should work with elemental goals must:
// 1. Inherit from ISpellProjectile
// 2. Implement GetSpellElement_Implementation()
// 3. Return the projectile's element type as FName
//
// Example Implementation in Projectile.h:
//   class AProjectile : public AActor, public ISpellProjectile
//   {
//       virtual FName GetSpellElement_Implementation() const override
//       {
//           return SpellElement;
//       }
//   };
//
// Example Query in QuidditchGoal.cpp:
//   if (OtherActor->Implements<USpellProjectile>())
//   {
//       FName Element = ISpellProjectile::Execute_GetSpellElement(OtherActor);
//   }
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ISpellProjectile.generated.h"

// This class does not need to be modified
UINTERFACE(MinimalAPI, Blueprintable)
class USpellProjectile : public UInterface
{
    GENERATED_BODY()
};

// ============================================================================
// INTERFACE DEFINITION
// ============================================================================

class END2507_API ISpellProjectile
{
    GENERATED_BODY()

public:
    // ========================================================================
    // ELEMENT IDENTITY
    // ========================================================================

    // Get the spell element type (Flame, Ice, Lightning, Arcane, etc.)
    // QuidditchGoal uses this for elemental matching without casting
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Spell|Element")
    FName GetSpellElement() const;

    // Get the element color for visual feedback
    // Optional - return White if not implemented
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Spell|Element")
    FLinearColor GetSpellColor() const;

    // ========================================================================
    // OWNERSHIP
    // ========================================================================

    // Get the actor that fired this projectile
    // Used for scoring attribution
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Spell|Ownership")
    AActor* GetProjectileOwner() const;
};
