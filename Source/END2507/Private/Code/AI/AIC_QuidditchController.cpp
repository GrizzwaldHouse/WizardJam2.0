// ============================================================================
// AIC_QuidditchController.cpp
// Developer: Marcus Daley
// Date: January 20, 2026
// Project: END2507
// ============================================================================

#include "Code/AI/AIC_QuidditchController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AIPerceptionTypes.h"
#include "Code/Actors/BaseAgent.h"
#include "Code/Flight/AC_BroomComponent.h"
#include "Code/Utilities/AC_SpellCollectionComponent.h"
#include "GenericTeamAgentInterface.h"

DEFINE_LOG_CATEGORY(LogQuidditchAI);

AAIC_QuidditchController::AAIC_QuidditchController()
    : BehaviorTreeAsset(nullptr)
    , BlackboardAsset(nullptr)
    , SightRadius(2000.0f)
    , LoseSightRadius(2500.0f)
    , PeripheralVisionAngle(90.0f)
    , TargetLocationKeyName(TEXT("TargetLocation"))
    , TargetActorKeyName(TEXT("TargetActor"))
    , IsFlyingKeyName(TEXT("IsFlying"))
    , SelfActorKeyName(TEXT("SelfActor"))
    , PerceivedCollectibleKeyName(TEXT("PerceivedCollectible"))
    , IsBoostingKeyName(TEXT("IsBoosting"))
    , HasBroomChannelKeyName(TEXT("HasBroomChannel"))
    , CachedBroomComponent(nullptr)
    , CachedSpellComponent(nullptr)
{
    // Create perception component
    AIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
    SetPerceptionComponent(*AIPerceptionComp);

    // Create and configure sight sense
    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    SightConfig->SightRadius = SightRadius;
    SightConfig->LoseSightRadius = LoseSightRadius;
    SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngle;
    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = true;  // For collectibles
    SightConfig->SetMaxAge(5.0f);

    AIPerceptionComp->ConfigureSense(*SightConfig);
    AIPerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());

    UE_LOG(LogQuidditchAI, Log,
        TEXT("[AIC_QuidditchController] Created with Sight: Radius=%.0f, LoseRadius=%.0f, Angle=%.0f"),
        SightRadius, LoseSightRadius, PeripheralVisionAngle);
}

void AAIC_QuidditchController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (!InPawn)
    {
        UE_LOG(LogQuidditchAI, Error, TEXT("[%s] OnPossess called with null pawn!"), *GetName());
        return;
    }

    UE_LOG(LogQuidditchAI, Display, TEXT("[%s] Possessed '%s'"), *GetName(), *InPawn->GetName());

    // ========================================================================
    // FACTION INITIALIZATION - CRITICAL FOR TEAM ID
    // This runs AFTER BeginPlay, so controller is now valid on the pawn.
    // Fixes Team ID 255 issue where faction was set before possession.
    // Works for both placed-in-level and spawned agents.
    // ========================================================================
    if (ABaseAgent* Agent = Cast<ABaseAgent>(InPawn))
    {
        // Get the agent's configured faction (set in Blueprint or level)
        int32 FactionID = Agent->GetPlacedFactionID();
        FLinearColor FactionColor = Agent->GetPlacedFactionColor();

        UE_LOG(LogQuidditchAI, Display,
            TEXT("[%s] Initializing faction from pawn: ID=%d, Color=%s"),
            *GetName(), FactionID, *FactionColor.ToString());

        // Set our team ID first (so GetGenericTeamId works)
        SetGenericTeamId(FGenericTeamId(static_cast<uint8>(FactionID)));

        // Now call the pawn's faction assignment (updates visual appearance and blackboard)
        Agent->OnFactionAssigned_Implementation(FactionID, FactionColor);

        UE_LOG(LogQuidditchAI, Display,
            TEXT("[%s] Faction initialized: Team ID = %d"),
            *GetName(), GetGenericTeamId().GetId());
    }
    else
    {
        UE_LOG(LogQuidditchAI, Warning,
            TEXT("[%s] Pawn '%s' is not a BaseAgent - cannot initialize faction"),
            *GetName(), *InPawn->GetName());
    }

    // Initialize blackboard
    SetupBlackboard(InPawn);

    // ========================================================================
    // OBSERVER PATTERN - Bind to component delegates
    // Controller is the "brain" that interprets "body" state changes
    // and updates Blackboard for BT decorator observation
    // ========================================================================
    BindComponentDelegates(InPawn);

    // Run behavior tree if assigned
    if (BehaviorTreeAsset)
    {
        if (RunBehaviorTree(BehaviorTreeAsset))
        {
            UE_LOG(LogQuidditchAI, Display, TEXT("[%s] Started behavior tree: %s"),
                *GetName(), *BehaviorTreeAsset->GetName());
        }
        else
        {
            UE_LOG(LogQuidditchAI, Error, TEXT("[%s] Failed to start behavior tree: %s"),
                *GetName(), *BehaviorTreeAsset->GetName());
        }
    }
    else
    {
        UE_LOG(LogQuidditchAI, Warning,
            TEXT("[%s] No BehaviorTreeAsset assigned - create Blueprint child and assign BT_QuidditchAI"),
            *GetName());
    }
}

