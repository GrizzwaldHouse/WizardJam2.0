// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseGenerator.h"
#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "../END2507.h"

DEFINE_LOG_CATEGORY_STATIC(LogBaseGenerator, Log, All);
// Sets default values
ABaseGenerator::ABaseGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	RoomAmount = 10;
	SpecialRoomIteration = 3;
	Seed = -1;
	MaxGenerateTime = 30.0f;
	ItemAmount = 5;

	// Runtime state initialization
	bIsBuildComplete = false;
	LatestRoom = nullptr;
	GenerationStartTime = 0.0f;
}

void ABaseGenerator::InitializeSeed()
{
	if (Seed == -1)
	{
		Seed = FMath::RandRange(0, 999999);
		UE_LOG(LogBaseGenerator, Warning, TEXT("[%s] Using RANDOM seed: %d"), *GetName(), Seed);
	}
	else
	{
		UE_LOG(LogBaseGenerator, Log, TEXT("[%s] Using DETERMINISTIC seed: %d"), *GetName(), Seed);
	}

	Stream.Initialize(Seed);
}

// GENERATION CONTROL FUNCTIONS
void ABaseGenerator::SpawnStartRoom()
{
	if (BaseRoomList.Num() == 0)
	{
		UE_LOG(LogBaseGenerator, Error, TEXT("[%s] Cannot spawn start room - BaseRoomList is empty!"), *GetName());
		return;
	}
	UE_LOG(LogBaseGenerator, Log, TEXT("[%s] Spawning start room..."), *GetName());

	AActor* StartRoom = SpawnRandomRoom(false); // false = not special room

	if (StartRoom)
	{
		LatestRoom = StartRoom;
		RoomList.Add(StartRoom);
		UE_LOG(LogBaseGenerator, Log, TEXT("[%s] Start room spawned: %s"), *GetName(), *StartRoom->GetName());
	}
	else
	{
		UE_LOG(LogBaseGenerator, Error, TEXT("[%s] Failed to spawn start room!"), *GetName());
	}
}

