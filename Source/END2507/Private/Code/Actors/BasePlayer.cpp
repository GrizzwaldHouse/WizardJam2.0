// Fill out your copyright notice in the Description page of Project Settings.


#include "Code/Actors/BasePlayer.h"
#include "Both/PlayerHUD.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "Code/AC_HealthComponent.h"
#include "Code/Actors/BaseRifle.h"
#include "Code/Actors/BaseAgent.h"
#include "Code/Actors/Spawner.h"
#include "Both/CharacterAnimation.h"
#include "Camera/CameraComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GenericTeamAgentInterface.h"
#include "../END2507.h"
#include "Code/Actors/AIC_CodeBaseAgentController.h"
DEFINE_LOG_CATEGORY_STATIC(LogCodePlayer, Log, All);
ABasePlayer::ABasePlayer()
{
	PrimaryActorTick.bCanEverTick = true;
	//SpringArm = CreateDefaultSubobject<USpringArmComponent>("SpringArm");
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetRelativeLocation(FVector(0.0f, 80.0f, 90.0f));
	SpringArm->SetupAttachment(GetRootComponent());
	SpringArm->TargetArmLength = 300.0f;
	SpringArm->bUsePawnControlRotation = true;
	/*SpringArm->bInheritPitch = true;
	SpringArm->bInheritYaw = true;
	SpringArm->bInheritRoll = false;*/

	//Create and attach the camera to the SpringArm

	//Camera = CreateDefaultSubobject<UCameraComponent>("Camera");
	//Camera->SetupAttachment(SpringArm);//Camera attaches to the SpringArm
	 // Create camera and attach to spring arm
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	PlayerTeamID = 0;
}

void ABasePlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	//Bind Rotation Functions
	PlayerInputComponent->BindAxis("TurnRight", this, &ABasePlayer::AddControllerYawInput);
	PlayerInputComponent->BindAxis("LookUp", this, &ABasePlayer::LookUp);

	//Bind Movement Functions
	PlayerInputComponent->BindAxis("MoveForward", this, &ABasePlayer::InputAxisMoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ABasePlayer::InputAxisMoveRight); //Bind the axis for moving right
	PlayerInputComponent->BindAxis("Strafe", this, &ABasePlayer::InputAxisStrafe); //Bind the axis for strafing (turning left/right)
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ABasePlayer::InputActionJump);
	PlayerInputComponent->BindAction("Attack", IE_Pressed, this, &ABasePlayer::InputAttack); //Bind the action for attacking
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ABasePlayer::InputReload);

}

void ABasePlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateCrosshairTrace();
}


void ABasePlayer::BeginPlay()
{
	Super::BeginPlay();
	MyTeamID = FGenericTeamId(PlayerTeamID);
	APlayerController* playerController = Cast<APlayerController>(GetController());
	if (playerController)
	{
		FInputModeGameOnly InputMode;
		playerController->SetInputMode(InputMode);
		playerController->bShowMouseCursor = false;


		UE_LOG(Game, Log, TEXT(" [%s] Input mode restored to Game and UI — Player controls active"), *GetName());
	}
	else
	{
		UE_LOG(Game, Warning, TEXT(" [%s] No PlayerController found — Input mode not configured!"), *GetName());
	}
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);
		GetCapsuleComponent()->SetGenerateOverlapEvents(true);
		UE_LOG(Game, Log, TEXT("[%s] Player capsule set to respond to projectile channel"), *GetName());
	}
	if (GetMesh())
	{
		CharacterAnimationInstance = Cast<UCharacterAnimation>(GetMesh()->GetAnimInstance());
	}

	if (PlayerHUDClass)
	{

		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			PlayerHUDWidget = CreateWidget<UPlayerHUD>(PC, PlayerHUDClass);
			if (PlayerHUDWidget)
			{
				PlayerHUDWidget->AddToViewport();
				UE_LOG(Game, Log, TEXT("Player HUD created and added to viewport"));

				// Bind HUD to health component events
				if (HealthComponent)
				{
					// Bind to damage event 
					HealthComponent->OnHealthChanged.AddDynamic(this, &ABasePlayer::HandleHealthChanged);
					// Bind to heal event
					HealthComponent->OnHealed.AddDynamic(this, &ABasePlayer::HandleHealed);

					// Initialize HUD with current health
					PlayerHUDWidget->UpdateHealthBar(HealthComponent->GetHealthRatio());
					UE_LOG(Game, Log, TEXT("[%s] Health HUD bound and initialized"), *GetName());

				}
				else
				{
					UE_LOG(Game, Error, TEXT("[%s] No HealthComponent — HUD will not update!"), *GetName());
				}

				//BIND AMMO DISPLAY FROM RIFLE
				if (EquippedRifle)
				{
					EquippedRifle->OnAmmoChanged.AddDynamic(this, &ABasePlayer::HandleAmmoChanged);

					// Initialize HUD with current ammo values
					PlayerHUDWidget->SetAmmo(
						EquippedRifle->GetCurrentAmmo(),
						EquippedRifle->GetMaxAmmo());

					UE_LOG(Game, Warning, TEXT("[%s] Harry Potter's ammo display bound — Initial ammo: %d/%d"),
						*GetName(), EquippedRifle->GetCurrentAmmo(), EquippedRifle->GetMaxAmmo());
				}
				else
				{
					UE_LOG(Game, Error, TEXT("[%s] No rifle equipped — ammo display will not work!"), *GetName());
				}
			}
		}
	}
	else
	{
		UE_LOG(Game, Warning, TEXT("PlayerHUDClass is not set in the Blueprint!"));
	}
}

