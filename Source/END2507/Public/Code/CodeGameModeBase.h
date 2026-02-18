// Game mode for Week 3 submission - handles win/loss conditions
// Author: Marcus Daley
// Date: 10/13/2025


#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CodeGameModeBase.generated.h"

class UUserWidget;
class ABaseAgent;
class ABasePlayer;
class ASpawner;
UCLASS()
class END2507_API ACodeGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
public:
	ACodeGameModeBase();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// [2026-01-20] Override to fix spawn collision issues
	// Forces spawn even when collision is detected at PlayerStart
	virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) override;

	// Counts remaining enemies and checks win condition
	UFUNCTION(BlueprintCallable, Category = "Game Rules")
	void CheckWinCondition();

	// Called when an enemy is destroyed 
	// Decrements enemy count and checks if player won
	UFUNCTION()
	void RemoveEnemy(AActor* DestroyedEnemy);

	// Called when player dies or touches damage pickup
// Displays loss screen with manual buttons 
	UFUNCTION()
	void RemovePlayer(AActor* DestroyedEnemy);

	// Called when player dies or touches damage pickup
	UFUNCTION(BlueprintCallable, Category = "Game Rules")
	void EndGame(bool bPlayerWon);
	UFUNCTION()
	void RegisterSpawner(ASpawner* Spawner);

	UFUNCTION()
	void UnregisterSpawner(AActor* DestroyedSpawner);

private:
	FTimerHandle InitializationTimerHandle;

	// Results widget class to spawn on game end
	UPROPERTY(EditDefaultsOnly, Category = "UI", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> ResultsWidgetClass;

	// Cached results widget instance
	UPROPERTY()
	UUserWidget* ResultsWidgetInstance;
	UPROPERTY()
	ABasePlayer* CurrentPlayer;
	// Current number of living enemies in the level
	int32 EnemyCount;

	// Timer handle for auto-return to menu after victory
	FTimerHandle VictoryReturnTimerHandle;

	UPROPERTY()
	TArray<ASpawner*> ActiveSpawners;

	// Creates and caches the results widget 
	bool CreateResultsWidget();
	// Displays cached results widget and configures it for win/lose state
	void ShowResultsWidget(bool bPlayerWon);
	// Automatically returns to main menu after victory
	void AutoReturnToMenu();

	// Counts agents and binds death observers after level fully loads
	void CountAndBindAgents();
};
