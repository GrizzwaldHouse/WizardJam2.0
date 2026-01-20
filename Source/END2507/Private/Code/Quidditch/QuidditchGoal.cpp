// ============================================================================
// QuidditchGoal.cpp (FIXED)
// Developer: Marcus Daley
// Date: January 19, 2026 (FIXED: Removed polling, fixed class references)
// Project: WizardJam2.0
// ============================================================================
// Purpose:
// Implementation of elemental goal post scoring system with match-end awareness.
//
// FIXES APPLIED (January 19, 2026):
// 1. Changed ABaseProjectile cast to AProjectile (GAR class)
// 2. REMOVED polling calls to non-existent GameMode functions:
//    - GetTimeRemaining() - NOT in WizardJamGameMode
//    - GetPlayerScore() - NOT in WizardJamGameMode
//    - GetAIScore() - NOT in WizardJamGameMode
//    - GetWinningScore() - NOT in WizardJamGameMode
// 3. Now uses IsGameOver() which EXISTS in WizardJamGameMode
// 4. Removed dependency on ElementDatabaseSubsystem (uses hardcoded fallback)
//
// Observer Pattern Flow:
//   Projectile overlaps ScoringZone
//   -> OnScoringZoneBeginOverlap fires
//   -> IsCorrectElement checks via GetSpellElement()
//   -> OnGoalScored.Broadcast(4 params)
//   -> GameMode::OnGoalScored receives
//   -> GameMode broadcasts to HUD
// ============================================================================

#include "Code/Quidditch/QuidditchGoal.h"
#include "Code/GameModes/WizardJamGameMode.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Code/Actors/Projectile.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogQuidditchGoal);

// Static member definitions
FOnAnyGoalScored AQuidditchGoal::OnAnyGoalScored;
FOnGoalRegistered AQuidditchGoal::OnGoalRegistered;
FOnGoalUnregistered AQuidditchGoal::OnGoalUnregistered;
TArray<AQuidditchGoal*> AQuidditchGoal::ActiveGoals;

// ============================================================================
// CONSTRUCTOR
// All values initialized in initialization list per Nick's standards
// ============================================================================

AQuidditchGoal::AQuidditchGoal()
    : GoalElement(NAME_None)
    , TeamID(0)
    , CorrectElementPoints(10)
    , WrongElementPoints(0)
    , HitFlashDuration(0.5f)
    , CurrentColor(FLinearColor::White)
    , bMatchEnded(false)
    , TeamId(FGenericTeamId(0))
    , DynamicMaterial(nullptr)
{
    PrimaryActorTick.bCanEverTick = false;

    // Create root scene component for proper transform hierarchy
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    SetRootComponent(Root);

    // Create goal mesh component
    // Designer assigns actual mesh in Blueprint (hoop, ring, target, etc.)
    GoalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GoalMesh"));
    if (GoalMesh)
    {
        GoalMesh->SetupAttachment(RootComponent);
        GoalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    // Create scoring zone (overlap box in front of goal)
    // Projectiles must overlap this zone to score
    ScoringZone = CreateDefaultSubobject<UBoxComponent>(TEXT("ScoringZone"));
    if (ScoringZone)
    {
        ScoringZone->SetupAttachment(GoalMesh);
        ScoringZone->SetBoxExtent(FVector(100.0f, 100.0f, 100.0f));
        ScoringZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        ScoringZone->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
        ScoringZone->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);

        // Respond to WorldDynamic objects (projectiles use OverlapAllDynamic profile)
        ScoringZone->SetCollisionResponseToChannel(
            ECollisionChannel::ECC_WorldDynamic,
            ECollisionResponse::ECR_Overlap);

        // Also respond to pawns in case we want character-based scoring later
        ScoringZone->SetCollisionResponseToChannel(
            ECollisionChannel::ECC_Pawn,
            ECollisionResponse::ECR_Overlap);
    }

    UE_LOG(LogQuidditchGoal, Log, TEXT("[QuidditchGoal] Constructor initialized"));
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void AQuidditchGoal::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    // Bind overlap event (must happen after components are initialized)
    if (ScoringZone)
    {
        ScoringZone->OnComponentBeginOverlap.AddDynamic(
            this,
            &AQuidditchGoal::OnScoringZoneBeginOverlap);

        UE_LOG(LogQuidditchGoal, Log,
            TEXT("[%s] Scoring zone overlap delegate bound"), *GetName());
    }
}

