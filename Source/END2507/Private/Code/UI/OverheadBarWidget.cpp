// OverheadBarWidget - Widget that displays health/status above characters
// Author: Marcus Daley
// Date: 1/26/2026

#include "Code/UI/OverheadBarWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UOverheadBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Initialize bar color
	if (HealthBar)
	{
		HealthBar->SetFillColorAndOpacity(DefaultBarColor);
		HealthBar->SetPercent(1.0f);
	}
}

void UOverheadBarWidget::SetHealthPercent(float Percent)
{
	if (HealthBar)
	{
		HealthBar->SetPercent(FMath::Clamp(Percent, 0.0f, 1.0f));
	}
}

void UOverheadBarWidget::SetDisplayName(const FText& Name)
{
	if (NameText)
	{
		NameText->SetText(Name);
	}
}

void UOverheadBarWidget::SetBarColor(FLinearColor Color)
{
	if (HealthBar)
	{
		HealthBar->SetFillColorAndOpacity(Color);
	}
}
