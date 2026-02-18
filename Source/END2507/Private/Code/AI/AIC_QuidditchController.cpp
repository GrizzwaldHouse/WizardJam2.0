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
#include "Code/Quidditch/QuidditchStagingZone.h"
#include "GenericTeamAgentInterface.h"
#include "StructuredLoggingMacros.h"

DEFINE_LOG_CATEGORY(LogQuidditchAI);

AAIC_QuidditchController::AAIC_QuidditchController()
    // ========================================================================
    // INITIALIZATION ORDER: Must match declaration order in header
    // Protected members first, then private members
    // ========================================================================
    // Protected: Components (AIPerceptionComp, SightConfig created in body)
    : BehaviorTreeAsset(nullptr)
    , BlackboardAsset(nullptr)
    // Protected: Perception configuration
    , SightRadius(2000.0f)
    , LoseSightRadius(2500.0f)
    , PeripheralVisionAngle(90.0f)
    // Protected: Blackboard key names (TargetLocation -> SelfActor)
    , TargetLocationKeyName(TEXT("TargetLocation"))
    , TargetActorKeyName(TEXT("TargetActor"))
    , IsFlyingKeyName(TEXT("IsFlying"))
    , SelfActorKeyName(TEXT("SelfActor"))
    // Protected: Quidditch agent configuration
    , AgentQuidditchTeam(EQuidditchTeam::TeamA)
    , AgentPreferredRole(EQuidditchRole::Seeker)
    // Private: Perceived collectible key
    , PerceivedCollectibleKeyName(TEXT("PerceivedCollectible"))
    // Private: Sync key names (MatchStarted -> HasBroom)
    , MatchStartedKeyName(TEXT("MatchStarted"))
    , ShouldSwapTeamKeyName(TEXT("ShouldSwapTeam"))
    , QuidditchRoleKeyName(TEXT("QuidditchRole"))
    , HasBroomKeyName(TEXT("HasBroom"))
    // Private: Staging zone tracking
    , bNotifiedStagingZoneArrival(false)
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

    // Structured logging - controller initialization
    SLOG_EVENT(this, "AI.Perception", "ControllerInitialized",
        Metadata.Add(TEXT("sight_radius"), FString::SanitizeFloat(SightRadius));
        Metadata.Add(TEXT("lose_radius"), FString::SanitizeFloat(LoseSightRadius));
        Metadata.Add(TEXT("peripheral_angle"), FString::SanitizeFloat(PeripheralVisionAngle));
    );

    UE_LOG(LogQuidditchAI, Log,
        TEXT("[AIC_QuidditchController] Created with Sight: Radius=%.0f, LoseRadius=%.0f, Angle=%.0f"),
        SightRadius, LoseSightRadius, PeripheralVisionAngle);
}

