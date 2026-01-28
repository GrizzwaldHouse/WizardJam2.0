// AC_OverheadBarComponent.h
// Developer: Marcus Daley
// Date: January 25, 2026
// Purpose: Component that creates and manages floating overhead health/stamina bars for AI agents
// Usage: Automatically created by BaseCharacter for AI-controlled characters.
//        Designer sets OverheadWidgetClass to WBP_OverheadBar in Blueprint defaults.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/WidgetComponent.h"
#include "Code/UI/OverheadBarWidget.h"
#include "Code/AC_HealthComponent.h"
#include "Code/Utilities/AC_StaminaComponent.h"
#include "AC_OverheadBarComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogOverheadBar, Log, All);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class END2507_API UAC_OverheadBarComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAC_OverheadBarComponent();

protected:
	virtual void BeginPlay() override;

public:
	// ========== WIDGET CONFIGURATION ==========

	// Widget class to spawn (set to WBP_OverheadBar in Blueprint)
	UPROPERTY(EditDefaultsOnly, Category = "Widget")
	TSubclassOf<UOverheadBarWidget> OverheadWidgetClass;

	// Socket name to attach widget to (add socket to skeleton, typically on head bone)
	// If socket doesn't exist, falls back to height offset from root
	UPROPERTY(EditDefaultsOnly, Category = "Widget")
	FName OverheadSocketName;

	// Fallback height above character's root when socket not found (in cm)
	UPROPERTY(EditDefaultsOnly, Category = "Widget")
	float OverheadBarHeight;

	// Widget draw width (in pixels)
	UPROPERTY(EditDefaultsOnly, Category = "Widget")
	float OverheadBarWidth;

	// Widget draw height (in pixels)
	UPROPERTY(EditDefaultsOnly, Category = "Widget")
	float OverheadBarDrawHeight;

private:
	// ========== INTERNAL REFERENCES ==========

	// The 3D widget component in the world
	UPROPERTY()
	UWidgetComponent* WidgetComp;

	// The widget instance (cast from WidgetComp)
	UPROPERTY()
	UOverheadBarWidget* OverheadWidget;

	// Cached health component
	UPROPERTY()
	UAC_HealthComponent* HealthComp;

	// Cached stamina component
	UPROPERTY()
	UAC_StaminaComponent* StaminaComp;

	// ========== INITIALIZATION ==========

	// Create and configure the UWidgetComponent
	void CreateWidgetComponent();

	// ========== DELEGATE HANDLERS ==========

	// Called when owner's health changes
	UFUNCTION()
	void HandleHealthChanged(float HealthRatio);

	// Called when owner's stamina changes
	UFUNCTION()
	void HandleStaminaChanged(AActor* Owner, float NewStamina, float Delta);
};