void ABasePlayer::InputAxisMoveForward(float AxisValue)
{
	

	AddMovementInput(FRotator(0.0f,GetControlRotation().Yaw, 0.0f).Vector(), AxisValue);
}

void ABasePlayer::InputAxisMoveRight(float Value)
{
	if (Value != 0.0f)
	{
		//Get the right direction
		FVector RightDirection = GetActorRightVector(); //Get the right vector of the player
		//Add movement input in the right direction
		AddMovementInput(RightDirection, Value); //Add movement input in the right direction scaled by MovementSpeed
	}
}

void ABasePlayer::InputAxisStrafe(float AxisValue)
{
	if (AxisValue != 0.0f) //Check if the input value is not zero
	{
		AddMovementInput(FRotationMatrix(FRotator(0.0f, GetControlRotation().Yaw, 0.0f)).GetScaledAxis(EAxis::Y), AxisValue); //Add movement input in the strafe direction (left/right) based on the control rotation
	}
}

void ABasePlayer::InputActionJump()
{//Call the parent class's Jump function to handle jumping logic
	Super::Jump();
}

void ABasePlayer::InputAttack()
{
	if (EquippedRifle)
	{
		EquippedRifle->Fire();
	}

	
	if (CharacterAnimationInstance)
	{
	CharacterAnimationInstance->FireAnimation();
	}
	else
	{
		UE_LOG(Game, Warning, TEXT("CharacterAnimationInstance is null, cannot play fire animation"));
	}
}

void ABasePlayer::InputReload()
{
	if (!EquippedRifle)
	{
		UE_LOG(LogCodePlayer, Warning, TEXT("[%s] No rifle equipped — cannot reload!"), *GetName());
		return;
	}

	UE_LOG(LogCodePlayer, Log, TEXT("[%s] Harry Potter casts RELOAD spell — R key pressed"), *GetName());

	// RequestReload checks bActionHappening gate, then broadcasts OnReloadStart delegate
	// OnReloadStart → BaseCharacter::HandleReloadStart() → CharacterAnimation::ReloadAnimationFunction()
	EquippedRifle->RequestReload();
}

void ABasePlayer::UpdateCrosshairTrace()
{
	if (!PlayerHUDWidget || !Camera)
	{
		return;
	}

	// Get camera location and forward direction
	const FVector CameraLocation = Camera->GetComponentLocation();
	const FVector CameraForward = Camera->GetForwardVector();
	const float TraceDistance = 5000.0f;
	const FVector TraceEnd = CameraLocation + (CameraForward * TraceDistance);

	// Setup trace parameters
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	// Perform line trace
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		CameraLocation,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);

	// Default color
	FLinearColor ReticleColor = FLinearColor::White;

	if (bHit && HitResult.GetActor())
	{
		// Check if we hit a spawner
		if (HitResult.GetActor()->IsA<ASpawner>())
		{
			ReticleColor = FLinearColor::Red;  // Change to red over spawner
		}
		// Check if we hit an enemy agent
		else if (ABaseAgent* Agent = Cast<ABaseAgent>(HitResult.GetActor()))
		{
			ReticleColor = FLinearColor(1.0f, 0.5f, 0.0f);  // Orange over enemy
		}
	}

	// Update HUD
	PlayerHUDWidget->SetReticleColor(ReticleColor);
}

void ABasePlayer::HandleAmmoChanged(float CurrentAmmo, float MaxAmmo)
{
	if (!PlayerHUDWidget)
	{
		UE_LOG(LogCodePlayer, Error, TEXT("[%s] PlayerHUDWidget is null — cannot update ammo display"), *GetName());
		return;
	}

	// Cast floats to int32 since SetAmmo expects integers
	PlayerHUDWidget->SetAmmo(static_cast<int32>(CurrentAmmo), static_cast<int32>(MaxAmmo));
	UE_LOG(LogCodePlayer, Log, TEXT("[%s] Ammo UI updated: %.0f/%.0f"), *GetName(), CurrentAmmo, MaxAmmo);
}


