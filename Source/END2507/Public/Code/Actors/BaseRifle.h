// ABaseRifle - Weapon actor with ammo, reloading, and delegate communication
// Handles firing, reload gating, and animation synchronization
// Author: Marcus Daley
// Date: 10/6/2025
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseRifle.generated.h"
class USkeletalMeshComponent;
class AProjectile;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRifleAttack);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoChangedSignature, float, Current, float, Max);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReloadStartSignature);

UCLASS()
class END2507_API ABaseRifle : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABaseRifle();
	// Delegate instances for event broadcasting
	UPROPERTY(BlueprintAssignable, Category = "Weapon Events")
	FOnRifleAttack OnRifleAttack;

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Delegates")
	FOnReloadStartSignature  OnReloadStart;

	UPROPERTY(BlueprintAssignable, Category = "Weapon Events")
	FOnAmmoChangedSignature OnAmmoChanged;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void RequestReload();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void ReloadAmmo();
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Weapon")
	void Attack();
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void Fire();


	// Called when owner dies
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void OwnerDied();


	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void ActionStopped();
	// Gets current ammo count 
	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetCurrentAmmo() const;
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool CanFire() const;
	// Gets max ammo
	UFUNCTION(BlueprintPure, Category = "Weapon")
	int32 GetMaxAmmo() const;
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void AddMaxAmmo(int32 Amount);
	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool GetIsActionHappening() const;
	
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void ExecuteActionStopped();
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* RifleMesh;
protected:
	virtual void BeginPlay() override;
	// Called when the game starts or when spawned

	//Projectile class to spawn 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<AProjectile> ProjectileClass;

	// Socket name for projectile spawn 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName MuzzleSocketName;

	// Fire rate (shots per second) 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	float FireRate;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	float Damage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	float DamageMultiplier;
	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	bool bIsPlayerWeapon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo")
	bool bInfiniteAmmo;
	int32 MaxAmmo;
private:
	// Ammo state
	int32 CurrentAmmo;
	// Action gate for preventing overlapping actions
	bool bActionHappening;
	float LastFireTime;

	
	//Spawns projectile at muzzle 
	void SpawnProjectile();
	void OnFireRateComplete();
};
