/**
 * BaseGenerator - Procedural dungeon generation system with object pooling
 *
 * Spawns interconnected rooms using seeded random generation with overlap detection,
 * timer-based spawning, automatic item/wall placement, and object pooling for performance.
 *
 * Performance Features:
 * - Object pooling for rooms, items, and walls (reduces Spawn/Destroy overhead)
 * - Component caching to minimize GetComponentsByTag calls
 * - Exit removal optimization to prevent reuse
 *
 * Author: Marcus Daley
 * Date: 10/29/25
 * Project: Game Div 6
 */
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/RandomStream.h" 
#include "BaseGenerator.generated.h"
class USceneComponent;
class UPrimitiveComponent;
class AActor;
//Wrapper struct for nested exit point arrays. This struct is used to store arrays of exit points for each spawned room, allowing for organized management of exit locations and efficient access during wall spawning and room connection processes.
USTRUCT(BlueprintType)
struct FExitPointArray
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<USceneComponent*> ExitPoints;
};
/**
 * ABaseGenerator - Procedural dungeon generator using seeded randomization
 *
 * USAGE:
 * 1. Place in level
 * 2. Configure properties in Details panel (room pools, counts, seed)
 * 3. Press Play - generation runs automatically
 *
 * GENERATION FLOW:
 * BeginPlay → Seed Init → Spawn Start Room → Timer Loop → Spawn Rooms →
 * Finalize → Close Walls → Spawn Items
 */
UCLASS()
class END2507_API ABaseGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABaseGenerator();
	// ===== STATUS QUERIES =====

	/** Returns true when all rooms, walls, and items have been spawned */
	UFUNCTION(BlueprintPure, Category = "Generator|Status")
	bool IsBuildComplete() const;
	/** Returns current number of spawned rooms */
	UFUNCTION(BlueprintPure, Category = "Generator|Status")
	int32 GetCurrentRoomCount() const;

	/** Returns the seed used for this generation */
	UFUNCTION(BlueprintPure, Category = "Generator|Status")
	int32 GetActiveSeed() const ;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// ===== GENERATION SETTINGS (Configure in Editor) =====

	/**
	 * Total number of rooms to spawn in the dungeon
	 * Default: 10 rooms
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Generation Settings", meta = (ClampMin = "1", ClampMax = "100"))
	int32 RoomAmount;
	/**
	 * How often to spawn a special room (e.g., every 3rd room)
	 * Set to 0 to disable special rooms
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Generation Settings", meta = (ClampMin = "0", ClampMax = "20"))
	int32 SpecialRoomIteration;

	/**
	 * Random seed for generation
	 * Set to -1 for random seed, any other value for deterministic generation
	 */
		UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Generation Settings")
	int32 Seed;

	/**
	 * Fail-safe timeout in seconds
	 * Prevents infinite loops if generation gets stuck
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Generation Settings", meta = (ClampMin = "1.0", ClampMax = "300.0"))
	float MaxGenerateTime;

	/**
	 * How many items to spawn on floor points
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Generation Settings", meta = (ClampMin = "0", ClampMax = "100"))
	int32 ItemAmount;

	// ===== ROOM POOLS (Assign in Editor) =====

	/**
	 * Standard room actor classes to randomly select during generation
	 * REQUIRED: Must have at least one room class assigned
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Room Pools")
	TArray<TSubclassOf<AActor>> BaseRoomList;

	/**
	 * Special room actor classes (treasure rooms, boss rooms, etc.)
	 * Leave empty to disable special rooms
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Room Pools")
	TArray<TSubclassOf<AActor>> SpecialRoomList;

	// ===== ITEM & WALL CONFIGURATION =====

	/**
	 * Item/pickup actor classes to spawn on floor points
	 * Leave empty to disable item spawning
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Configuration")
	TArray<TSubclassOf<AActor>> ItemList;

	/**
	 * Wall/door actor class to spawn at unused exits
	 * Leave empty to disable wall spawning
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wall Configuration")
	TSubclassOf<AActor> WallActorClass;

private:
	// ===== RUNTIME STATE (Do Not Edit) =====

	// All spawned rooms for overlap checking and data collection 
	UPROPERTY()
	TArray<AActor*> RoomList;

	//Most recently spawned room for validation 
	UPROPERTY()
	AActor* LatestRoom;

	//All collected floor spawn points from rooms 
	UPROPERTY()
	TArray<USceneComponent*> FloorSpawnList;

	//All collected exit points from rooms (nested arrays) 
	UPROPERTY()
	TArray<FExitPointArray> ExitsList;

	//Components currently overlapping with latest room 
	UPROPERTY()
	TArray<UPrimitiveComponent*> OverlappedList;

	//Random stream for deterministic generation 
	UPROPERTY()
	FRandomStream Stream;

	// Fail-safe timer handle 
	UPROPERTY()
	FTimerHandle Timerhandle;

	//Timer for spacing out room spawns
	UPROPERTY()
	FTimerHandle RoomSpawnTimerHandle;

	// Timestamp when generation started 
	UPROPERTY()
	float GenerationStartTime;

	// Flag indicating generation is complete 
	UPROPERTY()
	bool bIsBuildComplete;

	// ===== INTERNAL GENERATION FUNCTIONS =====

	// Initializes the random seed 
	void InitializeSeed();

	//Spawns the first room at generator's location 
	void SpawnStartRoom();

	// Spawns the next room in the generation sequence 
	void SpawnNextRoom();

	// Checks if latest room overlaps with existing rooms 
	bool CheckForOverlap();

	// Collects all exit and floor spawn points from rooms 
	void CollectAllSpawnPoints();

	// Spawns walls at all unused exits 
	void CloseAllWalls();

	// Spawns items at random floor locations 
	void SpawnAllItems();

	// Marks generation as complete and cleans up 
	void FinalizeGeneration();

	//Called when generation timer expires 
	void OnGenerationTimerExpired();

	// ===== HELPER FUNCTIONS =====

	// Spawns a random room from base or special pool 
	AActor* SpawnRandomRoom(bool bIsSpecialRoom);

	//Selects a random exit point from latest room 
	USceneComponent* GetRandomExitPoint();

	// Selects a random floor point (non-repeating) 
	USceneComponent* GetRandomFloorSpawnPoint();

	//Spawns a wall actor at the given exit 
	void SpawnWallAtExit(USceneComponent* ExitComponent);

	// Spawns an item actor at the given floor point 
	void SpawnItemAtFloor(USceneComponent* FloorComponent);
};