void AAIC_QuidditchController::OnUnPossess()
{
    UE_LOG(LogQuidditchAI, Display, TEXT("[%s] OnUnPossess"), *GetName());

    // Unbind component delegates to prevent stale references
    UnbindComponentDelegates();

    if (UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(BrainComponent))
    {
        BTComp->StopTree(EBTStopMode::Safe);
    }

    Super::OnUnPossess();
}

void AAIC_QuidditchController::SetupBlackboard(APawn* InPawn)
{
    UBlackboardData* BBAsset = BlackboardAsset;
    if (!BBAsset && BehaviorTreeAsset)
    {
        BBAsset = BehaviorTreeAsset->BlackboardAsset;
    }

    if (!BBAsset)
    {
        UE_LOG(LogQuidditchAI, Warning, TEXT("[%s] No blackboard asset found!"), *GetName());
        return;
    }

    // UseBlackboard expects a raw pointer reference, not TObjectPtr
    UBlackboardComponent* BBComp = nullptr;
    if (UseBlackboard(BBAsset, BBComp))
    {
        UE_LOG(LogQuidditchAI, Display, TEXT("[%s] Initialized blackboard: %s"),
            *GetName(), *BBAsset->GetName());

        if (BBComp)
        {
            BBComp->SetValueAsObject(SelfActorKeyName, InPawn);
            BBComp->SetValueAsBool(IsFlyingKeyName, false);
        }
    }
    else
    {
        UE_LOG(LogQuidditchAI, Error, TEXT("[%s] Failed to initialize blackboard!"), *GetName());
    }
}

void AAIC_QuidditchController::SetFlightTarget(const FVector& TargetLocation)
{
    if (!Blackboard)
    {
        UE_LOG(LogQuidditchAI, Warning, TEXT("[%s] SetFlightTarget failed - no blackboard!"), *GetName());
        return;
    }

    Blackboard->SetValueAsVector(TargetLocationKeyName, TargetLocation);
    UE_LOG(LogQuidditchAI, Log, TEXT("[%s] Flight target: %s"), *GetName(), *TargetLocation.ToString());
}

void AAIC_QuidditchController::SetFlightTargetActor(AActor* TargetActor)
{
    if (!Blackboard)
    {
        UE_LOG(LogQuidditchAI, Warning, TEXT("[%s] SetFlightTargetActor failed - no blackboard!"), *GetName());
        return;
    }

    Blackboard->SetValueAsObject(TargetActorKeyName, TargetActor);

    if (TargetActor)
    {
        UE_LOG(LogQuidditchAI, Log, TEXT("[%s] Flight target actor: %s"),
            *GetName(), *TargetActor->GetName());
    }
    else
    {
        UE_LOG(LogQuidditchAI, Log, TEXT("[%s] Cleared flight target actor"), *GetName());
    }
}

