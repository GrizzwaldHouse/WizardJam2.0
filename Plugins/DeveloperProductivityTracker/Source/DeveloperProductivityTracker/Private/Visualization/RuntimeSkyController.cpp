// ============================================================================
// RuntimeSkyController.cpp
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================

#include "Visualization/RuntimeSkyController.h"
#include "Visualization/ProductivitySkyActor.h"
#include "Visualization/ProductivitySkyConfig.h"
#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/SkyLight.h"
#include "Components/SkyLightComponent.h"
#include "Atmosphere/AtmosphericFog.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

ARuntimeSkyController::ARuntimeSkyController()
	: bAutoProgressTime(true)
	, TimeSpeed(10.0f)
	, CurrentTimeOfDay(0.25f)
	, bSimulateWellnessChanges(false)
	, SkyActor(nullptr)
	, SunLight(nullptr)
	, SkyLight(nullptr)
	, SkyAtmosphere(nullptr)
	, PreviousTimeOfDay(0.25f)
	, CurrentWellnessTint(FLinearColor::White)
	, WellnessSimulationTimer(0.0f)
	, CurrentWellnessState(0)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ARuntimeSkyController::BeginPlay()
{
	Super::BeginPlay();

	FindLevelReferences();

	// Apply initial config to sky actor
	if (SkyActor && SkyConfig)
	{
		SkyActor->ApplySkyConfig(SkyConfig);
	}

	// Set initial time
	if (SkyConfig)
	{
		CurrentTimeOfDay = SkyConfig->SessionStartTimeOfDay;
	}

	RefreshSkyVisuals();

	UE_LOG(LogTemp, Log, TEXT("RuntimeSkyController initialized - Time: %s, Auto: %s, Speed: %.1fx"),
		*GetTimeDisplayString(),
		bAutoProgressTime ? TEXT("Yes") : TEXT("No"),
		TimeSpeed);
}

void ARuntimeSkyController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bAutoProgressTime && SkyConfig)
	{
		// Calculate time progression
		float CycleDuration = SkyConfig->WorkDayCycleDurationSeconds;
		float ConfigTimeScale = SkyConfig->TimeScaleMultiplier;

		// Apply both config scale and demo speed
		float EffectiveSpeed = ConfigTimeScale * TimeSpeed;
		float TimeIncrement = (DeltaTime * EffectiveSpeed) / CycleDuration;

		CurrentTimeOfDay = FMath::Fmod(CurrentTimeOfDay + TimeIncrement, 1.0f);
	}

	// Check if time changed significantly
	if (FMath::Abs(CurrentTimeOfDay - PreviousTimeOfDay) > 0.0001f)
	{
		RefreshSkyVisuals();
		OnTimeChanged.Broadcast(CurrentTimeOfDay);
		PreviousTimeOfDay = CurrentTimeOfDay;
	}

	// Wellness simulation for demo
	if (bSimulateWellnessChanges)
	{
		WellnessSimulationTimer += DeltaTime;
		if (WellnessSimulationTimer > 5.0f)
		{
			WellnessSimulationTimer = 0.0f;
			CurrentWellnessState = (CurrentWellnessState + 1) % 5;
			SimulateWellnessState(CurrentWellnessState);
		}
	}
}

void ARuntimeSkyController::SetTimeOfDay(float NewTime)
{
	CurrentTimeOfDay = FMath::Fmod(NewTime, 1.0f);
	if (CurrentTimeOfDay < 0.0f)
	{
		CurrentTimeOfDay += 1.0f;
	}

	RefreshSkyVisuals();
	OnTimeChanged.Broadcast(CurrentTimeOfDay);
}

void ARuntimeSkyController::AdvanceTime(float Hours)
{
	SetTimeOfDay(CurrentTimeOfDay + (Hours / 24.0f));
}

FString ARuntimeSkyController::GetTimeDisplayString() const
{
	float Hours24 = CurrentTimeOfDay * 24.0f;
	int32 Hours = FMath::FloorToInt(Hours24);
	int32 Minutes = FMath::FloorToInt(FMath::Fmod(Hours24, 1.0f) * 60.0f);

	bool bIsPM = Hours >= 12;
	int32 Hours12 = Hours % 12;
	if (Hours12 == 0) Hours12 = 12;

	return FString::Printf(TEXT("%d:%02d %s"), Hours12, Minutes, bIsPM ? TEXT("PM") : TEXT("AM"));
}

bool ARuntimeSkyController::IsDaytime() const
{
	if (SkyConfig)
	{
		return SkyConfig->IsSunVisibleAtTime(CurrentTimeOfDay);
	}
	return CurrentTimeOfDay > 0.25f && CurrentTimeOfDay < 0.75f;
}

void ARuntimeSkyController::RefreshSkyVisuals()
{
	// Update ProductivitySkyActor
	if (SkyActor)
	{
		SkyActor->UpdateForTimeOfDay(CurrentTimeOfDay);
		SkyActor->ApplyWellnessTint(CurrentWellnessTint);
	}

	// Update directional light (sun)
	UpdateSunLightRotation(CurrentTimeOfDay);

	// Update sky light
	UpdateSkyLightIntensity(CurrentTimeOfDay);

	// Update atmosphere
	UpdateAtmosphereColors(CurrentTimeOfDay);
}