void AAIC_QuidditchController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    // Note: StructuredLogging subsystem may be NULL during early PIE initialization
    // SLOG_* macros handle null gracefully - this is expected and harmless

    if (!InPawn)
    {
        SLOG_ERROR(this, "AI.Lifecycle", "ControllerPossessedNull");
        UE_LOG(LogQuidditchAI, Error, TEXT("[%s] OnPossess called with null pawn!"), *GetName());
        return;
    }

    // Structured logging - controller possessed
    SLOG_EVENT(this, "AI.Lifecycle", "ControllerPossessed",
        Metadata.Add(TEXT("pawn_name"), InPawn->GetName());
        Metadata.Add(TEXT("pawn_class"), InPawn->GetClass()->GetName());
        Metadata.Add(TEXT("team_id"), FString::FromInt(GetGenericTeamId().GetId()));
    );

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

        // Structured logging - faction assigned
        SLOG_EVENT(this, "AI.Team", "FactionAssigned",
            Metadata.Add(TEXT("faction_id"), FString::FromInt(FactionID));
            Metadata.Add(TEXT("faction_color"), FactionColor.ToString());
        );

        UE_LOG(LogQuidditchAI, Display,
            TEXT("[%s] Faction initialized: Team ID = %d"),
            *GetName(), GetGenericTeamId().GetId());

        // Role-specific tags are now applied in HandleQuidditchRoleAssigned()
        // after GameMode assigns the actual role. Only Seekers get "Seeker" tag
        // so the Snitch only evades actual Seekers, not all agents.
    }
    else
    {
        UE_LOG(LogQuidditchAI, Warning,
            TEXT("[%s] Pawn '%s' is not a BaseAgent - cannot initialize faction"),
            *GetName(), *InPawn->GetName());
    }

    // ========================================================================
    // BROOM COMPONENT SYNC - Keep BB.IsFlying synced with actual flight state
    // ========================================================================
    if (UAC_BroomComponent* BroomComp = InPawn->FindComponentByClass<UAC_BroomComponent>())
    {
        BroomComp->OnFlightStateChanged.AddDynamic(this, &AAIC_QuidditchController::HandleFlightStateChanged);
        UE_LOG(LogQuidditchAI, Display, TEXT("[%s] Bound to BroomComponent flight state"), *GetName());
    }
    else
    {
        UE_LOG(LogQuidditchAI, Warning,
            TEXT("[%s] Pawn has no BroomComponent - flight state sync disabled"), *GetName());
    }

    // ========================================================================
    // STAGING ZONE OVERLAP DETECTION (Bee and Flower Pattern)
    // Agent detects landing on staging zone and notifies GameMode
    // ========================================================================
    BindToPawnOverlapEvents();
    bNotifiedStagingZoneArrival = false;  // Reset for new possession

    // Initialize blackboard
    SetupBlackboard(InPawn);

    // ========================================================================
    // GAMEMODE DELEGATE BINDING - MUST happen BEFORE RegisterAgentWithGameMode
    // Gas Station Pattern: Bind to GameMode delegates for synchronization
    // Registration broadcasts OnQuidditchRoleAssigned, so we must be bound first!
    // ========================================================================
    BindToGameModeEvents();

    // ========================================================================
    // QUIDDITCH REGISTRATION - Register agent with GameMode to receive role
    // Must happen AFTER blackboard setup AND delegate binding
    // so HandleQuidditchRoleAssigned can write role to blackboard
    // ========================================================================
    RegisterAgentWithGameMode(InPawn);

    // Run behavior tree if assigned
    if (BehaviorTreeAsset)
    {
        if (RunBehaviorTree(BehaviorTreeAsset))
        {
            // Structured logging - BT started successfully
            SLOG_EVENT(this, "AI.BehaviorTree", "BehaviorTreeStarted",
                Metadata.Add(TEXT("tree_name"), BehaviorTreeAsset->GetName());
                Metadata.Add(TEXT("success"), TEXT("true"));
            );

            UE_LOG(LogQuidditchAI, Display, TEXT("[%s] Started behavior tree: %s"),
                *GetName(), *BehaviorTreeAsset->GetName());
        }
        else
        {
            // Structured logging - BT failed to start
            SLOG_ERROR(this, "AI.BehaviorTree", "BehaviorTreeStartFailed",
                Metadata.Add(TEXT("tree_name"), BehaviorTreeAsset->GetName());
                Metadata.Add(TEXT("success"), TEXT("false"));
            );

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
    APawn* CurrentPawn = GetPawn();
    FString PawnName = CurrentPawn ? CurrentPawn->GetName() : TEXT("None");

    // Structured logging - controller unpossessed
    SLOG_EVENT(this, "AI.Lifecycle", "ControllerUnpossessed",
        Metadata.Add(TEXT("pawn_name"), PawnName);
    );

    UE_LOG(LogQuidditchAI, Display, TEXT("[%s] OnUnPossess"), *GetName());

    // Unbind perception delegate to prevent stale reference crash
    if (AIPerceptionComp)
    {
        AIPerceptionComp->OnTargetPerceptionUpdated.RemoveDynamic(
            this, &AAIC_QuidditchController::HandlePerceptionUpdated);
        UE_LOG(LogQuidditchAI, Log, TEXT("[%s] Unbound from perception delegate"), *GetName());
    }

    // Unbind from BroomComponent delegate to prevent stale reference crash
    if (CurrentPawn)
    {
        if (UAC_BroomComponent* BroomComp = CurrentPawn->FindComponentByClass<UAC_BroomComponent>())
        {
            BroomComp->OnFlightStateChanged.RemoveDynamic(this, &AAIC_QuidditchController::HandleFlightStateChanged);
            UE_LOG(LogQuidditchAI, Log, TEXT("[%s] Unbound from BroomComponent flight state"), *GetName());
        }
    }

    // Unbind from pawn overlap events before unpossessing
    UnbindFromPawnOverlapEvents();

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
        SLOG_WARNING(this, "AI.Blackboard", "BlackboardAssetNotFound");
        UE_LOG(LogQuidditchAI, Warning, TEXT("[%s] No blackboard asset found!"), *GetName());
        return;
    }

    // UseBlackboard expects a raw pointer reference, not TObjectPtr
    UBlackboardComponent* BBComp = nullptr;
    if (UseBlackboard(BBAsset, BBComp))
    {
        // Structured logging - blackboard initialized successfully
        SLOG_EVENT(this, "AI.Blackboard", "BlackboardInitialized",
            Metadata.Add(TEXT("bb_asset_name"), BBAsset->GetName());
        );

        UE_LOG(LogQuidditchAI, Display, TEXT("[%s] Initialized blackboard: %s"),
            *GetName(), *BBAsset->GetName());

        if (BBComp)
        {
            // Write initial blackboard values - Actor/Object keys
            BBComp->SetValueAsObject(SelfActorKeyName, InPawn);

            // Initialize HomeLocation to spawn position (for BTTask_ReturnToHome)
            BBComp->SetValueAsVector(FName("HomeLocation"), InPawn->GetActorLocation());

            // Initialize all Bool keys to false (prevents (invalid) markers in debugger)
            BBComp->SetValueAsBool(IsFlyingKeyName, false);
            BBComp->SetValueAsBool(MatchStartedKeyName, false);
            BBComp->SetValueAsBool(ShouldSwapTeamKeyName, false);
            BBComp->SetValueAsBool(HasBroomKeyName, false);

            // Initialize Vector keys to zero
            BBComp->SetValueAsVector(TargetLocationKeyName, FVector::ZeroVector);

            // Initialize additional Vector keys that BT Services expect to exist
            // Without initialization these show as (invalid) in Blackboard debugger
            BBComp->SetValueAsVector(FName("SnitchLocation"), FVector::ZeroVector);
            BBComp->SetValueAsVector(FName("SnitchVelocity"), FVector::ZeroVector);
            BBComp->SetValueAsVector(FName("StageLocation"), FVector::ZeroVector);

            // Initialize staging zone keys for perception-based navigation (January 28, 2026)
            // BTService_FindStagingZone writes to these keys when it perceives a staging zone
            BBComp->SetValueAsVector(FName("StagingZoneLocation"), FVector::ZeroVector);
            // Note: StagingZoneActor is Object type - left unset until perception finds one

            // Initialize GoalCenter for BTTask_PositionInGoal and BTTask_BlockShot
            BBComp->SetValueAsVector(FName("GoalCenter"), FVector::ZeroVector);

            // Initialize additional Bool keys
            BBComp->SetValueAsBool(FName("ReachedStagingZone"), false);
            BBComp->SetValueAsBool(FName("IsReady"), false);

            // Note: The following keys are intentionally left unset until runtime:
            // - TargetActorKeyName: Set when perception finds a target
            // - PerceivedCollectibleKeyName: Set when perception finds a collectible
            // - QuidditchRoleKeyName: Set when role is assigned by GameMode

            UE_LOG(LogQuidditchAI, Display,
                TEXT("[%s] Blackboard fully initialized | HomeLocation=%s | MatchStarted=false | HasBroom=false"),
                *GetName(), *InPawn->GetActorLocation().ToString());

            // SILENT FAILURE FIX - Validate blackboard writes
            if (!BBComp->GetValueAsObject(SelfActorKeyName))
            {
                SLOG_WARNING(this, "AI.Blackboard", "BlackboardKeyWriteFailed",
                    Metadata.Add(TEXT("key_name"), SelfActorKeyName.ToString());
                    Metadata.Add(TEXT("expected_value"), InPawn ? InPawn->GetName() : TEXT("null"));
                );
            }
        }
    }
    else
    {
        // Structured logging - blackboard initialization failed
        SLOG_ERROR(this, "AI.Blackboard", "BlackboardInitFailed",
            Metadata.Add(TEXT("bb_asset_name"), BBAsset->GetName());
        );

        UE_LOG(LogQuidditchAI, Error, TEXT("[%s] Failed to initialize blackboard!"), *GetName());
    }
}

