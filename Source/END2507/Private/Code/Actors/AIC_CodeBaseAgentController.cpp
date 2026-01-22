// AIC_CodeBaseAgentController.cpp
// AI Controller implementation - Observer pattern + IGenericTeamAgentInterface
//
// Developer: Marcus Daley
// Date: January 20, 2026 (Updated from 9/26/2025)
// Project: WizardJam / GAR (END2507)
//
// Architecture:
// - Sight perception detects enemies based on team affiliation
// - Hearing perception receives WorldSignalEmitter broadcasts via stimulus tags
// - Delegate binding receives HealthComponent notifications (health change, death)
// - All Blackboard keys defined as FName constants for maintainability
//
// Designer Configuration (EditDefaultsOnly):
// - SightRadius, LoseSightRadius, PeripheralVisionAngle, AutoSuccessRange
// - HearingRange
// - BehaviorTreeAsset

#include "Code/Actors/AIC_CodeBaseAgentController.h"
// Engine includes
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionTypes.h"
#include "GenericTeamAgentInterface.h"

// Project includes
#include "Code/IEnemyInterface.h"
#include "Code/AC_HealthComponent.h"
#include "Code/Actors/BaseAgent.h"
#include "Code/Actors/Spawner.h"
 #include "Code/Actors/WorldSignalEmitter.h"
#include "Code/Utilities/SignalTypes.h"
 #include "Code/Flight/AC_BroomComponent.h"
#include "../END2507.h"

DEFINE_LOG_CATEGORY(LogAgentController);

AAIC_CodeBaseAgentController::AAIC_CodeBaseAgentController()
    : SightRadius(900.0f)
    , LoseSightRadius(1100.0f)
    , PeripheralVisionAngle(90.0f)
    , AutoSuccessRange(400.0f)
    , HearingRange(5000.0f)
    , SightConfig(nullptr)
    , HearingConfig(nullptr)
    , PlayerKeyName(TEXT("Player"))
    , HasTargetKeyName(TEXT("bHasTarget"))
    , HealthRatioKeyName(TEXT("HealthRatio"))
    , CanAttackKeyName(TEXT("bCanAttack"))
    , ShouldFleeKeyName(TEXT("bShouldFlee"))
    , ShouldFlyKeyName(TEXT("bShouldFly"))
    , DistractionLocationKeyName(TEXT("DistractionLocation"))
    , HeardDistractionKeyName(TEXT("bHeardDistraction"))
    , MatchStartedKeyName(TEXT("bMatchStarted"))
    , NewWaveStartedKeyName(TEXT("bNewWaveStarted"))
{
    PrimaryActorTick.bCanEverTick = true;
    // Create perception component
    AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
    BehaviorTreeAsset = nullptr;
    // Set default team via parent class method
    SetGenericTeamId(FGenericTeamId(1));
    UE_LOG(LogAgentController, Log, TEXT("SightConfig created in constructor"));
}
void AAIC_CodeBaseAgentController::UpdateFactionFromPawn(int32 FactionID, const FLinearColor& FactionColor)
{
    UE_LOG(LogTemp, Display, TEXT("AgentController: Updating faction - ID=%d"), FactionID);

    // Update Blackboard keys for BehaviorTree
    UBlackboardComponent* BlackboardComp = GetBlackboardComponent();
    if (BlackboardComp)
    {
        BlackboardComp->SetValueAsInt(BB_FactionID, FactionID);

        // Convert color to vector for Blackboard storage
        FVector ColorVector = FVector(FactionColor.R, FactionColor.G, FactionColor.B);
        BlackboardComp->SetValueAsVector(BB_FactionColor, ColorVector);

        UE_LOG(LogTemp, Display, TEXT("AgentController: Updated Blackboard with faction data"));
    }

    // Update team for perception
    SetGenericTeamId(FGenericTeamId(static_cast<uint8>(FactionID)));
}
void AAIC_CodeBaseAgentController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
    // Call parent class implementation to store team ID
    Super::SetGenericTeamId(NewTeamID);

    UE_LOG(LogAgentController, Display, TEXT("[%s] SetGenericTeamId: Team=%d"),
        *GetName(), NewTeamID.GetId());

    //Update perception system when team changes
	// This ensures AI perception filters work correctly with new team
    if (AIPerception)
    {
        // RequestStimuliListenerUpdate tells perception system to re-evaluate
        // all stimuli using the new team ID via IGenericTeamAgentInterface
        AIPerception->RequestStimuliListenerUpdate();

        UE_LOG(LogAgentController, Log, TEXT("[%s] Perception updated"), *GetName());

    }
    else
    {
        UE_LOG(LogAgentController, Warning, TEXT("[%s] No AIPerception component for team update"),
            *GetName());
    }
}

