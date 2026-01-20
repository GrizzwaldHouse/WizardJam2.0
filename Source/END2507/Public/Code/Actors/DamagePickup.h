// DamagePickup - Applies damage to actors that overlap with it
// Features fire particle effect to indicate danger
// Author: Marcus Daley 
// Date: 10/9/2025

#pragma once

#include "CoreMinimal.h"
#include "Code/Actors/BasePickup.h"
#include "DamagePickup.generated.h"

// Forward declarations
class UParticleSystemComponent;
class UParticleSystem;
UCLASS()
class END2507_API ADamagePickup : public ABasePickup
{
	GENERATED_BODY()
	
public:
	ADamagePickup();
	UFUNCTION(BlueprintPure, Category = "Damage")
	float GetDamageAmount() const;
	// Getter for ParticleSystem component
	UFUNCTION(BlueprintPure, Category = "Effects")
	UParticleSystemComponent* GetParticleSystem() const;

private:
	// Fire particle effect indicating this is a damage source
	UPROPERTY(VisibleDefaultsOnly, Category = "Effects", meta = (AllowPrivateAccess = "true"))
	UParticleSystemComponent* ParticleSystem;



protected:
	virtual void BeginPlay() override;
	// Applies damage to the overlapping actor
	virtual void HandlePickup(AActor* OtherActor) override;
	// Overridden to prevent destruction
	virtual void PostPickup() override;
	// Amount of damage to apply on pickup
	UPROPERTY(EditDefaultsOnly, Category = "Damage", meta = (ClampMin = "0.0", AllowPrivateAccess = "true"))
	float DamageAmount;
};
