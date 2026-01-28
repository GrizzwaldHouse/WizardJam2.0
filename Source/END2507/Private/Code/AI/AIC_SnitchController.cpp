// ============================================================================
// AIC_SnitchController.cpp
// Developer: Marcus Daley
// Date: January 24, 2026
// Project: END2507
// ============================================================================

#include "Code/AI/AIC_SnitchController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "GameFramework/Pawn.h"
#include "StructuredLoggingMacros.h"

DEFINE_LOG_CATEGORY(LogSnitchController);

AAIC_SnitchController::AAIC_SnitchController()
	: SightConfig(nullptr)
	, DetectionRadius(2000.0f)
	, LoseDetectionRadius(2500.0f)
{
	// Create perception component
	AIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	SetPerceptionComponent(*AIPerceptionComp);

	// Snitch is neutral - doesn't belong to any team
	SetGenericTeamId(FGenericTeamId::NoTeam);

	// Initialize default pursuer tags (designer can modify in editor)
	ValidPursuerTags.Add(FName(TEXT("Seeker")));
	ValidPursuerTags.Add(FName(TEXT("Flying")));
	ValidPursuerTags.Add(FName(TEXT("Player")));
}

void AAIC_SnitchController::BeginPlay()
{
	Super::BeginPlay();
	SetupPerception();
}

void AAIC_SnitchController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (InPawn)
	{
		UE_LOG(LogSnitchController, Display, TEXT("[%s] Possessed Snitch: %s"),
			*GetName(), *InPawn->GetName());
	}
}

void AAIC_SnitchController::SetupPerception()
{
	if (!AIPerceptionComp)
	{
		UE_LOG(LogSnitchController, Error, TEXT("AIPerceptionComp is null!"));
		return;
	}

	// Create sight config - 360 degree vision (it's a magic ball!)
	SightConfig = NewObject<UAISenseConfig_Sight>(this);
	SightConfig->SightRadius = DetectionRadius;
	SightConfig->LoseSightRadius = LoseDetectionRadius;
	SightConfig->PeripheralVisionAngleDegrees = 180.0f;  // 360 total vision
	SightConfig->SetMaxAge(0.5f);  // Keep stimuli for half second

	// Detect ALL affiliations - Snitch evades everyone
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

	AIPerceptionComp->ConfigureSense(*SightConfig);
	AIPerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());

	// Bind perception callback - Observer Pattern
	AIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(
		this, &AAIC_SnitchController::HandlePerceptionUpdated);

	// Structured logging - perception configured
	SLOG_EVENT(this, "SnitchAI.Perception", "PerceptionConfigured",
		Metadata.Add(TEXT("detection_radius"), FString::SanitizeFloat(DetectionRadius));
		Metadata.Add(TEXT("lose_detection_radius"), FString::SanitizeFloat(LoseDetectionRadius));
		Metadata.Add(TEXT("max_age"), TEXT("0.5"));
	);

	UE_LOG(LogSnitchController, Display,
		TEXT("[Snitch] Perception setup: DetectionRadius=%.0f, LoseRadius=%.0f"),
		DetectionRadius, LoseDetectionRadius);
}

void AAIC_SnitchController::HandlePerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor)
	{
		return;
	}

	// Check if this actor qualifies as a pursuer
	if (!IsPursuer(Actor))
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		// New pursuer detected
		if (!TrackedPursuers.Contains(Actor))
		{
			TrackedPursuers.Add(Actor);
			OnPursuerDetected.Broadcast(Actor);

			// Structured logging - pursuer detected
			SLOG_EVENT(this, "SnitchAI.Perception", "PursuerDetected",
				Metadata.Add(TEXT("actor_name"), Actor->GetName());
				Metadata.Add(TEXT("total_tracked_pursuers"), FString::FromInt(TrackedPursuers.Num()));
			);

			UE_LOG(LogSnitchController, Display,
				TEXT("[Snitch] Pursuer DETECTED: %s (Total: %d)"),
				*Actor->GetName(), TrackedPursuers.Num());
		}
	}
	else
	{
		// Pursuer lost
		if (TrackedPursuers.Contains(Actor))
		{
			TrackedPursuers.Remove(Actor);
			OnPursuerLost.Broadcast(Actor);

			// Structured logging - pursuer lost
			SLOG_EVENT(this, "SnitchAI.Perception", "PursuerLost",
				Metadata.Add(TEXT("actor_name"), Actor->GetName());
				Metadata.Add(TEXT("total_tracked_pursuers"), FString::FromInt(TrackedPursuers.Num()));
			);

			UE_LOG(LogSnitchController, Display,
				TEXT("[Snitch] Pursuer LOST: %s (Total: %d)"),
				*Actor->GetName(), TrackedPursuers.Num());
		}
	}
}

bool AAIC_SnitchController::IsPursuer(AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	// Must be a pawn
	APawn* PursuerPawn = Cast<APawn>(Actor);
	if (!PursuerPawn)
	{
		// Structured logging - actor filtered out (not a pawn)
		SLOG_VERBOSE(this, "SnitchAI.Perception", "ActorFilteredOut",
			Metadata.Add(TEXT("actor_name"), Actor->GetName());
			Metadata.Add(TEXT("reason"), TEXT("not_pawn"));
		);
		return false;
	}

	// Check if actor has any of the configured pursuer tags
	for (const FName& Tag : ValidPursuerTags)
	{
		if (PursuerPawn->ActorHasTag(Tag))
		{
			return true;
		}
	}

	// Structured logging - actor filtered out (no valid tag)
	SLOG_VERBOSE(this, "SnitchAI.Perception", "ActorFilteredOut",
		Metadata.Add(TEXT("actor_name"), Actor->GetName());
		Metadata.Add(TEXT("actor_class"), Actor->GetClass()->GetName());
		Metadata.Add(TEXT("reason"), TEXT("no_valid_tag"));
	);

	return false;
}
