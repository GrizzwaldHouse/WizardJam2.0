
// BroomActor.cpp
// World-placeable broom implementation
//
// Developer: Marcus Daley
// Date: January 8, 2026
// Project: WizardJam
//
// This actor serves as the configuration source for broom flight behavior.
// Designers create child Blueprints with different FBroomConfiguration settings.
// When a player interacts, this actor passes its configuration to the player's
// AC_BroomComponent, which then handles all flight logic.

#include "Code/Flight/BroomActor.h"
#include "Code/Flight/AC_BroomComponent.h"
#include "Code/Utilities/AC_SpellCollectionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"

DEFINE_LOG_CATEGORY(LogBroomActor)

// ============================================================================
// CONSTRUCTOR
// ============================================================================

ABroomActor::ABroomActor()
    : InteractionRange(200.0f)
    , LockedPromptText(FText::FromString(TEXT("Requires broom unlock")))
    , MountPromptText(FText::FromString(TEXT("Press E to mount")))
    , InUseText(FText::FromString(TEXT("Broom in use")))
    , CurrentRider(nullptr)
    , RiderBroomComponent(nullptr)
    , bAutoRegisterForSight(true)
{
    PrimaryActorTick.bCanEverTick = false;

    // Create broom mesh as root component
    BroomMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BroomMesh"));
    RootComponent = BroomMesh;

    // Enable overlap events for interaction detection
    BroomMesh->SetGenerateOverlapEvents(true);
    BroomMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    BroomMesh->SetCollisionResponseToAllChannels(ECR_Overlap);

    // Create perception component so AI can detect this broom
    PerceptionSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("PerceptionSource"));
}

// ============================================================================
// BEGIN PLAY
// ============================================================================

void ABroomActor::BeginPlay()
{
    Super::BeginPlay();

    // Configure perception source for AI visibility
    if (PerceptionSource && bAutoRegisterForSight)
    {
        PerceptionSource->bAutoRegister = true;
        PerceptionSource->RegisterForSense(UAISense_Sight::StaticClass());

        UE_LOG(LogBroomActor, Log,
            TEXT("[%s] Registered for AI Sight perception"),
            *GetName());
    }

    UE_LOG(LogBroomActor, Display,
        TEXT("[BroomActor] %s initialized | DisplayName: %s | RequiredChannel: %s | AI Visible: %s"),
        *GetName(),
        *BroomConfiguration.BroomDisplayName.ToString(),
        (BroomConfiguration.RequiredChannel.IsNone() ? TEXT("None") : *BroomConfiguration.RequiredChannel.ToString()),
        bAutoRegisterForSight ? TEXT("YES") : TEXT("NO"));
}

// ============================================================================
// IINTERACTABLE IMPLEMENTATION
// ============================================================================

FText ABroomActor::GetTooltipText_Implementation() const
{
    return BroomConfiguration.BroomDisplayName;
}

FText ABroomActor::GetInteractionPrompt_Implementation() const
{
    // Check if broom is already being ridden
    if (CurrentRider != nullptr)
    {
        return InUseText;
    }

    // Check if player has required channel
    // Note: We can't check here since we don't have the interactor
    // This will be validated in CanInteract_Implementation
    return MountPromptText;
}

FText ABroomActor::GetDetailedInfo_Implementation() const
{
    return BroomConfiguration.BroomDescription;
}

bool ABroomActor::CanInteract_Implementation() const
{
    // Cannot interact if already being ridden
    if (CurrentRider != nullptr)
    {
        return false;
    }

    // If no required channel, anyone can interact
    if (BroomConfiguration.RequiredChannel.IsNone())
    {
        return true;
    }

    // We need the interactor to check channels, so this returns true
    // and we do the actual channel check in OnInteract
    return true;
}

void ABroomActor::OnInteract_Implementation(AActor* Interactor)
{
    if (!Interactor)
    {
        UE_LOG(LogBroomActor, Warning, TEXT("[BroomActor] OnInteract called with null Interactor"));
        return;
    }

    // Check if broom is already being ridden
    if (CurrentRider != nullptr)
    {
        UE_LOG(LogBroomActor, Log,
            TEXT("[BroomActor] %s is already being ridden by %s"),
            *GetName(), *CurrentRider->GetName());
        return;
    }

    // Check if interactor has required channel
    if (!HasRequiredChannel(Interactor))
    {
        UE_LOG(LogBroomActor, Log,
            TEXT("[BroomActor] %s lacks required channel '%s' to use %s"),
            *Interactor->GetName(),
            *BroomConfiguration.RequiredChannel.ToString(),
            *GetName());
        return;
    }

    // Get or create broom component on the interactor
    UAC_BroomComponent* BroomComp = GetOrCreateBroomComponent(Interactor);
    if (!BroomComp)
    {
        UE_LOG(LogBroomActor, Error,
            TEXT("[BroomActor] Failed to get/create BroomComponent on %s"),
            *Interactor->GetName());
        return;
    }

    // Configure the component with this broom's settings
    ConfigureBroomComponent(BroomComp);

    // Attach broom visual to rider
    AttachBroomToRider(Interactor);

    // Store references
    CurrentRider = Interactor;
    RiderBroomComponent = BroomComp;

    // Enable flight
    BroomComp->SetFlightEnabled(true);

    // Broadcast mount event
    OnBroomMounted.Broadcast(this, Interactor);

    UE_LOG(LogBroomActor, Display,
        TEXT("[BroomActor] %s mounted by %s | Config: %s"),
        *GetName(),
        *Interactor->GetName(),
        *BroomConfiguration.BroomDisplayName.ToString());
}

