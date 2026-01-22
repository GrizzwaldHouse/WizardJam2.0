// ============================================================================
// WizardPlayer.cpp
// Wizard player character implementation
//
// Developer: Marcus Daley
// Date: January 20, 2026
// Project: WizardJam / GAR (END2507)
// ============================================================================

#include "Code/Actors/WizardPlayer.h"

// Engine includes - need full type definitions
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

// Project includes
#include "Code/Flight/AC_BroomComponent.h"
#include "Code/Utilities/AC_StaminaComponent.h"
#include "Code/AC_HealthComponent.h"
#include "Code/Utilities/InteractionComponent.h"
#include "Both/PlayerHUD.h"

DEFINE_LOG_CATEGORY(LogWizardPlayer);

// ============================================================================
// CONSTRUCTOR
// ============================================================================

AWizardPlayer::AWizardPlayer()
    : PlayerTeamID(0)
    , QuidditchRole(EQuidditchRole::None)
    , BallThrowForce(2000.0f)
    , HeldBall(EQuidditchBall::None)
    , HeldBallActor(nullptr)
    , PlayerHUDWidget(nullptr)
{
    PrimaryActorTick.bCanEverTick = true;

    // Set default team
    TeamId = FGenericTeamId(PlayerTeamID);

    // Create Broom Component
    BroomComponent = CreateDefaultSubobject<UAC_BroomComponent>(TEXT("BroomComponent"));

    // Create Stamina Component
    StaminaComponent = CreateDefaultSubobject<UAC_StaminaComponent>(TEXT("StaminaComponent"));

    // Create Health Component
    HealthComponent = CreateDefaultSubobject<UAC_HealthComponent>(TEXT("HealthComponent"));

    // Create Interaction Component
    InteractionComponent = CreateDefaultSubobject<UInteractionComponent>(TEXT("InteractionComponent"));
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void AWizardPlayer::BeginPlay()
{
    Super::BeginPlay();

    // Sync team ID
    TeamId = FGenericTeamId(PlayerTeamID);

    // Bind component delegates
    BindComponentDelegates();

    // Setup HUD
    SetupHUD();

    UE_LOG(LogWizardPlayer, Display,
        TEXT("[%s] WizardPlayer initialized | Team: %d | Role: %s"),
        *GetName(),
        PlayerTeamID,
        *QuidditchHelpers::RoleToString(QuidditchRole));
}

void AWizardPlayer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Update HUD with current stamina if flying
    if (BroomComponent && BroomComponent->IsFlying())
    {
        UpdateHUDStamina(BroomComponent->GetFlightStaminaPercent());
    }
}

void AWizardPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    // Call parent to set up base input (move, look, jump, sprint, fire, spell cycle)
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (!EnhancedInput)
    {
        UE_LOG(LogWizardPlayer, Error,
            TEXT("[%s] Failed to cast to EnhancedInputComponent!"), *GetName());
        return;
    }

    // ========================================================================
    // QUIDDITCH-SPECIFIC INPUT BINDINGS
    // ========================================================================

    // Throw ball action
    if (ThrowBallAction)
    {
        EnhancedInput->BindAction(ThrowBallAction, ETriggerEvent::Started,
            this, &AWizardPlayer::HandleThrowBallInput);
        UE_LOG(LogWizardPlayer, Display,
            TEXT("[%s] ThrowBallAction bound"), *GetName());
    }

    // Toggle flight action (manual, separate from broom interaction)
    if (ToggleFlightAction)
    {
        EnhancedInput->BindAction(ToggleFlightAction, ETriggerEvent::Started,
            this, &AWizardPlayer::HandleToggleFlightInput);
        UE_LOG(LogWizardPlayer, Display,
            TEXT("[%s] ToggleFlightAction bound"), *GetName());
    }

    UE_LOG(LogWizardPlayer, Display,
        TEXT("[%s] Quidditch input setup complete"), *GetName());
}

// ============================================================================
// INPUT HANDLERS
// ============================================================================

void AWizardPlayer::HandleFireInput()
{
    // For Quidditch, fire input casts spells toward goals
    // The spell system will handle element matching with goals
    UE_LOG(LogWizardPlayer, Verbose,
        TEXT("[%s] Fire input - casting spell"), *GetName());

    // TODO: Integrate with spell/combat system
    // CombatComponent->FireProjectileByType(CurrentSpellElement);
}