void AAIC_QuidditchController::ClearFlightTarget()
{
    if (!Blackboard) return;

    Blackboard->ClearValue(TargetLocationKeyName);
    Blackboard->ClearValue(TargetActorKeyName);
    UE_LOG(LogQuidditchAI, Log, TEXT("[%s] Cleared flight targets"), *GetName());
}

bool AAIC_QuidditchController::GetFlightTarget(FVector& OutLocation) const
{
    if (!Blackboard) return false;

    // Check actor first
    if (UObject* TargetObj = Blackboard->GetValueAsObject(TargetActorKeyName))
    {
        if (AActor* TargetActor = Cast<AActor>(TargetObj))
        {
            OutLocation = TargetActor->GetActorLocation();
            return true;
        }
    }

    // Fall back to vector location
    OutLocation = Blackboard->GetValueAsVector(TargetLocationKeyName);
    return !OutLocation.IsZero();
}

void AAIC_QuidditchController::SetIsFlying(bool bIsFlying)
{
    if (!Blackboard)
    {
        UE_LOG(LogQuidditchAI, Warning, TEXT("[%s] SetIsFlying failed - no blackboard!"), *GetName());
        return;
    }

    Blackboard->SetValueAsBool(IsFlyingKeyName, bIsFlying);
    UE_LOG(LogQuidditchAI, Log, TEXT("[%s] IsFlying: %s"), *GetName(), bIsFlying ? TEXT("TRUE") : TEXT("FALSE"));
}

bool AAIC_QuidditchController::GetIsFlying() const
{
    if (!Blackboard) return false;
    return Blackboard->GetValueAsBool(IsFlyingKeyName);
}

// ============================================================================
// PERCEPTION HANDLING - Nick Penney Pattern
// ============================================================================

void AAIC_QuidditchController::BeginPlay()
{
    Super::BeginPlay();

    // Bind perception delegate - Nick Penney pattern
    // This connects perception events to our handler function
    if (AIPerceptionComp)
    {
        AIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(
            this, &AAIC_QuidditchController::HandlePerceptionUpdated);

        UE_LOG(LogQuidditchAI, Display,
            TEXT("[%s] Perception delegate bound successfully"), *GetName());
    }
    else
    {
        UE_LOG(LogQuidditchAI, Error,
            TEXT("[%s] AIPerceptionComp is null - cannot bind perception!"), *GetName());
    }
}

void AAIC_QuidditchController::HandlePerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    // Safety check - ignore null actors
    if (!Actor)
    {
        return;
    }

    // Get blackboard to store perception results
    UBlackboardComponent* BB = GetBlackboardComponent();
    if (!BB)
    {
        UE_LOG(LogQuidditchAI, Warning,
            TEXT("[%s] HandlePerceptionUpdated - No blackboard available!"), *GetName());
        return;
    }

    // Nick Penney pattern: Check if stimulus was successfully sensed
    if (Stimulus.WasSuccessfullySensed())
    {
        // Actor entered perception - log and potentially store
        UE_LOG(LogQuidditchAI, Display,
            TEXT("[%s] PERCEIVED: %s (Class: %s)"),
            *GetName(),
            *Actor->GetName(),
            *Actor->GetClass()->GetName());

        // Check if this is a collectible type we care about
        // For now, store any perceived actor that could be a collectible
        // The BTService_FindCollectible will filter appropriately

        // Store in TargetActor for general use
        // Individual services/tasks can filter by class
    }
    else
    {
        // Actor left perception
        UE_LOG(LogQuidditchAI, Display,
            TEXT("[%s] LOST: %s"), *GetName(), *Actor->GetName());

        // Only clear if this was our current target
        UObject* CurrentTarget = BB->GetValueAsObject(TargetActorKeyName);
        if (CurrentTarget == Actor)
        {
            BB->ClearValue(TargetActorKeyName);
            UE_LOG(LogQuidditchAI, Display,
                TEXT("[%s] Cleared TargetActor (was %s)"), *GetName(), *Actor->GetName());
        }

        // Also check PerceivedCollectible
        UObject* CurrentCollectible = BB->GetValueAsObject(PerceivedCollectibleKeyName);
        if (CurrentCollectible == Actor)
        {
            BB->ClearValue(PerceivedCollectibleKeyName);
            UE_LOG(LogQuidditchAI, Display,
                TEXT("[%s] Cleared PerceivedCollectible (was %s)"), *GetName(), *Actor->GetName());
        }
    }
}

