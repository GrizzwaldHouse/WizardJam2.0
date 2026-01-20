// ABaseRifle - Weapon actor that handles shooting and ammo, and reloading
// Author: Marcus Daley
// Date: 9/29/2025
#include "Code/Actors/BaseRifle.h"
#include "Code/Actors/BasePlayer.h"
#include "Code/Actors/BaseAgent.h"
#include "Code/Actors/Projectile.h"
#include "Components/SkeletalMeshComponent.h"
#include "TimerManager.h"
#include "../END2507.h"
DEFINE_LOG_CATEGORY_STATIC(LogRifle, Log, All);
// Sets default values
ABaseRifle::ABaseRifle()
{
	PrimaryActorTick.bCanEverTick = false;
    // Create mesh component
    RifleMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("RifleMesh"));
    RootComponent = RifleMesh;
    RifleMesh->SetVisibility(true);
    // Initialize default values
    MuzzleSocketName = FName(TEXT("MuzzleFlashSocket")); 
    FireRate = 0.8f;       
    Damage = 10.0f;
    DamageMultiplier = 1.0f;
    MaxAmmo = 30;
    bInfiniteAmmo = false;
    bActionHappening = false;
    LastFireTime = 0.0f;
}

void ABaseRifle::Fire()
{
    if (!CanFire())
    {
        UE_LOG(LogRifle, Warning, TEXT("%s: Fire rate cooldown active"), *GetName());
        return;
    }

    if (!bInfiniteAmmo && CurrentAmmo <= 0)
    {
        UE_LOG(LogRifle, Warning, TEXT("%s: Out of ammo!"), *GetName());
        return;
    }
   
    UE_LOG(LogRifle, Warning, TEXT("===== %s FIRING! Owner: %s ====="),
        *GetName(), GetOwner() ? *GetOwner()->GetName() : TEXT("NULL"));

    // Spawn projectile
    SpawnProjectile();

    // Consume ammo
    if (!bInfiniteAmmo)
    {
        CurrentAmmo--;
        UE_LOG(LogRifle, Log, TEXT("[%s] Ammo consumed: %d/%d remaining"), *GetName(), CurrentAmmo, MaxAmmo);
        // Broadcast ammo change delegate
        OnAmmoChanged.Broadcast(CurrentAmmo, MaxAmmo);
    }
    LastFireTime = GetWorld()->GetTimeSeconds();
    FTimerHandle FireRateTimer;
    GetWorldTimerManager().SetTimer(
        FireRateTimer,
        this,
        &ABaseRifle::OnFireRateComplete,
        FireRate,
        false  // Don't loop
    );
  
   

    UE_LOG(LogRifle, Log, TEXT("%s: Fired! Ammo: %d/%d"), *GetName(), CurrentAmmo, MaxAmmo);

}


void ABaseRifle::OwnerDied()
{
    UE_LOG(LogRifle, Warning, TEXT("%s: Owner died, rifle deactivated"), *GetName());

    // Destroy rifle or disable it
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
}

void ABaseRifle::RequestReload()
{
  
    if (bActionHappening)
    {
        UE_LOG(LogRifle, Warning, TEXT("[%s] Cannot reload — action already in progress!"), *GetName());
        return;
    }
    if (CurrentAmmo >= MaxAmmo)
    {
        UE_LOG(LogRifle, Log, TEXT("[%s] Already at max ammo (%d/%d)"), *GetName(), CurrentAmmo, MaxAmmo);
        return;
    }
    // Set action gate to prevent overlapping actions
    bActionHappening = true;
    UE_LOG(LogRifle, Warning, TEXT("[%s] Reload requested — bActionHappening set to TRUE"), *GetName());
    // Broadcast OnReloadStart delegate to trigger animation
    OnReloadStart.Broadcast();
    UE_LOG(LogRifle, Log, TEXT("Harry Potter says: OnReloadStart broadcasted — animation should play"));
 
}
void ABaseRifle::AddMaxAmmo(int32 Amount)
{
    if (Amount <= 0)
    {
        UE_LOG(LogRifle, Warning, TEXT("[%s] AddMaxAmmo called with invalid amount: %d"),
            *GetName(), Amount);
        return;
    }

    MaxAmmo += Amount;

    UE_LOG(LogRifle, Warning, TEXT("[%s] MaxAmmo increased by %d — Now: %d/%d"),
        *GetName(), Amount, CurrentAmmo, MaxAmmo);

    // Broadcast ammo changed delegate
    OnAmmoChanged.Broadcast(CurrentAmmo, MaxAmmo);
}
void ABaseRifle::ReloadAmmo()
{
    if (CurrentAmmo >= MaxAmmo)
    {
        UE_LOG(LogRifle, Warning, TEXT("[%s] Already at max ammo"), *GetName());
        return;
    }
    // Refill ammo
    CurrentAmmo = MaxAmmo;
    UE_LOG(LogRifle, Warning, TEXT("[%s] Submarine torpedoes reloaded — Ammo: %d/%d"), *GetName(), CurrentAmmo, MaxAmmo);

    // Broadcast ammo change delegate for UI update
    OnAmmoChanged.Broadcast(CurrentAmmo, MaxAmmo);

}