void AWizardPlayer::OnSprintStarted()
{
    // Check stamina before allowing sprint
    if (StaminaComponent && StaminaComponent->GetStaminaPercent() > 0.1f)
    {
        StaminaComponent->SetSprinting(true);
        Super::OnSprintStarted();

        UE_LOG(LogWizardPlayer, Verbose,
            TEXT("[%s] Sprint started with stamina check"), *GetName());
    }
    else
    {
        UE_LOG(LogWizardPlayer, Verbose,
            TEXT("[%s] Sprint denied - insufficient stamina"), *GetName());
    }
}

void AWizardPlayer::OnSprintStopped()
{
    if (StaminaComponent)
    {
        StaminaComponent->SetSprinting(false);
    }
    Super::OnSprintStopped();
}

void AWizardPlayer::HandleThrowBallInput()
{
    if (HeldBall == EQuidditchBall::None || !HeldBallActor)
    {
        UE_LOG(LogWizardPlayer, Verbose,
            TEXT("[%s] Cannot throw - no ball held"), *GetName());
        return;
    }

    // Get aim direction from camera
    FVector AimLocation = FVector::ZeroVector;
    if (FollowCamera)
    {
        // Trace from camera forward to find aim point
        FVector CameraLocation = FollowCamera->GetComponentLocation();
        FVector CameraForward = FollowCamera->GetForwardVector();
        AimLocation = CameraLocation + (CameraForward * 5000.0f);
    }

    // Throw using interface method
    ThrowBallAtTarget_Implementation(AimLocation);
}

void AWizardPlayer::HandleToggleFlightInput()
{
    if (!BroomComponent)
    {
        return;
    }

    // Toggle flight state
    BroomComponent->SetFlightEnabled(!BroomComponent->IsFlying());

    UE_LOG(LogWizardPlayer, Display,
        TEXT("[%s] Flight toggled via input - now %s"),
        *GetName(),
        BroomComponent->IsFlying() ? TEXT("FLYING") : TEXT("GROUNDED"));
}

// ============================================================================
// IGenericTeamAgentInterface Implementation
// ============================================================================

FGenericTeamId AWizardPlayer::GetGenericTeamId() const
{
    return TeamId;
}

void AWizardPlayer::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
    TeamId = NewTeamID;
    PlayerTeamID = static_cast<int32>(NewTeamID.GetId());

    UE_LOG(LogWizardPlayer, Display,
        TEXT("[%s] Team ID set to %d"), *GetName(), PlayerTeamID);
}

// ============================================================================
// IQuidditchAgent Implementation
// ============================================================================

EQuidditchRole AWizardPlayer::GetQuidditchRole_Implementation() const
{
    return QuidditchRole;
}

void AWizardPlayer::SetQuidditchRole_Implementation(EQuidditchRole NewRole)
{
    EQuidditchRole OldRole = QuidditchRole;
    QuidditchRole = NewRole;

    if (OldRole != NewRole)
    {
        OnRoleChanged.Broadcast(NewRole);
        UE_LOG(LogWizardPlayer, Display,
            TEXT("[%s] Quidditch role changed: %s -> %s"),
            *GetName(),
            *QuidditchHelpers::RoleToString(OldRole),
            *QuidditchHelpers::RoleToString(NewRole));
    }
}

int32 AWizardPlayer::GetQuidditchTeamID_Implementation() const
{
    return PlayerTeamID;
}

bool AWizardPlayer::IsOnBroom_Implementation() const
{
    return BroomComponent && BroomComponent->IsFlying();
}

bool AWizardPlayer::HasBall_Implementation() const
{
    return HeldBall != EQuidditchBall::None;
}

EQuidditchBall AWizardPlayer::GetHeldBallType_Implementation() const
{
    return HeldBall;
}

FVector AWizardPlayer::GetAgentLocation_Implementation() const
{
    return GetActorLocation();
}

FVector AWizardPlayer::GetAgentVelocity_Implementation() const
{
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        return MoveComp->Velocity;
    }
    return FVector::ZeroVector;
}

