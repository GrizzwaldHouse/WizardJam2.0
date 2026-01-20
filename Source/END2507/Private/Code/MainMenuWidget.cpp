// Fill out your copyright notice in the Description page of Project Settings.


#include "Code/MainMenuWidget.h"
#include "../END2507.h"
#include "Components/Button.h"
#include "Code/ButtonWidgetComponent.h"
#include "Code/CodeGameInstance.h"

DEFINE_LOG_CATEGORY_STATIC(LogMainMenu, Log, All);
void UMainMenuWidget::NativeConstruct()

{

	Super::NativeConstruct();

	// Validate Play Game button exists (auto-bound via BindWidget meta)
	if (!PlayGame_Button)
	{
		UE_LOG(LogMainMenu, Error, TEXT(" PlayGameButton is null — Designer widget name must be 'PlayGameButton'!"));
		return;
	}
	else
	{
		// Bind to button's OnClicked delegate
		// When button clicks → OnPlayGameClicked() is called
		PlayGame_Button->OnClickedEvent.AddDynamic(this, &UMainMenuWidget::OnPlayGameClicked);

		// Set button text 
		PlayGame_Button->SetButtonText(FText::FromString(TEXT("Play Game")));

		UE_LOG(LogMainMenu, Log, TEXT("Play Game button bound"));
	}

	// Validate Quit button exists
	if (!QuitGame_Button)
	{
		UE_LOG(LogMainMenu, Error, TEXT(" QuitGameButton is null — Designer widget name must be 'QuitGameButton'!"));
		return;
	}
	else
	{
		// Bind to button's OnClicked delegate
		QuitGame_Button->OnClickedEvent.AddDynamic(this, &UMainMenuWidget::OnQuitGameClicked);

		// Set button text
		QuitGame_Button->SetButtonText(FText::FromString(TEXT("Quit")));

		UE_LOG(LogMainMenu, Log, TEXT("Quit button bound"));
	}
}

void UMainMenuWidget::OnPlayGameClicked()
{
	UE_LOG(LogMainMenu, Log, TEXT("▶️ Play Game button clicked — Loading first level"));

	
	UCodeGameInstance* gameInstance = Cast<UCodeGameInstance>(GetGameInstance());
	if (!gameInstance)
	{
		UE_LOG(LogMainMenu, Error, TEXT("GameInstance is not UCodeGameInstance — Cannot load level!"));
		return;
	}

	// Delegate level loading to GameInstance
	gameInstance->LoadGameLevel();
}

void UMainMenuWidget::OnQuitGameClicked()
{
	UE_LOG(LogMainMenu, Log, TEXT(" Quit button clicked — Closing application"));

	// Get GameInstance
	UCodeGameInstance* gameInstance = Cast<UCodeGameInstance>(GetGameInstance());
	if (!gameInstance)
	{
		UE_LOG(LogMainMenu, Error, TEXT(" GameInstance is not UCodeGameInstance — Cannot quit!"));
		return;
	}

	// Delegate quit to GameInstance
	gameInstance->QuitGame();
}