// ============================================================================
// TEAM INTERFACE IMPLEMENTATION
// Required for AI perception filtering and collectible pickup permissions
// ============================================================================

void AAIC_QuidditchController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
    // Call parent class implementation to store team ID
    Super::SetGenericTeamId(NewTeamID);

    UE_LOG(LogQuidditchAI, Display, TEXT("[%s] SetGenericTeamId: Team=%d"),
        *GetName(), NewTeamID.GetId());

    // Update perception system when team changes
    // This ensures AI perception filters work correctly with new team
    if (AIPerceptionComp)
    {
        // RequestStimuliListenerUpdate tells perception system to re-evaluate
        // all stimuli using the new team ID via IGenericTeamAgentInterface
        AIPerceptionComp->RequestStimuliListenerUpdate();

        UE_LOG(LogQuidditchAI, Log, TEXT("[%s] Perception updated for new team"), *GetName());
    }
}

FGenericTeamId AAIC_QuidditchController::GetGenericTeamId() const
{
    return Super::GetGenericTeamId();
}

// ============================================================================
// OBSERVER PATTERN - Component Delegate Bindings
// The Controller is the "Brain" - it listens to "Body" (components) state
// and updates the Blackboard. BT decorators observe Blackboard with Aborts.
// ============================================================================

void AAIC_QuidditchController::BindComponentDelegates(APawn* InPawn)
{
    if (!InPawn)
    {
        UE_LOG(LogQuidditchAI, Warning,
            TEXT("[%s] BindComponentDelegates - null pawn"), *GetName());
        return;
    }

    // Bind to AC_BroomComponent delegates
    CachedBroomComponent = InPawn->FindComponentByClass<UAC_BroomComponent>();
    if (CachedBroomComponent)
    {
        CachedBroomComponent->OnFlightStateChanged.AddDynamic(
            this, &AAIC_QuidditchController::HandleFlightStateChanged);
        CachedBroomComponent->OnBoostStateChanged.AddDynamic(
            this, &AAIC_QuidditchController::HandleBoostStateChanged);

        UE_LOG(LogQuidditchAI, Display,
            TEXT("[%s] Bound to AC_BroomComponent delegates"), *GetName());

        // Initialize Blackboard with current state
        if (Blackboard)
        {
            Blackboard->SetValueAsBool(IsFlyingKeyName, CachedBroomComponent->IsFlying());
            Blackboard->SetValueAsBool(IsBoostingKeyName, CachedBroomComponent->IsBoosting());
        }
    }
    else
    {
        UE_LOG(LogQuidditchAI, Log,
            TEXT("[%s] Pawn has no AC_BroomComponent - flight bindings skipped"), *GetName());
    }

    // Bind to AC_SpellCollectionComponent delegates
    CachedSpellComponent = InPawn->FindComponentByClass<UAC_SpellCollectionComponent>();
    if (CachedSpellComponent)
    {
        CachedSpellComponent->OnChannelAdded.AddDynamic(
            this, &AAIC_QuidditchController::HandleChannelAdded);
        CachedSpellComponent->OnChannelRemoved.AddDynamic(
            this, &AAIC_QuidditchController::HandleChannelRemoved);

        UE_LOG(LogQuidditchAI, Display,
            TEXT("[%s] Bound to AC_SpellCollectionComponent delegates"), *GetName());

        // Initialize Blackboard with current channel state
        if (Blackboard)
        {
            bool bHasBroom = CachedSpellComponent->HasChannel(TEXT("Broom"));
            Blackboard->SetValueAsBool(HasBroomChannelKeyName, bHasBroom);

            UE_LOG(LogQuidditchAI, Log,
                TEXT("[%s] Initial HasBroomChannel: %s"),
                *GetName(), bHasBroom ? TEXT("TRUE") : TEXT("FALSE"));
        }
    }
    else
    {
        UE_LOG(LogQuidditchAI, Log,
            TEXT("[%s] Pawn has no AC_SpellCollectionComponent - channel bindings skipped"), *GetName());
    }
}

