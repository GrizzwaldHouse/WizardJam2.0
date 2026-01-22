// ============================================================================
// BroomCollectible.h
// Developer: Marcus Daley
// Date: January 20, 2026
// Project: END2507
// ============================================================================
// Purpose:
// Collectible that grants "Broom" channel for flight capability.
// Inherits from CollectiblePickup (not SpellCollectible) so it grants
// channels without appearing in the spell UI or being logged as a spell.
//
// Usage:
// 1. Place in level or spawn via spawner
// 2. AI/Player overlaps to collect
// 3. Grants "Broom" channel to AC_SpellCollectionComponent
// 4. AC_BroomComponent can now be enabled for flight
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Code/Actors/CollectiblePickup.h"
#include "BroomCollectible.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBroomCollectible, Log, All);

class UAIPerceptionStimuliSourceComponent;

UCLASS()
class END2507_API ABroomCollectible : public ACollectiblePickup
{
    GENERATED_BODY()

public:
    ABroomCollectible();

protected:
    virtual void BeginPlay() override;

    // AI Perception component so AI agents can detect this collectible
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UAIPerceptionStimuliSourceComponent* PerceptionSource;

    // Whether to automatically register for AI sight perception
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Broom|AI")
    bool bAutoRegisterForSight;
};
