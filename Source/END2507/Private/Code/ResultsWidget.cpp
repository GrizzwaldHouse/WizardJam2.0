// Author: Marcus Daley
// Date: 10/13/2025
#include "Code/ResultsWidget.h"
#include "Components/WidgetSwitcher.h"
#include "Components/VerticalBox.h"
#include "Code/ButtonWidgetComponent.h"
#include "Code/CodeGameInstance.h"
#include "../END2507.h"
DEFINE_LOG_CATEGORY_STATIC(LogResultsWidget, Log, All);

UResultsWidget::UResultsWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, TimeToMenu(3.0f)
	, TimeToAutoReturn(5.0f)
{

}
void UResultsWidget::WinConditionMet()
{
	UE_LOG(LogResultsWidget, Log, TEXT(" VICTORY! Configuring widget for win state"));

	// Hide buttons (player doesn't need to interact - auto-return handles menu load)
	if (ButtonArea)
	{
		ButtonArea->SetVisibility(ESlateVisibility::Hidden);
		UE_LOG(LogResultsWidget, Log, TEXT(" ButtonArea hidden for victory state"));
	}
	else
	{
		UE_LOG(LogResultsWidget, Warning, TEXT(" ButtonArea is null — Cannot hide buttons!"));
	}

	// Switch to victory text (index 1 = "You Win!")
	if (ResultsSwitch)
	{
		ResultsSwitch->SetActiveWidgetIndex(1);
		UE_LOG(LogResultsWidget, Log, TEXT(" Switcher changed to victory text (index 1)"));
	}
	else
	{
		UE_LOG(LogResultsWidget, Warning, TEXT(" ResultsSwitcher is null — Cannot show victory text!"));
	}

	// Directly call AutoReturnToMenu after delay
	
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			AutoReturnTimerHandle,
			this,
			&UResultsWidget::AutoReturnToMenu,
			TimeToAutoReturn,
			false
		);

		UE_LOG(LogResultsWidget, Log, TEXT(" Auto-return timer started (%.2f seconds) — will load main menu"), TimeToAutoReturn);
	}
	else
	{
		UE_LOG(LogResultsWidget, Error, TEXT("Cannot start timer — World is null"));
	}
}

void UResultsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UE_LOG(LogResultsWidget, Log, TEXT("Results widget constructed — Binding button delegates"));


	// Validate ResultsSwitcher exists 
	if (!ResultsSwitch)
	{
		UE_LOG(LogResultsWidget, Error, TEXT("ResultsSwitcher is null — Designer widget name must be 'ResultsSwitcher'!"));
	}
	else
	{
		// Default to defeat state (index 0 = "You Lose!")
		ResultsSwitch->SetActiveWidgetIndex(0);
		UE_LOG(LogResultsWidget, Log, TEXT("Switcher initialized to defeat state (index 0)"));
	}

	// Validate ButtonArea exists
	if (!ButtonArea)
	{
		UE_LOG(LogResultsWidget, Error, TEXT(" ButtonArea is null — Designer widget name must be 'ButtonArea'!"));
		return;
	}
	else
	{
		// Default to visible (defeat state shows buttons)
		ButtonArea->SetVisibility(ESlateVisibility::Visible);
		UE_LOG(LogResultsWidget, Log, TEXT("ButtonArea set to visible (defeat default)"));
	}

	// Validate Play Again button exists
	if (!RestartButton)
	{
		UE_LOG(LogResultsWidget, Error, TEXT("PlayAgainButton is null — Designer widget name must be 'PlayAgainButton'!"));
		return;
	}
	else
	{
		// Bind to button's OnClicked delegate
		RestartButton->OnClickedEvent.AddDynamic(this, &UResultsWidget::OnPlayAgainClicked);
		RestartButton->SetButtonText(FText::FromString(TEXT("Play Again")));
		UE_LOG(LogResultsWidget, Log, TEXT(" Play Again button bound"));
	}

	// Validate Main Menu button exists
	if (!MenuButton)
	{
		UE_LOG(LogResultsWidget, Error, TEXT("MainMenuButton is null — Designer widget name must be 'MainMenuButton'!"));
	}
	else
	{
		// Bind to button's OnClicked delegate
		MenuButton->OnClickedEvent.AddDynamic(this, &UResultsWidget::OnMainMenuClicked);
		MenuButton->SetButtonText(FText::FromString(TEXT("Main Menu")));
		UE_LOG(LogResultsWidget, Log, TEXT("Main Menu button bound"));
	}
	this->SetIsFocusable(true);
}

