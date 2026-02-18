// IEnemyInterface.h - Interface for enemy  behavior
// Author: Marcus Daley
// Date: 9/29/2025
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "IEnemyInterface.generated.h"
class AHideWall;
// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UEnemyInterface : public UInterface
{
	GENERATED_BODY()
};

class END2507_API IEnemyInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
    // Attack target while facing them
    virtual void EnemyAttack(AActor* Target) = 0;

   
	
	virtual void EnemyReload(AActor* Target) = 0;
  
};