void ARuntimeSkyController::SetWellnessTint(FLinearColor Tint)
{
	CurrentWellnessTint = Tint;

	if (SkyActor)
	{
		SkyActor->ApplyWellnessTint(Tint);
	}
}

void ARuntimeSkyController::SimulateWellnessState(int32 StateIndex)
{
	if (!SkyConfig)
	{
		return;
	}

	FLinearColor Tint = FLinearColor::White;
	FString StateName = TEXT("Optimal");

	switch (StateIndex)
	{
	case 0: // Optimal
		Tint = FLinearColor::White;
		StateName = TEXT("Optimal");
		break;
	case 1: // Good
		Tint = FLinearColor::White;
		StateName = TEXT("Good");
		break;
	case 2: // Break Approaching
		Tint = SkyConfig->BreakApproachingTint;
		StateName = TEXT("Break Approaching");
		break;
	case 3: // Break Recommended
		Tint = SkyConfig->BreakRecommendedTint;
		StateName = TEXT("Break Recommended");
		break;
	case 4: // Overdue
		Tint = SkyConfig->BreakOverdueTint;
		StateName = TEXT("Overdue");
		break;
	}

	SetWellnessTint(Tint);

	UE_LOG(LogTemp, Log, TEXT("Wellness State: %s"), *StateName);
}

void ARuntimeSkyController::FindLevelReferences()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Find ProductivitySkyActor if not set
	if (!SkyActor)
	{
		for (TActorIterator<AProductivitySkyActor> It(World); It; ++It)
		{
			SkyActor = *It;
			UE_LOG(LogTemp, Log, TEXT("Found ProductivitySkyActor: %s"), *SkyActor->GetName());
			break;
		}
	}

	// Find DirectionalLight if not set
	if (!SunLight)
	{
		for (TActorIterator<ADirectionalLight> It(World); It; ++It)
		{
			SunLight = *It;
			UE_LOG(LogTemp, Log, TEXT("Found DirectionalLight for sun: %s"), *SunLight->GetName());
			break;
		}
	}

	// Find SkyLight if not set
	if (!SkyLight)
	{
		for (TActorIterator<ASkyLight> It(World); It; ++It)
		{
			SkyLight = *It;
			UE_LOG(LogTemp, Log, TEXT("Found SkyLight: %s"), *SkyLight->GetName());
			break;
		}
	}
}

void ARuntimeSkyController::UpdateSunLightRotation(float TimeOfDay)
{
	if (!SunLight)
	{
		return;
	}

	// Convert time of day to sun angle
	// 0.0 = midnight (sun below horizon at -90)
	// 0.25 = sunrise (sun at horizon 0)
	// 0.5 = noon (sun overhead at 90)
	// 0.75 = sunset (sun at horizon 0)

	float SunAngle = (TimeOfDay - 0.25f) * 360.0f;

	// Sun rises in the East (-Y in UE) and sets in the West (+Y)
	FRotator SunRotation = FRotator(SunAngle, -90.0f, 0.0f);
	SunLight->SetActorRotation(SunRotation);

	// Adjust sun intensity based on angle
	if (SunLight->GetLightComponent())
	{
		UDirectionalLightComponent* LightComp = Cast<UDirectionalLightComponent>(SunLight->GetLightComponent());
		if (LightComp)
		{
			float NormalizedHeight = FMath::Sin(FMath::DegreesToRadians(SunAngle));
			float Intensity = FMath::Max(0.0f, NormalizedHeight) * 10.0f;

			// Get sun color from config
			if (SkyConfig)
			{
				FLinearColor SunColor = SkyConfig->GetSunColorAtTime(TimeOfDay);
				LightComp->SetLightColor(SunColor);
				Intensity = SkyConfig->GetSunIntensityAtTime(TimeOfDay);
			}

			LightComp->SetIntensity(Intensity);
		}
	}
}

void ARuntimeSkyController::UpdateSkyLightIntensity(float TimeOfDay)
{
	if (!SkyLight)
	{
		return;
	}

	USkyLightComponent* SkyLightComp = SkyLight->GetLightComponent();
	if (SkyLightComp)
	{
		// Daytime = brighter ambient, nighttime = darker
		float DayFactor = FMath::Clamp((FMath::Sin((TimeOfDay - 0.25f) * PI * 2.0f) + 1.0f) * 0.5f, 0.1f, 1.0f);
		SkyLightComp->SetIntensity(DayFactor * 1.0f);

		// Request recapture if using real-time capture
		if (SkyLightComp->SourceType == ESkyLightSourceType::SLS_CapturedScene)
		{
			SkyLightComp->RecaptureSky();
		}
	}
}

void ARuntimeSkyController::UpdateAtmosphereColors(float TimeOfDay)
{
	// Sky atmosphere color adjustments are done through the ProductivitySkyActor
	// This method is here for future expansion with ASkyAtmosphere actor
}
