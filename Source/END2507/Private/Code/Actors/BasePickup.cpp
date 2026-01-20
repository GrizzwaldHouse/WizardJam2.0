// Fill out your copyright notice in the Description page of Project Settings.


#include "Code/Actors/BasePickup.h"
#include "Components/BoxComponent.h"
#include "../END2507.h"
DEFINE_LOG_CATEGORY_STATIC(LogBasePickup, Log, All);

// Sets default values
ABasePickup::ABasePickup()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
	RootComponent = BoxCollision;
	//Configure collision settings
	BoxCollision->SetBoxExtent(FVector(50.0f, 50.0f, 50.0f));
	BoxCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BoxCollision->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	BoxCollision->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	BoxCollision->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);

	// Create mesh component (always created, visibility controlled by bUseMesh)
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	bUseMesh = true;
	// Bind overlap event
	BoxCollision->OnComponentBeginOverlap.AddDynamic(this, &ABasePickup::OnBoxBeginOverlap);
	UE_LOG(LogBasePickup, Log, TEXT("[%s] BoxCollision component initialized"), *GetName());
}

UBoxComponent* ABasePickup::GetBoxCollision() const
{
	return BoxCollision;
}

// Called when the game starts or when spawned
void ABasePickup::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("Base Pickup Spawned: %s"), *GetName());
	if (MeshComponent)
	{
		MeshComponent->SetVisibility(bUseMesh);
		MeshComponent->SetHiddenInGame(!bUseMesh);
	}
}


void ABasePickup::OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this)
	{
		UE_LOG(LogBasePickup, Warning, TEXT("[%s] nullptr or self-overlap detected"), *GetName());
		return;
	}

	UE_LOG(LogBasePickup, Log, TEXT("[%s] Overlap detected with [%s] — initiating template method sequence"),
		*GetName(), *OtherActor->GetName());

	//Eligibility Check
	if (!CanPickup(OtherActor))
	{
		UE_LOG(LogBasePickup, Warning, TEXT("[%s] failed CanPickup() check"),
			*GetName(), *OtherActor->GetName());
		return;
	}

	UE_LOG(LogBasePickup, Log, TEXT("[%s] CanPickup() passed for [%s] — proceeding to HandlePickup()"),
		*GetName(), *OtherActor->GetName());

	//  Main Pickup Logic
	HandlePickup(OtherActor);

	// Cleanup
	PostPickup();
}

bool ABasePickup::CanPickup(AActor* OtherActor)
{
	return true;
}

void ABasePickup::HandlePickup(AActor* OtherActor)
{
	UE_LOG(LogBasePickup, Log, TEXT("[%s] HandlePickup() called for [%s] — no base implementation"),
		*GetName(), OtherActor ? *OtherActor->GetName() : TEXT("NULL"));

}

void ABasePickup::PostPickup()
{
	UE_LOG(LogBasePickup, Log, TEXT("[%s] PostPickup() triggered —Destroying actor..."), *GetName());
	Destroy();
}

UStaticMeshComponent* ABasePickup::GetMeshComponent() const
{
	return MeshComponent;
}