bool AWizardPlayer::TryMountBroom_Implementation(AActor* BroomActor)
{
    if (!BroomComponent)
    {
        return false;
    }

    // Enable flight
    BroomComponent->SetFlightEnabled(true);

    UE_LOG(LogWizardPlayer, Display,
        TEXT("[%s] Mounted broom"), *GetName());

    return BroomComponent->IsFlying();
}

void AWizardPlayer::DismountBroom_Implementation()
{
    if (BroomComponent)
    {
        BroomComponent->SetFlightEnabled(false);
    }

    UE_LOG(LogWizardPlayer, Display,
        TEXT("[%s] Dismounted broom"), *GetName());
}

bool AWizardPlayer::TryPickUpBall_Implementation(AActor* Ball)
{
    if (!Ball)
    {
        return false;
    }

    // Check if already holding a ball
    if (HeldBall != EQuidditchBall::None)
    {
        UE_LOG(LogWizardPlayer, Warning,
            TEXT("[%s] Cannot pick up ball - already holding one"), *GetName());
        return false;
    }

    // Determine ball type (would check interface or class)
    // For now, default to Quaffle
    HeldBall = EQuidditchBall::Quaffle;
    HeldBallActor = Ball;

    // Attach ball to player (simplified)
    Ball->AttachToActor(this, FAttachmentTransformRules::SnapToTargetIncludingScale);

    OnBallChanged.Broadcast(HeldBall);

    UE_LOG(LogWizardPlayer, Display,
        TEXT("[%s] Picked up ball: %s"),
        *GetName(),
        *QuidditchHelpers::BallToString(HeldBall));

    return true;
}

bool AWizardPlayer::ThrowBallAtTarget_Implementation(FVector TargetLocation)
{
    if (HeldBall == EQuidditchBall::None || !HeldBallActor)
    {
        return false;
    }

    // Detach ball
    HeldBallActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

    // Calculate throw direction
    FVector ThrowDirection = (TargetLocation - GetActorLocation()).GetSafeNormal();

    // Apply impulse (if ball has physics)
    if (UPrimitiveComponent* BallPrimitive = Cast<UPrimitiveComponent>(HeldBallActor->GetRootComponent()))
    {
        if (BallPrimitive->IsSimulatingPhysics())
        {
            BallPrimitive->AddImpulse(ThrowDirection * BallThrowForce);
        }
    }

    UE_LOG(LogWizardPlayer, Display,
        TEXT("[%s] Threw ball toward %s"),
        *GetName(),
        *TargetLocation.ToString());

    // Clear held ball state
    EQuidditchBall ThrownBall = HeldBall;
    HeldBall = EQuidditchBall::None;
    HeldBallActor = nullptr;

    OnBallChanged.Broadcast(EQuidditchBall::None);

    return true;
}

bool AWizardPlayer::PassBallToTeammate_Implementation(AActor* Teammate)
{
    if (!Teammate)
    {
        return false;
    }

    // Pass is just a throw toward teammate
    return ThrowBallAtTarget_Implementation(Teammate->GetActorLocation());
}

void AWizardPlayer::GetFlockMembers_Implementation(TArray<AActor*>& OutFlockMembers, float SearchRadius)
{
    // Find all teammates within search radius
    for (TActorIterator<APawn> It(GetWorld()); It; ++It)
    {
        APawn* OtherPawn = *It;

        // Skip self
        if (OtherPawn == this)
        {
            continue;
        }

        // Check if same team
        if (IGenericTeamAgentInterface* OtherTeam = Cast<IGenericTeamAgentInterface>(OtherPawn))
        {
            if (OtherTeam->GetGenericTeamId() == TeamId)
            {
                // Check distance
                float Distance = FVector::Dist(GetActorLocation(), OtherPawn->GetActorLocation());
                if (Distance <= SearchRadius)
                {
                    OutFlockMembers.Add(OtherPawn);
                }
            }
        }
    }
}

FName AWizardPlayer::GetFlockTag_Implementation() const
{
    // Return team-specific flock tag
    return FName(*FString::Printf(TEXT("Team%d_Players"), PlayerTeamID));
}

// ============================================================================
// COMPONENT EVENT HANDLERS
// ============================================================================