void ABaseGenerator::SpawnNextRoom()
{
	// Check if we've reached room limit
	if (RoomList.Num() >= RoomAmount)
	{
		UE_LOG(LogBaseGenerator, Warning, TEXT("[%s] Room limit reached (%d/%d) - Finalizing..."),
			*GetName(), RoomList.Num(), RoomAmount);

		// Stop spawn timer
		GetWorldTimerManager().ClearTimer(RoomSpawnTimerHandle);

		// Collect spawn points and finalize
		CollectAllSpawnPoints();
		CloseAllWalls();
		SpawnAllItems();
		FinalizeGeneration();
		return;
	}

	if (!LatestRoom)
	{
		UE_LOG(LogBaseGenerator, Error, TEXT("[%s] LatestRoom is NULL! Cannot spawn next room."), *GetName());
		GetWorldTimerManager().ClearTimer(RoomSpawnTimerHandle);
		return;
	}

	// Determine if this should be a special room
	bool bIsSpecialRoom = false;
	if (SpecialRoomIteration > 0 && SpecialRoomList.Num() > 0)
	{
		int32 SpecialInterval = FMath::Max(1, RoomAmount / SpecialRoomIteration);
		bIsSpecialRoom = (RoomList.Num() > 0) && (RoomList.Num() % SpecialInterval == 0);
	}

	// Get random exit point from latest room
	USceneComponent* ExitPoint = GetRandomExitPoint();
	if (!ExitPoint)
	{
		UE_LOG(LogBaseGenerator, Warning, TEXT("[%s] No exit points available - Skipping room"), *GetName());
		return;
	}

	// Get exit's world transform
	FTransform ExitTransform = ExitPoint->GetComponentTransform();

	// Select appropriate room pool
	TArray<TSubclassOf<AActor>>& PoolToUse = bIsSpecialRoom ? SpecialRoomList : BaseRoomList;

	if (PoolToUse.Num() == 0)
	{
		UE_LOG(LogBaseGenerator, Error, TEXT("[%s] Selected room pool is empty! (Special=%d)"), *GetName(), bIsSpecialRoom);
		return;
	}

	// Spawn room at exit point
	int32 RandomIndex = Stream.RandRange(0, PoolToUse.Num() - 1);
	TSubclassOf<AActor> RoomClass = PoolToUse[RandomIndex];

	if (!RoomClass)
	{
		UE_LOG(LogBaseGenerator, Error, TEXT("[%s] Room class at index %d is NULL!"), *GetName(), RandomIndex);
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* NewRoom = GetWorld()->SpawnActor<AActor>(
		RoomClass,
		ExitTransform.GetLocation(),
		ExitTransform.Rotator(),
		SpawnParams
	);

	if (!NewRoom)
	{
		UE_LOG(LogBaseGenerator, Error, TEXT("[%s] Failed to spawn room from class!"), *GetName());
		return;
	}

	// Overlap check
	AActor* PreviousLatestRoom = LatestRoom;
	LatestRoom = NewRoom;

	if (CheckForOverlap())
	{
		UE_LOG(LogBaseGenerator, Warning, TEXT("[%s] Room overlaps - Deleting and retrying..."), *GetName());
		NewRoom->Destroy();
		LatestRoom = PreviousLatestRoom;
		return; // Timer will retry next tick
	}

	// Valid placement
	RoomList.Add(NewRoom);
	UE_LOG(LogBaseGenerator, Log, TEXT("[%s] Room %d/%d placed successfully (Special=%d)"),
		*GetName(), RoomList.Num(), RoomAmount, bIsSpecialRoom);
}


void ABaseGenerator::CloseAllWalls()
{
	UE_LOG(LogBaseGenerator, Log, TEXT("[%s] Closing all walls..."), *GetName());

	if (!WallActorClass)
	{
		UE_LOG(LogBaseGenerator, Error, TEXT("[%s] WallActorClass is not set! Call SetWallActorClass() first."), *GetName());
		return;
	}
	UE_LOG(LogBaseGenerator, Log, TEXT("[%s] Spawning walls at exits..."), *GetName());

	int32 WallCount = 0;
	for (const FExitPointArray& ExitArray : ExitsList)
	{
		for (USceneComponent* Exit : ExitArray.ExitPoints)
		{
			if (Exit)
			{
				SpawnWallAtExit(Exit);
				WallCount++;
			}
		}
	}

	UE_LOG(LogBaseGenerator, Log, TEXT("[%s] Closed %d walls"), *GetName(), WallCount);
}

void ABaseGenerator::SpawnAllItems()
{
	UE_LOG(LogBaseGenerator, Log, TEXT("[%s] Spawning items..."), *GetName());

	if (ItemList.Num() == 0)
	{
		UE_LOG(LogBaseGenerator, Warning, TEXT("[%s] ItemList is empty - cannot spawn items"), *GetName());
		return;
	}

	if (FloorSpawnList.Num() == 0)
	{
		UE_LOG(LogBaseGenerator, Warning, TEXT("[%s] FloorSpawnList is empty - cannot spawn items"), *GetName());
		return;
	}

	// Spawn up to ItemAmount, but don't exceed available spawn points
	int32 ItemsToSpawn = FMath::Min(ItemAmount, FloorSpawnList.Num());
	int32 ItemsSpawned = 0;

	for (int32 i = 0; i < ItemsToSpawn; i++)
	{
		USceneComponent* FloorPoint = GetRandomFloorSpawnPoint();
		if (FloorPoint)
		{
			SpawnItemAtFloor(FloorPoint);
			ItemsSpawned++;
		}
		else
		{
			UE_LOG(LogBaseGenerator, Warning, TEXT("[%s] Ran out of floor spawn points at item %d/%d"),
				*GetName(), i, ItemsToSpawn);
			break;
		}
	}

	UE_LOG(LogBaseGenerator, Log, TEXT("[%s] Spawned %d/%d items"), *GetName(), ItemsSpawned, ItemAmount);
}
// VALIDATION FUNCTIONS
bool ABaseGenerator::CheckForOverlap()
{
	if (!LatestRoom)
	{
		UE_LOG(LogBaseGenerator, Error, TEXT("[%s] CheckForOverlap called but LatestRoom is NULL!"), *GetName());
		return false;
	}

	// Clear previous overlap list
	OverlappedList.Empty();

	// Get all primitive components from LatestRoom
	TArray<UPrimitiveComponent*> Primitives;
	LatestRoom->GetComponents<UPrimitiveComponent>(Primitives);

	if (Primitives.Num() == 0)
	{
		UE_LOG(LogBaseGenerator, Warning, TEXT("[%s] LatestRoom has no primitive components for overlap detection"), *GetName());
		return false; // No primitives to check
	}

	// Check each primitive for overlapping components
	for (UPrimitiveComponent* Prim : Primitives)
	{
		if (!Prim)
		{
			continue;
		}

		// Get all components overlapping this primitive
		TArray<UPrimitiveComponent*> OverlappingComponents;
		Prim->GetOverlappingComponents(OverlappingComponents);

		// Filter out self-overlaps and add unique overlaps
		for (UPrimitiveComponent* OverlappingComp : OverlappingComponents)
		{
			if (!OverlappingComp)
			{
				continue;
			}

			// Ignore overlaps with components from the same actor (self)
			if (OverlappingComp->GetOwner() == LatestRoom)
			{
				continue;
			}

			// Ignore overlaps with the generator itself
			if (OverlappingComp->GetOwner() == this)
			{
				continue;
			}

			// Add unique overlapping component
			OverlappedList.AddUnique(OverlappingComp);
		}
	}

	bool bIsOverlapping = (OverlappedList.Num() > 0);

	if (bIsOverlapping)
	{
		UE_LOG(LogBaseGenerator, Warning, TEXT("[%s] Overlap detected! %d overlapping components found"),
			*GetName(), OverlappedList.Num());

		// Debug visualization in non-shipping builds
#if !UE_BUILD_SHIPPING
		for (UPrimitiveComponent* Comp : OverlappedList)
		{
			if (Comp)
			{
				DrawDebugSphere(
					GetWorld(),
					Comp->GetComponentLocation(),
					50.0f,
					12,
					FColor::Red,
					false,
					5.0f
				);
			}
		}
#endif
	}
	else
	{
		UE_LOG(LogBaseGenerator, Log, TEXT("[%s] No overlaps detected - Room placement is valid"), *GetName());
	}

	return bIsOverlapping;
}


// UTILITY FUNCTIONS
void ABaseGenerator::CollectAllSpawnPoints()
{
	UE_LOG(LogBaseGenerator, Log, TEXT("[%s] Collecting spawn points from %d rooms..."), *GetName(), RoomList.Num());

	ExitsList.Empty();
	FloorSpawnList.Empty();

	int32 TotalExits = 0;
	int32 TotalFloors = 0;

	for (AActor* Room : RoomList)
	{
		if (!Room) continue;

		// Collect exits
		TArray<UActorComponent*> ExitComponents = Room->GetComponentsByTag(USceneComponent::StaticClass(), FName("ExitPoint"));
		FExitPointArray RoomExits;
		for (UActorComponent* Comp : ExitComponents)
		{
			if (USceneComponent* SceneComp = Cast<USceneComponent>(Comp))
			{
				RoomExits.ExitPoints.Add(SceneComp);
			}
		}
		if (RoomExits.ExitPoints.Num() > 0)
		{
			ExitsList.Add(RoomExits);
			TotalExits += RoomExits.ExitPoints.Num();
		}

		// Collect floor spawn points
		TArray<UActorComponent*> FloorComponents = Room->GetComponentsByTag(USceneComponent::StaticClass(), FName("FloorSpawn"));
		for (UActorComponent* Comp : FloorComponents)
		{
			if (USceneComponent* SceneComp = Cast<USceneComponent>(Comp))
			{
				FloorSpawnList.Add(SceneComp);
				TotalFloors++;
			}
		}
	}

	UE_LOG(LogBaseGenerator, Warning, TEXT("[%s] ✓ Collected %d exits and %d floor points"),
		*GetName(), TotalExits, TotalFloors);
}

void ABaseGenerator::FinalizeGeneration()
{
	UE_LOG(LogBaseGenerator, Log, TEXT("[%s] Finalizing generation..."), *GetName());

	// Clear timer if still running
	if (GetWorldTimerManager().IsTimerActive(Timerhandle))
	{
		GetWorldTimerManager().ClearTimer(Timerhandle);
		UE_LOG(LogBaseGenerator, Log, TEXT("[%s] Generation timer cleared"), *GetName());
	}

	bIsBuildComplete = true;

	UE_LOG(LogBaseGenerator, Warning, TEXT("[%s] === GENERATION COMPLETE ==="), *GetName());
	UE_LOG(LogBaseGenerator, Warning, TEXT("[%s] Total rooms spawned: %d"), *GetName(), RoomList.Num());
	UE_LOG(LogBaseGenerator, Warning, TEXT("[%s] Seed used: %d"), *GetName(), Seed);
}


// Called when the game starts or when spawned
void ABaseGenerator::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogBaseGenerator, Warning, TEXT("=== DUNGEON GENERATOR STARTING ==="));
	UE_LOG(LogBaseGenerator, Log, TEXT("[%s] Target Rooms: %d | Special Every: %d | Items: %d"),
		*GetName(), RoomAmount, SpecialRoomIteration, ItemAmount);

	// VALIDATION: Check critical properties
	if (BaseRoomList.Num() == 0)
	{
		UE_LOG(LogBaseGenerator, Error, TEXT("[%s] CRITICAL ERROR: BaseRoomList is EMPTY! Cannot generate dungeon."), *GetName());
		UE_LOG(LogBaseGenerator, Error, TEXT("[%s] Assign room classes in Blueprint defaults panel!"), *GetName());
		return;
	}

	if (RoomAmount <= 0)
	{
		UE_LOG(LogBaseGenerator, Error, TEXT("[%s] CRITICAL ERROR: RoomAmount is %d! Must be at least 1."), *GetName(), RoomAmount);
		return;
	}

	// === PHASE 1: INITIALIZATION ===
	InitializeSeed();

	// Start fail-safe timer
	if (MaxGenerateTime > 0.0f)
	{
		GenerationStartTime = GetWorld()->GetTimeSeconds();
		GetWorldTimerManager().SetTimer(
			Timerhandle,
			this,
			&ABaseGenerator::OnGenerationTimerExpired,
			MaxGenerateTime,
			false
		);
		UE_LOG(LogBaseGenerator, Log, TEXT("[%s] Fail-safe timer started: %.1fs"), *GetName(), MaxGenerateTime);
	}

	// === PHASE 2: SPAWN START ROOM ===
	SpawnStartRoom();

	if (!LatestRoom)
	{
		UE_LOG(LogBaseGenerator, Error, TEXT("[%s] Failed to spawn start room! Aborting generation."), *GetName());
		return;
	}

	// === PHASE 3: START ROOM GENERATION LOOP ===
	// Use a timer to spawn rooms gradually (prevents lag spikes)
	GetWorldTimerManager().SetTimer(
		RoomSpawnTimerHandle,
		this,
		&ABaseGenerator::SpawnNextRoom,
		0.1f, // Spawn a room every 0.1 seconds
		true  // Loop
	);

	UE_LOG(LogBaseGenerator, Log, TEXT("[%s] Generation loop started"), *GetName());
}
// Called every frame
void ABaseGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Debug visualization in non-shipping builds
#if !UE_BUILD_SHIPPING
	if (bIsBuildComplete && ExitsList.Num() > 0)
	{
		// Draw debug spheres at all exits (green = closed)
		for (const FExitPointArray& ExitArray : ExitsList)
		{
			for (USceneComponent* Exit : ExitArray.ExitPoints)
			{
				if (Exit)
				{
					DrawDebugSphere(GetWorld(), Exit->GetComponentLocation(), 25.0f, 8, FColor::Green, false, 0.1f);
				}
			}
		}
	}
