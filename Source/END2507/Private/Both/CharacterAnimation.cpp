// Fill out your copyright notice in the Description page of Project Settings.


#include "Both/CharacterAnimation.h"
#include "Code/Actors/BaseCharacter.h"
#include "KismetAnimationLibrary.h"
#include "Animation/AnimSequence.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "../END2507.h"
DEFINE_LOG_CATEGORY_STATIC(LogCharacterAnimation, Log, All);

UCharacterAnimation::UCharacterAnimation()
    : Velocity(0.0f)
    , MovementDirection(0.0f)
    , SpineRotation(FRotator::ZeroRotator)
    , SpineBoneName(TEXT("spine_02"))
    , bIsFiring(false)
    , FireCooldownTime(0.8f)
    , bIsHit(false)
    , bIsDead(false)
    , bIsReloading(false)
    , OwningCharacter(nullptr)
    , ActionSlotName(TEXT("ActionSlotName"))
    , FireAsset(nullptr)
    , ReloadAsset(nullptr)
    , HitAsset(nullptr)
    , CurrentHitAsset(nullptr)
    , CurrentDeathAsset(nullptr)
    , CurrentReloadAsset(nullptr)
    , bDebug(false)
{
}

void UCharacterAnimation::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    // Cache owning character reference and bind death delegate
    APawn* Pawn = TryGetPawnOwner();
    if (Pawn)
    {
        OwningCharacter = Cast<ABaseCharacter>(Pawn);
        if (OwningCharacter)
        {
            // Bind to death delegate - animation will play when character dies
            OwningCharacter->OnCharacterDeath.AddDynamic(this, &UCharacterAnimation::HandleCharacterDeath);
            UE_LOG(LogTemp, Log, TEXT("✓ Death delegate bound for %s"), *OwningCharacter->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("✗ Failed to cast pawn to BaseCharacter"));
        }
    }
}

void UCharacterAnimation::NativeUninitializeAnimation()
{
    // Unbind death delegate to prevent stale reference crash
    if (OwningCharacter)
    {
        OwningCharacter->OnCharacterDeath.RemoveDynamic(this, &UCharacterAnimation::HandleCharacterDeath);
    }

    Super::NativeUninitializeAnimation();
}

void UCharacterAnimation::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
	PreviewWindowUpdate();

	APawn* Pawn = TryGetPawnOwner();
    if (!Pawn)
    {
        Velocity = 0.0f;
        MovementDirection = 0.0f;
        SpineRotation = FRotator::ZeroRotator;
        PreviewWindowUpdate();
        return;
    }
    if (!OwningCharacter)
    {
        OwningCharacter = Cast<ABaseCharacter>(Pawn);
    }
    Velocity = Pawn->GetVelocity().Size();
    MovementDirection = UKismetAnimationLibrary::CalculateDirection(
        Pawn->GetVelocity(),
        Pawn->GetActorRotation()
    );
    // Update animation assets and spine rotation from character
    if (OwningCharacter)
    {
        SpineRotation = OwningCharacter->GetSpineTargetRotation();
        HitAsset = OwningCharacter->GetHitAsset();
        DeathAssets = OwningCharacter->GetDeathAssets();
    }
    else
    {
       
        SpineRotation = FRotator::ZeroRotator;
    }

}

bool UCharacterAnimation::GetIsFiring() const
{
    return bIsFiring;
}

bool UCharacterAnimation::GetIsHit() const
{
    return bIsHit;
}

