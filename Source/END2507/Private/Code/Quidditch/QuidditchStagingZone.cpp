// ============================================================================
// QuidditchStagingZone.cpp
// Developer: Marcus Daley
// Date: January 23, 2026 (Refactored January 28, 2026)
// Project: END2507
// ============================================================================
// Bee and Flower Pattern Implementation:
// This is the "flower" - it just broadcasts "I exist" and reports when
// something lands on it. No filtering, no game logic - just perception
// and event broadcasting.
// ============================================================================

#include "Code/Quidditch/QuidditchStagingZone.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BillboardComponent.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "GameFramework/Pawn.h"

DEFINE_LOG_CATEGORY(LogQuidditchStagingZone);

AQuidditchStagingZone::AQuidditchStagingZone()
    : ZoneIdentifier(NAME_None)
    , TeamHint(0)
    , RoleHint(0)
    , TriggerRadius(300.0f)
{
    PrimaryActorTick.bCanEverTick = false;

    // Create root component
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    // Create trigger volume - detects when pawns enter
    TriggerVolume = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerVolume"));
    TriggerVolume->SetupAttachment(Root);
    TriggerVolume->SetSphereRadius(TriggerRadius);
    TriggerVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    TriggerVolume->SetGenerateOverlapEvents(true);

    // Create visual mesh (optional - designers can assign)
    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
    VisualMesh->SetupAttachment(Root);
    VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    VisualMesh->SetVisibility(true);

    // Create editor billboard for visibility
    EditorBillboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("EditorBillboard"));
    EditorBillboard->SetupAttachment(Root);
    EditorBillboard->bIsScreenSizeScaled = true;

    // Create AI Perception source - this is how agents "see" us
    // Same pattern as SnitchBall - we broadcast, agents perceive
    PerceptionSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("PerceptionSource"));
    PerceptionSource->bAutoRegister = true;
    PerceptionSource->RegisterForSense(UAISense_Sight::StaticClass());

    // Add generic tag for perception filtering
    Tags.Add(TEXT("StagingZone"));
    Tags.Add(TEXT("LandingZone"));
}

void AQuidditchStagingZone::BeginPlay()
{
    Super::BeginPlay();

    // Bind overlap events - we just report, no filtering
    TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AQuidditchStagingZone::OnOverlapBegin);
    TriggerVolume->OnComponentEndOverlap.AddDynamic(this, &AQuidditchStagingZone::OnOverlapEnd);

    UE_LOG(LogQuidditchStagingZone, Log,
        TEXT("[StagingZone] '%s' initialized | Identifier=%s | TeamHint=%d | RoleHint=%d | Location=%s"),
        *GetName(),
        *ZoneIdentifier.ToString(),
        TeamHint,
        RoleHint,
        *GetActorLocation().ToString());
}

void AQuidditchStagingZone::OnOverlapBegin(UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor, UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // Only care about pawns
    APawn* Pawn = Cast<APawn>(OtherActor);
    if (!Pawn)
    {
        return;
    }

    // Track occupant (just for queries, no logic)
    OccupyingPawn = Pawn;

    UE_LOG(LogQuidditchStagingZone, Display,
        TEXT("[StagingZone] '%s' entered by '%s' | Identifier=%s"),
        *GetName(),
        *Pawn->GetName(),
        *ZoneIdentifier.ToString());

    // Broadcast event - listeners decide what to do with this info
    // The PAWN is responsible for deciding if this is the right zone
    // and notifying GameMode if needed
    OnZoneEntered.Broadcast(this, Pawn);
}

void AQuidditchStagingZone::OnOverlapEnd(UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor, UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex)
{
    APawn* Pawn = Cast<APawn>(OtherActor);
    if (!Pawn)
    {
        return;
    }

    // Clear occupant if it's the one leaving
    if (OccupyingPawn.Get() == Pawn)
    {
        UE_LOG(LogQuidditchStagingZone, Log,
            TEXT("[StagingZone] '%s' exited by '%s'"),
            *GetName(), *Pawn->GetName());

        OccupyingPawn.Reset();
    }

    // Broadcast exit event
    OnZoneExited.Broadcast(this, Pawn);
}
