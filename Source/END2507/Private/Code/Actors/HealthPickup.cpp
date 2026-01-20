// HealthPickup.cpp
// Implementation of health restoration pickup system
// Author: Marcus Daley | Date: 10/9/2025

#include "Code/Actors/HealthPickup.h"
#include "Code/Actors/BasePickup.h"
#include "Code/PickupInterface.h"
#include "Code/Actors/BaseAgent.h"
#include "../END2507.h"
DEFINE_LOG_CATEGORY_STATIC(LogHealthPickup, Log, All);

AHealthPickup::AHealthPickup()
{
	DamageAmount = -20.0f;  // Heals 20 HP

	UE_LOG(LogHealthPickup, Log, TEXT("[%s] Health pickup initialized — HealAmount (BaseDamage) set to %.1f"),
		*GetName(), DamageAmount);
}


bool AHealthPickup::CanPickup(AActor* OtherActor)
{
	if (!OtherActor)
	{
		UE_LOG(LogHealthPickup, Warning, TEXT("[%s] CanPickup called with nullptr — Harry Potter's detection spell failed"), *GetName());
		return false;
	}
	if (OtherActor->IsA<ABaseAgent>())
	{
		UE_LOG(LogHealthPickup, Log, TEXT("[%s] REJECTED: [%s] is an agent — only players can collect health"),
			*GetName(), *OtherActor->GetName());
		return false;
	}

	bool bCanPick = CanPickHealthInternal(OtherActor);

	UE_LOG(LogHealthPickup, Log, TEXT("[%s] Interface check for [%s] — CanPickHealth = %s"),
		*GetName(), *OtherActor->GetName(), bCanPick ? TEXT("true") : TEXT("false"));

	return bCanPick;
}
void AHealthPickup::PostPickup()
{
	// Health pickups destroy themselves after collection 
	UE_LOG(LogHealthPickup, Display, TEXT("[%s] Healing complete — Destroying actor"), *GetName());
	Destroy();
}
bool AHealthPickup::CanPickHealthInternal(AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	// Check if actor implements IPickupInterface (pure C++ check)
	if (Actor->Implements<UPickupInterface>())
	{
		IPickupInterface* PickupInterface = Cast<IPickupInterface>(Actor);
		if (PickupInterface)
		{
			// Call pure C++ function (NOT BlueprintNativeEvent)
			return PickupInterface->CanPickHealth();
		}
	}

	return false;
}