bool UCharacterAnimation::GetIsDead() const
{
    return bIsDead;
}
bool UCharacterAnimation::GetIsReloading() const
{
    return bIsReloading;
}
void UCharacterAnimation::HandleCharacterDeath()
{
    UE_LOG(LogTemp, Warning, TEXT("===== HandleCharacterDeath CALLED ====="));

    // Set death state 
    bIsDead = true;
    bIsHit = false;
    bIsFiring = false;

  
    if (DeathAssets.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT(" DeathAssets array is EMPTY - cannot play death animation!"));
        return;
    }

    // Select random death animation 
    const int32 RandomIndex = FMath::RandRange(0, DeathAssets.Num() - 1);
    CurrentDeathAsset = DeathAssets[RandomIndex];

    if (!CurrentDeathAsset)
    {
        UE_LOG(LogTemp, Error, TEXT(" Selected death asset at index %d is NULL"), RandomIndex);
        return;
    }

    // Play death montage 
    UAnimMontage* DeathMontage = PlaySlotAnimationAsDynamicMontage(
        CurrentDeathAsset,
        ActionSlotName,
        0.25f,   // Faster blend in for immediate response
        0.25f,  // BlendOutTime
        1.0f,   // PlayRate
        1,      // LoopCount (play once)
        0.0f,   // BlendOutTriggerTime
        0.0f    // InTimeToStartMontageAt
    );
    if (DeathMontage)
    {
        // Force the montage to play at maximum priority
        if (UAnimInstance* AnimInstance = Cast<UAnimInstance>(this))
        {
            FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveMontageInstance();
            if (MontageInstance)
            {
                MontageInstance->SetWeight(1.0f);
                MontageInstance->SetPosition(0.0f);
                UE_LOG(LogCharacterAnimation, Warning,
                    TEXT(" Death montage weight set to MAXIMUM"));
            }
        }

        UE_LOG(LogCharacterAnimation, Warning,
            TEXT("Death animation %d of %d playing: %s | Length: %.2fs"),
            RandomIndex + 1, DeathAssets.Num(),
            *CurrentDeathAsset->GetName(),
            CurrentDeathAsset->GetPlayLength());
    }
    else
    {
        UE_LOG(LogCharacterAnimation, Error,
            TEXT("PlaySlotAnimationAsDynamicMontage FAILED for %s"),
            *CurrentDeathAsset->GetName());
    }

    // Clear all active timers
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearAllTimersForObject(this);
    }

}
void UCharacterAnimation::CallOnActionEnded()
{
    UE_LOG(LogCharacterAnimation, Log, TEXT("[%s]  action complete — Broadcasting OnActionEnded delegate"), *GetName());
    // Reset animation states
    bIsReloading = false;
    bIsFiring = false;
    CurrentReloadAsset = nullptr;
    OnActionEnded.Broadcast();
}

void UCharacterAnimation::CallOnReloadNow()
{
    UE_LOG(LogCharacterAnimation, Log, TEXT("[%s] ammo reload initiated — Broadcasting OnReloadNow delegate"), *GetName());

    // Broadcast to BaseCharacter → triggers rifle ReloadAmmo()
    OnReloadNow.Broadcast();
}
void UCharacterAnimation::ReloadAnimationFunction_Implementation()
{
    if (!ReloadAsset)
    {
        UE_LOG(LogCharacterAnimation, Error, TEXT("[%s] ReloadAsset is null — cannot play reload animation!"), *GetName());
        return;
    }
    if (bIsDead)
    {
        UE_LOG(LogCharacterAnimation, Warning, TEXT("Cannot reload — character is dead"));
        return;
    }

    // Set reloading state
    bIsReloading = true;
    CurrentReloadAsset = ReloadAsset;
    PlaySlotAnimationAsDynamicMontage(ReloadAsset, ActionSlotName);
    UE_LOG(LogCharacterAnimation, Log, TEXT("Reloading spell activated — Animation playing: %s"), *ReloadAsset->GetName());

}
void UCharacterAnimation::HitAnimation_Implementation(float Ratio)
{
    if (bIsDead)
    {
        UE_LOG(Game, Warning, TEXT("Cannot play hit animation - character is dead"));
        return;
    }

    bIsHit = true;
    CurrentHitAsset = HitAsset;

    if (HitAsset)
    {
        PlaySlotAnimationAsDynamicMontage(HitAsset, ActionSlotName);
        UE_LOG(Game, Log, TEXT("Hit animation played: %s"), *HitAsset->GetName());

        // Reset hit flag after animation duration
        if (UWorld* World = GetWorld())
        {
            FTimerHandle HitResetClock;
            World->GetTimerManager().SetTimer(
                HitResetClock,
                [this]() {
                    bIsHit = false;
                    CurrentHitAsset = nullptr;
                },
                HitAsset->GetPlayLength(),
                false
            );
        }
    }
    else
    {
        UE_LOG(Game, Error, TEXT("HitAsset is null - cannot play hit animation"));
    }
}