void AAIC_QuidditchController::SetFlightTarget(const FVector& TargetLocation)
{
    if (!Blackboard)
    {
        SLOG_WARNING(this, "AI.Flight", "FlightTargetSetFailed",
            Metadata.Add(TEXT("reason"), TEXT("no_blackboard"));
        );
        UE_LOG(LogQuidditchAI, Warning, TEXT("[%s] SetFlightTarget failed - no blackboard!"), *GetName());
        return;
    }

    Blackboard->SetValueAsVector(TargetLocationKeyName, TargetLocation);

    // Structured logging - flight target set
    SLOG_EVENT(this, "AI.Flight", "FlightTargetSet",
        Metadata.Add(TEXT("target_location"), TargetLocation.ToString());
    );

    UE_LOG(LogQuidditchAI, Log, TEXT("[%s] Flight target: %s"), *GetName(), *TargetLocation.ToString());
}

void AAIC_QuidditchController::SetFlightTargetActor(AActor* TargetActor)
{
    if (!Blackboard)
    {
        SLOG_WARNING(this, "AI.Flight", "FlightTargetActorSetFailed",
            Metadata.Add(TEXT("reason"), TEXT("no_blackboard"));
        );
        UE_LOG(LogQuidditchAI, Warning, TEXT("[%s] SetFlightTargetActor failed - no blackboard!"), *GetName());
        return;
    }

    Blackboard->SetValueAsObject(TargetActorKeyName, TargetActor);

    if (TargetActor)
    {
        // Structured logging - flight target actor set
        SLOG_EVENT(this, "AI.Flight", "FlightTargetActorSet",
            Metadata.Add(TEXT("target_actor_name"), TargetActor->GetName());
        );

        UE_LOG(LogQuidditchAI, Log, TEXT("[%s] Flight target actor: %s"),
            *GetName(), *TargetActor->GetName());
    }
    else
    {
        // Structured logging - flight target actor cleared
        SLOG_EVENT(this, "AI.Flight", "FlightTargetActorCleared");

        UE_LOG(LogQuidditchAI, Log, TEXT("[%s] Cleared flight target actor"), *GetName());
    }
}

