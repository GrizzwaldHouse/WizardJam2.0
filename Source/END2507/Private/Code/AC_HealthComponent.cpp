// Fill out your copyright notice in the Description page of Project Settings.


#include "Code/AC_HealthComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "TimerManager.h"
#include "Code/Actors/BaseCharacter.h"
#include "../END2507.h"

DEFINE_LOG_CATEGORY_STATIC(LogHealthComponent, Log, All);
// Sets default values for this component's properties
UAC_HealthComponent::UAC_HealthComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	MaxHealth = 100.f;
	CurrentHealth = MaxHealth;
	bIsDead = false;
	
}

float UAC_HealthComponent::GetMaxHealth() const
{
	return MaxHealth;
}

float UAC_HealthComponent::GetCurrentHealth() const
{
	return CurrentHealth;
}

float UAC_HealthComponent::GetHealthRatio() const
{
	return MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f;
}

void UAC_HealthComponent::SetMaxHealth(float NewMaxHealth)
{
	if (NewMaxHealth <= 0.0f)
	{
		UE_LOG(LogHealthComponent, Warning, TEXT("Attempted to set invalid MaxHealth: %.1f"), NewMaxHealth);
		return;
	}

	MaxHealth = FMath::Max(0.0f, NewMaxHealth);
	// Clamp current health to new max
	CurrentHealth = FMath::Clamp(CurrentHealth, 0.0f, MaxHealth);

	UE_LOG(LogHealthComponent, Log, TEXT("MaxHealth set to %.1f"), MaxHealth);
}

void UAC_HealthComponent::SetCurrentHealth(float NewHealth)
{
	if (bIsDead)
	{
		return;
	}

	float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(NewHealth, 0.0f, MaxHealth);

	if (CurrentHealth != OldHealth)
	{
		float HealthRatio = GetHealthRatio();
		OnHealthChanged.Broadcast(HealthRatio);

		UE_LOG(LogHealthComponent, Log, TEXT("Health changed from %.1f to %.1f"), OldHealth, CurrentHealth);

		if (CurrentHealth <= 0.0f)
		{
			Die();
		}
	}
}

void UAC_HealthComponent::Heal(float Amount)
{
	
	float OldHealth = CurrentHealth;
	float NewHealth = FMath::Clamp(CurrentHealth + Amount, 0.0f, MaxHealth);
	CurrentHealth = NewHealth;

	// Broadcast health change but DON'T trigger damage effects
	OnHealed.Broadcast(CurrentHealth, MaxHealth, CurrentHealth / MaxHealth);

	UE_LOG(LogHealthComponent, Log, TEXT("Healed from %.1f to %.1f (Amount: %.1f)"),
		OldHealth, NewHealth, Amount);
}

// Called when the game starts
void UAC_HealthComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
	bIsDead = false;
	AActor* Owner = GetOwner();
	if (!Owner) {
		UE_LOG(LogHealthComponent, Error, TEXT("Health Component not set"));
	}
	UE_LOG(LogHealthComponent, Log, TEXT("%s: Health component initialized with %.1f health"), *GetOwner()->GetName(), MaxHealth);
	// Bind to damage delegate only in game world
	if (Owner->GetWorld() && Owner->GetWorld()->IsGameWorld())
	{
		// Clear existing bindings to prevent duplicates
		Owner->OnTakeAnyDamage.RemoveDynamic(this, &UAC_HealthComponent::HandleTakeAnyDamage);
		Owner->OnTakeAnyDamage.AddDynamic(this, &UAC_HealthComponent::HandleTakeAnyDamage);
	}

}

void UAC_HealthComponent::Die()
{
    if (bIsDead)
    {
        return;
    }

    bIsDead = true;
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		UE_LOG(LogHealthComponent, Error, TEXT(" Die() called but no owner exists!"));
		return;
	}

	// Broadcast immediate death event (for stopping movement, AI, etc.)
	OnDeath.Broadcast(Owner);
	UE_LOG(LogHealthComponent, Warning, TEXT("%s: Died! OnDeath broadcasted"), *Owner->GetName());

	// Unbind damage delegate
		Owner->OnTakeAnyDamage.RemoveDynamic(this, &UAC_HealthComponent::HandleTakeAnyDamage);
		float DeathAnimDuration = 2.0f; // Default fallback

		if (ABaseCharacter* Character = Cast<ABaseCharacter>(Owner))
		{
			TArray<UAnimSequence*> DeathAssets = Character->GetDeathAssets();
			if (DeathAssets.Num() > 0 && DeathAssets[0])
			{
				DeathAnimDuration = DeathAssets[0]->GetPlayLength();
				UE_LOG(LogHealthComponent, Log, TEXT("Using death animation duration: %.2fs"), DeathAnimDuration);
			}
		}

		// Broadcast OnDeathEnded after animation completes
		// This allows GameMode to wait for death animation before showing results screen
		if (UWorld* World = GetWorld())
		{
			FTimerHandle DeathEndedTimerHandle;
			World->GetTimerManager().SetTimer(
				DeathEndedTimerHandle,
				[this, Owner]()
				{
					if (IsValid(Owner))
					{
						OnDeathEnded.Broadcast(Owner);
						UE_LOG(LogHealthComponent, Warning, TEXT("%s: Death animation complete! OnDeathEnded broadcasted"), *Owner->GetName());
					}
				},
				DeathAnimDuration,
				false
			);
		}
}



void UAC_HealthComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamgeCauser)
{
	if ( bIsDead)
	{
		return; // No damage or already dead
	}
	// Apply damage to current health using clamp
	CurrentHealth = FMath::Clamp(CurrentHealth - Damage, 0.0f, MaxHealth); // Ensure health does not go below 0 or above max health
	float HealthRatio = GetHealthRatio();

	UE_LOG(Game, Log, TEXT("%s took %f damage. Current health: %f"), *DamagedActor->GetName(), Damage, CurrentHealth);
	if (CurrentHealth > 0.0f)
	{
		OnHealthChanged.Broadcast(HealthRatio); // Send health ratio for hurt
		UE_LOG(Game, Log, TEXT("%s hurt event broadcasted with ratio %f"), *DamagedActor->GetName(), HealthRatio);
	}
	else
	{
		Die(); // Handle death
	}
}