#endif
}
// INTERNAL HELPER FUNCTIONS
AActor* ABaseGenerator::SpawnRandomRoom(bool bIsSpecialRoom)
{
	TArray<TSubclassOf<AActor>>& PoolToUse = bIsSpecialRoom ? SpecialRoomList : BaseRoomList;

	if (PoolToUse.Num() == 0)
	{
		UE_LOG(LogBaseGenerator, Error, TEXT("[%s] Room pool is empty! (Special=%d)"), *GetName(), bIsSpecialRoom);
		return nullptr;
	}
	// Get random class from pool using Stream
	int32 RandomIndex = Stream.RandRange(0, PoolToUse.Num() - 1);
	TSubclassOf<AActor> RoomClass = PoolToUse[RandomIndex];

	if (!RoomClass)
	{
		UE_LOG(LogBaseGenerator, Error, TEXT("[%s] Room class at index %d is NULL!"), *GetName(), RandomIndex);
		return nullptr;
	}

	// Spawn room at this generator's location (for start room)
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* SpawnedRoom = GetWorld()->SpawnActor<AActor>(RoomClass, GetActorTransform(), SpawnParams);

	if (SpawnedRoom)
	{
		UE_LOG(LogBaseGenerator, Log, TEXT("[%s] Spawned room: %s (Special=%d)"), *GetName(), *SpawnedRoom->GetName(), bIsSpecialRoom);
	}
	else
	{
		UE_LOG(LogBaseGenerator, Error, TEXT("[%s] Failed to spawn room from class!"), *GetName());
	}

	return SpawnedRoom;
}

