// OverheadBarWidget - Widget that displays health/status above characters
// Designer creates child Blueprint, code updates values via delegates
// Author: Marcus Daley
// Date: 1/26/2026

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OverheadBarWidget.generated.h"

class UProgressBar;
class UTextBlock;

// Overhead bar widget displayed above character heads
// Shows health bar and optional name text
UCLASS()
class END2507_API UOverheadBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Updates the health bar fill percentage (0.0 - 1.0)
	UFUNCTION(BlueprintCallable, Category = "Overhead Bar")
	void SetHealthPercent(float Percent);

	// Sets the display name shown above the bar
	UFUNCTION(BlueprintCallable, Category = "Overhead Bar")
	void SetDisplayName(const FText& Name);

	// Sets the bar fill color based on team or status
	UFUNCTION(BlueprintCallable, Category = "Overhead Bar")
	void SetBarColor(FLinearColor Color);

protected:
	virtual void NativeConstruct() override;

	// Health progress bar - bind in child Blueprint
	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar* HealthBar;

	// Character name text - bind in child Blueprint
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* NameText;

	// Default bar color
	UPROPERTY(EditDefaultsOnly, Category = "Overhead Bar")
	FLinearColor DefaultBarColor = FLinearColor::Green;
};
