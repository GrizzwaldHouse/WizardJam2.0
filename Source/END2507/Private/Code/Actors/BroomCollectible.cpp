// ============================================================================
// BroomCollectible.cpp
// Developer: Marcus Daley
// Date: January 20, 2026
// Project: END2507
// ============================================================================

#include "Code/Actors/BroomCollectible.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"

DEFINE_LOG_CATEGORY(LogBroomCollectible);

ABroomCollectible::ABroomCollectible()
    : bAutoRegisterForSight(true)
{
    // Create perception component so AI can detect this collectible
    PerceptionSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("PerceptionSource"));

    // Set item identity from CollectiblePickup
    ItemName = FName("Broom");

    // Grant the "Broom" channel when collected
    GrantsSpellChannels.Add(FName("Broom"));

    // Allow all actor types to collect
    bPlayerCanCollect = true;
    bEnemyCanCollect = true;
    bCompanionCanCollect = true;

    // Enable logging for debug
    bShowChannelGrantLog = true;
}

void ABroomCollectible::BeginPlay()
{
    Super::BeginPlay();

    // Configure perception source for AI visibility
    if (PerceptionSource && bAutoRegisterForSight)
    {
        PerceptionSource->bAutoRegister = true;
        PerceptionSource->RegisterForSense(UAISense_Sight::StaticClass());

        UE_LOG(LogBroomCollectible, Log,
            TEXT("[%s] Registered for AI Sight perception"),
            *GetName());
    }

    UE_LOG(LogBroomCollectible, Log,
        TEXT("[%s] BroomCollectible ready | Grants: 'Broom' | AI Visible: %s"),
        *GetName(),
        bAutoRegisterForSight ? TEXT("YES") : TEXT("NO"));
}
