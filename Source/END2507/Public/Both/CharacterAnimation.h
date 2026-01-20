// CharacterAnimation.h
// Animation controller for player and AI characters
// Author: Marcus Daley
// Date: 9/28/2025

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "CharacterAnimation.generated.h"
// Forward declarations
class ABaseCharacter;
class UAnimSequence;

// Delegate for animation events
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReloadNowSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnActionEndedSignature);


UCLASS()
class END2507_API UCharacterAnimation : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;

	UFUNCTION(BlueprintNativeEvent)
	void PreviewWindowUpdate();
	// Animation functions
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Animation")
	void HitAnimation(float Ratio);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Animation")
	void DeathAnimation();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Animation")
	void FireAnimation();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Animation")
	void ReloadAnimationFunction();
	virtual void ReloadAnimationFunction_Implementation();
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void CallOnActionEnded();
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void CallOnReloadNow();
	UPROPERTY(BlueprintAssignable, Category = "Animation|Delegates")
	FOnReloadNowSignature OnReloadNow;

	UPROPERTY(BlueprintAssignable, Category = "Animation|Delegates")
	FOnActionEndedSignature OnActionEnded;

	// Animation state getters
	bool GetIsFiring() const;
	bool GetIsHit() const;
	bool GetIsDead() const;
	bool GetIsReloading() const;
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Default)
	float Velocity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement Direction")
	float MovementDirection = 0.0f;

	// Spine aiming
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spine Animation")
	FRotator SpineRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spine Animation")
	FName SpineBoneName = TEXT("spine_02");

	// Combat state
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bIsFiring = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float FireCooldownTime = 0.8f;

	FTimerHandle FireCooldownTimer;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation State")
	bool bIsHit = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation State")
	bool bIsDead = false;
	UPROPERTY(BlueprintReadOnly, Category = "Animation State")
	bool bIsReloading;
	UFUNCTION()
	void HandleCharacterDeath();

	// References
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "References")
	class ABaseCharacter* OwningCharacter;

	// Animation assets
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	FName ActionSlotName = TEXT("ActionSlotName");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	class UAnimSequenceBase* FireAsset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	class UAnimSequence* ReloadAsset;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	UAnimSequence* HitAsset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TArray<UAnimSequence*> DeathAssets;

	// Current animation assets
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimSequence* CurrentHitAsset;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimSequence* CurrentDeathAsset;
	//Current reload asset tracking
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Animation")
	UAnimSequence* CurrentReloadAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bDebug = false;


private:
	// Timer handles for animation cooldowns

	FTimerHandle ReloadCooldownTimer;
	FTimerHandle HitResetTimer;

};
