// Fill out your copyright notice in the Description page of Project Settings.


#include "Code/Actors/HideWall.h"
#include "Code/Actors/BasePlayer.h"
#include "Code/Actors/BaseAgent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/DamageEvents.h"
#include "Engine/StaticMesh.h"
#include "../END2507.h"

DEFINE_LOG_CATEGORY(LogHideWall);
// Sets default values
AHideWall::AHideWall()
{

	PrimaryActorTick.bCanEverTick = true;


    UBoxComponent* CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    RootComponent = CollisionBox;
    CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    CollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    CollisionBox->SetGenerateOverlapEvents(true);
    CollisionBox->SetBoxExtent(FVector(50.f, 450.f, 150.f));


    WallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WallMesh"));
    WallMesh->SetupAttachment(RootComponent);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
        TEXT("/Engine/BasicShapes/Cube")
    );
    if (CubeMesh.Succeeded())
    {
        WallMesh->SetStaticMesh(CubeMesh.Object);
        WallMesh->SetRelativeScale3D(FVector(1.f, 9.f, 3.f)); // Match blueprint wall dimensions

    }    

    //static ConstructorHelpers::FObjectFinder<UMaterial> MaterialAsset(TEXT("/Game/SuperGrid/Materials/M_SuperGrid_Mat"));
    //if (MaterialAsset.Succeeded())
    //{
    //    WallMesh->SetMaterial(0, MaterialAsset.Object);
    //}
    //else {
    //    UE_LOG(LogHideWall, Warning, TEXT("%s: Failed to load default material"), *GetName());
    //    //Default color palette instead of single color
    //    WallColors.Add(FLinearColor(0.3f, 0.3f, 0.3f, 1.0f)); // Gray
    //    WallColors.Add(FLinearColor(0.5f, 0.3f, 0.2f, 1.0f)); // Brown
    //    WallColors.Add(FLinearColor(0.2f, 0.2f, 0.25f, 1.0f)); // Dark blue-gray
    //}
    // Setup wall collision for projectiles
    WallMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    WallMesh->SetCollisionObjectType(ECC_WorldStatic);
    WallMesh->SetCollisionResponseToAllChannels(ECR_Block);
    WallMesh->SetGenerateOverlapEvents(true);
    WallMesh->SetNotifyRigidBodyCollision(true);

    // Create hide zone (agents use this for cover)
    HideZone = CreateDefaultSubobject<UBoxComponent>(TEXT("HideZone"));
    HideZone->SetupAttachment(RootComponent);
    HideZone->SetBoxExtent(FVector(150.0f, 500.0f, 200.0f));
    HideZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    HideZone->SetCollisionObjectType(ECC_WorldStatic);
    HideZone->SetCollisionResponseToAllChannels(ECR_Ignore);
    HideZone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    HideZone->SetGenerateOverlapEvents(true);

    // Create damage collision (for spinning walls)
    DamageCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("DamageCollision"));
    DamageCollision->SetupAttachment(WallMesh);
    DamageCollision->SetBoxExtent(FVector(120.0f, 120.0f, 120.0f));
    DamageCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    DamageCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
    DamageCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    DamageCollision->SetGenerateOverlapEvents(true);

    // Designer defaults
    WallScale = FVector(1.0f, 1.0f, 1.0f);
    bProvideCover = true;
   // Initialize default wall colors
        WallColors.Add(FLinearColor(0.3f, 0.3f, 0.3f, 1.0f)); // Gray
    WallColors.Add(FLinearColor(0.5f, 0.3f, 0.2f, 1.0f)); // Brown
    WallColors.Add(FLinearColor(0.2f, 0.2f, 0.25f, 1.0f)); // Dark blue-gray

    // STYLE: uses TArray - Initialize 3 hit flash colors
    HitFlashColors.Add(FLinearColor(1.0f, 0.0f, 0.0f, 1.0f)); // Red
    HitFlashColors.Add(FLinearColor(1.0f, 0.5f, 0.0f, 1.0f)); // Orange
    HitFlashColors.Add(FLinearColor(1.0f, 1.0f, 0.0f, 1.0f)); // Yellow

    FlashDuration = 0.3f;
    LastHitColorIndex = -1;

    // Spinning defaults
    bIsSpinning = false;
    SpinAxis = ESpinAxis::Z_Axis;
    SpinSpeed = 90.0f;
    bShouldSpin = false;
    bShowTriggerVisuals = true;
    SwitchCooldown = 5.0f;
    bSwitchOnCooldown = false;
    // Health defaults
    MaxHealth = 100.0f;
    CurrentHealth = MaxHealth;
    PlayerDamage = 10.0f;
    AgentClass = ABaseAgent::StaticClass();

    SwitchMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SwitchMesh"));
    SwitchMesh->SetupAttachment(WallMesh);
    // Setup basic mesh properties 
    SwitchMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 200.0f));
    SwitchMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    // Ensure the switch is shootable
    SwitchMesh->SetCollisionResponseToAllChannels(ECR_Block);
    SwitchMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    SwitchMesh->SetGenerateOverlapEvents(true);

    
  

}

