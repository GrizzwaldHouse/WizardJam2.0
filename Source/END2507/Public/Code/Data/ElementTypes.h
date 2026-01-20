// ============================================================================
// ElementTypes.h
// Developer: Marcus Daley
// Date: January 7, 2026
// ============================================================================
// Purpose:
// Defines FElementDefinition struct containing all properties for a single
// element type. This is the data structure stored in UElementDatabase.
//
// Usage:
// - Include this header in any file that needs element data
// - Query via UElementDatabaseSubsystem::Get(this)->GetElementDefinition()
// - Designer configures values in DA_Elements Data Asset
//
// Why Data Asset over Enum:
// - FName-based element names allow designer to add new elements without C++
// - Single source of truth eliminates duplicate color definitions
// - Hot-reloadable during PIE for fast iteration
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "Sound/SoundBase.h"
#include "Particles/ParticleSystem.h"
#include "ElementTypes.generated.h"

// ============================================================================
// FElementDefinition
// Contains all visual, gameplay, and audio properties for one element type
// ============================================================================

USTRUCT(BlueprintType)
struct END2507_API FElementDefinition
{
    GENERATED_BODY()

    // ========================================================================
    // IDENTIFICATION
    // ========================================================================

    // Unique identifier for this element
    // Must match SpellCollectible SpellTypeName, QuidditchGoal GoalElement, etc.
    // Examples: "Flame", "Ice", "Lightning", "Arcane"
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Element")
    FName ElementName;

    // ========================================================================
    // VISUAL PROPERTIES
    // ========================================================================

    // Display color used for goals, collectibles, projectiles, UI tinting
    // This is the SINGLE source of truth for element colors
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Element|Visuals")
    FLinearColor Color;

    // Icon texture for HUD spell slots (unlocked state)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Element|Visuals")
    TSoftObjectPtr<UTexture2D> Icon;

    // Icon texture for HUD spell slots (locked/unavailable state)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Element|Visuals")
    TSoftObjectPtr<UTexture2D> LockedIcon;

    // Emissive multiplier for glowing effects
    // Higher values = brighter glow on goals, walls, collectibles
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Element|Visuals",
        meta = (ClampMin = "0.5", ClampMax = "10.0"))
    float EmissiveMultiplier;

    // ========================================================================
    // GAMEPLAY PROPERTIES
    // ========================================================================

    // Base points awarded when scoring with this element (Quidditch goals)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Element|Gameplay",
        meta = (ClampMin = "0"))
    int32 BasePoints;

    // Damage multiplier for combat (future use)
    // 1.0 = normal, 2.0 = double damage, 0.5 = half damage
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Element|Gameplay",
        meta = (ClampMin = "0.1", ClampMax = "5.0"))
    float DamageMultiplier;

    // ========================================================================
    // AUDIO PROPERTIES
    // ========================================================================

    // Sound played when collecting this spell type
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Element|Audio")
    TSoftObjectPtr<USoundBase> CollectSound;

    // Sound played when projectile hits with this element
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Element|Audio")
    TSoftObjectPtr<USoundBase> HitSound;

    // ========================================================================
    // VFX PROPERTIES
    // ========================================================================

    // Particle effect for projectile trails
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Element|VFX")
    TSoftObjectPtr<UParticleSystem> ProjectileTrail;

    // Particle effect for impact/explosion
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Element|VFX")
    TSoftObjectPtr<UParticleSystem> ImpactEffect;

    // ========================================================================
    // UI PROPERTIES
    // ========================================================================

    // Display name shown in UI (can include spaces, special chars)
    // If empty, ElementName is used
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Element|UI")
    FText DisplayName;

    // Sort order for UI display (0 = first, higher = later)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Element|UI",
        meta = (ClampMin = "0"))
    int32 SortOrder;

    // ========================================================================
    // CONSTRUCTOR
    // ========================================================================

    FElementDefinition()
        : ElementName(NAME_None)
        , Color(FLinearColor::White)
        , EmissiveMultiplier(2.0f)
        , BasePoints(100)
        , DamageMultiplier(1.0f)
        , SortOrder(0)
    {
    }

    // ========================================================================
    // UTILITY METHODS
    // ========================================================================

    // Returns true if this definition has a valid element name
    bool IsValid() const
    {
        return ElementName != NAME_None;
    }

    // Gets display name, falling back to ElementName if not set
    FText GetDisplayName() const
    {
        if (DisplayName.IsEmpty())
        {
            return FText::FromName(ElementName);
        }
        return DisplayName;
    }
};