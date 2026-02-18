// OverheadBarWidget.h
// Developer: Marcus Daley
// Date: January 25, 2026
// Purpose: Floating overhead health/stamina bar widget for AI agents
// Usage: Create Blueprint widget (WBP_OverheadBar) inheriting from this class,
//        add HealthProgressBar and StaminaProgressBar widgets, designer configures colors

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "OverheadBarWidget.generated.h"

UCLASS()
class END2507_API UOverheadBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UOverheadBarWidget(const FObjectInitializer& ObjectInitializer);

	// ========== WIDGET REFERENCES (Designer creates in Blueprint) ==========

	// Health progress bar - must be named "HealthProgressBar" in Blueprint
	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar* HealthProgressBar;

	// Stamina progress bar - must be named "StaminaProgressBar" in Blueprint
	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar* StaminaProgressBar;

	// ========== APPEARANCE CONFIGURATION ==========

	// Health color when >60%
	UPROPERTY(EditDefaultsOnly, Category = "Appearance|Health")
	FLinearColor HealthColorHigh;

	// Health color when 30-60%
	UPROPERTY(EditDefaultsOnly, Category = "Appearance|Health")
	FLinearColor HealthColorMedium;

	// Health color when <30%
	UPROPERTY(EditDefaultsOnly, Category = "Appearance|Health")
	FLinearColor HealthColorLow;

	// Stamina color when >50%
	UPROPERTY(EditDefaultsOnly, Category = "Appearance|Stamina")
	FLinearColor StaminaColorHigh;

	// Stamina color when <50%
	UPROPERTY(EditDefaultsOnly, Category = "Appearance|Stamina")
	FLinearColor StaminaColorLow;

	// ========== VISIBILITY THRESHOLDS ==========

	// Consider health "full" at this ratio (0.99 = 99%)
	UPROPERTY(EditDefaultsOnly, Category = "Visibility")
	float FullHealthThreshold;

	// Consider stamina "full" at this ratio (0.99 = 99%)
	UPROPERTY(EditDefaultsOnly, Category = "Visibility")
	float FullStaminaThreshold;

	// ========== PUBLIC UPDATE FUNCTIONS ==========

	// Update health bar (called by AC_OverheadBarComponent)
	UFUNCTION(BlueprintCallable, Category = "OverheadBar")
	void UpdateHealth(float HealthRatio);

	// Update stamina bar (called by AC_OverheadBarComponent)
	UFUNCTION(BlueprintCallable, Category = "OverheadBar")
	void UpdateStamina(float CurrentStamina, float MaxStamina);

	// Check if widget should be visible (hidden when both bars full)
	void CheckAndUpdateVisibility();
	bool ShouldBeVisible() const;

private:
	// Cached health ratio for visibility checks
	float CurrentHealthRatio;

	// Cached stamina ratio for visibility checks
	float CurrentStaminaRatio;
};
