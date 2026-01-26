// AC_OverheadBarComponent - Component that creates and manages overhead health bar widget
// Author: Marcus Daley
// Date: 1/26/2026
//
// NOTE: This component uses TSubclassOf<> pattern instead of ConstructorHelpers
// ConstructorHelpers can ONLY be used in constructors - using them in BeginPlay crashes!

#include "Code/UI/AC_OverheadBarComponent.h"
#include "Code/UI/OverheadBarWidget.h"
#include "Code/AC_HealthComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Character.h"

DEFINE_LOG_CATEGORY_STATIC(LogOverheadBar, Log, All);

UAC_OverheadBarComponent::UAC_OverheadBarComponent()
	: HeightOffset(120.0f)
	, DrawSize(FVector2D(150.0f, 20.0f))
	, bHideWhenFull(false)
	, WidgetComp(nullptr)
	, BarWidget(nullptr)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAC_OverheadBarComponent::BeginPlay()
{
	Super::BeginPlay();

	// Validate widget class is set - designer must configure this in Blueprint!
	if (!WidgetClass)
	{
		UE_LOG(LogOverheadBar, Warning,
			TEXT("[%s] WidgetClass not set! Set it in Blueprint defaults for component."),
			*GetOwner()->GetName());
		return;
	}

	CreateWidgetComponent();

	// Bind to health component if owner has one
	if (AActor* Owner = GetOwner())
	{
		if (UAC_HealthComponent* HealthComp = Owner->FindComponentByClass<UAC_HealthComponent>())
		{
			HealthComp->OnHealthChanged.AddDynamic(this, &UAC_OverheadBarComponent::OnHealthChanged);
			UE_LOG(LogOverheadBar, Log, TEXT("[%s] Bound to health component"), *Owner->GetName());
		}
		else
		{
			UE_LOG(LogOverheadBar, Warning,
				TEXT("[%s] No health component found - bar won't update automatically"),
				*Owner->GetName());
		}
	}
}

void UAC_OverheadBarComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Unbind from health component
	if (AActor* Owner = GetOwner())
	{
		if (UAC_HealthComponent* HealthComp = Owner->FindComponentByClass<UAC_HealthComponent>())
		{
			HealthComp->OnHealthChanged.RemoveDynamic(this, &UAC_OverheadBarComponent::OnHealthChanged);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UAC_OverheadBarComponent::CreateWidgetComponent()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		UE_LOG(LogOverheadBar, Error, TEXT("CreateWidgetComponent called with no owner!"));
		return;
	}

	// WidgetClass is set via Blueprint defaults - NO ConstructorHelpers needed!
	// This is the correct pattern for runtime widget creation

	// Create widget component
	WidgetComp = NewObject<UWidgetComponent>(Owner, UWidgetComponent::StaticClass(), TEXT("OverheadBarWidgetComp"));
	if (!WidgetComp)
	{
		UE_LOG(LogOverheadBar, Error, TEXT("[%s] Failed to create widget component!"), *Owner->GetName());
		return;
	}

	// Attach to owner root
	WidgetComp->SetupAttachment(Owner->GetRootComponent());
	WidgetComp->SetRelativeLocation(FVector(0.0f, 0.0f, HeightOffset));
	WidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	WidgetComp->SetDrawSize(DrawSize);
	WidgetComp->SetWidgetClass(WidgetClass);
	WidgetComp->RegisterComponent();

	// Get reference to the created widget
	BarWidget = Cast<UOverheadBarWidget>(WidgetComp->GetWidget());

	if (BarWidget)
	{
		UE_LOG(LogOverheadBar, Log, TEXT("[%s] Overhead bar created successfully"), *Owner->GetName());

		// Hide if full health and configured to do so
		if (bHideWhenFull)
		{
			WidgetComp->SetVisibility(false);
		}
	}
	else
	{
		UE_LOG(LogOverheadBar, Warning,
			TEXT("[%s] Widget created but cast to UOverheadBarWidget failed"),
			*Owner->GetName());
	}
}

void UAC_OverheadBarComponent::OnHealthChanged(float HealthRatio)
{
	if (BarWidget)
	{
		BarWidget->SetHealthPercent(HealthRatio);
	}

	// Show/hide based on health if configured
	if (bHideWhenFull && WidgetComp)
	{
		bool bShouldShow = HealthRatio < 1.0f;
		WidgetComp->SetVisibility(bShouldShow);
	}
}

void UAC_OverheadBarComponent::SetDisplayName(const FText& Name)
{
	if (BarWidget)
	{
		BarWidget->SetDisplayName(Name);
	}
}

void UAC_OverheadBarComponent::SetBarColor(FLinearColor Color)
{
	if (BarWidget)
	{
		BarWidget->SetBarColor(Color);
	}
}