void ABaseRifle::ActionStopped()
{
    bActionHappening = false;
	UE_LOG(LogRifle, Warning, TEXT("[%s] Action stopped — bActionHappening set to FALSE"), *GetName());
}

int32 ABaseRifle::GetCurrentAmmo() const
{
    return CurrentAmmo;
}

bool ABaseRifle::CanFire() const
{
    if (!GetWorld())
    {
        return false;
    }

    const float TimeSinceLastFire = GetWorld()->GetTimeSeconds() - LastFireTime;
    return TimeSinceLastFire >= FireRate;
}

int32 ABaseRifle::GetMaxAmmo() const
{
    return MaxAmmo;
}

bool ABaseRifle::GetIsActionHappening() const
{
    return bActionHappening;
}

// Called when the game starts or when spawned
void ABaseRifle::BeginPlay()
{
	Super::BeginPlay();

    CurrentAmmo = MaxAmmo;
    
    LastFireTime = -FireRate;
    bActionHappening = false;

    UE_LOG(LogRifle, Log, TEXT("[%s] Rifle initialized: %d/%d ammo"), *GetName(), CurrentAmmo, MaxAmmo);

}
void ABaseRifle::ExecuteActionStopped()
{
    ActionStopped();
}


void ABaseRifle::OnFireRateComplete()
{
   
        OnRifleAttack.Broadcast();
        UE_LOG(LogRifle, Warning, TEXT("[%s] Fire rate complete - Broadcasting OnRifleAttack"),
            *GetName());
    
    
}

void ABaseRifle::SpawnProjectile()
{
    if (!ProjectileClass)
    {
        UE_LOG(LogRifle, Error, TEXT("%s: No ProjectileClass set!"), *GetName());
        return;
    }
    //Get owner to determine faction
    AActor* ShooterPawn = GetOwner();
    if (!ShooterPawn)
    {
        UE_LOG(LogRifle, Error, TEXT("%s: Rifle has no owner! Cannot fire."), *GetName());
        return;
    }
    //Determine faction based on owner class
    ECombatFaction Faction = ECombatFaction::Unknown;
    if (ShooterPawn->IsA<ABasePlayer>())
    {
        Faction = ECombatFaction::Player;
    }
    else if (ShooterPawn->IsA<ABaseAgent>())
    {
        Faction = ECombatFaction::Agent;
    }
 

    // Get muzzle socket location
    FVector SpawnLocation;
    FVector FireDirection;
    if (RifleMesh && !MuzzleSocketName.IsNone() && RifleMesh->DoesSocketExist(MuzzleSocketName))
    {
        FTransform SocketTransform = RifleMesh->GetSocketTransform(MuzzleSocketName);
        SpawnLocation = SocketTransform.GetLocation();
        FireDirection = SocketTransform.GetRotation().GetForwardVector();
    }
    else
    {
        SpawnLocation = ShooterPawn->GetActorLocation();
        FireDirection = ShooterPawn->GetActorForwardVector();
        UE_LOG(LogRifle, Warning, TEXT("[%s] Muzzle socket not found — using actor location"), *GetName());

    }
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = ShooterPawn;
    SpawnParams.Instigator = Cast<APawn>(ShooterPawn);
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AProjectile* Projectile = GetWorld()->SpawnActor<AProjectile>(
        ProjectileClass,
        SpawnLocation,
        FRotator::ZeroRotator,
        SpawnParams
    );

    if (Projectile)
    {
        float FinalDamage = Damage * DamageMultiplier;
        Projectile->InitializeProjectile(FinalDamage, FireDirection, Faction);
        UE_LOG(LogRifle, Log, TEXT("[%s] Damage: %.1f (Base: %.1f x Multiplier: %.1f)"),
            *GetName(), FinalDamage, Damage, DamageMultiplier);
    }
    else
    {
        UE_LOG(LogRifle, Error, TEXT("%s: Failed to spawn projectile!"), *GetName());
    }
}


