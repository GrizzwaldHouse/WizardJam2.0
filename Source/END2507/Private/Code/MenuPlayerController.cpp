// Fill out your copyright notice in the Description page of Project Settings.


#include "Code/MenuPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "../END2507.h"
DEFINE_LOG_CATEGORY_STATIC(LogMenuController, Log, All);
AMenuPlayerController::AMenuPlayerController()
{
	// Disable tick - menu controller doesn't need per-frame updates
	PrimaryActorTick.bCanEverTick = false;

	// Show mouse cursor by default for menu interaction
	bShowMouseCursor = true;

	// Initialize widget instance as null
	MenuWidgetInstance = nullptr;
}

void AMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogMenuController, Log, TEXT("Menu controller initialized — Harry Potter enters Great Hall"));

	// Configure input mode for UI-only interaction
	SetInputModeUIOnly();

	// Create and display main menu widget
	if (!CreateAndShowMenuWidget())
	{
		UE_LOG(LogMenuController, Error, TEXT("Failed to create menu widget — menu will not be visible!"));
	}
}

bool AMenuPlayerController::CreateAndShowMenuWidget()
{
	// Validate widget class is assigned in Blueprint wrapper
	if (!MenuWidgetClass)
	{
		UE_LOG(LogMenuController, Error, TEXT("MenuWidgetClass not set in BP_MenuPlayerController!"));
		return false;
	}

	// Create widget instance
	MenuWidgetInstance = CreateWidget<UUserWidget>(this, MenuWidgetClass);

	if (!MenuWidgetInstance)
	{
		UE_LOG(LogMenuController, Error, TEXT("Failed to create menu widget instance!"));
		return false;
	}

	// Add widget to viewport (makes it visible immediately)
	MenuWidgetInstance->AddToViewport();

	UE_LOG(LogMenuController, Log, TEXT("Main menu widget created and displayed"));
	return true;
}
void AMenuPlayerController::SetInputModeUIOnly()
{// Create UI-only input mode structure
	FInputModeUIOnly InputMode;

	if (MenuWidgetInstance)
	{
		InputMode.SetWidgetToFocus(MenuWidgetInstance->TakeWidget());
	}

	// Lock mouse to viewport
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);

	// Apply input mode to this controller
	SetInputMode(InputMode);

	UE_LOG(LogMenuController, Log, TEXT("Input mode set to UI-only — Game input disabled, mouse cursor visible"));
}