void AAIC_QuidditchController::ClearFlightTarget()
{
    if (!Blackboard) return;

    Blackboard->ClearValue(TargetLocationKeyName);
    Blackboard->ClearValue(TargetActorKeyName);

    // Structured logging - flight targets cleared
    SLOG_EVENT(this, "AI.Flight", "FlightTargetCleared");

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
// LIFECYCLE - Observer Pattern Binding
// ============================================================================

void AAIC_QuidditchController::BeginPlay()
{
    Super::BeginPlay();

    // Bind perception delegate - Nick Penney pattern
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

    // NOTE: BindToGameModeEvents() is now called in OnPossess() BEFORE RegisterAgentWithGameMode()
    // This ensures the delegate handler is bound when OnQuidditchRoleAssigned broadcasts
}

void AAIC_QuidditchController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Unbind from GameMode delegates to prevent stale references
    UnbindFromGameModeEvents();

    Super::EndPlay(EndPlayReason);
}

void AAIC_QuidditchController::BindToGameModeEvents()
{
    // Cache GameMode reference ONCE - Observer Pattern (no repeated lookups)
    CachedGameMode = Cast<AQuidditchGameMode>(GetWorld()->GetAuthGameMode());

    if (!CachedGameMode.IsValid())
    {
        UE_LOG(LogQuidditchAI, Warning,
            TEXT("[%s] No QuidditchGameMode found - sync disabled"), *GetName());
        return;
    }

    // Bind to synchronization delegates
    CachedGameMode->OnMatchStarted.AddDynamic(this, &AAIC_QuidditchController::HandleMatchStarted);
    CachedGameMode->OnMatchEnded.AddDynamic(this, &AAIC_QuidditchController::HandleMatchEnded);
    CachedGameMode->OnAgentSelectedForSwap.AddDynamic(this, &AAIC_QuidditchController::HandleAgentSelectedForSwap);
    CachedGameMode->OnTeamSwapComplete.AddDynamic(this, &AAIC_QuidditchController::HandleTeamSwapComplete);
    CachedGameMode->OnQuidditchRoleAssigned.AddDynamic(this, &AAIC_QuidditchController::HandleQuidditchRoleAssigned);

    UE_LOG(LogQuidditchAI, Display,
        TEXT("[%s] Bound to GameMode sync delegates"), *GetName());
}

void AAIC_QuidditchController::UnbindFromGameModeEvents()
{
    if (!CachedGameMode.IsValid())
    {
        return;
    }

    CachedGameMode->OnMatchStarted.RemoveDynamic(this, &AAIC_QuidditchController::HandleMatchStarted);
    CachedGameMode->OnMatchEnded.RemoveDynamic(this, &AAIC_QuidditchController::HandleMatchEnded);
    CachedGameMode->OnAgentSelectedForSwap.RemoveDynamic(this, &AAIC_QuidditchController::HandleAgentSelectedForSwap);
    CachedGameMode->OnTeamSwapComplete.RemoveDynamic(this, &AAIC_QuidditchController::HandleTeamSwapComplete);
    CachedGameMode->OnQuidditchRoleAssigned.RemoveDynamic(this, &AAIC_QuidditchController::HandleQuidditchRoleAssigned);

    UE_LOG(LogQuidditchAI, Log,
        TEXT("[%s] Unbound from GameMode sync delegates"), *GetName());
}

// ============================================================================
// SYNCHRONIZATION HANDLERS - Gas Station Pattern
// Update Blackboard when GameMode broadcasts events
// BT decorators read Blackboard - no polling!
// ============================================================================

void AAIC_QuidditchController::HandleMatchStarted(float CountdownSeconds)
{
    // Gas Station Pattern: gunCondition.notify_all() equivalent
    // Update Blackboard - BT decorators will automatically re-evaluate
    if (UBlackboardComponent* BB = GetBlackboardComponent())
    {
        BB->SetValueAsBool(MatchStartedKeyName, true);

        // Structured logging - match started
        SLOG_EVENT(this, "AI.Sync", "MatchStarted",
            Metadata.Add(TEXT("countdown_seconds"), FString::SanitizeFloat(CountdownSeconds));
        );

        UE_LOG(LogQuidditchAI, Display,
            TEXT("[%s] HandleMatchStarted - BB.%s = true"),
            *GetName(), *MatchStartedKeyName.ToString());
    }
}

void AAIC_QuidditchController::HandleMatchEnded()
{
    // Gas Station Pattern: testOver = true equivalent
    if (UBlackboardComponent* BB = GetBlackboardComponent())
    {
        BB->SetValueAsBool(MatchStartedKeyName, false);

        // Structured logging - match ended
        SLOG_EVENT(this, "AI.Sync", "MatchEnded");

        UE_LOG(LogQuidditchAI, Display,
            TEXT("[%s] HandleMatchEnded - BB.%s = false"),
            *GetName(), *MatchStartedKeyName.ToString());
    }
}