void AHideWall::OnHitByProjectile(AActor* ProjectileActor, float DamageAmount)
{
    if (!ProjectileActor)
    {
        return;
    }

    UE_LOG(LogHideWall, Log, TEXT("%s: Hit by projectile %s for %.1f damage"),
        *GetName(), *ProjectileActor->GetName(), DamageAmount);

    // Flash hit color
    FlashHitColor();

    // Apply damage to wall
    TakeDamage(DamageAmount);
    OnSwitchHit(DamageAmount);
    // Destroy projectile 
    ProjectileActor->Destroy();
}



bool AHideWall::IsSafeForCover() const
{
    return !bIsSpinning;
}

// Called when the game starts or when spawned
void AHideWall::BeginPlay()
{
	Super::BeginPlay();
    SetActorScale3D(WallScale);
    SetupWallAppearance();
    StoreOriginalColors();
    // Bind hide zone overlaps if cover is enabled
    if (bProvideCover && HideZone)
    {
        HideZone->OnComponentBeginOverlap.AddDynamic(this, &AHideWall::OnHideZoneOverlapBegin);
        HideZone->OnComponentEndOverlap.AddDynamic(this, &AHideWall::OnHideZoneOverlapEnd);
    }

    // Bind damage collision if spinning
    if (bIsSpinning && DamageCollision)
    {
        DamageCollision->OnComponentBeginOverlap.AddDynamic(this, &AHideWall::OnDamageCollisionOverlapBegin);
    }

    // Bind projectile hit detection on wall mesh
    if (WallMesh)
    {
        WallMesh->OnComponentBeginOverlap.AddDynamic(this, &AHideWall::OnHideZoneOverlapBegin); // Reuse for projectile detection
    }
    // Toggle debug visuals for the SwitchMesh
    if (SwitchMesh)
    {
        SwitchMesh->SetHiddenInGame(!bShowTriggerVisuals);
    }

    // Initial color for the switch
    SetSwitchColor(bShouldSpin ? FLinearColor::Red : FLinearColor::Green);
    UE_LOG(LogHideWall, Log, TEXT("%s: Initialized at %s | Health=%.1f | Spinning=%d | Cover=%d"),
        *GetName(), *GetActorLocation().ToString(), CurrentHealth, bIsSpinning, bProvideCover);

}