void UResultsWidget::OnPlayAgainClicked()
{
	UE_LOG(LogResultsWidget, Log, TEXT("Play Again button clicked — Reloading level"));
	UWorld* World = GetWorld();
	if (World && AutoReturnTimerHandle.IsValid())
	{
		World->GetTimerManager().ClearTimer(AutoReturnTimerHandle);
		UE_LOG(LogResultsWidget, Log, TEXT("Auto-return timer cancelled"));
	}

	// Get GameInstance 
	UCodeGameInstance* gameInstance = Cast<UCodeGameInstance>(GetGameInstance());
	if (!gameInstance)
	{
		UE_LOG(LogResultsWidget, Error, TEXT(" GameInstance is not UGameInstanceWeek3 — Cannot reload level!"));
		return;
	}

	// Reload current level
	gameInstance->LoadCurrentLevelSafe();
}

void UResultsWidget::OnMainMenuClicked()
{
	UE_LOG(LogResultsWidget, Log, TEXT("Main Menu button clicked — Returning to menu"));
	UWorld* World = GetWorld();
	if (World && AutoReturnTimerHandle.IsValid())
	{
		World->GetTimerManager().ClearTimer(AutoReturnTimerHandle);
		UE_LOG(LogResultsWidget, Log, TEXT("Auto-return timer cancelled"));
	}

	// Get GameInstance
	UCodeGameInstance* gameInstance = Cast<UCodeGameInstance>(GetGameInstance());
	if (!gameInstance)
	{
		UE_LOG(LogResultsWidget, Error, TEXT("GameInstance is not UCodeGameInstance — Cannot load menu!"));
		return;
	}

	// Return to main menu
	gameInstance->LoadMainMenu();
}

void UResultsWidget::LoadMainMenuAfterDelay()
{
	UE_LOG(LogResultsWidget, Log, TEXT(" Timer expired — Showing buttons for player choice"));

	// Make buttons visible after delay
	if (ButtonArea)
	{
		ButtonArea->SetVisibility(ESlateVisibility::Visible);
		UE_LOG(LogResultsWidget, Log, TEXT(" ButtonArea now visible — Player can choose action"));
	}
	else
	{
		UE_LOG(LogResultsWidget, Error, TEXT("ButtonArea is null — Cannot show buttons after timer"));
		return;
	}
	//  Set focus to the first button for controller/keyboard navigation
	if (RestartButton)
	{
		RestartButton->SetKeyboardFocus();
	}
	// Start auto-return timer if player doesn't interact
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(
			AutoReturnTimerHandle,
			this,
			&UResultsWidget::AutoReturnToMenu,
			TimeToAutoReturn,
			false
		);

		UE_LOG(LogResultsWidget, Log, TEXT("Auto-return timer started (%.2f seconds)"), TimeToAutoReturn);
	}
}
void UResultsWidget::AutoReturnToMenu()
{
	UE_LOG(LogResultsWidget, Log, TEXT("Auto-return triggered — Player did not select option"));

	// Get GameInstance
	UCodeGameInstance* GameInstance = Cast<UCodeGameInstance>(GetGameInstance());
	if (!GameInstance)
	{
		UE_LOG(LogResultsWidget, Error, TEXT("GameInstance is null — Cannot auto-return"));
		return;
	}

	// Return to main menu automatically
	GameInstance->LoadMainMenu();
}