USceneComponent* ABaseGenerator::GetRandomExitPoint()
{
	if (!LatestRoom)
	{
		UE_LOG(LogBaseGenerator, Error, TEXT("[%s] GetRandomExitPoint called but LatestRoom is NULL!"), *GetName());
		return nullptr;
	}

	TArray<UActorComponent*> ComponentsWithTag = LatestRoom->GetComponentsByTag(USceneComponent::StaticClass(), FName("ExitPoint"));

	TArray<USceneComponent*> ExitPoints;
	for (UActorComponent* Comp : ComponentsWithTag)
	{
		if (USceneComponent* SceneComp = Cast<USceneComponent>(Comp))
		{
			ExitPoints.Add(SceneComp);
		}
	}

	if (ExitPoints.Num() == 0)
	{
		UE_LOG(LogBaseGenerator, Error, TEXT("[%s] LatestRoom '%s' has no components tagged 'ExitPoint'!"),
			*GetName(), *LatestRoom->GetName());
		return nullptr;
	}

	// Select random exit using Stream
	int32 RandomIndex = Stream.RandRange(0, ExitPoints.Num() - 1);
	return ExitPoints[RandomIndex];
}


USceneComponent* ABaseGenerator::GetRandomFloorSpawnPoint()
{
	if (FloorSpawnList.Num() == 0)
	{
		UE_LOG(LogBaseGenerator, Error, TEXT("[%s] FloorSpawnList is empty - no spawn points available!"), *GetName());
		return nullptr;
	}

	// Select random spawn point using Stream
	int32 RandomIndex = Stream.RandRange(0, FloorSpawnList.Num() - 1);
	USceneComponent* SelectedPoint = FloorSpawnList[RandomIndex];

	// Remove from list to prevent duplicate spawns at same location
	FloorSpawnList.RemoveAt(RandomIndex);

	UE_LOG(LogBaseGenerator, Log, TEXT("[%s] Selected floor spawn point (%d remaining)"),
		*GetName(), FloorSpawnList.Num());

	return SelectedPoint;
}

