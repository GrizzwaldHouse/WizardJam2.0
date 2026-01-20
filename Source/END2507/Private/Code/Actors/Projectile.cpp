// AProjectile - Projectile actor with color and damage
// Author: Marcus Daley
// Date: 9/29/2025
#include "Code/Actors/Projectile.h"
#include "Code/Actors/BaseAgent.h"
#include "Code/Actors/BaseCharacter.h"
#include "Code/Actors/HideWall.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Code/Actors/BasePlayer.h"
#include "Code/Actors/Spawner.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Code/AC_HealthComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/DamageEvents.h"
#include "../END2507.h"
DEFINE_LOG_CATEGORY_STATIC(LogProjectile, Log, All);
const FName AProjectile::ColorParamName = FName(TEXT("Color"));
// Sets default values
AProjectile::AProjectile() :
	Size(0.18f, 0.18f, 0.18f)
, Damage(10.0f)
, ProjectileColor(FLinearColor::Blue)
, SpellElement(NAME_None)
, CombatFaction(ECombatFaction::Unknown)
, OwnerPawn(nullptr)
, bMaterialInitialized(false)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	// Collision sphere
	SphereCollision = CreateDefaultSubobject<USphereComponent>("SphereCollision");
	SetRootComponent(SphereCollision);
	SphereCollision->SetWorldScale3D(Size);

	
	SphereCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);  // Overlap only
	SphereCollision->SetCollisionObjectType(ECC_GameTraceChannel1);  // Projectile channel
	SphereCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SphereCollision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	SphereCollision->SetCollisionProfileName(TEXT("Projectile"));  
	SphereCollision->SetGenerateOverlapEvents(true);
	// Mesh 
	SphereMesh = CreateDefaultSubobject<UStaticMeshComponent>("SphereMesh");
	SphereMesh->SetCollisionProfileName("NoCollision");
	SphereMesh->SetRelativeScale3D(FVector(0.6f, 0.6f, 0.6f)); 
	SphereMesh->SetupAttachment(SphereCollision);

	
	SphereCollision->OnComponentBeginOverlap.AddDynamic(this, &AProjectile::HandleOverlap);
	

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement")); 
	ProjectileMovement->InitialSpeed = 1500;
	ProjectileMovement->MaxSpeed = 3000;
	ProjectileMovement->UpdatedComponent = SphereCollision;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->bRotationFollowsVelocity = true; // Make the projectile rotate with its velocity
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	InitialLifeSpan = 5.0f;

}


void AProjectile::InitializeProjectile(float InDamage, const FVector& Direction, ECombatFaction Faction)
{
	
	Damage = InDamage;
	CombatFaction = Faction;
	//Set velocity to move the projectile
	if (ProjectileMovement)
	{
		ProjectileMovement->Velocity = Direction.GetSafeNormal() * ProjectileMovement->InitialSpeed;
		UE_LOG(LogProjectile, Log, TEXT("[%s] Velocity set: %s (Speed: %.1f)"),
			*GetName(), *ProjectileMovement->Velocity.ToString(), ProjectileMovement->Velocity.Size());
	}
	
	// Set random color
	TArray<FLinearColor> Colors = {
			FLinearColor::Red,
			FLinearColor::Green,
			FLinearColor(0.0f, 0.5f, 1.0f),    // Bright Blue
			FLinearColor::Yellow,
			FLinearColor(1.0f, 0.0f, 1.0f),    // Magenta
			FLinearColor(0.0f, 1.0f, 1.0f),    // Cyan
			FLinearColor(1.0f, 0.5f, 0.0f),    // Orange
			FLinearColor(0.5f, 0.0f, 1.0f)     // Purple
	};
	int32 RandomIndex = FMath::RandRange(0, Colors.Num() - 1);
	SetColor(Colors[RandomIndex]);
	UE_LOG(LogProjectile, Log, TEXT("[%s] Initialized | Damage: %.1f | Faction: %s"),
		*GetName(), InDamage, Faction == ECombatFaction::Player ? TEXT("Player") : TEXT("Agent"));

}

void AProjectile::SetPoolReturnCallback(TFunction<void(AProjectile*)> Callback)
{
	PoolReturnCallback = Callback;
}

