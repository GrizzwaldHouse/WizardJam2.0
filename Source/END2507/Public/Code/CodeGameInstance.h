// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "CodeGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class END2507_API UCodeGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
	// Override Init function to initialize game instance
	virtual void Init() override;

	//Loads the Main Menu level
	UFUNCTION(BlueprintCallable, Category = "Level")
	void LoadMainMenu();

	//Loads the Game level
	UFUNCTION(BlueprintCallable, Category = "Level")
	void LoadGameLevel();

	// Loads the Quidditch level
	UFUNCTION(BlueprintCallable, Category = "Level")
	void LoadQuidditchLevel();

	//Loads the current level safely
	UFUNCTION(BlueprintCallable, Category = "Level")
	void LoadCurrentLevelSafe();

	//Quits the game
	UFUNCTION(BlueprintCallable, Category = "Level")
	void QuitGame();

private:
	const FName MainMenuLevelName = FName(TEXT("CodeMainMenu"));
	const FName FirstGameLevelName = FName(TEXT("CodeTestingMap"));
	const FName QuidditchLevelName = FName(TEXT("QuidditchLevel"));

};
