// ============================================================================
// CelestialBodyComponent.cpp
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================

#include "Visualization/CelestialBodyComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"

UCelestialBodyComponent::UCelestialBodyComponent()
	: BodyType(ECelestialBodyType::Sun)
	, BaseColor(FLinearColor::White)
	, EmissiveStrength(5.0f)
	, BodyScale(100.0f)
	, OrbitRadius(5000.0f)
	, PhaseOffset(0.0f)
	, OrbitSpeedMultiplier(1.0f)
	, CurrentAlpha(1.0f)
	, TargetAlpha(1.0f)
	, FadeSpeed(1.0f)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCelestialBodyComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeVisuals();
}

void UCelestialBodyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Handle alpha fading
	if (CurrentAlpha != TargetAlpha)
	{
		if (CurrentAlpha < TargetAlpha)
		{
			CurrentAlpha = FMath::Min(CurrentAlpha + DeltaTime * FadeSpeed, TargetAlpha);
		}
		else
		{
			CurrentAlpha = FMath::Max(CurrentAlpha - DeltaTime * FadeSpeed, TargetAlpha);
		}
		UpdateMaterial();
	}
}

void UCelestialBodyComponent::UpdatePosition(float TimeOfDay)
{
	FVector NewPosition = CalculateOrbitalPosition(TimeOfDay);
	SetRelativeLocation(NewPosition);

	// Make sun/moon face the center (camera)
	FVector LookDirection = -NewPosition.GetSafeNormal();
	FRotator LookRotation = LookDirection.Rotation();
	SetRelativeRotation(LookRotation);
}

void UCelestialBodyComponent::SetVisibilitySmooth(bool bNewVisibility, float FadeDuration)
{
	TargetAlpha = bNewVisibility ? 1.0f : 0.0f;
	FadeSpeed = FadeDuration > 0.0f ? 1.0f / FadeDuration : 100.0f;
}

void UCelestialBodyComponent::InitializeVisuals()
{
	// Create mesh component if not exists
	if (!MeshComponent)
	{
		MeshComponent = NewObject<UStaticMeshComponent>(GetOwner());
		MeshComponent->SetupAttachment(this);
		MeshComponent->RegisterComponent();

		// Use a simple sphere mesh
		UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
		if (SphereMesh)
		{
			MeshComponent->SetStaticMesh(SphereMesh);
		}

		MeshComponent->SetRelativeScale3D(FVector(BodyScale));
		MeshComponent->SetCastShadow(false);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Create dynamic material
	UMaterialInterface* BaseMaterial = MeshComponent->GetMaterial(0);
	if (BaseMaterial)
	{
		DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		MeshComponent->SetMaterial(0, DynamicMaterial);
	}

	UpdateMaterial();
}

void UCelestialBodyComponent::UpdateMaterial()
{
	if (!DynamicMaterial)
	{
		return;
	}

	FLinearColor EmissiveColor = BaseColor * EmissiveStrength * CurrentAlpha;
	DynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), EmissiveColor);
	DynamicMaterial->SetScalarParameterValue(TEXT("Opacity"), CurrentAlpha);
}

FVector UCelestialBodyComponent::CalculateOrbitalPosition(float TimeOfDay) const
{
	// Apply phase offset and speed multiplier
	float AdjustedTime = TimeOfDay * OrbitSpeedMultiplier + PhaseOffset;
	AdjustedTime = FMath::Fmod(AdjustedTime, 1.0f);

	// Convert to angle (0 = east horizon, 0.5 = zenith from north)
	float Angle = AdjustedTime * PI * 2.0f - PI * 0.5f;

	// Calculate position on orbital arc
	float X = FMath::Cos(Angle) * OrbitRadius;
	float Y = 0.0f;
	float Z = FMath::Sin(Angle) * OrbitRadius;

	return FVector(X, Y, Z);
}
