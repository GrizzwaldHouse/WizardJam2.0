// ============================================================================
// ElementDatabaseSubsystem.cpp
// Developer: Marcus Daley
// Date: January 7, 2026
// ============================================================================
// Implementation of Element Database Subsystem
// ============================================================================

#include "Code/Data/ElementDatabaseSubsystem.h"
#include "Code/Data/ElementDatabase.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

DEFINE_LOG_CATEGORY(LogElementSubsystem);

// ============================================================================
// LIFECYCLE
// ============================================================================

void UElementDatabaseSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UE_LOG(LogElementSubsystem, Log, TEXT("[ElementSubsystem] Initializing..."));

    // Try to load the default database
    LoadDefaultDatabase();

    if (Database)
    {
        UE_LOG(LogElementSubsystem, Display,
            TEXT("[ElementSubsystem] Initialized with database: %s (%d elements)"),
            *Database->GetName(), Database->GetElementCount());
    }
    else
    {
        UE_LOG(LogElementSubsystem, Warning,
            TEXT("[ElementSubsystem] Initialized WITHOUT database - using fallback colors"));
    }
}

void UElementDatabaseSubsystem::Deinitialize()
{
    UE_LOG(LogElementSubsystem, Log, TEXT("[ElementSubsystem] Deinitializing..."));
    Database = nullptr;
    Super::Deinitialize();
}

// ============================================================================
// STATIC ACCESSOR
// ============================================================================

UElementDatabaseSubsystem* UElementDatabaseSubsystem::Get(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        return nullptr;
    }

    UGameInstance* GameInstance = World->GetGameInstance();
    if (!GameInstance)
    {
        return nullptr;
    }

    return GameInstance->GetSubsystem<UElementDatabaseSubsystem>();
}

// ============================================================================
// DATABASE MANAGEMENT
// ============================================================================

void UElementDatabaseSubsystem::SetDatabase(UElementDatabase* NewDatabase)
{
    Database = NewDatabase;

    if (Database)
    {
        UE_LOG(LogElementSubsystem, Display,
            TEXT("[ElementSubsystem] Database set: %s (%d elements)"),
            *Database->GetName(), Database->GetElementCount());
    }
    else
    {
        UE_LOG(LogElementSubsystem, Warning,
            TEXT("[ElementSubsystem] Database cleared - using fallback colors"));
    }
}

bool UElementDatabaseSubsystem::IsDatabaseValid() const
{
    return Database != nullptr;
}

// ============================================================================
// FORWARDING FUNCTIONS
// ============================================================================

FLinearColor UElementDatabaseSubsystem::GetColorForElement(FName ElementName) const
{
    if (Database)
    {
        return Database->GetColorForElement(ElementName);
    }
    return GetFallbackColorForElement(ElementName);
}

int32 UElementDatabaseSubsystem::GetPointsForElement(FName ElementName) const
{
    if (Database)
    {
        return Database->GetPointsForElement(ElementName);
    }
    return 100;  // Default fallback points
}

bool UElementDatabaseSubsystem::GetElementDefinition(FName ElementName, FElementDefinition& OutDefinition) const
{
    if (Database)
    {
        return Database->GetElementDefinition(ElementName, OutDefinition);
    }
    return false;
}

TArray<FName> UElementDatabaseSubsystem::GetAllElementNames() const
{
    if (Database)
    {
        return Database->GetAllElementNames();
    }

    // Return default element names as fallback
    return TArray<FName>{
        FName(TEXT("Flame")),
        FName(TEXT("Ice")),
        FName(TEXT("Lightning")),
        FName(TEXT("Arcane"))
    };
}

bool UElementDatabaseSubsystem::HasElement(FName ElementName) const
{
    if (Database)
    {
        return Database->HasElement(ElementName);
    }

    // Check against default elements
    FName Normalized = NormalizeElementName(ElementName);
    return Normalized == FName(TEXT("Flame")) ||
           Normalized == FName(TEXT("Ice")) ||
           Normalized == FName(TEXT("Lightning")) ||
           Normalized == FName(TEXT("Arcane"));
}