void AWizardPlayer::OnHealthChanged(float HealthRatio)
{
    UpdateHUDHealth(HealthRatio);

    UE_LOG(LogWizardPlayer, Log,
        TEXT("[%s] Health changed: %.1f%%"), *GetName(), HealthRatio * 100.0f);
}

void AWizardPlayer::OnDeath(AActor* DeadActor)
{
    UE_LOG(LogWizardPlayer, Warning,
        TEXT("[%s] Player died!"), *GetName());

    // Dismount broom on death
    DismountBroom_Implementation();

    // Drop held ball
    if (HeldBallActor)
    {
        HeldBallActor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        HeldBall = EQuidditchBall::None;
        HeldBallActor = nullptr;
        OnBallChanged.Broadcast(EQuidditchBall::None);
    }
}

void AWizardPlayer::OnFlightStateChanged(bool bIsFlying)
{
    UE_LOG(LogWizardPlayer, Display,
        TEXT("[%s] Flight state: %s"),
        *GetName(),
        bIsFlying ? TEXT("FLYING") : TEXT("GROUNDED"));
}

void AWizardPlayer::OnStaminaDepleted(AActor* DepletedActor)
{
    UE_LOG(LogWizardPlayer, Warning,
        TEXT("[%s] Stamina depleted!"), *GetName());

    // Force dismount if flying
    if (BroomComponent && BroomComponent->IsFlying())
    {
        DismountBroom_Implementation();
    }
}

// ============================================================================
// PUBLIC API
// ============================================================================

bool AWizardPlayer::IsFlying() const
{
    return BroomComponent && BroomComponent->IsFlying();
}

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

void AWizardPlayer::SetupHUD()
{
    if (!PlayerHUDClass)
    {
        UE_LOG(LogWizardPlayer, Warning,
            TEXT("[%s] PlayerHUDClass not set - no HUD will be created"), *GetName());
        return;
    }

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
    {
        return;
    }

    PlayerHUDWidget = CreateWidget<UPlayerHUD>(PC, PlayerHUDClass);
    if (PlayerHUDWidget)
    {
        PlayerHUDWidget->AddToViewport();

        // Initialize with current values
        if (HealthComponent)
        {
            UpdateHUDHealth(HealthComponent->GetHealthRatio());
        }
        if (StaminaComponent)
        {
            UpdateHUDStamina(StaminaComponent->GetStaminaPercent());
        }

        UE_LOG(LogWizardPlayer, Display,
            TEXT("[%s] Player HUD created and initialized"), *GetName());
    }
}

void AWizardPlayer::BindComponentDelegates()
{
    // Bind health component
    if (HealthComponent)
    {
        HealthComponent->OnHealthChanged.AddDynamic(this, &AWizardPlayer::OnHealthChanged);
        HealthComponent->OnDeath.AddDynamic(this, &AWizardPlayer::OnDeath);
        UE_LOG(LogWizardPlayer, Log,
            TEXT("[%s] Bound to HealthComponent delegates"), *GetName());
    }

    // Bind broom component
    if (BroomComponent)
    {
        BroomComponent->OnFlightStateChanged.AddDynamic(this, &AWizardPlayer::OnFlightStateChanged);
        UE_LOG(LogWizardPlayer, Log,
            TEXT("[%s] Bound to BroomComponent delegates"), *GetName());
    }

    // Bind stamina component
    if (StaminaComponent)
    {
        StaminaComponent->OnStaminaDepleted.AddDynamic(this, &AWizardPlayer::OnStaminaDepleted);
        UE_LOG(LogWizardPlayer, Log,
            TEXT("[%s] Bound to StaminaComponent delegates"), *GetName());
    }
}

void AWizardPlayer::UpdateHUDStamina(float StaminaPercent)
{
    if (PlayerHUDWidget)
    {
        // PlayerHUD should have a method for stamina
        // PlayerHUDWidget->UpdateStaminaBar(StaminaPercent);
    }
}

void AWizardPlayer::UpdateHUDHealth(float HealthPercent)
{
    if (PlayerHUDWidget)
    {
        PlayerHUDWidget->UpdateHealthBar(HealthPercent);
    }
}