void AAIC_QuidditchController::HandleAgentSelectedForSwap(APawn* SelectedAgent)
{
    // Only respond if WE are the selected agent
    if (SelectedAgent != GetPawn())
    {
        return;
    }

    if (UBlackboardComponent* BB = GetBlackboardComponent())
    {
        BB->SetValueAsBool(ShouldSwapTeamKeyName, true);

        // Structured logging - agent selected for swap
        SLOG_EVENT(this, "AI.Sync", "AgentSelectedForSwap",
            Metadata.Add(TEXT("selected_agent_name"), SelectedAgent ? SelectedAgent->GetName() : TEXT("null"));
        );

        UE_LOG(LogQuidditchAI, Display,
            TEXT("[%s] Selected for team swap - BB.%s = true"),
            *GetName(), *ShouldSwapTeamKeyName.ToString());
    }
}

void AAIC_QuidditchController::HandleTeamSwapComplete(APawn* SwappedAgent, EQuidditchTeam OldTeam, EQuidditchTeam NewTeam)
{
    // Only respond if WE are the swapped agent
    if (SwappedAgent != GetPawn())
    {
        return;
    }

    // Update our team ID
    SetGenericTeamId(FGenericTeamId(static_cast<uint8>(NewTeam)));

    // Clear swap flag
    if (UBlackboardComponent* BB = GetBlackboardComponent())
    {
        BB->SetValueAsBool(ShouldSwapTeamKeyName, false);
    }

    // Structured logging - team swap complete
    SLOG_EVENT(this, "AI.Sync", "TeamSwapComplete",
        Metadata.Add(TEXT("old_team"), FString::FromInt(static_cast<int32>(OldTeam)));
        Metadata.Add(TEXT("new_team"), FString::FromInt(static_cast<int32>(NewTeam)));
    );

    UE_LOG(LogQuidditchAI, Display,
        TEXT("[%s] Team swap complete: %d -> %d"),
        *GetName(), static_cast<int32>(OldTeam), static_cast<int32>(NewTeam));
}

// ============================================================================
// PERCEPTION HANDLING - Nick Penney Pattern
// ============================================================================

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
        // Structured logging - actor perceived
        SLOG_EVENT(this, "AI.Perception", "ActorPerceived",
            Metadata.Add(TEXT("actor_name"), Actor->GetName());
            Metadata.Add(TEXT("actor_class"), Actor->GetClass()->GetName());
            Metadata.Add(TEXT("stimulus_tag"), Stimulus.Tag.ToString());
        );

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
        bool bTargetCleared = false;

        // Only clear if this was our current target
        UObject* CurrentTarget = BB->GetValueAsObject(TargetActorKeyName);
        if (CurrentTarget == Actor)
        {
            BB->ClearValue(TargetActorKeyName);
            bTargetCleared = true;
            UE_LOG(LogQuidditchAI, Display,
                TEXT("[%s] Cleared TargetActor (was %s)"), *GetName(), *Actor->GetName());
        }

        // Also check PerceivedCollectible
        UObject* CurrentCollectible = BB->GetValueAsObject(PerceivedCollectibleKeyName);
        if (CurrentCollectible == Actor)
        {
            BB->ClearValue(PerceivedCollectibleKeyName);
            bTargetCleared = true;
            UE_LOG(LogQuidditchAI, Display,
                TEXT("[%s] Cleared PerceivedCollectible (was %s)"), *GetName(), *Actor->GetName());
        }

        // Structured logging - actor lost
        SLOG_EVENT(this, "AI.Perception", "ActorLost",
            Metadata.Add(TEXT("actor_name"), Actor->GetName());
            Metadata.Add(TEXT("current_target_cleared"), bTargetCleared ? TEXT("true") : TEXT("false"));
        );

        UE_LOG(LogQuidditchAI, Display,
            TEXT("[%s] LOST: %s"), *GetName(), *Actor->GetName());
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

    // Update perception system when team changes
    // This ensures AI perception filters work correctly with new team
    bool bPerceptionUpdated = false;
    if (AIPerceptionComp)
    {
        // RequestStimuliListenerUpdate tells perception system to re-evaluate
        // all stimuli using the new team ID via IGenericTeamAgentInterface
        AIPerceptionComp->RequestStimuliListenerUpdate();
        bPerceptionUpdated = true;

        UE_LOG(LogQuidditchAI, Log, TEXT("[%s] Perception updated for new team"), *GetName());
    }

    // Structured logging - team ID changed
    SLOG_EVENT(this, "AI.Team", "TeamIdChanged",
        Metadata.Add(TEXT("new_team_id"), FString::FromInt(NewTeamID.GetId()));
        Metadata.Add(TEXT("perception_updated"), bPerceptionUpdated ? TEXT("true") : TEXT("false"));
    );

    UE_LOG(LogQuidditchAI, Display, TEXT("[%s] SetGenericTeamId: Team=%d"),
        *GetName(), NewTeamID.GetId());
}

