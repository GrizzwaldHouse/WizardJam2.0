// AC_OverheadBarComponent - Component that creates and manages overhead health bar widget
// Attach to any character to display floating health bar above their head
// Author: Marcus Daley
// Date: 1/26/2026
//
// IMPORTANT: WidgetClass must be set in Blueprint defaults - NO ConstructorHelpers allowed!

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AC_OverheadBarComponent.generated.h"

class UOverheadBarWidget;
class UWidgetComponent;
class UAC_HealthComponent;

// Component that creates and manages an overhead health bar widget
// Automatically binds to owner's health component for updates
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class END2507_API UAC_OverheadBarComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAC_OverheadBarComponent();

	// Sets the display name shown on the overhead bar
	UFUNCTION(BlueprintCallable, Category = "Overhead Bar")
	void SetDisplayName(const FText& Name);

	// Updates the bar color
	UFUNCTION(BlueprintCallable, Category = "Overhead Bar")
	void SetBarColor(FLinearColor Color);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Widget class to spawn - MUST be set in Blueprint defaults!
	// NO ConstructorHelpers - use TSubclassOf property instead
	UPROPERTY(EditDefaultsOnly, Category = "Overhead Bar")
	TSubclassOf<UOverheadBarWidget> WidgetClass;

	// Height offset above character
	UPROPERTY(EditDefaultsOnly, Category = "Overhead Bar")
	float HeightOffset = 120.0f;

	// Widget draw size
	UPROPERTY(EditDefaultsOnly, Category = "Overhead Bar")
	FVector2D DrawSize = FVector2D(150.0f, 20.0f);

	// Whether to hide bar when health is full
	UPROPERTY(EditDefaultsOnly, Category = "Overhead Bar")
	bool bHideWhenFull = false;

private:
	// Creates the widget component and attaches it
	void CreateWidgetComponent();

	// Health changed handler
	UFUNCTION()
	void OnHealthChanged(float HealthRatio);

	// Cached references
	UPROPERTY()
	UWidgetComponent* WidgetComp;

	UPROPERTY()
	UOverheadBarWidget* BarWidget;
};
