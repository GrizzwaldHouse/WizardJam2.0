// HideWall.h
// Wall actor that agents can hide behind for cover
// Author: Marcus Daley
// Date: 9/30/2025
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HideWall.generated.h"
class UStaticMesh;
class UStaticMeshComponent;
class UBoxComponent;
class ABaseAgent;
class UMaterialInstanceDynamic;
DECLARE_LOG_CATEGORY_EXTERN(LogHideWall, Log, All);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWallSpinToggled, AHideWall*, Wall, bool, bIsSpinning);

// Enum for spin axis selection 
UENUM(BlueprintType)
enum class ESpinAxis : uint8
{
	None UMETA(DisplayName = "No Spin"),
	X_Axis UMETA(DisplayName = "X Axis"),
	Y_Axis UMETA(DisplayName = "Y Axis"),
	Z_Axis UMETA(DisplayName = "Z Axis")
};


UCLASS()
class END2507_API AHideWall : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AHideWall();
	UFUNCTION(BlueprintCallable, Category = "Wall")
	void OnHitByProjectile(AActor* ProjectileActor, float DamageAmount);
	
	UPROPERTY(BlueprintAssignable, Category = "Wall|Events")
	FOnWallSpinToggled OnWallSpinToggled;
	UFUNCTION(BlueprintCallable, Category = "Wall")
	bool IsSafeForCover() const;
	UPROPERTY()
	bool bShouldSpin;
protected:

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	UFUNCTION()
	void OnHideZoneOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnHideZoneOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// Overlap callback for spinning wall damage
	UFUNCTION()
	void OnDamageCollisionOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnSwitchCollisionOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	void OnSwitchHit(float DamageFromProjectile);
	void ResetSwitchCooldown();
private:
	// Static mesh asset for the wall (set in Blueprint, replaces ConstructorHelpers)
	UPROPERTY(EditDefaultsOnly, Category = "Mesh", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMesh> WallMeshAsset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* WallMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* HideZone;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UBoxComponent* DamageCollision;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* SwitchMesh;



	UPROPERTY()
	bool bShowTriggerVisuals; // Debug visuals toggle for the switch

	UPROPERTY()
	float SwitchCooldown;

	UPROPERTY()
	bool bSwitchOnCooldown;

	UPROPERTY()
	FTimerHandle SwitchCooldownTimer;

	void SetSwitchColor(const FLinearColor& NewColor);


	// Designer-configurable properties
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Settings", meta = (AllowPrivateAccess = "true"))
	FVector WallScale;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Settings", meta = (AllowPrivateAccess = "true", DisplayName = "Wall Colors"))
	TArray<FLinearColor> WallColors;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Wall Settings", meta = (AllowPrivateAccess = "true"))
	bool bProvideCover;

	// === HIT FLASH PROPERTIES ===
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Hit Reaction", meta = (AllowPrivateAccess = "true"))
	TArray<FLinearColor> HitFlashColors;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Hit Reaction", meta = (AllowPrivateAccess = "true", ClampMin = "0.1", ClampMax = "2.0"))
	float FlashDuration;

	UPROPERTY()
	int32 LastHitColorIndex; // Track which color was used last

	// === SPINNING PROPERTIES ===
	
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true"))
	bool bIsSpinning;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true", EditCondition = "bIsSpinning"))
	ESpinAxis SpinAxis;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "true", EditCondition = "bIsSpinning", ClampMin = "0.0", ClampMax = "360.0"))
	float SpinSpeed;

	// === HEALTH & DAMAGE PROPERTIES ===
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Health", meta = (AllowPrivateAccess = "true"))
	float MaxHealth;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health", meta = (AllowPrivateAccess = "true"))
	float CurrentHealth;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	float PlayerDamage;

	UPROPERTY(EditDefaultsOnly, Category = "Agent Spawning", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<ABaseAgent> AgentClass;
	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> DynamicMaterials;
	UPROPERTY()
	TArray<FLinearColor> OriginalColors; 

	
	void SetupWallAppearance();
	void StoreOriginalColors();
	void FlashHitColor();
	void RevertToOriginalColor();
	void ApplyInternalDamage(float DamageAmount);
	void DestroyWall();
	void TrySpawnAgentThroughDoor();

	FTimerHandle ColorRevertTimerHandle;
	FTimerHandle AgentSpawnCooldownTimer;
};
