// AC_OverheadBarComponent.cpp
// Developer: Marcus Daley
// Date: January 25, 2026

#include "Code/UI/AC_OverheadBarComponent.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY(LogOverheadBar);

UAC_OverheadBarComponent::UAC_OverheadBarComponent()
	: OverheadSocketName(TEXT("OverheadHUD"))
	, OverheadBarHeight(120.0f)
	, OverheadBarWidth(200.0f)
	, OverheadBarDrawHeight(60.0f)
	, WidgetComp(nullptr)
	, OverheadWidget(nullptr)
	, HealthComp(nullptr)
	, StaminaComp(nullptr)
{
	// NO TICK - delegate-driven updates only
	PrimaryComponentTick.bCanEverTick = false;
}

void UAC_OverheadBarComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		UE_LOG(LogOverheadBar, Error, TEXT("No owner actor!"));
		return;
	}

	// Create and configure UWidgetComponent
	CreateWidgetComponent();

	// Find and cache health/stamina components
	HealthComp = Owner->FindComponentByClass<UAC_HealthComponent>();
	StaminaComp = Owner->FindComponentByClass<UAC_StaminaComponent>();

	if (!HealthComp || !StaminaComp)
	{
		UE_LOG(LogOverheadBar, Error,
			TEXT("[%s] Missing health or stamina component!"), *Owner->GetName());
		return;
	}

	// Bind to existing delegates (Observer pattern - no polling)
	HealthComp->OnHealthChanged.AddDynamic(this, &UAC_OverheadBarComponent::HandleHealthChanged);
	StaminaComp->OnStaminaChanged.AddDynamic(this, &UAC_OverheadBarComponent::HandleStaminaChanged);

	// Initialize widget with current values
	float HealthRatio = HealthComp->GetHealthRatio();
	float CurrentStamina = StaminaComp->GetCurrentStamina();

	HandleHealthChanged(HealthRatio);
	HandleStaminaChanged(Owner, CurrentStamina, 0.0f);

	UE_LOG(LogOverheadBar, Display,
		TEXT("[%s] Overhead bar bound to delegates"), *Owner->GetName());
}

void UAC_OverheadBarComponent::CreateWidgetComponent()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Validate widget class is set by designer in Blueprint defaults
	if (!OverheadWidgetClass)
	{
		UE_LOG(LogOverheadBar, Warning,
			TEXT("[%s] OverheadWidgetClass not set! Assign WBP_OverheadBar in Blueprint defaults."),
			*Owner->GetName());
		return;
	}

	// Create UWidgetComponent dynamically
	WidgetComp = NewObject<UWidgetComponent>(Owner, TEXT("OverheadBarWidgetComp"));
	if (!WidgetComp)
	{
		UE_LOG(LogOverheadBar, Error,
			TEXT("[%s] Failed to create UWidgetComponent!"), *Owner->GetName());
		return;
	}

	// Register component
	WidgetComp->RegisterComponent();

	// Try socket-based attachment first (preferred - artist-controlled placement)
	USkeletalMeshComponent* MeshComp = Owner->FindComponentByClass<USkeletalMeshComponent>();
	bool bAttachedToSocket = false;

	if (MeshComp && !OverheadSocketName.IsNone() && MeshComp->DoesSocketExist(OverheadSocketName))
	{
		// Attach to socket - socket defines exact position
		WidgetComp->AttachToComponent(MeshComp,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			OverheadSocketName);
		bAttachedToSocket = true;

		UE_LOG(LogOverheadBar, Display,
			TEXT("[%s] Attached overhead bar to socket '%s'"),
			*Owner->GetName(), *OverheadSocketName.ToString());
	}
	else
	{
		// Fallback: attach to root with height offset
		WidgetComp->AttachToComponent(Owner->GetRootComponent(),
			FAttachmentTransformRules::KeepRelativeTransform);
		WidgetComp->SetRelativeLocation(FVector(0.0f, 0.0f, OverheadBarHeight));

		if (!OverheadSocketName.IsNone())
		{
			UE_LOG(LogOverheadBar, Warning,
				TEXT("[%s] Socket '%s' not found on mesh - using fallback height %.0f cm"),
				*Owner->GetName(), *OverheadSocketName.ToString(), OverheadBarHeight);
		}
		else
		{
			UE_LOG(LogOverheadBar, Display,
				TEXT("[%s] Using height offset %.0f cm (no socket configured)"),
				*Owner->GetName(), OverheadBarHeight);
		}
	}

	// Configure widget space (Screen = billboarded, always faces camera)
	WidgetComp->SetWidgetSpace(EWidgetSpace::Screen);

	// Set widget class
	WidgetComp->SetWidgetClass(OverheadWidgetClass);

	// Draw size (affects resolution/readability)
	WidgetComp->SetDrawSize(FVector2D(OverheadBarWidth, OverheadBarDrawHeight));

	// Visibility settings
	WidgetComp->SetVisibility(true);

	// Cache widget instance
	OverheadWidget = Cast<UOverheadBarWidget>(WidgetComp->GetUserWidgetObject());
	if (!OverheadWidget)
	{
		UE_LOG(LogOverheadBar, Error,
			TEXT("[%s] Widget is not UOverheadBarWidget type!"), *Owner->GetName());
		return;
	}

	UE_LOG(LogOverheadBar, Display,
		TEXT("[%s] Widget component created at height %.0f"), *Owner->GetName(), OverheadBarHeight);
}

void UAC_OverheadBarComponent::HandleHealthChanged(float HealthRatio)
{
	if (OverheadWidget)
	{
		OverheadWidget->UpdateHealth(HealthRatio);
	}
}

void UAC_OverheadBarComponent::HandleStaminaChanged(AActor* Owner, float NewStamina, float Delta)
{
	if (OverheadWidget && StaminaComp)
	{
		float MaxStamina = StaminaComp->GetMaxStamina();
		OverheadWidget->UpdateStamina(NewStamina, MaxStamina);
	}
}