void AQuidditchGoal::BeginPlay()
{
    Super::BeginPlay();

    // Sync TeamId with TeamID property for IGenericTeamAgentInterface
    TeamId = FGenericTeamId(TeamID);

    // Apply element color to goal mesh material
    ApplyElementColor();
    ActiveGoals.Add(this);
    OnGoalRegistered.Broadcast(this);

    UE_LOG(LogQuidditchGoal, Display,
        TEXT("[%s] Goal REGISTERED | Element: '%s' | Team: %d | Active Goals: %d"),
        *GetName(), *GoalElement.ToString(), TeamID, ActiveGoals.Num());
}

void AQuidditchGoal::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ActiveGoals.Remove(this);
    OnGoalUnregistered.Broadcast(this);

    // Clear timer
    GetWorldTimerManager().ClearTimer(HitFlashTimer);

    UE_LOG(LogQuidditchGoal, Log,
        TEXT("[%s] Goal UNREGISTERED | Remaining Active Goals: %d"),
        *GetName(), ActiveGoals.Num());

    Super::EndPlay(EndPlayReason);
}

// ============================================================================
// OVERLAP HANDLER
// Main scoring logic when projectile enters scoring zone
// ============================================================================

void AQuidditchGoal::OnScoringZoneBeginOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    // Only process Projectile actors (GAR projectile class)
    // FIXED: Changed from ABaseProjectile to AProjectile
    AProjectile* Projectile = Cast<AProjectile>(OtherActor);
    if (!Projectile)
    {
        return;
    }
    if (bMatchEnded)
    {
        UE_LOG(LogQuidditchGoal, Log,
            TEXT("[%s] Match ended - destroying projectile"), *GetName());
        Projectile->Destroy();
        return;
    }

    // Get projectile owner (who fired it) for scoring attribution
    AActor* Shooter = Projectile->GetOwner();
    if (!Shooter)
    {
        // Try getting from GetOwnerPawn() as fallback
        Shooter = Projectile->GetOwnerPawn();
    }

    if (!Shooter)
    {
        UE_LOG(LogQuidditchGoal, Warning,
            TEXT("[%s] Projectile '%s' has no owner - cannot award points"),
            *GetName(), *Projectile->GetName());
        Projectile->Destroy();
        return;
    }

    // Check if projectile element matches goal element
    bool bCorrectElement = IsCorrectElement(Projectile);

    // Calculate points (0 if wrong element)
    int32 Points = CalculatePoints( bCorrectElement);

    // Get projectile element name for broadcast
    FName ProjectileElement = Projectile->GetSpellElement();

    // Broadcast scoring event to GameMode (Observer pattern)
    // Delegate takes 4 parameters: ScoringActor, Element, Points, bCorrectElement
    OnAnyGoalScored.Broadcast(this, Shooter, ProjectileElement, Points, bCorrectElement);

    // Also broadcast to instance delegate (for Blueprint on this specific goal)
    OnGoalScored.Broadcast(Shooter, ProjectileElement, Points, bCorrectElement);

    // Wrong element feedback
    if (!bCorrectElement)
    {
        OnWrongElementHit.Broadcast(Shooter, ProjectileElement, GoalElement);

        UE_LOG(LogQuidditchGoal, Display,
            TEXT("[%s] Wrong element! '%s' used '%s' (need '%s') - %d points"),
            *GetName(), *Shooter->GetName(), *ProjectileElement.ToString(),
            *GoalElement.ToString(), Points);
    }
    else
    {
        UE_LOG(LogQuidditchGoal, Display,
            TEXT("[%s] === GOAL! === '%s' scored %d points with '%s'"),
            *GetName(), *Shooter->GetName(), Points, *ProjectileElement.ToString());
    }
    // Play visual feedback
    PlayHitFeedback(bCorrectElement);
    // Destroy projectile after scoring attempt
    Projectile->Destroy();
}

// ============================================================================
// ELEMENT MATCHING
// ============================================================================