void ABasePlayer::HandleHealthChanged(float HealthRatio)
{
	if (!PlayerHUDWidget)
	{
		UE_LOG(LogCodePlayer, Error, TEXT("[%s] PlayerHUDWidget is null — cannot update health"), *GetName());
		return;
	}

	PlayerHUDWidget->UpdateHealthBar(HealthRatio);
	UE_LOG(LogCodePlayer, Log, TEXT("[%s] Player health bar updated: %.2f%%"), *GetName(), HealthRatio * 100.0f);

}
void ABasePlayer::HandleHealed(float CurrentHealth, float MaxHealth, float HealthRatio)
{
	if (!PlayerHUDWidget)
	{
		UE_LOG(LogCodePlayer, Error, TEXT("[%s] PlayerHUDWidget is null — cannot update health"), *GetName());
		return;
	}
	PlayerHUDWidget->UpdateHealthBar(HealthRatio);
	UE_LOG(LogCodePlayer, Log, TEXT("[%s] Player healed: %.0f/%.0f (%.2f%%)"), *GetName(), CurrentHealth, MaxHealth, HealthRatio * 100.0f);
}

void ABasePlayer::PlayerWin()
{
	UE_LOG(LogCodePlayer, Log, TEXT(" PlayerWin called — Disabling input and removing HUD"));
	// Disable player input
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
		UE_LOG(LogCodePlayer, Log, TEXT(" Player input disabled"));
	}
	else
	{
		UE_LOG(LogCodePlayer, Warning, TEXT(" No PlayerController — Input not disabled!"));
	}
	// Remove HUD from viewport
	if (PlayerHUDWidget)
	{
		PlayerHUDWidget->RemoveFromParent();
		UE_LOG(LogCodePlayer, Log, TEXT(" Player HUD removed from viewport"));
	}
	else
	{
		UE_LOG(LogCodePlayer, Warning, TEXT("PlayerHUDWidget is null — HUD not removed!"));
	}
}
void ABasePlayer::AddMaxAmmo(int32 AmountToAdd)
{
	if (AmountToAdd <= 0)
	{
		UE_LOG(LogCodePlayer, Warning, TEXT("[%s] AddMaxAmmo called with invalid amount: %d"),
			*GetName(), AmountToAdd);
		return;
	}
	if (!EquippedRifle)
	{
		UE_LOG(LogCodePlayer, Warning, TEXT("[%s] No rifle equipped — cannot add max ammo!"), *GetName());
		return;
	}
	EquippedRifle->AddMaxAmmo(AmountToAdd);
	UE_LOG(LogCodePlayer, Warning, TEXT("[%s] Max ammo increased by %d — New max: %d"),
		*GetName(), AmountToAdd, EquippedRifle->GetMaxAmmo());
}
bool ABasePlayer::CanPickHealth() const
{

	UE_LOG(LogCodePlayer, Display, TEXT("[%s] CanPickHealth queried — returning true (player access granted)"),
		*GetName());
	return true;
}
bool ABasePlayer::CanPickAmmo() const
{
	// Only players can pick up ammo
	UE_LOG(LogCodePlayer, Display, TEXT("[%s] CanPickAmmo queried — returning true (player access granted)"),
		*GetName());
	return true;
}
FGenericTeamId ABasePlayer::GetGenericTeamId() const
{
	return FGenericTeamId(PlayerTeamID);
}

void ABasePlayer::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	MyTeamID = NewTeamID;
	PlayerTeamID = static_cast<uint8>(NewTeamID.GetId());
	UE_LOG(LogCodePlayer, Display, TEXT("BasePlayer: Team ID set to %d"), PlayerTeamID);
}
void ABasePlayer::OnFactionAssigned_Implementation(int32 FactionID, const FLinearColor& FactionColor)
{
	UE_LOG(LogTemp, Display, TEXT("BasePlayer: Received faction assignment - ID=%d"), FactionID);

	// Update team ID
	MyTeamID = FGenericTeamId(static_cast<uint8>(FactionID));
	PlayerTeamID = static_cast<uint8>(FactionID);

}
void ABasePlayer::PlayerLost()
{
	UE_LOG(LogCodePlayer, Log, TEXT(" PlayerLost called — Configuring defeat state"));
	// Show mouse cursor 
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetShowMouseCursor(true);
		UE_LOG(LogCodePlayer, Log, TEXT(" Mouse cursor enabled"));
	}
	else
	{
		UE_LOG(LogCodePlayer, Warning, TEXT(" No PlayerController — Mouse cursor not shown!"));
	}

	// Remove HUD from viewport
	if (PlayerHUDWidget)
	{
		PlayerHUDWidget->RemoveFromParent();
		UE_LOG(LogCodePlayer, Log, TEXT(" Player HUD removed from viewport"));
	}
	else
	{
		UE_LOG(LogCodePlayer, Warning, TEXT("PlayerHUDWidget is null — HUD not removed!"));
	}

}
void ABasePlayer::Turn(float Value)
{
	if (Value != 0.0f) //Check if the input value is not zero
	{
		//Add rotation around the yaw axis based on the input value
		AddControllerYawInput(Value);
	}
}
void ABasePlayer::LookUp(float Value)
{
	if (Value != 0.0f) //Check if the input value is not zero
	{
		//Add rotation around the pitch axis based on the input value
		AddControllerPitchInput(Value);
	}
}