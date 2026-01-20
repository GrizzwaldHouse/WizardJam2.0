// ============================================================================
// ElementDatabaseSubsystem.h
// Developer: Marcus Daley
// Date: January 7, 2026
// ============================================================================
// Purpose:
// Game Instance Subsystem providing global access to the Element Database.
// This is the PRIMARY way to query element data from gameplay code.
//
// Usage:
// if (UElementDatabaseSubsystem* ElementDB = UElementDatabaseSubsystem::Get(this))
// {
//     FLinearColor GoalColor = ElementDB->GetColorForElement("Flame");
// }
//
// Why Subsystem:
// - Automatically created per GameInstance
// - Global access without singletons or statics
// - Proper lifecycle management
// - Blueprint accessible
// - Doesn't require passing database reference everywhere
//
// Database Loading:
// - Set DefaultDatabasePath in Project Settings or GameInstance
// - Or call SetDatabase() at runtime from GameMode::BeginPlay
// - Fallback: Looks for "DA_Elements" in Content/Data/
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ElementTypes.h"
#include "ElementDatabaseSubsystem.generated.h"

// Forward declaration
class UElementDatabase;

DECLARE_LOG_CATEGORY_EXTERN(LogElementSubsystem, Log, All);

// ============================================================================
// UElementDatabaseSubsystem
// Global accessor for element data
// ============================================================================

UCLASS()
class END2507_API UElementDatabaseSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ========================================================================
    // STATIC ACCESSOR
    // Preferred way to get the subsystem from any UObject
    // ========================================================================

    // Get the subsystem from any world context object
    // Returns nullptr if called before GameInstance exists or after shutdown
    UFUNCTION(BlueprintPure, Category = "Elements",
        meta = (WorldContext = "WorldContextObject"))
    static UElementDatabaseSubsystem* Get(const UObject* WorldContextObject);

    // ========================================================================
    // DATABASE MANAGEMENT
    // ========================================================================

    // Get the currently loaded database (may be nullptr before Init)
    UFUNCTION(BlueprintPure, Category = "Elements")
    UElementDatabase* GetDatabase() const { return Database; }

    // Set the database at runtime
    // Call from GameMode::BeginPlay if using level-specific databases
    UFUNCTION(BlueprintCallable, Category = "Elements")
    void SetDatabase(UElementDatabase* NewDatabase);

    // Check if database is loaded and valid
    UFUNCTION(BlueprintPure, Category = "Elements")
    bool IsDatabaseValid() const;

    // ========================================================================
    // FORWARDING FUNCTIONS
    // Convenience wrappers that null-check the database
    // ========================================================================

    // Get color for an element (White if not found or no database)
    UFUNCTION(BlueprintPure, Category = "Elements")
    FLinearColor GetColorForElement(FName ElementName) const;

    // Get points for an element (0 if not found or no database)
    UFUNCTION(BlueprintPure, Category = "Elements")
    int32 GetPointsForElement(FName ElementName) const;

    // Get full element definition
    // Returns false if element not found or no database
    UFUNCTION(BlueprintPure, Category = "Elements")
    bool GetElementDefinition(FName ElementName, FElementDefinition& OutDefinition) const;

    // Get all element names in sorted order
    UFUNCTION(BlueprintPure, Category = "Elements")
    TArray<FName> GetAllElementNames() const;

    // Check if element exists in database
    UFUNCTION(BlueprintPure, Category = "Elements")
    bool HasElement(FName ElementName) const;

    // Normalize element name (handles typos)
    UFUNCTION(BlueprintPure, Category = "Elements")
    FName NormalizeElementName(FName ElementName) const;

    // ========================================================================
    // DEFAULT VALUES
    // Used when database is unavailable
    // ========================================================================

    // Hardcoded fallback colors when database is not loaded
    // These match the original QuidditchGoal::GetColorForElement values
    UFUNCTION(BlueprintPure, Category = "Elements")
    static FLinearColor GetFallbackColorForElement(FName ElementName);

protected:
    // ========================================================================
    // DATABASE LOADING
    // ========================================================================

    // Attempt to load the default database asset
    void LoadDefaultDatabase();

private:
    // The currently loaded element database
    UPROPERTY()
    TObjectPtr<UElementDatabase> Database;

    // Path to default database (set via config or defaults)
    // Format: /Game/Data/DA_Elements.DA_Elements
    UPROPERTY()
    FSoftObjectPath DefaultDatabasePath;
};