void ABaseGenerator::OnGenerationTimerExpired()
{
	UE_LOG(LogBaseGenerator, Error, TEXT("[%s] ===== GENERATION TIMER EXPIRED ====="), *GetName());
	UE_LOG(LogBaseGenerator, Error, TEXT("[%s] Generation took longer than %.1f seconds!"), *GetName(), MaxGenerateTime);
	UE_LOG(LogBaseGenerator, Error, TEXT("[%s] Rooms spawned: %d/%d"), *GetName(), RoomList.Num(), RoomAmount);
	UE_LOG(LogBaseGenerator, Error, TEXT("[%s] Forcing finalization..."), *GetName());
	GetWorldTimerManager().ClearTimer(RoomSpawnTimerHandle);
	CollectAllSpawnPoints();
	CloseAllWalls();
	SpawnAllItems();
	FinalizeGeneration();
}

void ABaseGenerator::SpawnWallAtExit(USceneComponent* ExitComponent)
{
	if (!ExitComponent || !WallActorClass)
	{
		UE_LOG(LogBaseGenerator, Error, TEXT("[%s] SpawnWallAtExit called with invalid parameters! ExitComponent: %s, WallActorClass: %s"),
			*GetName(),
			ExitComponent ? *ExitComponent->GetName() : TEXT("NULL"),
			WallActorClass ? *WallActorClass->GetName() : TEXT("NULL"));
		return;
	}

	FTransform WallTransform = ExitComponent->GetComponentTransform();

	UE_LOG(LogBaseGenerator, Log, TEXT("[%s] Wall spawn location: %s"),
		*GetName(), *WallTransform.GetLocation().ToString());
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* Wall = GetWorld()->SpawnActor<AActor>(
		WallActorClass,
		WallTransform.GetLocation(),
		WallTransform.Rotator(),
		SpawnParams
	);

#if !UE_BUILD_SHIPPING
	if (Wall)
	{
		DrawDebugSphere(GetWorld(), WallTransform.GetLocation(), 30.0f, 12, FColor::Yellow, false, 10.0f, 0, 2.0f);
		DrawDebugDirectionalArrow(
			GetWorld(),
			WallTransform.GetLocation(),
			WallTransform.GetLocation() + (WallTransform.GetRotation().GetForwardVector() * 100.0f),
			50.0f, FColor::Yellow, false, 10.0f, 0, 3.0f
		);
	}
#endif
}