void AProjectile::PostInitializeComponents()
{
	Super::PostInitializeComponents();


	// Create dynamic material
	if (!bMaterialInitialized && SphereMesh)
	{
		UMaterialInterface* BaseMaterial = SphereMesh->GetMaterial(0);
		if (BaseMaterial && !BaseMaterial->IsA(UMaterialInstanceDynamic::StaticClass()))
		{
			// Create from base material only
			DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			SphereMesh->SetMaterial(0, DynamicMaterial);
			bMaterialInitialized = true;

			UE_LOG(LogProjectile, Log, TEXT("PostInitializeComponents: Created dynamic material"));
		}
	}
}



// Called when the game starts or when spawned
void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	if (ProjectileMovement && SphereCollision)
		
	{
		
		UE_LOG(LogProjectile, Warning, TEXT("[%s] Collision: %s | Profile: %s | OverlapEvents: %d"),
			*GetName(),
			SphereCollision->GetCollisionEnabled() == ECollisionEnabled::QueryOnly ? TEXT("QueryOnly") :
			SphereCollision->GetCollisionEnabled() == ECollisionEnabled::NoCollision ? TEXT("DISABLED") : TEXT("Other"),
			*SphereCollision->GetCollisionProfileName().ToString(),
			SphereCollision->GetGenerateOverlapEvents());
		ProjectileMovement->UpdatedComponent = SphereCollision;
		UE_LOG(LogProjectile, Warning, TEXT("[%s] OnComponentBeginOverlap bound: %d"),
			*GetName(), SphereCollision->OnComponentBeginOverlap.IsBound());
		UE_LOG(LogProjectile, Warning, TEXT("[%s] UpdatedComponent set: %d"),
			+*GetName(), ProjectileMovement && ProjectileMovement->UpdatedComponent == SphereCollision);
	}
	UE_LOG(LogProjectile, Log, TEXT("[%s] Projectile spawned | Damage: %.1f | Owner: %s"),
		*GetName(), Damage, GetOwner() ? *GetOwner()->GetName() : TEXT("None"));
}


// Called every frame
void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
void AProjectile::SetColor(const FLinearColor& NewColor)
{
	ProjectileColor = NewColor;

	// Ensure dynamic material exists
	if (!DynamicMaterial && SphereMesh)
	{
		UMaterialInterface* BaseMaterial = SphereMesh->GetMaterial(0);
		if (BaseMaterial && !BaseMaterial->IsA<UMaterialInstanceDynamic>())
		{
			DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			SphereMesh->SetMaterial(0, DynamicMaterial);
		}
	}

	// Apply color to material
	if (DynamicMaterial)
	{
		DynamicMaterial->SetVectorParameterValue(ColorParamName, NewColor);
		UE_LOG(LogProjectile, Log, TEXT("[%s] SetColor: (%.2f, %.2f, %.2f)"),
			*GetName(), NewColor.R, NewColor.G, NewColor.B);
	}
	else
	{
		UE_LOG(LogProjectile, Error, TEXT("[%s] SetColor FAILED: DynamicMaterial is null!"), *GetName());

	}
}
void AProjectile::ActivateProjectile(const FVector& SpawnLocation, const FRotator& SpawnRotation, const FVector& Direction, float InDamage, AActor* NewOwner, ECombatFaction Faction)
{
	if (!NewOwner)
	{
		UE_LOG(LogProjectile, Error, TEXT("[%s] ActivateProjectile: NewOwner is NULL!"), *GetName());
		return;
	}
	// Reset state
	SetActorLocationAndRotation(SpawnLocation, SpawnRotation);
	SetOwner(NewOwner);
	SetInstigator(Cast<APawn>(NewOwner));
	
	//Assign faction and owner pawn
	CombatFaction = Faction;
	OwnerPawn = NewOwner;
	Damage = InDamage;
	// Re-enable collision and visibility
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);

	if (ProjectileMovement)
	{
		ProjectileMovement->Velocity = Direction.GetSafeNormal() * ProjectileMovement->InitialSpeed;
		ProjectileMovement->Activate();
	}

	//bActiveInPool = true;
	// Set random color
	TArray<FLinearColor> Colors = {
		FLinearColor::Red, FLinearColor::Green, FLinearColor(0.0f, 0.5f, 1.0f),
		FLinearColor::Yellow, FLinearColor(1.0f, 0.0f, 1.0f), FLinearColor(0.0f, 1.0f, 1.0f),
		FLinearColor(1.0f, 0.5f, 0.0f), FLinearColor(0.5f, 0.0f, 1.0f)
	};
	SetColor(Colors[FMath::RandRange(0, Colors.Num() - 1)]);
	GetWorldTimerManager().ClearTimer(ExpirationTimer);
	//GetWorldTimerManager().SetTimer(ExpirationTimer, [this]()
	//	{
	//		if (bActiveInPool)
	//		{
	//			UE_LOG(LogProjectile, Log, TEXT("[%s] Expired - returning to pool"), *GetName());
	//			//DeactivateProjectile();
	//		}
	//	}, 5.0f, false);

	UE_LOG(LogProjectile, Warning, TEXT("[%s] Spawned NEW projectile | Damage: %.1f | Velocity: %s | Faction: %s"),
		*GetName(), Damage, *ProjectileMovement->Velocity.ToString(),
		CombatFaction == ECombatFaction::Player ? TEXT("Player") : TEXT("Agent"));
}

