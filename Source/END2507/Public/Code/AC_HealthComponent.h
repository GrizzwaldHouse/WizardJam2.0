// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AC_HealthComponent.generated.h"
class UUserWidget;
class AActor;
class AController; 
class UDamageType;
//// Delegate for health changes 
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, float, HealthRatio);
//Delegate for healing (no hit animation)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnHealed, float, CurrentHealth, float, MaxHealth, float, HealthRatio);
// Delegate for death
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeath, AActor*, DestroyedActor);
// Delegate for death animation completion (fires after death animation finishes)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeathEnded, AActor*, DestroyedActor);

UCLASS(ClassGroup=(Custom),meta=(BlueprintSpawnableComponent))
class END2507_API UAC_HealthComponent : public UActorComponent
{
	GENERATED_BODY()
private:

	void Die();
	UPROPERTY(EditDefaultsOnly, Category = "Health", meta = (ClampMin = "0.0", AllowPrivateAccess = "true"))
	float MaxHealth;

	UPROPERTY(VisibleAnywhere, Category = "Health", meta = (AllowPrivateAccess = "true"))
	float CurrentHealth;

	UPROPERTY(VisibleAnywhere, Category = "Health", meta = (AllowPrivateAccess = "true"))
	bool bIsDead;

	


public:	
	// Sets default values for this component's properties
	UAC_HealthComponent();
	// Broadcasts when health changes 
	UPROPERTY(BlueprintAssignable, Category = "Health Events")
	FOnHealthChanged OnHealthChanged;
	//Broadcasts when actor is healed (health > 0 and health < max health)
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnHealed OnHealed;
	//Broadcasts when actor dies (health <= 0) 
	UPROPERTY(BlueprintAssignable, Category = "Health Events")
	FOnDeath OnDeath;
	// Broadcasts when death animation completes 
	UPROPERTY(BlueprintAssignable, Category = "Health Events")
	FOnDeathEnded OnDeathEnded;

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const;
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetCurrentHealth() const;
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealthRatio() const;
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetMaxHealth(float NewMaxHealth);

	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetCurrentHealth(float NewHealth);
	UFUNCTION(BlueprintCallable, Category = "Health")
	void Heal(float Amount);
	
	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
		AController* InstigatedBy, AActor* DamageCauser);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;


	


};
