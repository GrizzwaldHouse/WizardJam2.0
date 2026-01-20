// PlayerHUD.h
// HUD widget for displaying player health and ammo with color feedback
// Author: Marcus Daley
// Date: 10/08/2025


#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerHUD.generated.h"
class UProgressBar;
class UTextBlock;
class UImage;
class UMaterialInstanceDynamic;

UCLASS()
class END2507_API UPlayerHUD : public UUserWidget
{
	GENERATED_BODY()
	
protected:

	virtual void NativeConstruct() override;

	// Bound widget for health bar display
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UProgressBar* HealthBar;
	// Bound widget for current ammo text display
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTextBlock* CurrentAmmo;
	// Bound widget for max ammo text display
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	class UTextBlock* MaxAmmo;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UImage* Crosshair;
public:
	// Updates ammo display text and color based on current ammo
		// Color changes: Green (>50%) → Orange (20-50%) → Red (<20%)
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateHealthBar(float HealthPercent);
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetAmmo(float Current, float Max);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetReticleColor(const FLinearColor& NewColor);
private:


};
