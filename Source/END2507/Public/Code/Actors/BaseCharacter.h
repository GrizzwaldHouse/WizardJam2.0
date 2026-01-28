// ABaseCharacter - Base class for player and AI characters
// Manages health, rifle spawning, and animation/weapon delegate bindings
// Author: Marcus Daley
// Date: 10/6/2025

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Code/PickupInterface.h"
#include "BaseCharacter.generated.h"

class UAC_HealthComponent;
class UAC_OverheadBarComponent;
class ABaseRifle;
class UAnimSequence;
class UCharacterAnimation;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacterDeath);

//DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponFired);
UCLASS(ABSTRACT)
class END2507_API ABaseCharacter : public ACharacter, public IPickupInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABaseCharacter();
	UFUNCTION(BlueprintPure, Category = "Character")
	UAC_HealthComponent* GetHealthComponent() const;

	// Gets the equipped rifle 
	UFUNCTION(BlueprintPure, Category = "Character")
	ABaseRifle* GetRifle() const;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	UFUNCTION(BlueprintPure, Category = "Animation")
	FRotator GetSpineTargetRotation() const;
	UFUNCTION(BlueprintPure, Category = "Animation")
	UAnimSequence* GetHitAsset() const;
	UFUNCTION(BlueprintPure, Category = "Animation")
	TArray<UAnimSequence*> GetDeathAssets() const;
	UPROPERTY(BlueprintAssignable, Category = "Character Events")
	FOnCharacterDeath OnCharacterDeath;
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual bool CanPickHealth() const override;
	virtual bool CanPickAmmo() const override;
	virtual void AddMaxAmmo(int32 Amount) override;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UAC_HealthComponent* HealthComponent;

	// Overhead health/stamina bar for AI agents (null for player)
	UPROPERTY()
	UAC_OverheadBarComponent* OverheadBarComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	ABaseRifle* EquippedRifle;

	// Rifle class to spawn 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	TSubclassOf<ABaseRifle> RifleClass;
	// Animation assets
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	UAnimSequence* HitAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TArray<UAnimSequence*> DeathAssets;
	virtual void HandleHurt(float DamageRatio);
	// Called when character dies 
	virtual void HandleDeathStart(float DamageRatio);
private:
	UFUNCTION()
	void OnHealthChanged(float HealthRatio);
	UFUNCTION()
	void HandleReloadStart();

	UFUNCTION()
	void HandleReloadNow();
	UFUNCTION()
	void HandleActionEnded();
	UFUNCTION()
	void OnDeath(AActor* DestroyedActor);
	
	void SpawnAndAttachRifle();
	void BindRifleDelegates();
	void BindAnimationDelegates();
};