float ABroomActor::GetInteractionRange_Implementation() const
{
    return InteractionRange;
}

// ============================================================================
// PUBLIC API
// ============================================================================

void ABroomActor::OnRiderDismounted()
{
    if (!CurrentRider)
    {
        return;
    }

    AActor* PreviousRider = CurrentRider;

    // Detach broom visual
    DetachBroomFromRider();

    // Clear references
    CurrentRider = nullptr;
    RiderBroomComponent = nullptr;

    // Broadcast dismount event
    OnBroomDismounted.Broadcast(this, PreviousRider);

    UE_LOG(LogBroomActor, Display,
        TEXT("[BroomActor] %s dismounted by %s"),
        *GetName(),
        *PreviousRider->GetName());
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

bool ABroomActor::HasRequiredChannel(AActor* Interactor) const
{
    // If no channel required, always return true
    if (BroomConfiguration.RequiredChannel.IsNone())
    {
        return true;
    }

    // Check spell collection component for channel
    UAC_SpellCollectionComponent* SpellComp = Interactor->FindComponentByClass<UAC_SpellCollectionComponent>();
    if (SpellComp)
    {
        return SpellComp->HasChannel(BroomConfiguration.RequiredChannel);
    }

    UE_LOG(LogBroomActor, Warning,
        TEXT("[BroomActor] %s has no SpellCollectionComponent - cannot check channel requirement"),
        *Interactor->GetName());

    return false;
}

UAC_BroomComponent* ABroomActor::GetOrCreateBroomComponent(AActor* Interactor)
{
    if (!Interactor)
    {
        return nullptr;
    }

    // First, try to find existing component
    UAC_BroomComponent* BroomComp = Interactor->FindComponentByClass<UAC_BroomComponent>();

    if (BroomComp)
    {
        UE_LOG(LogBroomActor, Verbose,
            TEXT("[BroomActor] Found existing BroomComponent on %s"),
            *Interactor->GetName());
        return BroomComp;
    }

    // Component doesn't exist - create one dynamically
    // This allows any character to use a broom without pre-adding the component
    BroomComp = NewObject<UAC_BroomComponent>(Interactor, UAC_BroomComponent::StaticClass());
    if (BroomComp)
    {
        BroomComp->RegisterComponent();

        UE_LOG(LogBroomActor, Display,
            TEXT("[BroomActor] Created new BroomComponent on %s"),
            *Interactor->GetName());
    }

    return BroomComp;
}

void ABroomActor::ConfigureBroomComponent(UAC_BroomComponent* BroomComp)
{
    if (!BroomComp)
    {
        return;
    }

    // Pass the configuration to the component
    // The component will read all settings from this struct
    BroomComp->ApplyConfiguration(BroomConfiguration);

    // Store reference to this broom actor so component can notify us on dismount
    BroomComp->SetSourceBroom(this);

    UE_LOG(LogBroomActor, Verbose,
        TEXT("[BroomActor] Configured BroomComponent with: InfiniteDuration=%s, DrainOnlyWhenMoving=%s, Deceleration=%s"),
        BroomConfiguration.bInfiniteDuration ? TEXT("true") : TEXT("false"),
        BroomConfiguration.bDrainOnlyWhenMoving ? TEXT("true") : TEXT("false"),
        BroomConfiguration.bUseDeceleration ? TEXT("true") : TEXT("false"));
}

void ABroomActor::AttachBroomToRider(AActor* Rider)
{
    if (!Rider || !BroomMesh)
    {
        return;
    }

    ACharacter* CharRider = Cast<ACharacter>(Rider);
    if (!CharRider || !CharRider->GetMesh())
    {
        // Non-character rider - attach to actor root
        AttachToActor(Rider, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        return;
    }

    // Determine which socket to use based on whether rider is player or AI
    FName SocketToUse = BroomConfiguration.PlayerMountSocket;

    APawn* PawnRider = Cast<APawn>(Rider);
    if (PawnRider && !PawnRider->IsPlayerControlled())
    {
        // AI-controlled - use AI socket if specified
        if (!BroomConfiguration.AIMountSocket.IsNone())
        {
            SocketToUse = BroomConfiguration.AIMountSocket;
        }
    }

    // Check if socket exists on character mesh
    if (CharRider->GetMesh()->DoesSocketExist(SocketToUse))
    {
        AttachToComponent(
            CharRider->GetMesh(),
            FAttachmentTransformRules::SnapToTargetNotIncludingScale,
            SocketToUse);

        UE_LOG(LogBroomActor, Display,
            TEXT("[BroomActor] Attached to socket '%s' on %s"),
            *SocketToUse.ToString(),
            *Rider->GetName());
    }
    else
    {
        // Socket doesn't exist - attach to actor root as fallback
        AttachToActor(Rider, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

        UE_LOG(LogBroomActor, Warning,
            TEXT("[BroomActor] Socket '%s' not found on %s, using root attachment"),
            *SocketToUse.ToString(),
            *Rider->GetName());
    }
}

void ABroomActor::DetachBroomFromRider()
{
    // Detach from current parent
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

    // Optionally return to original spawn location
    // For now, broom stays where player dismounted
    // Designer can override in Blueprint if different behavior desired
}