void ABaseGenerator::SpawnItemAtFloor(USceneComponent* FloorComponent)
{
	if (!FloorComponent || ItemList.Num() == 0)
	{
		UE_LOG(LogBaseGenerator, Error, TEXT("[%s] SpawnItemAtFloor called with invalid parameters! FloorComponent: %s, ItemList count: %d"),
			*GetName(),
			FloorComponent ? *FloorComponent->GetName() : TEXT("NULL"),
			ItemList.Num());
		return;
	}


	// Get random item class from pool using Stream
	int32 RandomIndex = Stream.RandRange(0, ItemList.Num() - 1);
	TSubclassOf<AActor> ItemClass = ItemList[RandomIndex];

	if (!ItemClass)
	{
		UE_LOG(LogBaseGenerator, Error, TEXT("[%s] Item class at index %d is NULL!"), *GetName(), RandomIndex);
		return;
	}

	// Get spawn transform from floor component
	FTransform ItemTransform = FloorComponent->GetComponentTransform();

	// Spawn item actor
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AActor* Item = GetWorld()->SpawnActor<AActor>(
		ItemClass,
		ItemTransform.GetLocation(),
		ItemTransform.Rotator(),
		SpawnParams
	);

#if !UE_BUILD_SHIPPING
	if (Item)
	{
		DrawDebugSphere(GetWorld(), ItemTransform.GetLocation(), 40.0f, 12, FColor::Cyan, false, 10.0f, 0, 2.0f);
	}
#endif
}
bool ABaseGenerator::IsBuildComplete() const
{
	return bIsBuildComplete;
}

int32 ABaseGenerator::GetCurrentRoomCount() const
{
	return RoomList.Num();
}

int32 ABaseGenerator::GetActiveSeed() const
{
	return Seed;
}