FName UElementDatabaseSubsystem::NormalizeElementName(FName ElementName) const
{
    if (Database)
    {
        return Database->NormalizeElementName(ElementName);
    }

    // Fallback normalization without database
    if (ElementName.IsNone())
    {
        return NAME_None;
    }

    FString ElementStr = ElementName.ToString();

    if (ElementStr.Equals(TEXT("Lighting"), ESearchCase::IgnoreCase))
    {
        return FName(TEXT("Lightning"));
    }
    if (ElementStr.Equals(TEXT("Fire"), ESearchCase::IgnoreCase))
    {
        return FName(TEXT("Flame"));
    }
    if (ElementStr.Equals(TEXT("Frost"), ESearchCase::IgnoreCase))
    {
        return FName(TEXT("Ice"));
    }
    if (ElementStr.Equals(TEXT("Magic"), ESearchCase::IgnoreCase))
    {
        return FName(TEXT("Arcane"));
    }

    return ElementName;
}

// ============================================================================
// DEFAULT VALUES
// ============================================================================

FLinearColor UElementDatabaseSubsystem::GetFallbackColorForElement(FName ElementName)
{
    // Hardcoded fallback colors matching original QuidditchGoal values
    FString ElementStr = ElementName.ToString();

    if (ElementStr.Equals(TEXT("Flame"), ESearchCase::IgnoreCase) ||
        ElementStr.Equals(TEXT("Fire"), ESearchCase::IgnoreCase))
    {
        return FLinearColor(1.0f, 0.3f, 0.0f);  // Orange-red
    }
    if (ElementStr.Equals(TEXT("Ice"), ESearchCase::IgnoreCase) ||
        ElementStr.Equals(TEXT("Frost"), ESearchCase::IgnoreCase))
    {
        return FLinearColor(0.0f, 0.8f, 1.0f);  // Cyan
    }
    if (ElementStr.Equals(TEXT("Lightning"), ESearchCase::IgnoreCase) ||
        ElementStr.Equals(TEXT("Lighting"), ESearchCase::IgnoreCase))
    {
        return FLinearColor(1.0f, 1.0f, 0.0f);  // Yellow
    }
    if (ElementStr.Equals(TEXT("Arcane"), ESearchCase::IgnoreCase) ||
        ElementStr.Equals(TEXT("Magic"), ESearchCase::IgnoreCase))
    {
        return FLinearColor(0.6f, 0.0f, 1.0f);  // Purple
    }

    return FLinearColor::White;
}

// ============================================================================
// DATABASE LOADING
// ============================================================================

void UElementDatabaseSubsystem::LoadDefaultDatabase()
{
    // Try to load from default path if set
    if (!DefaultDatabasePath.IsNull())
    {
        Database = Cast<UElementDatabase>(DefaultDatabasePath.TryLoad());
        if (Database)
        {
            UE_LOG(LogElementSubsystem, Log,
                TEXT("[ElementSubsystem] Loaded database from path: %s"),
                *DefaultDatabasePath.ToString());
            return;
        }
    }

    // Try common default paths
    TArray<FString> DefaultPaths = {
        TEXT("/Game/Data/DA_Elements.DA_Elements"),
        TEXT("/Game/Blueprints/Data/DA_Elements.DA_Elements"),
        TEXT("/Game/Content/Data/DA_Elements.DA_Elements")
    };

    for (const FString& Path : DefaultPaths)
    {
        Database = Cast<UElementDatabase>(StaticLoadObject(
            UElementDatabase::StaticClass(),
            nullptr,
            *Path));

        if (Database)
        {
            UE_LOG(LogElementSubsystem, Log,
                TEXT("[ElementSubsystem] Found database at: %s"),
                *Path);
            return;
        }
    }

    UE_LOG(LogElementSubsystem, Warning,
        TEXT("[ElementSubsystem] Could not find DA_Elements data asset - using fallback colors"));
}
