// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MenuPlayerController.generated.h"
class UUserWidget;
// PlayerController is spawned per-player and manages input routing.
// Different controllers for different contexts:
// - MenuPlayerController: UI-only input, mouse cursor visible, no pawn
// - GameplayPlayerController: FPS controls, mouse for camera, possesses character pawn
UCLASS()
class END2507_API AMenuPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	AMenuPlayerController();
	virtual void BeginPlay() override;

private:
	
	// Main menu widget class to instantiate
	UPROPERTY(EditDefaultsOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> MenuWidgetClass;

	// Cached menu widget instance
	UPROPERTY()
	UUserWidget* MenuWidgetInstance;
	// Creates and displays the main menu widget
	bool CreateAndShowMenuWidget();

	// Configures input mode for UI-only interaction
	// Shows mouse cursor, disables game input
	void SetInputModeUIOnly();
};