FGenericTeamId AAIC_CodeBaseAgentController::GetGenericTeamId() const
{
    return Super::GetGenericTeamId();
}


void AAIC_CodeBaseAgentController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (!InPawn)
    {
        UE_LOG(LogAgentController, Error, TEXT("OnPossess called with null pawn"));
        return;
    }

    UE_LOG(LogAgentController, Log, TEXT("[%s] Possessed: %s"), *GetName(), *InPawn->GetName());

    // Bind to pawn's health/death delegates
    BindPawnDelegates(InPawn);

    // ========================================================================
    // FACTION INITIALIZATION - CRITICAL FOR TEAM ID
    // This runs AFTER BeginPlay, so controller is now valid on the pawn.
    // Check if pawn implements ICodeFactionInterface and apply its faction.
    // This fixes the "Team ID 255" issue where faction was set before possession.
    // ========================================================================
    if (ABaseAgent* Agent = Cast<ABaseAgent>(InPawn))
    {
        // Get the agent's configured faction (set in Blueprint or level)
        int32 FactionID = Agent->GetPlacedFactionID();
        FLinearColor FactionColor = Agent->GetPlacedFactionColor();

        UE_LOG(LogAgentController, Display,
            TEXT("[%s] Initializing faction from pawn: ID=%d, Color=%s"),
            *GetName(), FactionID, *FactionColor.ToString());

        // Set our team ID first (so GetGenericTeamId works)
        SetGenericTeamId(FGenericTeamId(static_cast<uint8>(FactionID)));

        // Now call the pawn's faction assignment (which will also update blackboard)
        Agent->OnFactionAssigned_Implementation(FactionID, FactionColor);

        UE_LOG(LogAgentController, Display,
            TEXT("[%s] Faction initialized: Team ID = %d"),
            *GetName(), GetGenericTeamId().GetId());
    }

    // Validate behavior tree asset
    if (!BehaviorTreeAsset)
    {
        UE_LOG(LogAgentController, Error, TEXT("No BehaviorTree asset set on controller"));
        return;
    }

    RunBehaviorTree(BehaviorTreeAsset);

    UE_LOG(LogAgentController, Display, TEXT("[%s] Behavior tree started, Team ID: %d"),
        *GetName(), GetGenericTeamId().GetId());
}

void AAIC_CodeBaseAgentController::HandleHealthChanged(float HealthRatio)
{
    UE_LOG(LogAgentController, Log, TEXT("[%s] Health changed: %.2f"), *GetName(), HealthRatio);
    UpdateBlackboardHealth(HealthRatio);
}

void AAIC_CodeBaseAgentController::UpdateBlackboardHealth(float HealthPercent)
{
    UBlackboardComponent* BB = GetBlackboardComponent();
    if (!BB)
    {
        return;
    }

    BB->SetValueAsFloat(HealthRatioKeyName, HealthPercent);

    // Update combat state flags based on health
    BB->SetValueAsBool(CanAttackKeyName, HealthPercent > 0.0f);
    BB->SetValueAsBool(ShouldFleeKeyName, HealthPercent < 0.25f);
}

void AAIC_CodeBaseAgentController::HandlePawnDeath(AActor* DestroyedActor)
{
    UE_LOG(LogAgentController, Display, TEXT("[%s] Pawn died: %s"),
        *GetName(), DestroyedActor ? *DestroyedActor->GetName() : TEXT("None"));

    // Stop behavior tree
    UBrainComponent* BrainComp = GetBrainComponent();
    if (BrainComp)
    {
        BrainComp->StopLogic(TEXT("Pawn Died"));
    }

    // Clear target from blackboard
    ForgetPlayer();
}
void AAIC_CodeBaseAgentController::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (!Actor)
    {
        return;
    }

    UBlackboardComponent* BB = GetBlackboardComponent();
    if (!BB)
    {
        return;
    }
  

    // If stimulus is active (successfully sensed)
    if (Stimulus.WasSuccessfullySensed())
    {
        UE_LOG(LogAgentController, Warning, TEXT("HOSTILE TARGET DETECTED: %s | BB Keys Set"), *Actor->GetName());

        // Set Player key in Blackboard
        BB->SetValueAsObject(PlayerKeyName, Actor);
        BB->SetValueAsBool(TEXT("bHasTarget"), true);
    }
    else // Lost sight of target
    {
        UE_LOG(LogAgentController, Log, TEXT("Lost sight of: %s"), *Actor->GetName());
        ForgetPlayer();
    }
}

