// HealthPickup - Restores health to eligible actors 
// // Features heart particle effect to indicate healing
// Author: Marcus Daley 
// Date: 10/9/2025
#pragma once

#include "CoreMinimal.h"
#include "Code/Actors/DamagePickup.h"
#include "HealthPickup.generated.h"


UCLASS()
class END2507_API AHealthPickup : public  ADamagePickup
{
	GENERATED_BODY()
public:
	AHealthPickup();


protected:
	// Checks if actor can collect this pickup via interface
	virtual bool CanPickup(AActor* OtherActor) override;
	virtual void PostPickup() override;
private:
	bool CanPickHealthInternal(AActor* Actor) const;
};
