// ============================================================================
// QuidditchStagingZone.cpp
// Implementation of team staging areas for Quidditch matches
//
// Developer: Marcus Daley
// Date: January 22, 2026
// Project: WizardJam
// ============================================================================

#include "Code/Quidditch/QuidditchStagingZone.h"
#include "Components/SphereComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/ArrowComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogQuidditchStaging);

// Static member initialization
FOnAnyStagingZoneReady AQuidditchStagingZone::OnAnyStagingZoneReady;
TArray<AQuidditchStagingZone*> AQuidditchStagingZone::AllStagingZones;

AQuidditchStagingZone::AQuidditchStagingZone()
    : TeamID(0)
    , TeamColor(FLinearColor::Blue)
    , FormationRadius(500.0f)
    , HoverAltitude(300.0f)
    , ArrivalRadius(800.0f)
    , RequiredAgentCount(1)  // Low for testing - set to 7 for full Quidditch
    , bTeamLaunched(false)
{
    PrimaryActorTick.bCanEverTick = false;

    // Root component
    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
    RootComponent = RootSceneComponent;

    // Arrival detection sphere
    ArrivalZone = CreateDefaultSubobject<USphereComponent>(TEXT("ArrivalZone"));
    ArrivalZone->SetupAttachment(RootComponent);
    ArrivalZone->SetSphereRadius(ArrivalRadius);
    ArrivalZone->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    ArrivalZone->SetGenerateOverlapEvents(true);

    // Editor visualization - billboard
    EditorBillboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("EditorBillboard"));
    EditorBillboard->SetupAttachment(RootComponent);
    EditorBillboard->SetRelativeLocation(FVector(0, 0, HoverAltitude));

    // Launch direction arrow
    LaunchDirectionArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("LaunchDirectionArrow"));
    LaunchDirectionArrow->SetupAttachment(RootComponent);
    LaunchDirectionArrow->SetRelativeLocation(FVector(0, 0, HoverAltitude));
    LaunchDirectionArrow->SetArrowColor(FLinearColor::Green);
    LaunchDirectionArrow->ArrowSize = 2.0f;
}

void AQuidditchStagingZone::BeginPlay()
{
    Super::BeginPlay();

    // Register in static collection
    AllStagingZones.Add(this);

    // Bind overlap events
    ArrivalZone->OnComponentBeginOverlap.AddDynamic(this, &AQuidditchStagingZone::OnArrivalZoneBeginOverlap);
    ArrivalZone->OnComponentEndOverlap.AddDynamic(this, &AQuidditchStagingZone::OnArrivalZoneEndOverlap);

    // Update sphere radius in case designer changed it
    ArrivalZone->SetSphereRadius(ArrivalRadius);

    UE_LOG(LogQuidditchStaging, Log, TEXT("%s: Staging zone initialized for Team %d at %s"),
        *GetName(), TeamID, *GetActorLocation().ToString());
}

void AQuidditchStagingZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Remove from static collection
    AllStagingZones.Remove(this);

    Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
void AQuidditchStagingZone::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    FName PropertyName = (PropertyChangedEvent.Property != nullptr)
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;

    // Update visuals when designer changes values
    if (PropertyName == GET_MEMBER_NAME_CHECKED(AQuidditchStagingZone, ArrivalRadius) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(AQuidditchStagingZone, HoverAltitude) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(AQuidditchStagingZone, TeamColor))
    {
        UpdateEditorVisualization();
    }
}
#endif

void AQuidditchStagingZone::UpdateEditorVisualization()
{
    if (ArrivalZone)
    {
        ArrivalZone->SetSphereRadius(ArrivalRadius);
    }

    if (EditorBillboard)
    {
        EditorBillboard->SetRelativeLocation(FVector(0, 0, HoverAltitude));
    }

    if (LaunchDirectionArrow)
    {
        LaunchDirectionArrow->SetRelativeLocation(FVector(0, 0, HoverAltitude));
        LaunchDirectionArrow->SetArrowColor(TeamColor);
    }
}

FVector AQuidditchStagingZone::GetStagingTargetLocation() const
{
    // Return center point at hover altitude
    return GetActorLocation() + FVector(0, 0, HoverAltitude);
}

FVector AQuidditchStagingZone::GetFormationPositionForRole(EQuidditchRole Role) const
{
    // Formation positions spread agents in a line facing the launch direction
    // This creates the classic Quidditch starting formation
    FVector Center = GetStagingTargetLocation();
    FVector Forward = GetLaunchDirection();
    FVector Right = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal();

    float Spacing = FormationRadius / 3.0f;

    switch (Role)
    {
    case EQuidditchRole::Seeker:
        // Seeker at center-front
        return Center + Forward * Spacing;

    case EQuidditchRole::Chaser:
        // Chasers in a line behind seeker (caller should offset for specific chaser)
        return Center;

    case EQuidditchRole::Beater:
        // Beaters on flanks
        return Center + Right * Spacing;

    case EQuidditchRole::Keeper:
        // Keeper at rear
        return Center - Forward * Spacing * 2.0f;

    default:
        return Center;
    }
}

bool AQuidditchStagingZone::IsTeamReady() const
{
    return StagedAgents.Num() >= RequiredAgentCount;
}

