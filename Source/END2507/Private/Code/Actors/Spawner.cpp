
#include "Code/Actors/Spawner.h"
#include "Components/StaticMeshComponent.h"
#include "Code/CodeFactionInterface.h"
#include "Code/AC_HealthComponent.h"
#include "Components/BoxComponent.h"
#include "Code/Actors/BaseAgent.h"
#include "Code/Actors/AIC_CodeBaseAgentController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "Code/CodeGameModeBase.h"
#include "../END2507.h"



DEFINE_LOG_CATEGORY_STATIC(LogSpawner, Log, All);

// Sets default values
ASpawner::ASpawner() : MaxSpawnCount(3)
, bInfiniteSpawn(false)
, SpawnInterval(10.0f)
, SpawnRadius(200.0f)
, FactionColor(FLinearColor::Red)
, TeamID(1)
, SpawnOffset(FVector(200.0f, 0.0f, 0.0f))
, FlashDuration(0.3f)
, SpawnedAgentCount(0)
, LastHitColorIndex(-1)
{
 	
	PrimaryActorTick.bCanEverTick = false;

	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
	RootComponent = BoxCollision;
	BoxCollision->SetBoxExtent(FVector(50.0f, 50.0f, 50.0f)); // Default 100x100x100 box
	BoxCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BoxCollision->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	BoxCollision->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	BoxCollision->SetGenerateOverlapEvents(true);
	BoxCollision->SetMobility(EComponentMobility::Movable);
	// Create barrel mesh attached to box collision
	BarrelMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BarrelMesh"));
	BarrelMesh->SetupAttachment(RootComponent);
	BarrelMesh->SetMobility(EComponentMobility::Movable);

	// Configure mesh collision - can use NoCollision since BoxCollision handles it
	BarrelMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BarrelMesh->SetRelativeLocation(FVector::ZeroVector);
	BarrelMesh->SetRelativeRotation(FRotator::ZeroRotator);
	// Create health component
	HealthComponent = CreateDefaultSubobject<UAC_HealthComponent>(TEXT("HealthComponent"));
	
	// Default hit colors
		HitFlashColors.Add(FLinearColor::Red);
	HitFlashColors.Add(FLinearColor::Yellow);
	HitFlashColors.Add(FLinearColor(1.0f, 0.5f, 0.0f)); // Orange
}

int32 ASpawner::GetSpawnedAgentCount() const
{
	return SpawnedAgentCount;
}
// Called when the game starts or when spawned
void ASpawner::BeginPlay()
{
	Super::BeginPlay();


	if (!BoxCollision)
	{
		UE_LOG(LogSpawner, Error, TEXT("[%s] BoxCollision is nullptr!"), *GetName());
		return;
	}
	if (!BarrelMesh)
	{
		UE_LOG(LogSpawner, Error, TEXT("[%s] BarrelMesh is nullptr!"), *GetName());
		return;
	}

	// Setup appearance
	SetupBarrelAppearance();

	// Bind health component delegates
	if (HealthComponent)
	{
		HealthComponent->OnHealthChanged.AddDynamic(this, &ASpawner::OnHealthChanged);
		HealthComponent->OnDeath.AddDynamic(this, &ASpawner::OnDeath);
		UE_LOG(LogSpawner, Log, TEXT("[%s] Health component bound with %.0f HP"),
			*GetName(), HealthComponent->GetCurrentHealth());
	}
	else
	{
		UE_LOG(LogSpawner, Error, TEXT("[%s] HealthComponent is nullptr!"), *GetName());
	}
	UE_LOG(LogSpawner, Log, TEXT("[%s] BoxCollision extent: %s"),
		*GetName(), *BoxCollision->GetScaledBoxExtent().ToString());

	// Start spawn timer
	if (AgentClassToSpawn)
	{
		GetWorldTimerManager().SetTimer(
			SpawnTimerHandle,
			this,
			&ASpawner::SpawnAgent,
			SpawnInterval,
			true,  // Loop
			SpawnInterval  // Initial delay
		);
		UE_LOG(LogSpawner, Log, TEXT("[%s] Spawn timer started - Interval: %.1fs, Max: %d"),
			*GetName(), SpawnInterval, MaxSpawnCount);
	}
	else
	{
		UE_LOG(LogSpawner, Warning, TEXT("[%s] No AgentClassToSpawn assigned!"), *GetName());
	}
}

void ASpawner::SetupBarrelAppearance()
{
	if (!BarrelMesh)
	{
		return;
	}

	// Create dynamic material instance
	UMaterialInterface* Material = BarrelMesh->GetMaterial(0);
	if (Material)
	{
		UMaterialInstanceDynamic* DynMat = BarrelMesh->CreateDynamicMaterialInstance(0, Material);
		if (DynMat)
		{
			DynamicMaterials.Add(DynMat);

			// Store original color from the first material
			FLinearColor CurrentColor;
			if (DynMat->GetVectorParameterValue(FName("Color"), CurrentColor))
			{
				OriginalColor = CurrentColor;
			}
			else
			{
				OriginalColor = FLinearColor::White;
			}
			UE_LOG(LogSpawner, Log, TEXT("[%s] Dynamic material created"), *GetName());
		}
	}
}

