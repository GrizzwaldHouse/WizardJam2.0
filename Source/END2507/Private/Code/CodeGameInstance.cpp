// Author: Marcus Daley 
// Date: 10/13/2025
#include "Code/CodeGameInstance.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "../END2507.h"
// Logging category for game instance events
DEFINE_LOG_CATEGORY_STATIC(LogGameInstance, Log, All);
void UCodeGameInstance::Init()
{
	UE_LOG(LogGameInstance, Log, TEXT(" GameInstance initialized"));
	 LoadMainMenu();
}

void UCodeGameInstance::LoadMainMenu()
{
	//Validate world exists before attempting level load
	UWorld* World = GetWorld();
	if(!World)
	{
		UE_LOG(LogGameInstance, Error, TEXT("Cannot load Main Menu: World is null"));
		return;
	}
	//Get first player controller to excute console command
	APlayerController* PlayerController = World->GetFirstPlayerController();
	if(!PlayerController)
	{
		UE_LOG(LogGameInstance, Error, TEXT("Cannot load Main Menu: PlayerController is null"));
		return;
	}
	UE_LOG(LogGameInstance, Log, TEXT(" Loading Main Menu: %s"), *MainMenuLevelName.ToString());

	//Use console command to load level
	FString Command = FString::Printf(TEXT("Open %s"), *MainMenuLevelName.ToString());
	PlayerController->ConsoleCommand(Command);  

}

void UCodeGameInstance::LoadGameLevel()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogGameInstance, Error, TEXT("Cannot load Main Menu: World is null"));
		return;
	}
	//Get first player controller to excute console command
	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!PlayerController)
	{
		UE_LOG(LogGameInstance, Error, TEXT("Cannot load Main Menu: PlayerController is null"));
		return;
	}
	UE_LOG(LogGameInstance, Log, TEXT("Loading first gameplay level: %s"), *FirstGameLevelName.ToString());
	FString Command = FString::Printf(TEXT("open %s"), *FirstGameLevelName.ToString());
	PlayerController->ConsoleCommand(Command);
}

void UCodeGameInstance::LoadCurrentLevelSafe()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogGameInstance, Error, TEXT(" LoadCurrentLevelSafe failed — World is null!"));
		return;
	}

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!PlayerController)
	{
		UE_LOG(LogGameInstance, Error, TEXT(" LoadCurrentLevelSafe failed — No PlayerController found!"));
		return;
	}
	FString CurrentLevelName = World->GetMapName();

	
	CurrentLevelName.RemoveFromStart(World->StreamingLevelsPrefix);

	UE_LOG(LogGameInstance, Log, TEXT(" Reloading current level: %s"), *CurrentLevelName);

	// Console command to reload current level
	FString Command = FString::Printf(TEXT("open %s"), *CurrentLevelName);
	PlayerController->ConsoleCommand(Command);
}

void UCodeGameInstance::QuitGame()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogGameInstance, Error, TEXT(" QuitTheGame failed — World is null!"));
		return;
	}

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!PlayerController)
	{
		UE_LOG(LogGameInstance, Warning, TEXT("QuitTheGame — No PlayerController, using fallback quit"));

		// Fallback: Use engine to quit directly
		if (GEngine)
		{
			GEngine->HandleDisconnect(World, World->GetNetDriver());
		}
		return;
	}

	UE_LOG(LogGameInstance, Log, TEXT("Quitting game"));

	// Console command to quit (works in both PIE and packaged builds)
	PlayerController->ConsoleCommand(TEXT("quit"));
}