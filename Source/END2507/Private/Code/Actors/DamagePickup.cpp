// Fill out your copyright notice in the Description page of Project Settings.


#include "Code/Actors/DamagePickup.h"
#include "Components/BoxComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/DamageEvents.h"
#include "../END2507.h"

DEFINE_LOG_CATEGORY_STATIC(LogDamagePickup, Log, All);
ADamagePickup::ADamagePickup()
{
	// Create fire particle system component
	ParticleSystem = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleSystem"));
	ParticleSystem->SetupAttachment(RootComponent);
	
	// Position particle at ground level (Z = -30)
	ParticleSystem->SetRelativeLocation(FVector(0.0f, 0.0f, -30.0f));
	// Scale particle down to match pickup size
	ParticleSystem->SetRelativeScale3D(FVector(0.4f, 0.4f, 0.25f));

	ParticleSystem->bAutoActivate = true;
	// Scale collision box to 1.6 uniformly
	if (UBoxComponent* BoxCollisionObject = GetBoxCollision())
	{
		BoxCollisionObject->SetRelativeScale3D(FVector(1.6f, 1.6f, 1.6f));
	}

	// Set damage amount 
	DamageAmount = 2.0f;
	// Particle template will be set in Blueprint child class (BP_DamagePickup)
	
	UE_LOG(LogDamagePickup, Log, TEXT("[%s] DamageAmount set to %.1f"),
		*GetName(), DamageAmount);
}
float ADamagePickup::GetDamageAmount() const
{
	return DamageAmount;
}
void ADamagePickup::BeginPlay()
{
	Super::BeginPlay();
	//Apply particle template if set in blueprint
	if(!ParticleSystem)
	{
		UE_LOG(LogDamagePickup, Error,
			TEXT("[%s] ParticleSystem component is NULL!"),
			*GetName());
		return;
	}
	if (!ParticleSystem->Template)
	{
		UE_LOG(LogDamagePickup, Error,
			TEXT("[%s] Submarine fire trap inactive — No particle template assigned in Blueprint! "
				"Solution: Open BP_DamagePickup → Select ParticleSystem component → "
				"Set Template property to P_Fire"),
			*GetName());
		return;
	}

	ParticleSystem->Activate(true);
	ParticleSystem->SetVisibility(true);


	UE_LOG(LogDamagePickup, Warning,
		TEXT("[%s] Fire trap ARMED — Particle template: [%s] at path: [%s]"),
		*GetName(),
		*ParticleSystem->Template->GetName(),
		*ParticleSystem->Template->GetPathName());

}
void ADamagePickup::HandlePickup(AActor* OtherActor)
{
	if (!OtherActor)
	{
		UE_LOG(LogDamagePickup, Error, TEXT("[%s] HandlePickup called with nullptr!"), *GetName());
		return;
	}

	UE_LOG(LogDamagePickup, Warning, TEXT("[%s] Applying %.1f damage to [%s]!"),
		*GetName(), DamageAmount, *OtherActor->GetName());

	
	// This triggers the actor's OnTakeAnyDamage delegate → HealthComponent::HandleTakeAnyDamage
	FDamageEvent DamageEvent;
	OtherActor->TakeDamage(
		DamageAmount,        // Damage amount
		DamageEvent,         // Damage event 
		nullptr,             // EventInstigator
		this                 // DamageCauser (self)
	);

	UE_LOG(LogDamagePickup, Log, TEXT("[%s] Damage applied via Actor::TakeDamage — awaiting next victim..."), *GetName());
}
void ADamagePickup::PostPickup()
{
	// Override parent to prevent destruction
	
	UE_LOG(LogDamagePickup, Log, TEXT("[%s] PostPickup triggered  (no destruction)"), *GetName());

	//Ensure particle system is active and visible
	if (ParticleSystem) {
		ParticleSystem->SetVisibility(true, true);
		ParticleSystem->SetActive(true, true);
	}
	UE_LOG(LogDamagePickup, Log,
		TEXT("[%s] Particle visibility/activity refreshed — trap remains visible"),
		*GetName());
	if(UBoxComponent* BoxCollisionObject = GetBoxCollision())
	{
		BoxCollisionObject->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		UE_LOG(LogDamagePickup, Log, TEXT("[%s] Collision disabled post-pickup"), *GetName());
	}
}
UParticleSystemComponent* ADamagePickup::GetParticleSystem() const
{
	return ParticleSystem;
}