void UCharacterAnimation::DeathAnimation_Implementation()
{
    bIsDead = true;
    bIsHit = false;
    bIsFiring = false;
    bIsReloading = false;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearAllTimersForObject(this);
    }
    if (DeathAssets.Num() == 0)
    {
        UE_LOG(LogCharacterAnimation, Error, TEXT("❌ DeathAssets array is EMPTY!"));
        return;
    }

    // Select random death animation
    const int32 RandomIndex = FMath::RandRange(0, DeathAssets.Num() - 1);
    CurrentDeathAsset = DeathAssets[RandomIndex];

    if (!CurrentDeathAsset)
    {
        UE_LOG(LogCharacterAnimation, Error, TEXT("❌ Selected death asset is NULL!"));
        return;
    }

    // ✅ PLAY THE ANIMATION (same pattern as HitAnimation and FireAnimation)
    UAnimMontage* DeathMontage = PlaySlotAnimationAsDynamicMontage(
        CurrentDeathAsset,
        ActionSlotName,
        0.25f,   // BlendInTime - smooth transition
        0.25f,   // BlendOutTime
        1.0f,    // PlayRate
        1,       // LoopCount
        0.0f,    // BlendOutTriggerTime
        0.0f     // InTimeToStartMontageAt
    );

    if (DeathMontage)
    {
        UE_LOG(LogCharacterAnimation, Warning,
            TEXT("DeathAnimation_Implementation: Animation playing: %s | Length: %.2fs"),
            *CurrentDeathAsset->GetName(),
            CurrentDeathAsset->GetPlayLength());
    }
    else
    {
        UE_LOG(LogCharacterAnimation, Error,
            TEXT("DeathAnimation_Implementation: PlaySlotAnimationAsDynamicMontage FAILED!"));
    }
}

void UCharacterAnimation::FireAnimation_Implementation()
{
    if (!FireAsset)
    {
        UE_LOG(LogCharacterAnimation, Warning, TEXT("[%s] FireAsset is null — cannot play fire animation"), *GetName());
        return;
    }
    if (bIsDead)
    {
        UE_LOG(LogCharacterAnimation, Warning, TEXT("Cannot fire — character is dead"));
        return;
    }

    if (FireAsset)
    {
        PlaySlotAnimationAsDynamicMontage(FireAsset, ActionSlotName);
        bIsFiring = true;

        UE_LOG(Game, Log, TEXT("Fire animation played"));

        // Set up cooldown timer
        if (GetWorld())
        {
            GetWorld()->GetTimerManager().SetTimer(
                FireCooldownTimer,
                [this]() { bIsFiring = false; },
                FireCooldownTime,
                false
            );
        }
    }
    else
    {
        bIsFiring = false;
        UE_LOG(Game, Warning, TEXT("FireAsset is null, cannot play fire animation"));
    }
}

void UCharacterAnimation::PreviewWindowUpdate_Implementation()
{
    // Update animation variables for editor preview window debugging
 

   

    APawn* Pawn = TryGetPawnOwner();
    if (!Pawn)
    {
        return;
    }

    // Update basic movement variables for preview
    Velocity = Pawn->GetVelocity().Size();
    MovementDirection = UKismetAnimationLibrary::CalculateDirection(
        Pawn->GetVelocity(),
        Pawn->GetActorRotation()
    );

    // Update spine rotation if character reference exists
    if (OwningCharacter)
    {
        SpineRotation = OwningCharacter->GetSpineTargetRotation();
    }


}
