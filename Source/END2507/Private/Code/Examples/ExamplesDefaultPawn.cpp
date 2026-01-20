// Fill out your copyright notice in the Description page of Project Settings.


#include "Code/Examples/ExamplesDefaultPawn.h"
#include "../END2507.h"
#include "GameFramework/PlayerInput.h"

// Sets default values
AExamplesDefaultPawn::AExamplesDefaultPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AExamplesDefaultPawn::BeginPlay()
{
	Super::BeginPlay();

	//UpCast
	AActor* Actor = this;

	//Down Cast
	APawn* Pawn = Cast<APawn>(Actor);

	if (Pawn)
	{
		UE_LOG(Game, Warning, TEXT("Actor is %s"), *Pawn->GetName());
	}
	else
	{
		UE_LOG(Game, Log, TEXT("Numbers are %d or %f"), 4, 7.1f);
	}
	UE_LOG(Game, Log, TEXT("Velocity is %s"), *GetVelocity().ToString());

	UE_LOG(Game, Error, TEXT("It is %hs"), true ? "True" : "False");
	
}

// Called every frame
void AExamplesDefaultPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AExamplesDefaultPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UPlayerInput::AddEngineDefinedActionMapping(FInputActionKeyMapping("Test_Spawn", EKeys::SpaceBar));

	PlayerInputComponent->BindAction("Test_Spawn", EInputEvent::IE_Pressed, this, &AExamplesDefaultPawn::Spawn);

}

void AExamplesDefaultPawn::Spawn()
{
	FActorSpawnParameters Params;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	GetWorld()->SpawnActor<AActor>(SpawnBlueprintClass, GetActorTransform(), Params);
}