bool AQuidditchGoal::IsCorrectElement(AProjectile* InProjectile) const
{
    if (!InProjectile)
    {
        return false;
    }

    FName ProjectileElement = InProjectile->GetSpellElement();

    // Direct comparison first
    if (ProjectileElement == GoalElement)
    {
        return true;
    }

    // Typo tolerance
    bool bProjectileIsLightning = (ProjectileElement == FName("Lightning") || ProjectileElement == FName("Lighting"));
    bool bGoalIsLightning = (GoalElement == FName("Lightning") || GoalElement == FName("Lighting"));
    if (bProjectileIsLightning && bGoalIsLightning)
    {
        return true;
    }

    bool bProjectileIsFire = (ProjectileElement == FName("Flame") || ProjectileElement == FName("Fire"));
    bool bGoalIsFire = (GoalElement == FName("Flame") || GoalElement == FName("Fire"));
    if (bProjectileIsFire && bGoalIsFire)
    {
        return true;
    }

    return false;
}

// ============================================================================
// SCORING
// ============================================================================
int32 AQuidditchGoal::CalculatePoints(bool bCorrectElement) const
{
    return bCorrectElement ? CorrectElementPoints : WrongElementPoints;
}


// ============================================================================
// VISUAL FEEDBACK
// ============================================================================

void AQuidditchGoal::ApplyElementColor()
{
    if (!GoalMesh)
    {
        return;
    }

    // Get color based on element type using hardcoded fallback
    if (GoalElement == FName("Flame") || GoalElement == FName("Fire"))
    {
        CurrentColor = FLinearColor(1.0f, 0.3f, 0.0f);  // Orange-red
    }
    else if (GoalElement == FName("Ice") || GoalElement == FName("Frost"))
    {
        CurrentColor = FLinearColor(0.0f, 0.8f, 1.0f);  // Cyan
    }
    else if (GoalElement == FName("Lighting") || GoalElement == FName("Lighting"))
    {
        CurrentColor = FLinearColor(1.0f, 1.0f, 0.0f);  // Yellow
    }
    else if (GoalElement == FName("Arcane") || GoalElement == FName("Magic"))
    {
        CurrentColor = FLinearColor(0.6f, 0.0f, 1.0f);  // Purple
    }
    else
    {
        CurrentColor = FLinearColor::White;
    }

    UE_LOG(LogQuidditchGoal, Log,
        TEXT("[%s] Applied color for element '%s'"),
        *GetName(), *GoalElement.ToString());

    // Create dynamic material instance for runtime color changes
    if (GoalMesh->GetNumMaterials() > 0)
    {
        DynamicMaterial = GoalMesh->CreateDynamicMaterialInstance(0);
        if (DynamicMaterial)
        {
            DynamicMaterial->SetVectorParameterValue(FName("BaseColor"), CurrentColor);
            DynamicMaterial->SetVectorParameterValue(FName("EmissiveColor"), CurrentColor * 2.0f);

            UE_LOG(LogQuidditchGoal, Log,
                TEXT("[%s] Applied color (R=%.2f G=%.2f B=%.2f) to material"),
                *GetName(), CurrentColor.R, CurrentColor.G, CurrentColor.B);
        }
    }
}

void AQuidditchGoal::PlayHitFeedback(bool bCorrectElement)
{
    if (!DynamicMaterial)
    {
        return;
    }

    // Flash bright on correct hit, dark on wrong hit
    FLinearColor FlashColor = bCorrectElement ? (CurrentColor * 5.0f) : FLinearColor::Black;
    DynamicMaterial->SetVectorParameterValue(FName("EmissiveColor"), FlashColor);

    FTimerDelegate ResetDelegate;
    ResetDelegate.BindLambda([this]()
        {
            if (DynamicMaterial)
            {
                DynamicMaterial->SetVectorParameterValue(FName("EmissiveColor"), CurrentColor * 2.0f);
            }
        });

    GetWorldTimerManager().SetTimer(HitFlashTimer, ResetDelegate, HitFlashDuration, false);
}

// ============================================================================
// IGenericTeamAgentInterface Implementation
// Allows AI to identify enemy goals using UE5 team system
// ============================================================================

void AQuidditchGoal::NotifyAllGoalsMatchEnded()
{    // Set flag on all active goals
    for (AQuidditchGoal* Goal : ActiveGoals)
    {
        if (Goal && IsValid(Goal))
        {
            Goal->bMatchEnded = true;
        }
    }

    UE_LOG(LogQuidditchGoal, Display,
        TEXT("[QuidditchGoal] === MATCH ENDED === Notified %d goals"),
        ActiveGoals.Num());
}

FGenericTeamId AQuidditchGoal::GetGenericTeamId() const
{
    return TeamId;
}

void AQuidditchGoal::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
    TeamId = NewTeamID;
    TeamID = NewTeamID.GetId();
}