FGenericTeamId AAIC_QuidditchController::GetGenericTeamId() const
{
    return Super::GetGenericTeamId();
}

// ============================================================================
// BROOM COMPONENT SYNC
// ============================================================================

void AAIC_QuidditchController::HandleFlightStateChanged(bool bNewFlightState)
{
    // Update Blackboard IsFlying to match actual BroomComponent state
    if (UBlackboardComponent* BB = GetBlackboardComponent())
    {
        BB->SetValueAsBool(IsFlyingKeyName, bNewFlightState);

        // CRITICAL FIX: When dismounting (flight disabled), the agent no longer has a usable broom
        // This prevents the BT from immediately trying to re-mount after force dismount
        // The BT must re-acquire a broom from the world if stamina depleted
        if (!bNewFlightState)
        {
            // Agent dismounted - clear HasBroom so BT knows it needs to find/acquire a new broom
            BB->SetValueAsBool(HasBroomKeyName, false);

            UE_LOG(LogQuidditchAI, Warning,
                TEXT("[%s] DISMOUNTED -> BB.IsFlying=false, BB.HasBroom=false"),
                *GetName());
        }
        else
        {
            // Agent mounted - set HasBroom
            BB->SetValueAsBool(HasBroomKeyName, true);

            UE_LOG(LogQuidditchAI, Display,
                TEXT("[%s] MOUNTED -> BB.IsFlying=true, BB.HasBroom=true"),
                *GetName());
        }
    }
}

// ============================================================================
// QUIDDITCH ROLE ASSIGNMENT
// ============================================================================

void AAIC_QuidditchController::HandleQuidditchRoleAssigned(APawn* Agent, EQuidditchTeam Team, EQuidditchRole AssignedRole)
{
    // Only respond if WE are the assigned agent
    if (Agent != GetPawn())
    {
        return;
    }

    if (UBlackboardComponent* BB = GetBlackboardComponent())
    {
        // Set the role as Name (enum string representation for BT compatibility)
        FName RoleName = *UEnum::GetValueAsString(AssignedRole);
        BB->SetValueAsName(QuidditchRoleKeyName, RoleName);

        // Structured logging - role assigned
        SLOG_EVENT(this, "AI.Quidditch", "RoleAssigned",
            Metadata.Add(TEXT("assigned_role"), UEnum::GetValueAsString(AssignedRole));
            Metadata.Add(TEXT("team"), UEnum::GetValueAsString(Team));
        );

        UE_LOG(LogQuidditchAI, Display,
            TEXT("[%s] HandleQuidditchRoleAssigned -> BB.%s = %s | Team=%s"),
            *GetName(),
            *QuidditchRoleKeyName.ToString(),
            *UEnum::GetValueAsString(AssignedRole),
            *UEnum::GetValueAsString(Team));
    }

    // Apply role-specific actor tags AFTER role is confirmed
    // SnitchController checks for "Seeker", "Flying", or "Player" tags
    // Only actual Seekers should trigger Snitch evasion behavior
    if (APawn* ControlledPawn = GetPawn())
    {
        if (AssignedRole == EQuidditchRole::Seeker)
        {
            ControlledPawn->Tags.AddUnique(TEXT("Seeker"));
            UE_LOG(LogQuidditchAI, Display, TEXT("[%s] Added 'Seeker' tag (role confirmed)"),
                *GetName());
        }
        else
        {
            // Remove Seeker tag if role was reassigned from Seeker to something else
            ControlledPawn->Tags.Remove(TEXT("Seeker"));
        }
    }
}

// ============================================================================
// QUIDDITCH REGISTRATION
// Registers AI agent with GameMode to receive team/role assignment
// ============================================================================

