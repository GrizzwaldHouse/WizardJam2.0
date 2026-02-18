// Fill out your copyright notice in the Description page of Project Settings.


#include "Code/CodeGameModeBase.h"
#include "Blueprint/UserWidget.h"
#include "Code/ResultsWidget.h"
#include "Code/Actors/Spawner.h"
#include "TimerManager.h"
#include "EngineUtils.h" 
#include "Code/Actors/BaseAgent.h"
#include "Code/Actors/BasePlayer.h"
#include "Code/AC_HealthComponent.h"
#include "Code/CodeGameInstance.h"
#include "../END2507.h"
// Logging category for game mode events
DEFINE_LOG_CATEGORY_STATIC(LogCodeGameMode, Log, All);


ACodeGameModeBase::ACodeGameModeBase()
	: EnemyCount(0)
	, ResultsWidgetInstance(nullptr)
	, CurrentPlayer(nullptr)
{
	PrimaryActorTick.bCanEverTick = false;

	// [2026-01-20] Fix spawn collision - always spawn player even if collision detected
	// This prevents "SpawnActor failed because of collision" errors
	bUseSeamlessTravel = false;

	// Default pawn class - Blueprint can override this
	// If BP_CodeBasePlayer exists, set it in BP_CodeGameMode instead
	DefaultPawnClass = ABasePlayer::StaticClass();
}

void ACodeGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogCodeGameMode, Log, TEXT(" GameMode initialized "));

	// Create and cache results widget 
	if (!CreateResultsWidget())
	{
		UE_LOG(LogCodeGameMode, Error, TEXT(" Failed to create Results widget — game ending will not display UI!"));
	}

	CountAndBindAgents();
	
}

void ACodeGameModeBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Unbind from spawner OnDestroyed delegates
	for (ASpawner* Spawner : ActiveSpawners)
	{
		if (Spawner)
		{
			Spawner->OnDestroyed.RemoveDynamic(this, &ACodeGameModeBase::UnregisterSpawner);
		}
	}

	// Unbind from player health delegate
	if (CurrentPlayer)
	{
		UAC_HealthComponent* HealthComp = CurrentPlayer->GetHealthComponent();
		if (HealthComp)
		{
			HealthComp->OnDeathEnded.RemoveDynamic(this, &ACodeGameModeBase::RemovePlayer);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ACodeGameModeBase::CheckWinCondition()

{

	if (!GetWorld())
	{
		return;
	}

	// Count living enemies using TActorIterator (NOT UGameplayStatics!)
	int32 LivingEnemies = 0;
	for (TActorIterator<ABaseAgent> It(GetWorld()); It; ++It)
	{
		ABaseAgent* Agent = *It;
		if (Agent)
		{
			// Check if agent is alive by checking its HealthComponent
			UAC_HealthComponent* HealthComp = Agent->FindComponentByClass<UAC_HealthComponent>();
			if (HealthComp && HealthComp->GetCurrentHealth() > 0.0f)
			{
				LivingEnemies++;
			}
		}
	}

	// Clean up ActiveSpawners array - remove nullptr and dead spawners
	int32 OriginalSpawnerCount = ActiveSpawners.Num();

	// 
	ActiveSpawners.RemoveAll([](ASpawner* Spawner)
		{
			if (!Spawner)
			{
				return true;  // Remove nullptr
			}

			// Check if spawner is alive by checking its HealthComponent
			UAC_HealthComponent* HealthComp = Spawner->FindComponentByClass<UAC_HealthComponent>();
			if (!HealthComp || HealthComp->GetCurrentHealth() <= 0.0f)
			{
				return true;  // Remove dead spawners
			}

			return false;  // Keep alive spawners
		});

	UE_LOG(LogTemp, Display, TEXT("Win Check — Living Enemies: %d, Active Spawners: %d"),
		LivingEnemies, ActiveSpawners.Num());

	// Player wins only if NO enemies AND NO spawners remain
	if (LivingEnemies == 0 && ActiveSpawners.Num() == 0)
	{
		UE_LOG(LogTemp, Display, TEXT("★ PLAYER WINS — All enemies and spawners eliminated!"));
		EndGame(true);
	}
}
void ACodeGameModeBase::RemoveEnemy(AActor* DestroyedEnemy)
{
	// Validate destroyed actor reference
		if (!DestroyedEnemy)
		{
			UE_LOG(LogCodeGameMode, Warning, TEXT("RemoveEnemy called with null actor!"));
			return;
		}

		EnemyCount--;
		// Decrement count
		EnemyCount = FMath::Max(0, EnemyCount - 1);

		UE_LOG(LogTemp, Display, TEXT("Enemy destroyed: %s (Remaining: %d)"),
			*DestroyedEnemy->GetName(), EnemyCount);

		// Check win condition
		CheckWinCondition();
}

void ACodeGameModeBase::RemovePlayer(AActor* DestroyedEnemy)
{
	// Player died - immediate game loss
	UE_LOG(LogCodeGameMode, Log, TEXT("Player eliminated DEFEAT!"));

	EndGame(false); // false = player lost
}

void ACodeGameModeBase::EndGame(bool bPlayerWon)
{
	if (!ResultsWidgetInstance && !CreateResultsWidget())
	{
		UE_LOG(LogCodeGameMode, Error,
			TEXT("Cannot show ResultsWidget — Creation failed!"));
		return;
	}
	// If player won, hide their HUD before showing results
	if (bPlayerWon && CurrentPlayer)
	{
		CurrentPlayer->PlayerWin();
		UE_LOG(LogCodeGameMode, Log, TEXT("Player HUD hidden for victory screen"));
	}

	ShowResultsWidget(bPlayerWon);
}

void ACodeGameModeBase::RegisterSpawner(ASpawner* Spawner)
{
	if (!Spawner)
	{
		return;
	}

	// Avoid duplicates
	if (ActiveSpawners.Contains(Spawner))
	{
		return;
	}

	ActiveSpawners.Add(Spawner);

	// Bind to OnDestroyed delegate
	Spawner->OnDestroyed.AddDynamic(this, &ACodeGameModeBase::UnregisterSpawner);

	UE_LOG(LogTemp, Log, TEXT("Spawner registered: %s (Total spawners: %d)"),
		*Spawner->GetName(), ActiveSpawners.Num());
}

void ACodeGameModeBase::UnregisterSpawner(AActor* DestroyedSpawner)
{
	if (!DestroyedSpawner)
	{
		return;
	}

	ASpawner* Spawner = Cast<ASpawner>(DestroyedSpawner);
	if (Spawner)
	{
		ActiveSpawners.Remove(Spawner);

		UE_LOG(LogTemp, Display, TEXT("Spawner destroyed: %s (Remaining: %d)"),
			*Spawner->GetName(), ActiveSpawners.Num());

		// Check if player won
		CheckWinCondition();
	}
}

bool ACodeGameModeBase::CreateResultsWidget()
{
	// Validate widget class is assigned in Blueprint wrapper
	if (!ResultsWidgetClass)
	{
		UE_LOG(LogCodeGameMode, Error,
			TEXT("ResultsWidgetClass NOT SET in GameMode Blueprint! "
				"Open BP_CodeGameMode and assign ResultsWidget class."));
		return false;
	}

	// Validate world exists
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogCodeGameMode, Error, TEXT("CreateResultsWidget failed — World is null!"));
		return false;
	}
	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC)
	{
		UE_LOG(LogCodeGameMode, Error,
			TEXT("No PlayerController found — Cannot create widget! "
				"Ensure player is properly spawned."));
		return false;
	}
	// Create widget instance 
	ResultsWidgetInstance = CreateWidget<UUserWidget>(World, ResultsWidgetClass);

	if (!ResultsWidgetInstance)
	{
		UE_LOG(LogCodeGameMode, Error,
			TEXT("Failed to create ResultsWidget! Verify Blueprint inheritance."));
		return false;
	}

	UE_LOG(LogCodeGameMode, Log, TEXT("Results widget created and cached (not visible yet)"));
	return true;
}

