// AAIC_CodeBaseAgentController.cpp
// AI Controller implementation -  Observer pattern + IGenericTeamAgentInterface
// Author: Marcus Daley

#include "Code/Actors/AIC_CodeBaseAgentController.h"
// Engine includes
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionTypes.h"
#include "GenericTeamAgentInterface.h"

// Project includes
#include "Code/IEnemyInterface.h"
#include "Code/AC_HealthComponent.h"
#include "Code/Actors/BaseAgent.h"
#include "Code/Actors/Spawner.h"
#include "../END2507.h"

DEFINE_LOG_CATEGORY_STATIC(LogAgentController, Log, All);

AAIC_CodeBaseAgentController::AAIC_CodeBaseAgentController()
    : PlayerKeyName(TEXT("Player"))
    , HealthRatioKeyName(TEXT("HealthRatio"))
    , SightConfig(nullptr)
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
    UE_LOG(LogAgentController, Log, TEXT("Possessed: %s"), *InPawn->GetName());


    if (!InPawn)
    {
        UE_LOG(LogAgentController, Error, TEXT("OnPossess called with null pawn"));
        return;
    }

    UE_LOG(LogAgentController, Log, TEXT("Possessed: %s"), *InPawn->GetName());
    AActor* OwnerActor = InPawn->GetOwner();
  
   
    // Validate behavior tree asset
    if (!BehaviorTreeAsset)
    {
        UE_LOG(LogAgentController, Error, TEXT("No BehaviorTree asset set on controller"));
        return;
    }
    RunBehaviorTree(BehaviorTreeAsset);
    UE_LOG(LogAgentController, Display, TEXT("[%s] Possessed pawn [%s] with Team ID: %d"),
        *GetName(), *InPawn->GetName(), GetGenericTeamId().GetId());
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

