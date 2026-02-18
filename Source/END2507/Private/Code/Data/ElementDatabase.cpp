// ============================================================================
// ElementDatabase.cpp
// Developer: Marcus Daley
// Date: January 7, 2026
// ============================================================================
// Implementation of Element Database Data Asset
// ============================================================================

#include "Code/Data/ElementDatabase.h"
#include "Code/Data/ElementDatabaseSubsystem.h"

DEFINE_LOG_CATEGORY(LogElementDatabase);

// ============================================================================
// LOOKUP FUNCTIONS
// ============================================================================

bool UElementDatabase::GetElementDefinition(FName ElementName, FElementDefinition& OutDefinition) const
{
    const FElementDefinition* Found = FindElement(ElementName);
    if (Found)
    {
        OutDefinition = *Found;
        return true;
    }
    return false;
}

FLinearColor UElementDatabase::GetColorForElement(FName ElementName) const
{
    const FElementDefinition* Found = FindElement(ElementName);
    if (Found)
    {
        return Found->Color;
    }
    return FLinearColor::White;
}

int32 UElementDatabase::GetPointsForElement(FName ElementName) const
{
    const FElementDefinition* Found = FindElement(ElementName);
    if (Found)
    {
        return Found->BasePoints;
    }
    return 0;
}

TArray<FName> UElementDatabase::GetAllElementNames() const
{
    TArray<FName> Names;
    Names.Reserve(Elements.Num());

    // Create a sorted copy based on SortOrder
    TArray<FElementDefinition> SortedElements = Elements;
    SortedElements.Sort([](const FElementDefinition& A, const FElementDefinition& B)
    {
        return A.SortOrder < B.SortOrder;
    });

    for (const FElementDefinition& Element : SortedElements)
    {
        if (Element.IsValid())
        {
            Names.Add(Element.ElementName);
        }
    }

    return Names;
}

// ============================================================================
// NORMALIZATION
// ============================================================================

FName UElementDatabase::NormalizeElementName(FName Element) const
{
    if (Element.IsNone())
    {
        return NAME_None;
    }

    FString ElementStr = Element.ToString();

    // Handle common typos
    if (ElementStr.Equals(TEXT("Lighting"), ESearchCase::IgnoreCase))
    {
        return ElementNames::Lightning;
    }
    if (ElementStr.Equals(TEXT("Fire"), ESearchCase::IgnoreCase))
    {
        return ElementNames::Flame;
    }
    if (ElementStr.Equals(TEXT("Frost"), ESearchCase::IgnoreCase))
    {
        return ElementNames::Ice;
    }
    if (ElementStr.Equals(TEXT("Magic"), ESearchCase::IgnoreCase))
    {
        return ElementNames::Arcane;
    }

    // Try to find exact match first
    for (const FElementDefinition& Def : Elements)
    {
        if (Def.ElementName == Element)
        {
            return Def.ElementName;
        }
    }

    // Try case-insensitive match
    for (const FElementDefinition& Def : Elements)
    {
        if (Def.ElementName.ToString().Equals(ElementStr, ESearchCase::IgnoreCase))
        {
            return Def.ElementName;
        }
    }

    return Element;
}

bool UElementDatabase::ElementsMatch(FName ElementA, FName ElementB) const
{
    return NormalizeElementName(ElementA) == NormalizeElementName(ElementB);
}

// ============================================================================
// VALIDATION
// ============================================================================

bool UElementDatabase::HasElement(FName ElementName) const
{
    return FindElement(ElementName) != nullptr;
}

#if WITH_EDITOR
void UElementDatabase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    // Validate all elements have unique names
    TSet<FName> UsedNames;
    for (int32 i = 0; i < Elements.Num(); ++i)
    {
        const FElementDefinition& Element = Elements[i];
        if (Element.ElementName.IsNone())
        {
            UE_LOG(LogElementDatabase, Warning,
                TEXT("[%s] Element at index %d has no name!"),
                *GetName(), i);
            continue;
        }

        if (UsedNames.Contains(Element.ElementName))
        {
            UE_LOG(LogElementDatabase, Warning,
                TEXT("[%s] Duplicate element name '%s' at index %d!"),
                *GetName(), *Element.ElementName.ToString(), i);
        }
        else
        {
            UsedNames.Add(Element.ElementName);
        }
    }

    UE_LOG(LogElementDatabase, Log,
        TEXT("[%s] Validated %d elements, %d unique names"),
        *GetName(), Elements.Num(), UsedNames.Num());
}
#endif

// ============================================================================
// INTERNAL
// ============================================================================

const FElementDefinition* UElementDatabase::FindElement(FName ElementName) const
{
    if (ElementName.IsNone())
    {
        return nullptr;
    }

    // Normalize the name first
    FName NormalizedName = NormalizeElementName(ElementName);

    for (const FElementDefinition& Element : Elements)
    {
        if (Element.ElementName == NormalizedName)
        {
            return &Element;
        }
    }

    return nullptr;
}
