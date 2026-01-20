// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Code/Actors/BasePickup.h"
#include "AmmoPickup.generated.h"

/**
 *  * Max Ammo Pickup - Increases player's maximum ammo capacity
 * Only collectible by player; disappears on pickup
 */
UCLASS()
class END2507_API AAmmoPickup : public ABasePickup
{
	GENERATED_BODY()
	
public:
	AAmmoPickup();

protected:
	// Template method overrides from BasePickup
	virtual bool CanPickup(AActor* OtherActor) override;
	virtual void HandlePickup(AActor* OtherActor) override;
private:
	UPROPERTY(EditDefaultsOnly, Category = "Pickup", meta = (ClampMin = "1", ClampMax = "50"))
	int32 AmmoToAdd;
};
