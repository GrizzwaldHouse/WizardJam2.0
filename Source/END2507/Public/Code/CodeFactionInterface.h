// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CodeFactionInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UCodeFactionInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class END2507_API ICodeFactionInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Faction")
	void OnFactionAssigned(int32 FactionID, const FLinearColor& FactionColor);

};
