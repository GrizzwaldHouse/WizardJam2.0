// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExampleFunctions.generated.h"

UCLASS()
class END2507_API AExampleFunctions : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AExampleFunctions();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void BlueprintCallable();

	//BlueprintImplementableEvent has to be Declared in C++ and Defined in Blueprint
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void BlueprintImplementableEvent();

	//Blueprint Native Event - Declare in C++, define in Blueprint and in c++, but have to add _Implementation to the name of the definition in c++
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void BlueprintNativeEvent();

private:
	UFUNCTION(BlueprintCallable)
	float PureFunction() const;

public:
	//Multi Return Values
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	bool MultipleReturnValues(AActor* Actor1, FRotator Rotation, AActor*& Actor2, int32& Index);
};