void AAIC_CodeBaseAgentController::HandleSightPerception(AActor* Actor, const FAIStimulus& Stimulus)
{
    UBlackboardComponent* BB = GetBlackboardComponent();
    if (!BB)
    {
        return;
    }

    // If stimulus is active (successfully sensed)
    if (Stimulus.WasSuccessfullySensed())
    {
        UE_LOG(LogAgentController, Warning, TEXT("HOSTILE TARGET DETECTED: %s | BB Keys Set"), *Actor->GetName());

        // Set target in Blackboard using FName constants
        BB->SetValueAsObject(PlayerKeyName, Actor);
        BB->SetValueAsBool(HasTargetKeyName, true);
    }
    else // Lost sight of target
    {
        UE_LOG(LogAgentController, Log, TEXT("Lost sight of: %s"), *Actor->GetName());
        ForgetPlayer();
    }
}

void AAIC_CodeBaseAgentController::HandleHearingPerception(AActor* Actor, const FAIStimulus& Stimulus)
{
    FName SignalTag = Stimulus.Tag;

    UBlackboardComponent* BB = GetBlackboardComponent();
    if (!BB)
    {
        return;
    }

    // Match Start Signal - AI should begin flying/fighting
    if (SignalTag == TEXT("Signal.Arena.MatchStart"))
    {
        UE_LOG(LogAgentController, Display, TEXT("[%s] HEARD: Match Start Signal"), *GetName());
        BB->SetValueAsBool(ShouldFlyKeyName, true);
        BB->SetValueAsBool(MatchStartedKeyName, true);
    }
    // Wave Start Signal - New wave of enemies incoming
    else if (SignalTag == TEXT("Signal.Arena.WaveStart"))
    {
        UE_LOG(LogAgentController, Display, TEXT("[%s] HEARD: Wave Start Signal"), *GetName());
        BB->SetValueAsBool(NewWaveStartedKeyName, true);
    }
    // Dog Bark Distraction - AI should investigate location
    else if (SignalTag == TEXT("Signal.Bark"))
    {
        UE_LOG(LogAgentController, Display, TEXT("[%s] HEARD: Dog Bark at %s"),
            *GetName(), *Stimulus.StimulusLocation.ToString());
        BB->SetValueAsVector(DistractionLocationKeyName, Stimulus.StimulusLocation);
        BB->SetValueAsBool(HeardDistractionKeyName, true);
    }
    // Unknown signal - log it for debugging
    else if (!SignalTag.IsNone())
    {
        UE_LOG(LogAgentController, Log, TEXT("[%s] HEARD: Unknown signal tag '%s' from %s"),
            *GetName(), *SignalTag.ToString(), *Actor->GetName());
    }
}

void AAIC_CodeBaseAgentController::ForgetPlayer()
{
    UBlackboardComponent* BB = GetBlackboardComponent();
    if (BB)
    {
        BB->ClearValue(PlayerKeyName);
        BB->SetValueAsBool(TEXT("bHasTarget"), false);
        UE_LOG(LogAgentController, Log, TEXT("Player forgotten - Blackboard cleared"));
    }
}

void AAIC_CodeBaseAgentController::BeginPlay()
{
    Super::BeginPlay();
   SetupPerception();
}

void AAIC_CodeBaseAgentController::OnUnPossess()
{ // Unbind delegates before losing pawn reference
    UnbindPawnDelegates();

    Super::OnUnPossess();
}

void AAIC_CodeBaseAgentController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}


