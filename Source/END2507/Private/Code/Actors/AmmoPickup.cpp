// Fill out your copyright notice in the Description page of Project Settings.


#include "Code/Actors/AmmoPickup.h"
#include "Code/Actors/BasePlayer.h"
#include "Components/StaticMeshComponent.h"
#include "../END2507.h"
DEFINE_LOG_CATEGORY_STATIC(LogAmmoPickup, Display, All);
AAmmoPickup::AAmmoPickup()
{

    //  add 10 to max ammo
    AmmoToAdd = 10;

    UE_LOG(LogAmmoPickup, Display, TEXT("[%s] AmmoPickup initialized with AmmoToAdd=%d"), *GetName(), AmmoToAdd);
}
bool AAmmoPickup::CanPickup(AActor* OtherActor)
{
    if (!OtherActor)
    {
        UE_LOG(LogAmmoPickup, Warning, TEXT("[%s] CanPickup: OtherActor is null"), *GetName());
        return false;
    }

	// Check if actor implements IPickupInterface
	if (!OtherActor->Implements<UPickupInterface>())
	{
		UE_LOG(LogAmmoPickup, Display, TEXT("[%s] Actor [%s] does not implement IPickupInterface"),
			*GetName(), *OtherActor->GetName());
		return false;
	}

	// Cast to interface and check if actor can pick ammo
	IPickupInterface* PickupInterface = Cast<IPickupInterface>(OtherActor);
	if (!PickupInterface)
	{
		UE_LOG(LogAmmoPickup, Warning, TEXT("[%s] Failed to cast to IPickupInterface"), *GetName());
		return false;
	}

	bool bCanPickAmmo = PickupInterface->CanPickAmmo();
	UE_LOG(LogAmmoPickup, Display, TEXT("[%s] Actor [%s] CanPickAmmo = %s"),
		*GetName(), *OtherActor->GetName(), bCanPickAmmo ? TEXT("true") : TEXT("false"));

	return bCanPickAmmo;
}

void AAmmoPickup::HandlePickup(AActor* OtherActor)
{
	if (!OtherActor)
	{
		UE_LOG(LogAmmoPickup, Error, TEXT("[%s] HandlePickup: OtherActor is null"), *GetName());
		return;
	}

	// Use interface instead of direct cast
	IPickupInterface* PickupInterface = Cast<IPickupInterface>(OtherActor);
	if (!PickupInterface)
	{
		UE_LOG(LogAmmoPickup, Error, TEXT("[%s] HandlePickup: Failed to cast to IPickupInterface"), *GetName());
		return;
	}

	// Call interface method to add max ammo
	PickupInterface->AddMaxAmmo(AmmoToAdd);

	UE_LOG(LogAmmoPickup, Warning, TEXT("[%s] Added %d max ammo to actor [%s] via interface — HUD should update"),
		*GetName(), AmmoToAdd, *OtherActor->GetName());
}