void AHideWall::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Apply rotation when spinning
    if (bIsSpinning && SpinAxis != ESpinAxis::None && bShouldSpin)
    {
        FRotator DeltaRotation = FRotator::ZeroRotator;
        float RotationAmount = SpinSpeed * DeltaTime;

        switch (SpinAxis)
        {
        case ESpinAxis::X_Axis:
            DeltaRotation.Roll = RotationAmount;
            break;
        case ESpinAxis::Y_Axis:
            DeltaRotation.Pitch = RotationAmount;
            break;
        case ESpinAxis::Z_Axis:
            DeltaRotation.Yaw = RotationAmount;
            break;
        default:
            break;
        }

        AddActorWorldRotation(DeltaRotation);
    }
        // TrySpawnAgentThroughDoor(); 
}



void AHideWall::OnHideZoneOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == this)
    {
        return;
    }

    // Check if it's a projectile 
    const FString ActorName = OtherActor->GetClass()->GetName();
    if (ActorName.Contains(TEXT("Projectile")))
    {

        float Damage = PlayerDamage; // Default damage

        // Call hit handler
        OnHitByProjectile(OtherActor, Damage);
        return;
    }
    UE_LOG(LogHideWall, Log, TEXT("%s: Actor %s entered hide zone"), *GetName(), *OtherActor->GetName());
    if (OtherActor->IsA(ABaseAgent::StaticClass()))
    {
        ABaseAgent* Agent = Cast<ABaseAgent>(OtherActor);
        if (Agent)
        {
            //Agent->NotifyHitWallHideZone(this, true);
        }
    }

}

void AHideWall::OnHideZoneOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (!OtherActor || OtherActor == this)
    {
        return;
    }

    UE_LOG(LogHideWall, Log, TEXT("%s: Actor %s left hide zone"), *GetName(), *OtherActor->GetName());
    if (OtherActor->IsA(ABaseAgent::StaticClass()))
    {
        ABaseAgent* Agent = Cast<ABaseAgent>(OtherActor);
        if (Agent)
        {
          //  Agent->NotifyHitWallHideZone(this, false);
        }
    }
}

void AHideWall::OnDamageCollisionOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == this)
    {
        return;
    }

    // Only damage player characters when spinning
    ACharacter* PlayerCharacter = Cast<ACharacter>(OtherActor);
    if (PlayerCharacter && PlayerCharacter->IsPlayerControlled())
    {
        
        OtherActor->TakeDamage(
            PlayerDamage,
            FDamageEvent(),
            GetInstigatorController(),
            this
        );

        UE_LOG(LogHideWall, Log, TEXT("%s: Spinning wall dealt %.1f damage to player"),
            *GetName(), PlayerDamage);
    }
}

void AHideWall::OnSwitchCollisionOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
}

void AHideWall::OnSwitchHit(float DamageFromProjectile)
{
    if (bSwitchOnCooldown) return;

    // Toggle spin state
    bShouldSpin = !bShouldSpin;

    // Change switch color to reflect new state
    SetSwitchColor(bShouldSpin ? FLinearColor::Red : FLinearColor::Green);

    // Start cooldown
    bSwitchOnCooldown = true;
    GetWorldTimerManager().SetTimer(SwitchCooldownTimer, this, &AHideWall::ResetSwitchCooldown, SwitchCooldown);

    //  Broadcast delegate so AI/perception knows state changed 
    OnWallSpinToggled.Broadcast(this, bShouldSpin);

    UE_LOG(LogHideWall, Log, TEXT("%s: Switch Hit! Spinning: %s, Cooldown: %f"), *GetName(), bShouldSpin ? TEXT("True") : TEXT("False"), SwitchCooldown);

}

void AHideWall::ResetSwitchCooldown()
{
    bSwitchOnCooldown = false;
    UE_LOG(LogHideWall, Log, TEXT("%s: Switch cooldown ended."), *GetName());

    // Visually update the switch to show it's ready again
    SetSwitchColor(bShouldSpin ? FLinearColor::Red : FLinearColor::Green);
}

