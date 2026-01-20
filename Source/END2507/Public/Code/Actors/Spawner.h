// Spawner.h
// Actor that spawns other actors at runtime and takes damage before destruction
// Author: Marcus Daley
// Date: [10/20/2025]

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Spawner.generated.h"
class UStaticMeshComponent;
class UAC_HealthComponent;
class ABaseAgent;
class UMaterialInstanceDynamic;
class UBoxComponent;
class IFactionInterface;
/**
 * Delegate broadcasted when an agent is spawned
 * @param SpawnedAgent - The agent that was just spawned
 * @param FactionColor - The color assigned to this agent's faction
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAgentSpawned, ABaseAgent*, SpawnedAgent, FLinearColor, FactionColor);
/**
 * ASpawner spawns agents at intervals and broadcasts faction colors to them
 * Handles damage and destruction through HealthComponent integration
 * Usage: Place in level, configure AgentClassToSpawn and FactionColor in Blueprint
 */
UCLASS()
class END2507_API ASpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASpawner();
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;
	// Delegate broadcasted when an agent is spawned
	UPROPERTY(BlueprintAssignable, Category = "Spawner|Events")
	FOnAgentSpawned OnAgentSpawned;

	// Getter for spawned agent count
	UFUNCTION(BlueprintPure, Category = "Spawner")
	int32 GetSpawnedAgentCount() const;

	UPROPERTY(EditInstanceOnly, Category = "Spawning")
	bool bInfiniteSpawn;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	// ===== COMPONENTS =====

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* BoxCollision;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* BarrelMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UAC_HealthComponent* HealthComponent;
	// ===== SPAWNING PROPERTIES =====
	UPROPERTY(EditDefaultsOnly, Category = "Spawning", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<ABaseAgent> AgentClassToSpawn;

	UPROPERTY(EditDefaultsOnly, Category = "Spawning", meta = (ClampMin = "1.0", ClampMax = "60.0"))
	float SpawnInterval;
	UPROPERTY(EditDefaultsOnly, Category = "Spawning", meta = (ClampMin = "100.0", ClampMax = "500.0", AllowPrivateAccess = "true"))
	float SpawnRadius;
	UPROPERTY(EditDefaultsOnly, Category = "Spawning", meta = (ClampMin = "1", ClampMax = "10"))
	int32 MaxSpawnCount;
	
	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	FVector SpawnOffset;
	// HIT FLASH PROPERTIES 
	UPROPERTY(EditDefaultsOnly, Category = "Hit Reaction")
	TArray<FLinearColor> HitFlashColors;

	UPROPERTY(EditDefaultsOnly, Category = "Hit Reaction", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float FlashDuration;
	// ===== FACTION/TEAM CONFIGURATION =====

//Color assigned to spawned agents 
	UPROPERTY(EditInstanceOnly, Category = "Faction", meta = (AllowPrivateAccess = "true"))
	FLinearColor FactionColor;

	//Team ID for spawned agents 
	UPROPERTY(EditInstanceOnly, Category = "Faction", meta = (ClampMin = "0", ClampMax = "255", AllowPrivateAccess = "true"))
	uint8 TeamID;
private:
	// Dynamic material for color flashing
	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> DynamicMaterials;

	// Track original color for reversion
	FLinearColor OriginalColor;

	// Track spawn count
	int32  SpawnedAgentCount;

	// Timers
	FTimerHandle SpawnTimerHandle;
	FTimerHandle ColorRevertTimerHandle;

	// Track which hit color to use next
	int32 LastHitColorIndex;
	
	// Initialize components and materials
	void SetupBarrelAppearance();

	// Spawning logic
	void SpawnAgent();

	// Visual feedback
	void FlashHitColor();
	void RevertToOriginalColor();


	// Health component callbacks
	UFUNCTION()
	void OnHealthChanged(float HealthRatio);
	UFUNCTION()
	void OnDeath(AActor* DestroyedActor);
};
