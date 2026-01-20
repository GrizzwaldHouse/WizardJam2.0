
// Main menu UI with Play Game and Quit buttons
// // Author: Marcus Daley 
// Date: 10/13/2025
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

class UCodeGameInstance;
class UButtonWidgetComponent;
/**
 *  * Main menu screen displayed by MenuPlayerController.
 * Designer contains: Two ButtonWidgetComponent instances.
 * C++ handles: Binding button clicks, triggering level transitions.
 * 
 */
UCLASS()
class END2507_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	// Called after widget is fully constructed
	// Binds button click delegates to handler functions
	virtual void NativeConstruct() override;

private:
	// Play Game button reference 
	// Clicking this loads the first gameplay level
	UPROPERTY(meta = (BindWidget))
	UButtonWidgetComponent* PlayGame_Button;
	// Quit Game button reference
	UPROPERTY(meta = (BindWidget))
	UButtonWidgetComponent* QuitGame_Button;

	// Loads the first gameplay level via GameInstance
	UFUNCTION()
	void OnPlayGameClicked();

	// Exits the application
	UFUNCTION()
	void OnQuitGameClicked();
};