void AAIC_QuidditchController::RegisterAgentWithGameMode(APawn* InPawn)
{
    if (!InPawn)
    {
        UE_LOG(LogQuidditchAI, Warning, TEXT("[%s] RegisterAgentWithGameMode - InPawn is null"), *GetName());
        return;
    }

    // Get QuidditchGameMode - use cached if available, otherwise lookup
    AQuidditchGameMode* QuidditchGM = CachedGameMode.Get();
    if (!QuidditchGM)
    {
        QuidditchGM = Cast<AQuidditchGameMode>(GetWorld()->GetAuthGameMode());
    }

    if (!QuidditchGM)
    {
        UE_LOG(LogQuidditchAI, Warning,
            TEXT("[%s] RegisterAgentWithGameMode - Not in QuidditchGameMode, skipping registration"),
            *GetName());
        return;
    }

    // ========================================================================
    // READ TEAM/ROLE FROM AGENT (not from controller properties)
    // Agent's getters check data asset first, then fall back to manual props
    // This is the industry-standard pattern: Pawn holds identity, Controller reads
    // ========================================================================
    EQuidditchTeam AgentTeam = EQuidditchTeam::None;
    EQuidditchRole AgentRole = EQuidditchRole::None;

    if (ABaseAgent* Agent = Cast<ABaseAgent>(InPawn))
    {
        AgentTeam = Agent->GetQuidditchTeam();
        AgentRole = Agent->GetPreferredQuidditchRole();

        UE_LOG(LogQuidditchAI, Display,
            TEXT("[%s] Read from Agent: Team=%s, Role=%s, HasDataAsset=%s"),
            *GetName(),
            *UEnum::GetValueAsString(AgentTeam),
            *UEnum::GetValueAsString(AgentRole),
            Agent->GetAgentDataAsset() ? TEXT("Yes") : TEXT("No"));
    }
    else
    {
        // Fallback for non-BaseAgent pawns - use controller properties (deprecated)
        AgentTeam = AgentQuidditchTeam;
        AgentRole = AgentPreferredRole;
        UE_LOG(LogQuidditchAI, Warning,
            TEXT("[%s] Pawn is not BaseAgent - using deprecated controller properties"),
            *GetName());
    }

    // Validate team configuration
    if (AgentTeam == EQuidditchTeam::None)
    {
        UE_LOG(LogQuidditchAI, Warning,
            TEXT("[%s] RegisterAgentWithGameMode - Team is None! Assign AgentDataAsset or set PlacedQuidditchTeam."),
            *GetName());
    }

    // Register with GameMode using values from agent
    EQuidditchRole AssignedRole = QuidditchGM->RegisterQuidditchAgent(
        InPawn,
        AgentRole,   // FROM AGENT
        AgentTeam);  // FROM AGENT

    if (AssignedRole == EQuidditchRole::None)
    {
        UE_LOG(LogQuidditchAI, Warning,
            TEXT("[%s] RegisterAgentWithGameMode - Failed to get role (team full?)"),
            *GetName());
    }
    else
    {
        // DIRECT WRITE FALLBACK: Write role to blackboard immediately
        // This ensures the role is set even if delegate binding timing is off
        // The delegate handler will also fire, but duplicate writes are harmless
        if (UBlackboardComponent* BB = GetBlackboardComponent())
        {
            FName RoleName = *UEnum::GetValueAsString(AssignedRole);
            BB->SetValueAsName(QuidditchRoleKeyName, RoleName);

            UE_LOG(LogQuidditchAI, Display,
                TEXT("[%s] RegisterAgentWithGameMode - Direct BB write: %s = %s"),
                *GetName(), *QuidditchRoleKeyName.ToString(), *RoleName.ToString());
        }

        UE_LOG(LogQuidditchAI, Display,
            TEXT("[%s] RegisterAgentWithGameMode SUCCESS | Team=%s | Preferred=%s | Assigned=%s"),
            *GetName(),
            *UEnum::GetValueAsString(AgentTeam),
            *UEnum::GetValueAsString(AgentRole),
            *UEnum::GetValueAsString(AssignedRole));
    }
}

// ============================================================================
// STAGING ZONE LANDING DETECTION (Bee and Flower Pattern)
// Agent (bee) detects landing on staging zone (flower) and decides if it's
// the correct one for our team/role. The zone doesn't know about us.
// ============================================================================

void AAIC_QuidditchController::BindToPawnOverlapEvents()
{
    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn)
    {
        return;
    }

    // Bind to pawn's overlap events (both enter and exit for staging zone tracking)
    ControlledPawn->OnActorBeginOverlap.AddDynamic(this, &AAIC_QuidditchController::HandlePawnBeginOverlap);
    ControlledPawn->OnActorEndOverlap.AddDynamic(this, &AAIC_QuidditchController::HandlePawnEndOverlap);

    UE_LOG(LogQuidditchAI, Display,
        TEXT("[%s] Bound to pawn overlap events (begin + end) for staging zone detection"),
        *GetName());
}

void AAIC_QuidditchController::UnbindFromPawnOverlapEvents()
{
    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn)
    {
        return;
    }

    // Unbind from pawn's overlap events (both enter and exit)
    ControlledPawn->OnActorBeginOverlap.RemoveDynamic(this, &AAIC_QuidditchController::HandlePawnBeginOverlap);
    ControlledPawn->OnActorEndOverlap.RemoveDynamic(this, &AAIC_QuidditchController::HandlePawnEndOverlap);

    UE_LOG(LogQuidditchAI, Log,
        TEXT("[%s] Unbound from pawn overlap events"),
        *GetName());
}