void AHideWall::SetSwitchColor(const FLinearColor& NewColor)
{
    if (SwitchMesh && SwitchMesh->GetMaterial(0))
    {
        // Get or create a dynamic material instance for the switch
        UMaterialInstanceDynamic* DynMat = Cast<UMaterialInstanceDynamic>(SwitchMesh->GetMaterial(0));
        if (!DynMat)
        {
            DynMat = UMaterialInstanceDynamic::Create(SwitchMesh->GetMaterial(0), this);
            SwitchMesh->SetMaterial(0, DynMat);
        }

        if (DynMat)
        {
            // Assuming the material has a vector parameter named "Color" or similar
            DynMat->SetVectorParameterValue(TEXT("BaseColor"), NewColor);
        }
    }
}


void AHideWall::SetupWallAppearance()
{
    if (!WallMesh )
    {
        UE_LOG(LogHideWall, Warning, TEXT("%s: Cannot setup appearance - missing mesh or colors"), *GetName());
        return;
    }

    const int32 NumColors = (WallColors.Num() > 0) ? WallColors.Num() : 1; // STYLE: ternary operator
    if (NumColors == 0)
    {
        UE_LOG(LogHideWall, Warning, TEXT("%s: No wall colors defined, using default gray"), *GetName());
        WallColors.Add(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f));
    }

    const int32 NumMaterialSlots = WallMesh->GetNumMaterials();
    if (NumMaterialSlots == 0)
    {
        UE_LOG(LogHideWall, Warning, TEXT("%s: Mesh has no material slots"), *GetName());
        return;
    }

    DynamicMaterials.Empty();

    // Assign materials to each slot
    int32 SlotIndex = 0;
    while (SlotIndex < NumMaterialSlots) 
    {
        const int32 ColorIndex = SlotIndex % WallColors.Num(); // Wrap around if more slots than colors
        UMaterialInterface* BaseMaterial = WallMesh->GetMaterial(SlotIndex);

        if (BaseMaterial)
        {
            UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(BaseMaterial, this);
            if (DynMat)
            {
                WallMesh->SetMaterial(SlotIndex, DynMat);

                
               
                 DynMat->SetVectorParameterValue(TEXT("BaseColor"), WallColors[ColorIndex]); // Fallback to secondary parameter
                 DynMat->SetVectorParameterValue(TEXT("Color"), WallColors[ColorIndex]);
                DynamicMaterials.Add(DynMat);
            }
        }

        SlotIndex++; // Increment for while loop
    }

    UE_LOG(LogHideWall, Log, TEXT("%s: Applied %d dynamic materials to %d slots"),
        *GetName(), DynamicMaterials.Num(), NumMaterialSlots);
}

void AHideWall::StoreOriginalColors()
{
    OriginalColors.Empty();

    //Store one color per material slot
    for (UMaterialInstanceDynamic* DynMat : DynamicMaterials)
    {
        if (DynMat)
        {
            FLinearColor CurrentColor;

            // Try BaseColor first, fallback to Color
            if (DynMat->GetVectorParameterValue(TEXT("BaseColor"), CurrentColor))
            {
                OriginalColors.Add(CurrentColor);
            }
            else if (DynMat->GetVectorParameterValue(TEXT("Color"), CurrentColor))
            {
                OriginalColors.Add(CurrentColor);
            }
            else
            {
                // Default fallback
                OriginalColors.Add(FLinearColor(0.5f, 0.5f, 0.5f, 1.0f));
            }
        }
    }

    UE_LOG(LogHideWall, Log, TEXT("%s: Stored %d original colors"), *GetName(), OriginalColors.Num());
}

