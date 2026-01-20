// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BasePickup.generated.h"
class UBoxComponent;
class UPrimitiveComponent;
class UStaticMeshComponent;

UCLASS(Abstract)
class END2507_API ABasePickup : public AActor
{
	GENERATED_BODY()
	
public:	
	//// Sets default values for this actor's properties
	ABasePickup();

	UFUNCTION(BlueprintPure, Category = "Components")
	UBoxComponent* GetBoxCollision() const;

	UFUNCTION(BlueprintPure, Category = "Components")
	UStaticMeshComponent* GetMeshComponent() const;
protected:
	//Checks if OtherActor is allowed to pick up this item
	UFUNCTION(BlueprintPure, Category = "Pickup")
	virtual bool CanPickup(AActor* OtherActor);
	// Override in child classes to implement specific behavior
	UFUNCTION(BlueprintCallable, Category = "Pickup")
	virtual void HandlePickup(AActor* OtherActor);
	// Override to prevent destruction (DamagePickup) or allow it (HealthPickup)
	UFUNCTION(BlueprintCallable, Category = "Pickup")
	virtual void PostPickup();
	UPROPERTY(EditDefaultsOnly, Category = "Pickup")
	bool bUseMesh;
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:	

	// Box collision component for overlap detection
	UPROPERTY(VisibleDefaultsOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* BoxCollision;

	// Visual mesh for the pickup 
	UPROPERTY(VisibleDefaultsOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* MeshComponent;
	// Called when another actor overlaps this pickup's collision box
	UFUNCTION()
	void OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);


};