FVector AAIC_CodeBaseAgentController::ValidateColorForBlackboard(const FLinearColor& InColor) const
{
    // Normalize the color values to ensure they're in valid range [0,1]
    float R = FMath::Clamp(InColor.R, 0.0f, 1.0f);
    float G = FMath::Clamp(InColor.G, 0.0f, 1.0f);
    float B = FMath::Clamp(InColor.B, 0.0f, 1.0f);

    // Convert to FVector for Blackboard storage
    FVector ColorVector(R, G, B);

    // Log if color needed clamping (designer might have invalid values)
    if (R != InColor.R || G != InColor.G || B != InColor.B)
    {
        UE_LOG(LogAgentController, Warning, TEXT("Color values were clamped to [0,1] range"));
    }

    return ColorVector;
}
void AAIC_CodeBaseAgentController::SetupPerception()
{
    if (!AIPerception)
    {
        UE_LOG(LogAgentController, Error, TEXT("AIPerception component is null"));
        return;
    }

    SightConfig = NewObject<UAISenseConfig_Sight>(this);
    if (!SightConfig)
    {
        UE_LOG(LogAgentController, Error, TEXT("Failed to create SightConfig!"));
        return;
    }
    
        SightConfig->SightRadius = 900.0f;
        SightConfig->LoseSightRadius = 1100.0f;
        SightConfig->PeripheralVisionAngleDegrees = 90.0f; 
        SightConfig->DetectionByAffiliation.bDetectEnemies = true;
        SightConfig->DetectionByAffiliation.bDetectNeutrals = false;
        SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
        SightConfig->AutoSuccessRangeFromLastSeenLocation = 400.0f;
        AIPerception->ConfigureSense(*SightConfig);
        AIPerception->SetDominantSense(SightConfig->GetSenseImplementation());

        AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &AAIC_CodeBaseAgentController::OnPerceptionUpdated);
        SightConfig->SetMaxAge(0.1f);
        UE_LOG(LogAgentController, Log, TEXT("SightConfig created in constructor"));
    
}

void AAIC_CodeBaseAgentController::BindPawnDelegates(APawn* InPawn)
{
    if (!InPawn)
    {
        return;
    }

    UE_LOG(LogAgentController, Log, TEXT("[%s] Binding to pawn delegates..."), *GetName());

    // Find HealthComponent on the pawn
    UAC_HealthComponent* HealthComp = InPawn->FindComponentByClass<UAC_HealthComponent>();
    if (HealthComp)
    {
        // Bind to OnHealthChanged - signature: (float HealthRatio)
        HealthComp->OnHealthChanged.AddDynamic(this, &AAIC_CodeBaseAgentController::HandleHealthChanged);
        UE_LOG(LogAgentController, Log, TEXT("[%s] Bound to OnHealthChanged"), *GetName());

        // Bind to OnDeath - signature: (AActor* DestroyedActor)
        HealthComp->OnDeath.AddDynamic(this, &AAIC_CodeBaseAgentController::HandlePawnDeath);
        UE_LOG(LogAgentController, Log, TEXT("[%s] Bound to OnDeath"), *GetName());
    }
    else
    {
        UE_LOG(LogAgentController, Warning, TEXT("[%s] Pawn has no HealthComponent - cannot bind delegates"), *GetName());
    }

    UE_LOG(LogAgentController, Display, TEXT("[%s] Delegate binding complete"), *GetName());
}

void AAIC_CodeBaseAgentController::UnbindPawnDelegates()
{
    APawn* CurrentPawn = GetPawn();
    if (!CurrentPawn)
    {
        return;
    }

    UE_LOG(LogAgentController, Log, TEXT("[%s] Unbinding pawn delegates..."), *GetName());

    // Find HealthComponent and unbind
    UAC_HealthComponent* HealthComp = CurrentPawn->FindComponentByClass<UAC_HealthComponent>();
    if (HealthComp)
    {
        HealthComp->OnHealthChanged.RemoveDynamic(this, &AAIC_CodeBaseAgentController::HandleHealthChanged);
        HealthComp->OnDeath.RemoveDynamic(this, &AAIC_CodeBaseAgentController::HandlePawnDeath);

        UE_LOG(LogAgentController, Log, TEXT("[%s] Unbound from HealthComponent delegates"), *GetName());
    }

    UE_LOG(LogAgentController, Log, TEXT("[%s] Delegate unbinding complete"), *GetName());
}