void ASpawner::SpawnAgent()
{
	// Check spawn limit
	if (!bInfiniteSpawn && SpawnedAgentCount >= MaxSpawnCount)
	{
		UE_LOG(LogSpawner, Warning, TEXT("[%s] Max spawn count reached (%d/%d)"),
			*GetName(), SpawnedAgentCount, MaxSpawnCount);
		GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
		return;
	}

	if (!AgentClassToSpawn)
	{
		UE_LOG(LogSpawner, Error, TEXT("[%s] AgentClassToSpawn is nullptr!"), *GetName());
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}


	// Calculate spawn location with random offset
	const FVector SpawnLocation = GetActorLocation() + SpawnOffset +
		FVector(FMath::RandRange(-SpawnRadius, SpawnRadius),
			FMath::RandRange(-SpawnRadius, SpawnRadius),
			0.0f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ABaseAgent* SpawnedAgent = GetWorld()->SpawnActor<ABaseAgent>(
		AgentClassToSpawn,
		SpawnLocation,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (SpawnedAgent)
	{
		SpawnedAgentCount++;
		UE_LOG(LogTemp, Display, TEXT("Spawner: Spawned agent #%d at location %s"),
			SpawnedAgentCount, *SpawnLocation.ToString());

		// Broadcast the standard event for UI/tracking purposes
		OnAgentSpawned.Broadcast(SpawnedAgent, FactionColor);

		// Use interface to assign faction data (observer pattern)
		if (SpawnedAgent->Implements<UCodeFactionInterface>())
		{
			ICodeFactionInterface::Execute_OnFactionAssigned(SpawnedAgent, TeamID, FactionColor);
			UE_LOG(LogTemp, Display, TEXT("Spawner: Assigned faction ID=%d to agent via interface"), TeamID);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Spawner: Spawned agent does not implement UCodeFactionInterface"));
		}
		ACodeGameModeBase* GameMode = Cast<ACodeGameModeBase>(GetWorld()->GetAuthGameMode());
		if (GameMode)
		{
			// Bind spawned agent's OnDestroyed to GameMode's RemoveEnemy
			SpawnedAgent->OnDestroyed.AddDynamic(GameMode, &ACodeGameModeBase::RemoveEnemy);
			UE_LOG(LogTemp, Warning, TEXT("★ Spawned agent registered with GameMode for win tracking"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to get GameMode — spawned agent NOT tracked for win condition!"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Spawner: Failed to spawn agent"));
	}
}
float ASpawner::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	// Call parent implementation - this triggers OnTakeAnyDamage delegate
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	UE_LOG(LogSpawner, Log, TEXT("[%s] TakeDamage called with %.1f damage from [%s]"),
		*GetName(), DamageAmount, DamageCauser ? *DamageCauser->GetName() : TEXT("Unknown"));

	// Visual feedback
	FlashHitColor();

	return ActualDamage;
}
void ASpawner::FlashHitColor()
{
	if (DynamicMaterials.Num() == 0 || HitFlashColors.Num() == 0)
	{
		return;
	}

	// Cycle through hit colors
	LastHitColorIndex = (LastHitColorIndex + 1) % HitFlashColors.Num();
	const FLinearColor& FlashColor = HitFlashColors[LastHitColorIndex];

	// Apply flash color to all stored materials
	for (UMaterialInstanceDynamic* DynMat : DynamicMaterials)
	{
		if (DynMat)
		{
			DynMat->SetVectorParameterValue(FName("Color"), FlashColor);
		}
	}
	UE_LOG(LogSpawner, Log, TEXT("[%s] Flashing color: R:%.2f G:%.2f B:%.2f"),
		*GetName(), FlashColor.R, FlashColor.G, FlashColor.B);

	// Clear existing timer
	GetWorldTimerManager().ClearTimer(ColorRevertTimerHandle);

	// Set timer to revert color
	GetWorldTimerManager().SetTimer(
		ColorRevertTimerHandle,
		this,
		&ASpawner::RevertToOriginalColor,
		FlashDuration,
		false
	);
}
void ASpawner::RevertToOriginalColor()
{
	if (DynamicMaterials.Num() > 0)
	{
		for (UMaterialInstanceDynamic* DynMat : DynamicMaterials)
		{
			if (DynMat)
			{
				DynMat->SetVectorParameterValue(FName("Color"), OriginalColor);
			}
		}
		UE_LOG(LogSpawner, Log, TEXT("[%s] Color reverted to original"), *GetName());
	}
}
void ASpawner::OnHealthChanged(float HealthRatio)
{
	UE_LOG(LogSpawner, Log, TEXT("[%s] Health ratio: %.0f%%"), *GetName(), HealthRatio * 100.0f);
}

void ASpawner::OnDeath(AActor* DestroyedActor)
{
	UE_LOG(LogSpawner, Display, TEXT("[%s] Spawner destroyed!"), *GetName());

	// Clear timers
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	GetWorldTimerManager().ClearTimer(ColorRevertTimerHandle);
	
	// Destroy will trigger OnDestroyed delegate that GameMode is bound to
	Destroy();
}