void AAIC_QuidditchController::UnbindComponentDelegates()
{
    // Unbind from AC_BroomComponent
    if (CachedBroomComponent)
    {
        CachedBroomComponent->OnFlightStateChanged.RemoveDynamic(
            this, &AAIC_QuidditchController::HandleFlightStateChanged);
        CachedBroomComponent->OnBoostStateChanged.RemoveDynamic(
            this, &AAIC_QuidditchController::HandleBoostStateChanged);

        UE_LOG(LogQuidditchAI, Display,
            TEXT("[%s] Unbound from AC_BroomComponent delegates"), *GetName());

        CachedBroomComponent = nullptr;
    }

    // Unbind from AC_SpellCollectionComponent
    if (CachedSpellComponent)
    {
        CachedSpellComponent->OnChannelAdded.RemoveDynamic(
            this, &AAIC_QuidditchController::HandleChannelAdded);
        CachedSpellComponent->OnChannelRemoved.RemoveDynamic(
            this, &AAIC_QuidditchController::HandleChannelRemoved);

        UE_LOG(LogQuidditchAI, Display,
            TEXT("[%s] Unbound from AC_SpellCollectionComponent delegates"), *GetName());

        CachedSpellComponent = nullptr;
    }
}

void AAIC_QuidditchController::UpdateChannelBlackboardKey(FName Channel, bool bHasChannel)
{
    if (!Blackboard)
    {
        return;
    }

    // Map channel names to Blackboard key names
    // Currently supporting "Broom" - extend as needed for Fire, Teleport, etc.
    if (Channel == TEXT("Broom"))
    {
        Blackboard->SetValueAsBool(HasBroomChannelKeyName, bHasChannel);
    }
    // Add more channel mappings as needed:
    // else if (Channel == TEXT("Fire")) { ... }
    // else if (Channel == TEXT("Teleport")) { ... }
}

// ============================================================================
// COMPONENT DELEGATE HANDLERS
// These are called when component state changes. They update Blackboard.
// ============================================================================

void AAIC_QuidditchController::HandleFlightStateChanged(bool bIsFlying)
{
    UE_LOG(LogQuidditchAI, Display,
        TEXT("[%s] HandleFlightStateChanged: %s"),
        *GetName(), bIsFlying ? TEXT("FLYING") : TEXT("GROUNDED"));

    // Update Blackboard - this is the proper observer pattern
    // BT decorators observing IsFlying will re-evaluate via Observer Aborts
    SetIsFlying(bIsFlying);
}

void AAIC_QuidditchController::HandleBoostStateChanged(bool bIsBoosting)
{
    UE_LOG(LogQuidditchAI, Log,
        TEXT("[%s] HandleBoostStateChanged: %s"),
        *GetName(), bIsBoosting ? TEXT("BOOSTING") : TEXT("NORMAL"));

    if (Blackboard)
    {
        Blackboard->SetValueAsBool(IsBoostingKeyName, bIsBoosting);
    }
}

void AAIC_QuidditchController::HandleChannelAdded(FName Channel)
{
    UE_LOG(LogQuidditchAI, Display,
        TEXT("[%s] HandleChannelAdded: %s"),
        *GetName(), *Channel.ToString());

    // Update Blackboard key for this channel
    UpdateChannelBlackboardKey(Channel, true);
}

void AAIC_QuidditchController::HandleChannelRemoved(FName Channel)
{
    UE_LOG(LogQuidditchAI, Display,
        TEXT("[%s] HandleChannelRemoved: %s"),
        *GetName(), *Channel.ToString());

    // Update Blackboard key for this channel
    UpdateChannelBlackboardKey(Channel, false);
}