void ACodeGameModeBase::ShowResultsWidget(bool bPlayerWon)
{

	// Validate cached widget exists
	if (!ResultsWidgetInstance)
	{
		UE_LOG(LogCodeGameMode, Error, TEXT(" ResultsWidgetInstance is NULL!"));
		return;
	}

	// Get PlayerController
	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		UE_LOG(LogCodeGameMode, Error, TEXT(" No PlayerController found!"));
		return;
	}

	UE_LOG(LogCodeGameMode, Warning, TEXT("PlayerController Valid: %s"), *PC->GetName());

	// Configure input mode ONLY for lose condition (player needs to click buttons)
	// Victory auto-returns to menu, so no mouse needed
	if (!bPlayerWon)
	{
		PC->SetShowMouseCursor(true);
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(ResultsWidgetInstance->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);

		UE_LOG(LogCodeGameMode, Warning, TEXT("Input mode configured for defeat (mouse enabled)"));
	}
	else
	{
		UE_LOG(LogCodeGameMode, Log, TEXT("Victory - no input mode change needed (auto-return active)"));
	}

	// Add to viewport with high Z-order
	ResultsWidgetInstance->AddToViewport(9999);
	ResultsWidgetInstance->SetVisibility(ESlateVisibility::Visible);

	// Cast to custom widget class
	UResultsWidget* ResultsWidget = Cast<UResultsWidget>(ResultsWidgetInstance);
	if (!ResultsWidget)
	{
		UE_LOG(LogCodeGameMode, Error, TEXT(" Cast to UResultsWidget FAILED!"));
		UE_LOG(LogCodeGameMode, Error, TEXT("   This means ResultsWidgetClass is NOT a UResultsWidget Blueprint!"));
		return;
	}

	UE_LOG(LogCodeGameMode, Warning, TEXT("Cast to UResultsWidget succeeded"));

	// Configure for win/lose state
	// If victory, WinConditionMet() hides buttons and starts auto-return timer
	// If defeat, buttons remain visible for manual selection
	bPlayerWon ? ResultsWidget->WinConditionMet() : void();

	UE_LOG(LogCodeGameMode, Log, TEXT("Results widget displayed — Game ended: %s"),
		bPlayerWon ? TEXT("Victory") : TEXT("Defeat"));
}

