// ABaseCharacter - Base class for player and AI characters
// Author: Marcus Daley
// Date: 10/6/2025
#include "Code/Actors/BaseCharacter.h"
#include "Code/AC_HealthComponent.h"
#include "Code/UI/AC_OverheadBarComponent.h"
#include "Code/Actors/BaseRifle.h"
#include "Both/CharacterAnimation.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "../END2507.h"
DEFINE_LOG_CATEGORY_STATIC(LogBaseCharacter, Log, All);
ABaseCharacter::ABaseCharacter()
	: OverheadBarComponent(nullptr)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	// Create health component
	HealthComponent = CreateDefaultSubobject<UAC_HealthComponent>(TEXT("HealthComponent"));
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		MeshComp->bPauseAnims = false;
		UE_LOG(LogBaseCharacter, Log, TEXT("Mesh tick options configured for death animations"));
	}
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> SkeletalMeshAsset(TEXT("/Script/Engine.SkeletalMesh'/Game/END_Starter/Mannequin/Meshes/SKM_Manny.SKM_Manny'"));

	if (SkeletalMeshAsset.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(SkeletalMeshAsset.Object);

		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
		GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	}
}
UAC_HealthComponent* ABaseCharacter::GetHealthComponent() const
{
	return HealthComponent;
}
void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
bool ABaseCharacter::CanPickHealth() const
{
	UE_LOG(LogBaseCharacter, Log, TEXT("[%s] CanPickHealth queried — returning false (AI restriction)"), *GetName());
	return true;
}
bool ABaseCharacter::CanPickAmmo() const
{
	return false;
}
void ABaseCharacter::AddMaxAmmo(int32 Amount)
{
	UE_LOG(LogBaseCharacter, Warning, TEXT("[%s] AddMaxAmmo called on AI — no-op"), *GetName());
}
ABaseRifle* ABaseCharacter::GetRifle() const
{
	return EquippedRifle;
}
void ABaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

FRotator ABaseCharacter::GetSpineTargetRotation() const
{
	return FRotator::ZeroRotator;
} // Override in child classes

UAnimSequence* ABaseCharacter::GetHitAsset() const
{
	return HitAsset;
}

TArray<UAnimSequence*> ABaseCharacter::GetDeathAssets() const
{
	return DeathAssets;
}

// Called when the game starts or when spawned
void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (!HealthComponent)
	{
		UE_LOG(LogBaseCharacter, Error, TEXT("%s: No HealthComponent!"), *GetName());
		return;
	}

	// Bind to health events
	HealthComponent->OnHealthChanged.AddDynamic(this, &ABaseCharacter::OnHealthChanged);
	HealthComponent->OnDeath.AddDynamic(this, &ABaseCharacter::OnDeath);

	// Create overhead bar for AI only (not player)
	bool bIsPlayerControlled = Cast<APlayerController>(GetController()) != nullptr;
	if (!bIsPlayerControlled)
	{
		OverheadBarComponent = NewObject<UAC_OverheadBarComponent>(this);
		OverheadBarComponent->RegisterComponent();
		UE_LOG(LogBaseCharacter, Display, TEXT("[%s] Overhead bar created (AI agent)"), *GetName());
	}
	else
	{
		UE_LOG(LogBaseCharacter, Display, TEXT("[%s] Overhead bar skipped (player)"), *GetName());
	}

	// Spawn and setup rifle
	SpawnAndAttachRifle();

	// Bind rifle delegates
	BindRifleDelegates();

	// Bind animation delegates
	BindAnimationDelegates();
}

void ABaseCharacter::OnHealthChanged(float HealthRatio)
{
	HandleHurt(HealthRatio);
}

void ABaseCharacter::HandleReloadStart()
{
	UCharacterAnimation* CharAnim = Cast<UCharacterAnimation>(GetMesh()->GetAnimInstance());
	if (!CharAnim)
	{
		UE_LOG(LogBaseCharacter, Warning, TEXT("[%s] No animation instance for reload"), *GetName());
		return;
	}

	// Call ReloadAnimationFunction which plays the montage
	CharAnim->ReloadAnimationFunction();
	UE_LOG(LogBaseCharacter, Log, TEXT("[%s] HandleReloadStart — Reload animation triggered"), *GetName());
}
//Delegate handler triggers actual ammo reload
void ABaseCharacter::HandleReloadNow()
{
	if (!EquippedRifle)
	{
		UE_LOG(LogBaseCharacter, Error, TEXT("[%s] Submarine malfunction — No rifle to reload!"), *GetName());
		return;
	}

	// Trigger actual ammo reload
	EquippedRifle->ReloadAmmo();
	UE_LOG(LogBaseCharacter, Log, TEXT("[%s] HandleReloadNow — Ammo refilled via AnimNotify"), *GetName());
}

void ABaseCharacter::HandleActionEnded()
{
	if (!EquippedRifle)
	{
		UE_LOG(LogBaseCharacter, Warning, TEXT("[%s] No rifle to reset action gate"), *GetName());
		return;
	}

	// Reset action gate
	EquippedRifle->ActionStopped();
	UE_LOG(LogBaseCharacter, Log, TEXT("[%s] HandleActionEnded — Action gate reset, rifle ready"), *GetName());
}

