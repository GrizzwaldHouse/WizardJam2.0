// OverheadBarWidget.cpp
// Developer: Marcus Daley
// Date: January 25, 2026

#include "Code/UI/OverheadBarWidget.h"
#include "Components/ProgressBar.h"


void UOverheadBarWidget::UpdateHealth(float HealthRatio)
{
	CurrentHealthRatio = HealthRatio;

	if (HealthProgressBar)
	{
		HealthProgressBar->SetPercent(HealthRatio);

		// Apply color gradient based on health ratio
		FLinearColor BarColor = HealthColorHigh;
		if (HealthRatio <= 0.3f)
		{
			BarColor = HealthColorLow;
		}
		else if (HealthRatio <= 0.6f)
		{
			BarColor = HealthColorMedium;
		}

		HealthProgressBar->SetFillColorAndOpacity(BarColor);
	}

	// Check if widget should be visible
	CheckAndUpdateVisibility();
}

void UOverheadBarWidget::UpdateStamina(float CurrentStamina, float MaxStamina)
{
	if (MaxStamina <= 0.0f)
	{
		return;
	}

	float StaminaRatio = CurrentStamina / MaxStamina;
	CurrentStaminaRatio = StaminaRatio;

	if (StaminaProgressBar)
	{
		StaminaProgressBar->SetPercent(StaminaRatio);

		// Apply color gradient based on stamina ratio
		FLinearColor BarColor = (StaminaRatio > 0.5f) ? StaminaColorHigh : StaminaColorLow;
		StaminaProgressBar->SetFillColorAndOpacity(BarColor);
	}

	// Check if widget should be visible
	CheckAndUpdateVisibility();
}

void UOverheadBarWidget::CheckAndUpdateVisibility()
{
	bool bShouldShow = ShouldBeVisible();

	// Only change visibility if state changed (avoid redundant calls)
	if (bShouldShow && GetVisibility() != ESlateVisibility::Visible)
	{
		SetVisibility(ESlateVisibility::Visible);
	}
	else if (!bShouldShow && GetVisibility() == ESlateVisibility::Visible)
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

bool UOverheadBarWidget::ShouldBeVisible() const
{
	// Show if either health OR stamina is not full
	bool bHealthFull = CurrentHealthRatio >= FullHealthThreshold;
	bool bStaminaFull = CurrentStaminaRatio >= FullStaminaThreshold;

	return !(bHealthFull && bStaminaFull);
}