void AHideWall::FlashHitColor()
{
    if (DynamicMaterials.Num() == 0 || HitFlashColors.Num() == 0)
    {
        UE_LOG(LogHideWall, Warning, TEXT("%s: Cannot flash - no materials or colors"), *GetName());
        return;
    }

    // Cycle to next hit color using modulo
    LastHitColorIndex = (LastHitColorIndex + 1) % HitFlashColors.Num();
    const FLinearColor& FlashColor = HitFlashColors[LastHitColorIndex];

    // Apply flash color to all material slots
    for (UMaterialInstanceDynamic* DynMat : DynamicMaterials)
    {
        if (DynMat)
        {
            DynMat->SetVectorParameterValue(TEXT("BaseColor"), FlashColor); // Fallback to secondary parameter 
            DynMat->SetVectorParameterValue(TEXT("Color"), FlashColor);
        }
    }

    UE_LOG(LogHideWall, Log, TEXT("%s: Flashed to color index %d"), *GetName(), LastHitColorIndex);

    // Clear existing timer
    if (GetWorldTimerManager().IsTimerActive(ColorRevertTimerHandle))
    {
        GetWorldTimerManager().ClearTimer(ColorRevertTimerHandle);
    }

    // Start revert timer
    GetWorldTimerManager().SetTimer(
        ColorRevertTimerHandle,
        this,
        &AHideWall::RevertToOriginalColor,
        FlashDuration,
        false
    );
}

void AHideWall::RevertToOriginalColor()
{
    if (DynamicMaterials.Num() != OriginalColors.Num())
    {
        UE_LOG(LogHideWall, Error, TEXT("%s: Material/Color count mismatch"), *GetName());
        return;
    }

    // Restore each material slot to its original color
    for (int32 i = 0; i < DynamicMaterials.Num(); i++)
    {
        if (DynamicMaterials[i])
        {
            DynamicMaterials[i]->SetVectorParameterValue(TEXT("BaseColor"), OriginalColors[i]); // Fallback to secondary parameter
            DynamicMaterials[i]->SetVectorParameterValue(TEXT("Color"), OriginalColors[i]);
        }
    }

    UE_LOG(LogHideWall, Log, TEXT("%s: Reverted to original colors"), *GetName());

}

void AHideWall::TakeDamage(float DamageAmount)
{
    if (CurrentHealth <= 0.0f)
    {
        return; // Already destroyed
    }

    CurrentHealth = FMath::Max(0.0f, CurrentHealth - DamageAmount);

    UE_LOG(LogHideWall, Log, TEXT("%s: Took %.1f damage | Health: %.1f/%.1f"),
        *GetName(), DamageAmount, CurrentHealth, MaxHealth);

    if (CurrentHealth <= 0.0f)
    {
        DestroyWall();
    }
}

void AHideWall::DestroyWall()
{
    UE_LOG(LogHideWall, Log, TEXT("%s: Health depleted - destroying"), *GetName());

    // Clear any active timers
    if (GetWorldTimerManager().IsTimerActive(ColorRevertTimerHandle))
    {
        GetWorldTimerManager().ClearTimer(ColorRevertTimerHandle);
    }

    Destroy();
}

void AHideWall::TrySpawnAgentThroughDoor()
{
    if (bIsSpinning && FMath::Abs(GetActorRotation().Yaw) < 10.0f)
    {
        if (!GetWorldTimerManager().IsTimerActive(AgentSpawnCooldownTimer))
        {
            FVector SpawnLocation = GetActorLocation() + GetActorForwardVector() * 200.0f;
            FRotator SpawnRotation = GetActorRotation();
            FActorSpawnParameters SpawnParams;
            SpawnParams.Owner = this;
            TSubclassOf<ABaseAgent> ClassToSpawn;
            if (AgentClass)
            {
                ClassToSpawn = AgentClass;
            }
            else
            {
                ClassToSpawn = ABaseAgent::StaticClass();
            }

            
            ABaseAgent* NewAgent = GetWorld()->SpawnActor<ABaseAgent>(
                ClassToSpawn,
                SpawnLocation,
                SpawnRotation,
                SpawnParams
            );

            if (NewAgent)
            {
                UE_LOG(LogHideWall, Log, TEXT("%s: Spawned agent through door"), *GetName());
            }

            // Set cooldown timer
            GetWorldTimerManager().SetTimer(AgentSpawnCooldownTimer, 5.0f, false);
        }
    }
}


