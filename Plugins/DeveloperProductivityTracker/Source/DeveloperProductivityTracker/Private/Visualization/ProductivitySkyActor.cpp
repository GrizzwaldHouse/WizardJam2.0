// ============================================================================
// ProductivitySkyActor.cpp
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================

#include "Visualization/ProductivitySkyActor.h"
#include "Visualization/ProductivitySkyConfig.h"
#include "Visualization/CelestialBodyComponent.h"
#include "Visualization/TimeOfDaySubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Editor.h"

AProductivitySkyActor::AProductivitySkyActor()
	: CurrentTimeOfDay(0.25f)
	, CurrentWellnessTint(FLinearColor::White)
{
	PrimaryActorTick.bCanEverTick = true;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = RootSceneComponent;

	SkyDomeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SkyDome"));
	SkyDomeMesh->SetupAttachment(RootComponent);
	SkyDomeMesh->SetCastShadow(false);
	SkyDomeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AProductivitySkyActor::BeginPlay()
{
	Super::BeginPlay();

	InitializeComponents();
	SubscribeToSubsystems();

	if (SkyConfig)
	{
		ApplySkyConfig(SkyConfig);
	}
}

void AProductivitySkyActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AProductivitySkyActor::ApplySkyConfig(UProductivitySkyConfig* Config)
{
	SkyConfig = Config;

	if (!SkyConfig)
	{
		return;
	}

	// Apply configuration to celestial bodies
	if (SunComponent)
	{
		SunComponent->BaseColor = FLinearColor::White;
		SunComponent->EmissiveStrength = SkyConfig->SunBaseIntensity;
		SunComponent->BodyScale = SkyConfig->SunDiskSize * 50.0f;
		SunComponent->OrbitRadius = SkyConfig->MoonOrbitRadius * 1.2f;
	}

	if (BlueMoonComponent)
	{
		BlueMoonComponent->BaseColor = SkyConfig->BlueMoonColor;
		BlueMoonComponent->EmissiveStrength = SkyConfig->MoonEmissiveStrength;
		BlueMoonComponent->BodyScale = SkyConfig->MoonScale;
		BlueMoonComponent->OrbitRadius = SkyConfig->MoonOrbitRadius;
		BlueMoonComponent->OrbitSpeedMultiplier = SkyConfig->MoonOrbitSpeedMultiplier;
	}

	if (OrangeMoonComponent)
	{
		OrangeMoonComponent->BaseColor = SkyConfig->OrangeMoonColor;
		OrangeMoonComponent->EmissiveStrength = SkyConfig->MoonEmissiveStrength;
		OrangeMoonComponent->BodyScale = SkyConfig->MoonScale * 0.8f;
		OrangeMoonComponent->OrbitRadius = SkyConfig->MoonOrbitRadius * 0.9f;
		OrangeMoonComponent->PhaseOffset = SkyConfig->OrangeMoonPhaseOffset;
		OrangeMoonComponent->OrbitSpeedMultiplier = SkyConfig->MoonOrbitSpeedMultiplier * 1.1f;
	}

	// Update visuals immediately
	UpdateForTimeOfDay(CurrentTimeOfDay);

	UE_LOG(LogProductivitySky, Log, TEXT("Applied sky config: %s"), *Config->GetName());
}

void AProductivitySkyActor::UpdateForTimeOfDay(float TimeOfDay)
{
	CurrentTimeOfDay = TimeOfDay;

	UpdateSkyColors(TimeOfDay);
	UpdateCelestialPositions(TimeOfDay);
	UpdateStarVisibility(TimeOfDay);
}

void AProductivitySkyActor::ApplyWellnessTint(const FLinearColor& Tint)
{
	CurrentWellnessTint = Tint;

	if (SkyMaterial)
	{
		SkyMaterial->SetVectorParameterValue(TEXT("WellnessTint"), Tint);
	}
}

void AProductivitySkyActor::InitializeComponents()
{
	InitializeSkyDome();
	InitializeCelestialBodies();
}

void AProductivitySkyActor::InitializeSkyDome()
{
	// Load or create sky dome mesh
	UStaticMesh* DomeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (DomeMesh && SkyDomeMesh)
	{
		SkyDomeMesh->SetStaticMesh(DomeMesh);
		SkyDomeMesh->SetRelativeScale3D(FVector(10000.0f));

		// Create dynamic material
		UMaterialInterface* BaseMaterial = SkyDomeMesh->GetMaterial(0);
		if (BaseMaterial)
		{
			SkyMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			SkyDomeMesh->SetMaterial(0, SkyMaterial);
		}
	}
}

void AProductivitySkyActor::InitializeCelestialBodies()
{
	// Create sun
	SunComponent = NewObject<UCelestialBodyComponent>(this, TEXT("Sun"));
	SunComponent->SetupAttachment(RootComponent);
	SunComponent->RegisterComponent();
	SunComponent->BodyType = ECelestialBodyType::Sun;
	SunComponent->BaseColor = FLinearColor::White;

	// Create blue moon
	BlueMoonComponent = NewObject<UCelestialBodyComponent>(this, TEXT("BlueMoon"));
	BlueMoonComponent->SetupAttachment(RootComponent);
	BlueMoonComponent->RegisterComponent();
	BlueMoonComponent->BodyType = ECelestialBodyType::BlueMoon;

	// Create orange moon
	OrangeMoonComponent = NewObject<UCelestialBodyComponent>(this, TEXT("OrangeMoon"));
	OrangeMoonComponent->SetupAttachment(RootComponent);
	OrangeMoonComponent->RegisterComponent();
	OrangeMoonComponent->BodyType = ECelestialBodyType::OrangeMoon;
}

void AProductivitySkyActor::SubscribeToSubsystems()
{
	// Only subscribe to editor subsystems when running in editor (not PIE)
	// In PIE/runtime, RuntimeSkyController drives this actor directly
#if WITH_EDITOR
	if (GEditor && !GetWorld()->IsPlayInEditor())
	{
		UTimeOfDaySubsystem* TimeSubsystem = GEditor->GetEditorSubsystem<UTimeOfDaySubsystem>();
		if (TimeSubsystem)
		{
			TimeSubsystem->OnTimeOfDayChanged.AddDynamic(this, &AProductivitySkyActor::HandleTimeOfDayChanged);
			UE_LOG(LogProductivitySky, Log, TEXT("ProductivitySkyActor subscribed to editor TimeOfDaySubsystem"));
		}
	}
	else
	{
		UE_LOG(LogProductivitySky, Log, TEXT("ProductivitySkyActor running in PIE - use RuntimeSkyController to drive visuals"));
	}
#endif
}

void AProductivitySkyActor::UpdateSkyColors(float TimeOfDay)
{
	if (!SkyConfig || !SkyMaterial)
	{
		return;
	}

	FLinearColor SkyColor = SkyConfig->GetSkyColorAtTime(TimeOfDay);
	SkyColor = SkyColor * CurrentWellnessTint;

	SkyMaterial->SetVectorParameterValue(TEXT("SkyColor"), SkyColor);
	SkyMaterial->SetVectorParameterValue(TEXT("SunColor"), SkyConfig->GetSunColorAtTime(TimeOfDay));
	SkyMaterial->SetScalarParameterValue(TEXT("SunIntensity"), SkyConfig->GetSunIntensityAtTime(TimeOfDay));
}

void AProductivitySkyActor::UpdateCelestialPositions(float TimeOfDay)
{
	if (SunComponent)
	{
		SunComponent->UpdatePosition(TimeOfDay);
		bool bSunVisible = SkyConfig ? SkyConfig->IsSunVisibleAtTime(TimeOfDay) : (TimeOfDay > 0.25f && TimeOfDay < 0.75f);
		SunComponent->SetVisibilitySmooth(bSunVisible, 2.0f);
	}

	if (BlueMoonComponent)
	{
		BlueMoonComponent->UpdatePosition(TimeOfDay);
		// Moons visible when sun is not
		bool bMoonVisible = SkyConfig ? !SkyConfig->IsSunVisibleAtTime(TimeOfDay) : (TimeOfDay <= 0.25f || TimeOfDay >= 0.75f);
		BlueMoonComponent->SetVisibilitySmooth(bMoonVisible, 2.0f);
	}

	if (OrangeMoonComponent)
	{
		OrangeMoonComponent->UpdatePosition(TimeOfDay);
		bool bMoonVisible = SkyConfig ? !SkyConfig->IsSunVisibleAtTime(TimeOfDay) : (TimeOfDay <= 0.25f || TimeOfDay >= 0.75f);
		OrangeMoonComponent->SetVisibilitySmooth(bMoonVisible, 2.0f);
	}
}

void AProductivitySkyActor::UpdateStarVisibility(float TimeOfDay)
{
	if (!SkyConfig || !SkyMaterial)
	{
		return;
	}

	float StarAlpha = SkyConfig->GetStarVisibilityAtTime(TimeOfDay);
	SkyMaterial->SetScalarParameterValue(TEXT("StarVisibility"), StarAlpha);
}

void AProductivitySkyActor::HandleTimeOfDayChanged(float NewTimeOfDay)
{
	UpdateForTimeOfDay(NewTimeOfDay);
}