int32 AQuidditchStagingZone::GetStagedAgentCount() const
{
    return StagedAgents.Num();
}

void AQuidditchStagingZone::RegisterStagedAgent(AActor* Agent)
{
    if (!Agent || StagedAgents.Contains(Agent))
    {
        return;
    }

    StagedAgents.Add(Agent);

    UE_LOG(LogQuidditchStaging, Log, TEXT("%s: Agent %s registered. Count: %d/%d"),
        *GetName(), *Agent->GetName(), StagedAgents.Num(), RequiredAgentCount);

    // Broadcast arrival
    OnAgentArrived.Broadcast(this, Agent, TeamID);

    // Check if team is now ready
    CheckTeamReadyState();
}

void AQuidditchStagingZone::UnregisterStagedAgent(AActor* Agent)
{
    if (StagedAgents.Remove(Agent) > 0)
    {
        UE_LOG(LogQuidditchStaging, Log, TEXT("%s: Agent %s unregistered. Count: %d/%d"),
            *GetName(), Agent ? *Agent->GetName() : TEXT("null"), StagedAgents.Num(), RequiredAgentCount);
    }
}

void AQuidditchStagingZone::LaunchTeam()
{
    if (bTeamLaunched)
    {
        UE_LOG(LogQuidditchStaging, Warning, TEXT("%s: Team already launched!"), *GetName());
        return;
    }

    bTeamLaunched = true;

    UE_LOG(LogQuidditchStaging, Log, TEXT("%s: Launching Team %d with %d agents!"),
        *GetName(), TeamID, StagedAgents.Num());

    // Broadcast launch event - AI will respond by switching behavior tree branches
    OnTeamLaunchedEvent.Broadcast(this, TeamID);
}

FVector AQuidditchStagingZone::GetLaunchDirection() const
{
    if (LaunchDirectionArrow)
    {
        return LaunchDirectionArrow->GetForwardVector();
    }
    return GetActorForwardVector();
}

AQuidditchStagingZone* AQuidditchStagingZone::FindStagingZoneForTeam(const UObject* WorldContextObject, int32 InTeamID)
{
    for (AQuidditchStagingZone* Zone : AllStagingZones)
    {
        if (Zone && Zone->TeamID == InTeamID)
        {
            return Zone;
        }
    }

    // Fallback: search world if static array is empty (late join scenario)
    if (WorldContextObject)
    {
        UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
        if (World)
        {
            TArray<AActor*> FoundActors;
            UGameplayStatics::GetAllActorsOfClass(World, AQuidditchStagingZone::StaticClass(), FoundActors);

            for (AActor* Actor : FoundActors)
            {
                AQuidditchStagingZone* Zone = Cast<AQuidditchStagingZone>(Actor);
                if (Zone && Zone->TeamID == InTeamID)
                {
                    return Zone;
                }
            }
        }
    }

    return nullptr;
}

TArray<AQuidditchStagingZone*> AQuidditchStagingZone::GetAllStagingZones(const UObject* WorldContextObject)
{
    // If static array is populated, return it
    if (AllStagingZones.Num() > 0)
    {
        return AllStagingZones;
    }

    // Fallback: search world
    TArray<AQuidditchStagingZone*> Result;
    if (WorldContextObject)
    {
        UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
        if (World)
        {
            TArray<AActor*> FoundActors;
            UGameplayStatics::GetAllActorsOfClass(World, AQuidditchStagingZone::StaticClass(), FoundActors);

            for (AActor* Actor : FoundActors)
            {
                if (AQuidditchStagingZone* Zone = Cast<AQuidditchStagingZone>(Actor))
                {
                    Result.Add(Zone);
                }
            }
        }
    }

    return Result;
}

void AQuidditchStagingZone::OnArrivalZoneBeginOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == this)
    {
        return;
    }

    // Only register valid Quidditch agents (pawns with broom component or AI controller)
    if (IsValidQuidditchAgent(OtherActor))
    {
        RegisterStagedAgent(OtherActor);
    }
}

void AQuidditchStagingZone::OnArrivalZoneEndOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex)
{
    if (OtherActor && !bTeamLaunched)
    {
        // Only unregister if team hasn't launched yet
        // After launch, we don't care if agents leave the zone
        UnregisterStagedAgent(OtherActor);
    }
}

bool AQuidditchStagingZone::IsValidQuidditchAgent(AActor* Actor) const
{
    if (!Actor)
    {
        return false;
    }

    // Check if it's a pawn (player or AI)
    APawn* Pawn = Cast<APawn>(Actor);
    if (!Pawn)
    {
        return false;
    }

    // Could add additional checks here:
    // - Has broom component
    // - Has correct team ID
    // - Is controlled by Quidditch AI

    return true;
}

void AQuidditchStagingZone::CheckTeamReadyState()
{
    if (!bTeamLaunched && IsTeamReady())
    {
        UE_LOG(LogQuidditchStaging, Log, TEXT("%s: Team %d is READY! (%d/%d agents)"),
            *GetName(), TeamID, StagedAgents.Num(), RequiredAgentCount);

        // Broadcast to instance delegate (Blueprint)
        OnTeamReady.Broadcast(this, TeamID);

        // Broadcast to static delegate (GameMode)
        OnAnyStagingZoneReady.Broadcast(this, TeamID);
    }
}
