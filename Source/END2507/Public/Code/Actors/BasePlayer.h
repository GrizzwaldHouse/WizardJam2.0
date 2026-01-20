// ABasePlayer - Player character with camera, input, and HUD
// Author: Marcus Daley
// Date: 10/6/2025

#pragma once

#include "CoreMinimal.h"
#include "Code/Actors/BaseCharacter.h"
#include "Both/CharacterAnimation.h"
#include "Code/CodeFactionInterface.h"
#include "GenericTeamAgentInterface.h"
#include "BasePlayer.generated.h"

/**
 * 
 */
UCLASS()
class END2507_API ABasePlayer : public ABaseCharacter, public IGenericTeamAgentInterface, public ICodeFactionInterface
{
	GENERATED_BODY()
	
public:
	ABasePlayer();

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaTime) override;
	UFUNCTION()
	void PlayerWin();
	void AddMaxAmmo(int32 AmountToAdd);
	virtual bool CanPickHealth() const override;
	virtual bool CanPickAmmo() const override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual void SetGenericTeamId(const FGenericTeamId& MyTeamID) override;
	// IFactionInterface implementation
	virtual void OnFactionAssigned_Implementation(int32 FactionID, const FLinearColor& FactionColor) override;



protected:
	FGenericTeamId MyTeamID;
	virtual void BeginPlay() override;
	UPROPERTY(Category = Character, VisibleAnywhere)
	class USpringArmComponent* SpringArm;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class UCameraComponent* Camera;
	//Character Animation Variables
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	class UCharacterAnimation* CharacterAnimationInstance;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UPlayerHUD> PlayerHUDClass;
	UPROPERTY()
	class UPlayerHUD* PlayerHUDWidget;
	UFUNCTION()
	void HandleHealthChanged(float HealthRatio);
	UFUNCTION()
	void HandleHealed(float CurrentHealth, float MaxHealth, float HealthRatio);


private:
	UPROPERTY(EditDefaultsOnly, Category = "Team", meta = (ClampMin = "0", ClampMax = "255"))
	uint8 PlayerTeamID;
	UFUNCTION()
	void PlayerLost();
	void Turn(float Value); //Handles Horizontal mouse movement
	void LookUp(float Value); //Handles Vertical mouse movement
	void InputAxisMoveForward(float AxisValue);
	void InputAxisMoveRight(float Value); //Handles right/left movement
	void InputAxisStrafe(float AxisValue);
	void InputActionJump(); //Handles jumping
	void InputAttack(); //Handles attacking
	void InputReload();
	void UpdateCrosshairTrace();
	// Updates HUD display when ammo count changes
	UFUNCTION()
	void HandleAmmoChanged(float CurrentAmmo, float MaxAmmo);
};