//void AProjectile::HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& hit)
//{
//	// Ignore self-hits and owner hitse
//		if (!OtherActor || OtherActor == GetOwner() || OtherActor == GetInstigator())
//		{
//			
//			return;
//		}
//
//	// Ignore other projectiles
//	if (OtherActor->IsA<AProjectile>())
//	{
//		UE_LOG(LogProjectile, Log, TEXT("[%s] Ignoring hit with another projectile"), *GetName());
//		return;
//	}
//	
//	
////Check faction before damage
//		ECombatFaction TargetFaction = ECombatFaction::Unknown;
//		if (OtherActor->IsA<ABasePlayer>())
//		{
//			TargetFaction = ECombatFaction::Player;
//		}
//		else if (OtherActor->IsA<ABaseAgent>())
//		{
//			TargetFaction = ECombatFaction::Agent;
//		}
//		// Ignore same-faction hits 
//		if (TargetFaction != ECombatFaction::Unknown && TargetFaction == CombatFaction)
//		{
//			UE_LOG(LogProjectile, Warning, TEXT("[%s] Ignored: Same faction hit (%s vs %s)"),
//				*GetName(),
//				CombatFaction == ECombatFaction::Player ? TEXT("Player") : TEXT("Agent"),
//				TargetFaction == ECombatFaction::Player ? TEXT("Player") : TEXT("Agent"));
//			Destroy();
//			return;
//		}
//		UE_LOG(LogProjectile, Warning, TEXT("[%s] HandleHit: Hit %s (Faction: %s)"),
//			*GetName(), *OtherActor->GetName(),
//			TargetFaction == ECombatFaction::Player ? TEXT("Player") : TEXT("Agent"));
//
//	// Apply damage before destroying
//	ApplyDamageToActor(OtherActor);
//	// Return to pool or destroy
//	if (bActiveInPool)
//	{
//		//DeactivateProjectile();
//	}
//	
//
//		Destroy();
//	
//
//}

void AProjectile::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int OtherBodyIndex, bool fromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this || OtherActor == OwnerPawn.Get())
	{
		
		return;
	}

	if (OtherActor->IsA<AHideWall>())
	{
		// Wall's OnHit will
		UE_LOG(LogProjectile, Log, TEXT("[%s] Hit HideWall: %s"), *GetName(), *OtherActor->GetName());
		Destroy();
		return;
	}
	if (ASpawner* HitSpawner = Cast<ASpawner>(OtherActor))
	{
		// Use FDamageEvent for proper damage application
		FDamageEvent DamageEvent;

		HitSpawner->TakeDamage(Damage, DamageEvent, nullptr, this);

		UE_LOG(LogProjectile, Log, TEXT("Projectile hit spawner [%s] with %.1f damage"),
			*HitSpawner->GetName(), Damage);

		Destroy();
		return;
	}

	// Skip friendly fire (existing code)
	if (IsFriendly(OtherActor))
	{
		return;
	}

	// Apply damage to other actors (existing code)
	ApplyDamageToActor(OtherActor);
	Destroy();

}