void ACodeGameModeBase::AutoReturnToMenu()
{
	UE_LOG(LogCodeGameMode, Log, TEXT(" Auto-returning to main menu after victory"));

	// Get GameInstance and trigger menu load
	UCodeGameInstance* gameInstance = Cast<UCodeGameInstance>(GetGameInstance());
	if (!gameInstance)
	{
		UE_LOG(LogCodeGameMode, Error, TEXT("AutoReturnToMenu failed — GameInstance is not UGameInstanceWeek3!"));
		return;
	}

	gameInstance->LoadMainMenu();
}


void ACodeGameModeBase::CountAndBindAgents()
{ 
	// Reset counter before scanning level
	EnemyCount = 0;

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogCodeGameMode, Error, TEXT("CountAndBindAgents failed — World is null!"));
		return;
	}
	for (TActorIterator<ASpawner> It(World); It; ++It)
	{
		ASpawner* Spawner = *It;
		if (Spawner)
		{
			RegisterSpawner(Spawner);
		}
	}
	UE_LOG(LogCodeGameMode, Display, TEXT("Registered %d spawners in level"), ActiveSpawners.Num());

	// Iterate once through all agents in the level
	for (TActorIterator<ABaseAgent> It(World); It; ++It)
	{
		ABaseAgent* Agent = *It;
		if (!Agent) continue;

		// Increment counter (only once per agent!)
		EnemyCount++;
		// Bind to OnDestroyed delegate
		Agent->OnDestroyed.AddDynamic(this, &ACodeGameModeBase::RemoveEnemy);

		UE_LOG(LogTemp, Log, TEXT("Agent registered: %s"), *Agent->GetName());
	}

	UE_LOG(LogCodeGameMode, Log, TEXT(" Initial enemy count: %d agents detected"), EnemyCount);

	
	// Iterate through all player characters in the level
	for (TActorIterator<ABasePlayer> It(World); It; ++It)
	{
		ABasePlayer* Player = *It;
		if (!Player) continue;

		// Cache player reference for win condition logic
		CurrentPlayer = Player;
		UE_LOG(LogCodeGameMode, Log, TEXT("Player reference cached: %s"), *Player->GetName());

		// Get health component for death binding
		UAC_HealthComponent* HealthComp = Player->GetHealthComponent();
		if (!HealthComp)
		{
			UE_LOG(LogCodeGameMode, Warning, TEXT("Player has no HealthComponent!"));
			continue;
		}

		// This allows the death animation to complete before showing results screen
		HealthComp->OnDeathEnded.AddDynamic(this, &ACodeGameModeBase::RemovePlayer);
		UE_LOG(LogCodeGameMode, Log, TEXT(" Registered death-ended observer for player (waits for animation)"));

		// Only one player should exist
		break;
	}
	for (TActorIterator<ASpawner> It(GetWorld()); It; ++It)
	{
		ASpawner* Spawner = *It;
		if (Spawner)
		{
			RegisterSpawner(Spawner);
		}
	}

	UE_LOG(LogTemp, Display, TEXT("Game initialized — Enemies: %d, Spawners: %d"),
		EnemyCount, ActiveSpawners.Num());

}

APawn* ACodeGameModeBase::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	// [2026-01-20] Fix for "SpawnActor failed because of collision at spawn location"
	// Override default spawn behavior to always spawn, adjusting position if needed

	UWorld* World = GetWorld();
	if (!World || !NewPlayer)
	{
		UE_LOG(LogCodeGameMode, Error, TEXT("SpawnDefaultPawnAtTransform: Invalid World or Controller"));
		return nullptr;
	}

	UClass* PawnClass = GetDefaultPawnClassForController(NewPlayer);
	if (!PawnClass)
	{
		UE_LOG(LogCodeGameMode, Error, TEXT("SpawnDefaultPawnAtTransform: No DefaultPawnClass set!"));
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = NewPlayer;
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.ObjectFlags |= RF_Transient;

	// KEY FIX: Always spawn, adjust if collision detected
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	APawn* SpawnedPawn = World->SpawnActor<APawn>(PawnClass, SpawnTransform, SpawnParams);

	if (SpawnedPawn)
	{
		UE_LOG(LogCodeGameMode, Log, TEXT("Player spawned successfully at %s"),
			*SpawnTransform.GetLocation().ToString());
	}
	else
	{
		UE_LOG(LogCodeGameMode, Error, TEXT("Failed to spawn player pawn of class %s at %s"),
			*PawnClass->GetName(), *SpawnTransform.GetLocation().ToString());
	}

	return SpawnedPawn;
}
