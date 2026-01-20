// Win/Lose results screen with state-dependent button visibility
// Author: Marcus Daley 
// Date: 10/13/2025
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ResultsWidget.generated.h"

class UWidgetSwitcher;
class UVerticalBox;
class UButtonWidgetComponent;
/**
 *  * Results screen shown by GameMode when game ends.
 * Designer contains: Widget Switcher + Button Area + Two Buttons.
 * C++ handles: State configuration (win vs lose), button bindings.
 * 
 */
UCLASS()
class END2507_API UResultsWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UResultsWidget(const FObjectInitializer& ObjectInitializer);

	// Configures widget for victory state
// Hides buttons, switches to victory text (index 1)
// Called by GameMode when all enemies are eliminated
	UFUNCTION(BlueprintCallable, Category = "Results")
	void WinConditionMet();

protected:
	// Called after widget is fully constructed
	// Binds button click delegates to handler functions
	virtual void NativeConstruct() override;

private:
	// Widget switcher - toggles between defeat/victory text
	UPROPERTY(meta = (BindWidget))
	UWidgetSwitcher* ResultsSwitch;

	// Container for buttons - hidden on victory, visible on defeat
	UPROPERTY(meta = (BindWidget))
	UVerticalBox* ButtonArea;

	// Play Again button reference
	UPROPERTY(meta = (BindWidget))
	UButtonWidgetComponent* RestartButton;

	// Quit to Main Menu button reference
	UPROPERTY(meta = (BindWidget))
	UButtonWidgetComponent* MenuButton;

	// Called when Play Again button is clicked
// Reloads current level via GameInstance
	UFUNCTION()
	void OnPlayAgainClicked();

	// Called when Main Menu button is clicked
// Returns to main menu via GameInstance
	UFUNCTION()
	void OnMainMenuClicked();
	// Private function called by timer to load menu
	void LoadMainMenuAfterDelay();
	// Automatically returns to menu if player doesn't interact
	void AutoReturnToMenu();
	float TimeToMenu;
	FTimerHandle AutoMenuTimerHandle;
	// Timer handle for auto-return to menu if no button press
	FTimerHandle AutoReturnTimerHandle;

	float TimeToAutoReturn;
};
