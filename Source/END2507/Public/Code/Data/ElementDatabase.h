// ============================================================================
// ElementDatabase.h
// Developer: Marcus Daley
// Date: January 7, 2026
// ============================================================================
// Purpose:
// Data Asset that contains all element definitions for WizardJam.
// This is the SINGLE SOURCE OF TRUTH for element properties.
//
// Designer Workflow:
// 1. Right-click Content Browser -> Miscellaneous -> Data Asset
// 2. Select ElementDatabase class
// 3. Name it "DA_Elements" and save to Content/Data/
// 4. Add entries for Flame, Ice, Lightning, Arcane
// 5. Configure colors, points, sounds, VFX per element
//
// Code Access:
// - Via subsystem: UElementDatabaseSubsystem::Get(this)->GetColorForElement("Flame")
// - Direct reference: DatabaseAsset->GetElementDefinition("Flame", OutDef)
//
// IMPORTANT:
// FElementDefinition is defined in ElementTypes.h - do NOT duplicate here!
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ElementTypes.h"
#include "ElementDatabase.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogElementDatabase, Log, All);

// ============================================================================
// UElementDatabase
// Primary Data Asset for element configuration
// NOTE: FElementDefinition struct is in ElementTypes.h
// ============================================================================

UCLASS(BlueprintType)
class END2507_API UElementDatabase : public UDataAsset
{
    GENERATED_BODY()

public:
    // ========================================================================
    // ELEMENT DEFINITIONS
    // Designer adds entries here - one per element type
    // ========================================================================

    // All element definitions in this database
    // Designer configures: Flame, Ice, Lightning, Arcane (and any custom elements)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Elements",
        meta = (TitleProperty = "ElementName"))
    TArray<FElementDefinition> Elements;

    // ========================================================================
    // LOOKUP FUNCTIONS
    // ========================================================================

    // Get full element definition by name
    // Returns true if found, false if element doesn't exist
    UFUNCTION(BlueprintPure, Category = "Elements")
    bool GetElementDefinition(FName ElementName, FElementDefinition& OutDefinition) const;

    // Get just the color for an element (most common query)
    // Returns White if element not found
    UFUNCTION(BlueprintPure, Category = "Elements")
    FLinearColor GetColorForElement(FName ElementName) const;

    // Get base points for an element
    // Returns 0 if element not found
    UFUNCTION(BlueprintPure, Category = "Elements")
    int32 GetPointsForElement(FName ElementName) const;

    // Get all element names in this database (sorted by SortOrder)
    UFUNCTION(BlueprintPure, Category = "Elements")
    TArray<FName> GetAllElementNames() const;

    // Get number of elements in database
    UFUNCTION(BlueprintPure, Category = "Elements")
    int32 GetElementCount() const { return Elements.Num(); }

    // ========================================================================
    // NORMALIZATION
    // Handles common typos and case variations
    // ========================================================================

    // Normalize element name to canonical form
    // Handles "Lighting" -> "Lightning" typo
    // Handles case insensitivity
    UFUNCTION(BlueprintPure, Category = "Elements")
    FName NormalizeElementName(FName Element) const;

    // Check if two element names refer to the same element (after normalization)
    UFUNCTION(BlueprintPure, Category = "Elements")
    bool ElementsMatch(FName ElementA, FName ElementB) const;

    // ========================================================================
    // VALIDATION
    // ========================================================================

    // Check if an element exists in this database
    UFUNCTION(BlueprintPure, Category = "Elements")
    bool HasElement(FName ElementName) const;

#if WITH_EDITOR
    // Called when asset is saved - validates all entries
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    // Internal lookup - returns nullptr if not found
    const FElementDefinition* FindElement(FName ElementName) const;
};