void AAIC_QuidditchController::HandlePawnBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
    // Skip if we've already notified (prevent double-counting)
    if (bNotifiedStagingZoneArrival)
    {
        return;
    }

    // Check if the overlapped actor is a staging zone
    AQuidditchStagingZone* StagingZone = Cast<AQuidditchStagingZone>(OtherActor);
    if (!StagingZone)
    {
        return;
    }

    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn)
    {
        return;
    }

    // ========================================================================
    // AGENT-SIDE FILTERING (Bee decides if this is the right flower)
    // Read the zone's hint properties and compare to our team/role
    // ========================================================================
    int32 ZoneTeam = StagingZone->TeamHint;
    int32 ZoneRole = StagingZone->RoleHint;

    // Get our team and role from GameMode registry
    int32 AgentTeam = 0;
    int32 AgentRole = 0;

    if (CachedGameMode.IsValid())
    {
        EQuidditchTeam MyTeam = CachedGameMode->GetAgentTeam(ControlledPawn);
        EQuidditchRole MyRole = CachedGameMode->GetAgentRole(ControlledPawn);
        AgentTeam = static_cast<int32>(MyTeam);
        AgentRole = static_cast<int32>(MyRole);
    }

    // Check if this zone matches our team/role
    if (ZoneTeam != AgentTeam || ZoneRole != AgentRole)
    {
        UE_LOG(LogQuidditchAI, Verbose,
            TEXT("[%s] Overlapped staging zone '%s' but it's not ours | ZoneTeam=%d AgentTeam=%d | ZoneRole=%d AgentRole=%d"),
            *GetName(), *StagingZone->GetName(), ZoneTeam, AgentTeam, ZoneRole, AgentRole);
        return;
    }

    // ========================================================================
    // THIS IS OUR STAGING ZONE - Notify GameMode (Agent broadcasts "I landed")
    // ========================================================================
    bNotifiedStagingZoneArrival = true;

    // Update blackboard
    if (UBlackboardComponent* BB = GetBlackboardComponent())
    {
        BB->SetValueAsBool(FName("ReachedStagingZone"), true);
        BB->SetValueAsBool(FName("IsReady"), true);
    }

    // Notify GameMode (the AGENT broadcasts, not the zone)
    if (CachedGameMode.IsValid())
    {
        CachedGameMode->HandleAgentReachedStagingZone(ControlledPawn);
    }

    // Structured logging - agent landed on staging zone
    SLOG_EVENT(this, "AI.Staging", "AgentLandedOnStagingZone",
        Metadata.Add(TEXT("zone_name"), StagingZone->GetName());
        Metadata.Add(TEXT("zone_identifier"), StagingZone->GetZoneIdentifier().ToString());
        Metadata.Add(TEXT("agent_team"), FString::FromInt(AgentTeam));
        Metadata.Add(TEXT("agent_role"), FString::FromInt(AgentRole));
    );

    UE_LOG(LogQuidditchAI, Display,
        TEXT("[%s] LANDED on staging zone '%s' | Identifier=%s | Notified GameMode"),
        *GetName(),
        *StagingZone->GetName(),
        *StagingZone->GetZoneIdentifier().ToString());
}

void AAIC_QuidditchController::HandlePawnEndOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
    // Only process if we previously notified arrival
    if (!bNotifiedStagingZoneArrival)
    {
        return;
    }

    // Check if the actor we stopped overlapping is a staging zone
    AQuidditchStagingZone* StagingZone = Cast<AQuidditchStagingZone>(OtherActor);
    if (!StagingZone)
    {
        return;
    }

    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn)
    {
        return;
    }

    // Verify this was our matching staging zone (same team/role check as begin overlap)
    int32 ZoneTeam = StagingZone->TeamHint;
    int32 ZoneRole = StagingZone->RoleHint;

    int32 AgentTeam = 0;
    int32 AgentRole = 0;

    if (CachedGameMode.IsValid())
    {
        EQuidditchTeam MyTeam = CachedGameMode->GetAgentTeam(ControlledPawn);
        EQuidditchRole MyRole = CachedGameMode->GetAgentRole(ControlledPawn);
        AgentTeam = static_cast<int32>(MyTeam);
        AgentRole = static_cast<int32>(MyRole);
    }

    if (ZoneTeam != AgentTeam || ZoneRole != AgentRole)
    {
        return;
    }

    // Agent left their staging zone - reset arrival state
    bNotifiedStagingZoneArrival = false;

    // Update blackboard
    if (UBlackboardComponent* BB = GetBlackboardComponent())
    {
        BB->SetValueAsBool(FName("ReachedStagingZone"), false);
        BB->SetValueAsBool(FName("IsReady"), false);
    }

    // Notify GameMode so it can decrement ready count
    if (CachedGameMode.IsValid())
    {
        CachedGameMode->HandleAgentLeftStagingZone(ControlledPawn);
    }

    SLOG_EVENT(this, "AI.Staging", "AgentLeftStagingZone",
        Metadata.Add(TEXT("zone_name"), StagingZone->GetName());
        Metadata.Add(TEXT("zone_identifier"), StagingZone->GetZoneIdentifier().ToString());
    );

    UE_LOG(LogQuidditchAI, Display,
        TEXT("[%s] LEFT staging zone '%s' | Notified GameMode"),
        *GetName(), *StagingZone->GetName());
}