void AProjectile::ApplyDamageToActor(AActor* HitActor)
{
if (!HitActor) return;

// Check if hit a HideWall and notify it for color change
AHideWall* HitWall = Cast<AHideWall>(HitActor);
if (HitWall)
{
	HitWall->OnHitByProjectile(this, Damage);
	UE_LOG(LogProjectile, Warning, TEXT("[%s] Applied %.1f damage to HideWall: %s"),
		*GetName(), Damage, *HitActor->GetName());
	return;
}
	HitActor->TakeDamage(Damage, FDamageEvent(), GetInstigatorController(), this);

	UE_LOG(LogProjectile, Warning, TEXT("[%s] Applied %.1f damage to %s"),
		*GetName(), Damage, *HitActor->GetName());

  /*  UE_LOG(LogProjectile, Warning, TEXT("Applied %.1f damage to %s"), Damage, *HitActor->GetName());
	UAC_HealthComponent* HealthComp = HitActor->FindComponentByClass<UAC_HealthComponent>();
	if (HealthComp)
	{
		HealthComp->HandleTakeAnyDamage(HitActor, Damage, nullptr, GetInstigatorController(), this);
		UE_LOG(LogProjectile, Warning, TEXT("[%s] Applied %.1f damage to %s | Health: %.1f/%.1f"),
			*GetName(), Damage, *HitActor->GetName(),
			HealthComp->GetCurrentHealth(), HealthComp->GetMaxHealth());
	}
	else
	{
		UE_LOG(LogProjectile, Warning, TEXT("[%s] Hit %s has no HealthComponent"), *GetName(), *HitActor->GetName());
	}*/
}

bool AProjectile::IsFriendly(AActor* OtherActor) const
{
	if (!OtherActor || !OwnerPawn.IsValid())
	{
		return false; // Unknown = not friendly
	}

	// Self-hit check
	if (OtherActor == OwnerPawn.Get() || OtherActor == GetInstigator())
	{
		return true; // Owner is friendly
	}
	//use simple owner comparison if both are agents or both are players
	bool bOwnerIsAgent = OwnerPawn->IsA<ABaseAgent>();
	bool bOtherIsAgent = OtherActor->IsA<ABaseAgent>();
	if (bOwnerIsAgent == bOtherIsAgent)
	{
		return true;
	}
	return false;
}


bool AProjectile::IsActiveInPool() const
{
	return bActiveInPool;
}
ECombatFaction AProjectile::GetCombatFaction()const {
	return CombatFaction;
}
AActor* AProjectile::GetOwnerPawn() const
{
	return OwnerPawn.Get();
}
//void AProjectile::ActivateProjectile(const FVector& SpawnLocation, const FRotator& SpawnRotation,
//	const FVector& Direction, float InDamage, AActor* NewOwner)
//{
//	// Reset state
//	SetActorLocationAndRotation(SpawnLocation, SpawnRotation);
//	SetOwner(NewOwner);
//	SetInstigator(Cast<APawn>(NewOwner));
//	Damage = InDamage;
//
//	// Re-enable collision and visibility
//	SetActorHiddenInGame(false);
//	SetActorEnableCollision(true);
//
//	if (ProjectileMovement)
//	{
//		ProjectileMovement->Velocity = Direction.GetSafeNormal() * ProjectileMovement->InitialSpeed;
//		ProjectileMovement->Activate();
//	}
//
//	bActiveInPool = true;
//
//	// Set random color
//	TArray<FLinearColor> Colors = {
//		FLinearColor::Red, FLinearColor::Green, FLinearColor(0.0f, 0.5f, 1.0f),
//		FLinearColor::Yellow, FLinearColor(1.0f, 0.0f, 1.0f), FLinearColor(0.0f, 1.0f, 1.0f),
//		FLinearColor(1.0f, 0.5f, 0.0f), FLinearColor(0.5f, 0.0f, 1.0f)
//	};
//	SetColor(Colors[FMath::RandRange(0, Colors.Num() - 1)]);
//	GetWorldTimerManager().ClearTimer(ExpirationTimer);
//	GetWorldTimerManager().SetTimer(ExpirationTimer, [this]()
//		{
//			if (bActiveInPool)
//			{
//				UE_LOG(LogProjectile, Log, TEXT("[%s] Expired - returning to pool"), *GetName());
//				DeactivateProjectile();
//			}
//		}, 5.0f, false);
//
//	UE_LOG(LogProjectile, Warning, TEXT("[%s] ActivateProjectile | Damage: %.1f | Velocity: %s"),
//		*GetName(), Damage, *ProjectileMovement->Velocity.ToString());
//
//}
//void AProjectile::DeactivateProjectile()
//{
//	SetActorHiddenInGame(true);
//	SetActorEnableCollision(false);
//
//	if (ProjectileMovement)
//	{
//		ProjectileMovement->Velocity = FVector::ZeroVector;
//		ProjectileMovement->Deactivate();
//	}
//
//	bActiveInPool = false;
//	if (PoolReturnCallback)
//		{
//		PoolReturnCallback(this);
//		}
//	UE_LOG(LogProjectile, Log, TEXT("[%s] DeactivateProjectile - returned to pool"), *GetName());
//}