void ABaseCharacter::OnDeath(AActor* DestroyedActor)
{
	if (!DestroyedActor)
	{
		UE_LOG(LogBaseCharacter, Error, TEXT("OnDeath called with null actor!"));
		return;
	}

	UE_LOG(LogBaseCharacter, Warning, TEXT("%s died — Triggering death sequence"),
		*DestroyedActor->GetName());

	// Pass 0.0f as health ratio (character is dead)
	HandleDeathStart(0.0f);
}

void ABaseCharacter::SpawnAndAttachRifle()
{
	if (!RifleClass)
	{
		UE_LOG(LogBaseCharacter, Warning, TEXT("%s: No RifleClass set"), *GetName());
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;

	EquippedRifle = GetWorld()->SpawnActor<ABaseRifle>(RifleClass, SpawnParams);

	if (EquippedRifle)
	{
		EquippedRifle->AttachToComponent(GetMesh(),FAttachmentTransformRules::SnapToTargetIncludingScale,TEXT("RifleHand"));
		UE_LOG(LogBaseCharacter, Log, TEXT("%s: Rifle spawned"), *GetName());
	}
	else
	{
		UE_LOG(LogBaseCharacter, Error, TEXT("[%s] Failed to spawn rifle!"), *GetName());
	}
}

void ABaseCharacter::BindRifleDelegates()
{
	if (!EquippedRifle)
	{
		UE_LOG(LogBaseCharacter, Warning, TEXT("[%s] No rifle to bind delegates"), *GetName());
		return;
	}
	// Bind OnReloadStart → triggers reload animation
	EquippedRifle->OnReloadStart.AddDynamic(this, &ABaseCharacter::HandleReloadStart);
	UE_LOG(LogBaseCharacter, Log, TEXT("[%s] Rifle delegates bound successfully"), *GetName());

}

void ABaseCharacter::BindAnimationDelegates()
{
	UCharacterAnimation* CharAnim = Cast<UCharacterAnimation>(GetMesh()->GetAnimInstance());
	if (!CharAnim)
	{
		UE_LOG(LogBaseCharacter, Warning, TEXT("[%s] No CharacterAnimation instance"), *GetName());
		return;
	}
	// Bind OnReloadNow → triggers actual ammo reload
	CharAnim->OnReloadNow.AddDynamic(this, &ABaseCharacter::HandleReloadNow);
	// Bind OnActionEnded → resets bActionHappening in rifle
	CharAnim->OnActionEnded.AddDynamic(this, &ABaseCharacter::HandleActionEnded);

	UE_LOG(LogBaseCharacter, Log, TEXT("[%s] Animation delegates bound successfully"), *GetName());


}


void ABaseCharacter::HandleHurt(float Ratio)
{
	if (UCharacterAnimation* CharAnim = Cast<UCharacterAnimation>(GetMesh()->GetAnimInstance()))
	{
		CharAnim->HitAnimation(Ratio);
		UE_LOG(Game, Log, TEXT("%s: Playing hit animation"), *GetName());
	}
}
void ABaseCharacter::HandleDeathStart(float Ratio)
{
	UE_LOG(Game, Warning, TEXT("%s HandleDeadStart called with ratio %f"), *GetName(), Ratio);

	OnCharacterDeath.Broadcast();
	if (EquippedRifle)
	{
		UE_LOG(Game, Warning, TEXT("[%s] Destroying rifle: %s"), *GetName(), *EquippedRifle->GetName());
		EquippedRifle->Destroy();
		EquippedRifle = nullptr;
	}
	else
	{
		UE_LOG(Game, Warning, TEXT("[%s] No rifle to destroy on death"), *GetName());
	}
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetComponentTickEnabled(true);
		MeshComp->SetVisibility(true, true);
		MeshComp->bRecentlyRendered = true;  
		MeshComp->VisibilityBasedAnimTickOption =
			EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		MeshComp->bPauseAnims = false;
		UE_LOG(Game, Warning, TEXT("%s: Mesh forced visible + tick enabled"), *GetName());
	}

	// Disable input
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}
	FTimerHandle CollisionDisableTimer;
	GetWorldTimerManager().SetTimer(
		CollisionDisableTimer,
		[this]()
		{
			if (IsValid(this))  // ✓ SAFE: Check validity before access
			{
				SetActorEnableCollision(false);
				UE_LOG(Game, Warning, TEXT("%s: Collision disabled"), *GetName());
			}
		},
		0.2f,
		false
	);

	
	float DestructionDelay = 3.0f; // Default fallback

	if (DeathAssets.Num() > 0 && DeathAssets[0])
	{
		DestructionDelay = DeathAssets[0]->GetPlayLength() + 0.5f; // Animation + buffer
		UE_LOG(Game, Log, TEXT("%s: Destruction scheduled for %.2f seconds"), *GetName(), DestructionDelay);
	}
	TWeakObjectPtr<ABaseCharacter> WeakThis(this);
	FTimerHandle DeathTimer;
	GetWorldTimerManager().SetTimer(
		DeathTimer,
		[WeakThis]()  // ✓ Capture weak pointer instead of raw 'this'
		{
			if (WeakThis.IsValid())  // 
			{
				ABaseCharacter* Character = WeakThis.Get();
				UE_LOG(Game, Warning, TEXT("%s: Destroying after death animation"),
					*Character->GetName());
				Character->Destroy();
			}
			else
			{
				UE_LOG(Game, Warning, TEXT("Death timer fired but character already destroyed"));
			}
		},
		DestructionDelay,
		false
	);
}