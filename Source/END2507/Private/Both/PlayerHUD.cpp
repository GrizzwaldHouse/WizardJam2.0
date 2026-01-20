// Fill out your copyright notice in the Description page of Project Settings.


#include "Both/PlayerHUD.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/TextBlock.h"
#include "../END2507.h"

DEFINE_LOG_CATEGORY_STATIC(LogPlayerHUD, Log, All);
void UPlayerHUD::NativeConstruct()
{
    Super::NativeConstruct();

    // Crosshair is already created in UMG - just verify it exists
    if (Crosshair)
    {
        // Set default white color
        Crosshair->SetColorAndOpacity(FLinearColor::White);
        UE_LOG(LogPlayerHUD, Log, TEXT("Crosshair widget initialized - ready for color changes"));
    }
    else
    {
        UE_LOG(LogPlayerHUD, Warning, TEXT("Crosshair widget not found - check UMG binding"));
    }
}
void UPlayerHUD::UpdateHealthBar(float HealthPercent)
{
    if (HealthBar)
    {
        HealthBar->SetPercent(HealthPercent);

        // Calculate and set color based on health
        FLinearColor HealthColor = (HealthPercent <= 0.2f) ? FLinearColor::Red :
            (HealthPercent <= 0.5f) ? FLinearColor(1.0f, 0.5f, 0.0f, 1.0f) :
            FLinearColor::Green;

        // Apply the color
        HealthBar->SetFillColorAndOpacity(HealthColor);

        UE_LOG(LogTemp, Log, TEXT("Health bar updated to: %f%% with color R:%f G:%f B:%f"),
            HealthPercent * 100.0f, HealthColor.R, HealthColor.G, HealthColor.B);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("HealthBar widget is not bound!"));
    }
}

void UPlayerHUD::SetAmmo(float Current, float Max)
{
    // Validate text widgets exist (exact names in UMG: CurrentAmmo and MaxAmmo)
    if (!CurrentAmmo || !MaxAmmo)
    {
        UE_LOG(LogPlayerHUD, Error, TEXT("Ammo text widgets not bound — Check UMG widget names match exactly!"));
        return;
    }

    // Convert to numbers for display; AsNumber accepts floats/ints
    CurrentAmmo->SetText(FText::AsNumber(Current));
    MaxAmmo->SetText(FText::AsNumber(Max));

    // Calculate ammo ratio and apply color gradient (avoid divide by zero)
    const float AmmoRatio = (Max > 0.0f) ? (Current / Max) : 0.0f;
    const float ClampedRatio = FMath::Clamp(AmmoRatio, 0.0f, 1.0f);

    const FLinearColor AmmoColor = (ClampedRatio <= 0.2f) ? FLinearColor::Red :
        (ClampedRatio <= 0.5f) ? FLinearColor(1.0f, 0.5f, 0.0f, 1.0f) :
        FLinearColor::Green;

    // Apply color to current ammo text
    CurrentAmmo->SetColorAndOpacity(FSlateColor(AmmoColor));

    // For the log: print integer-style ammo counts and ratio as percent.
    const int32 CurrentInt = FMath::RoundToInt(Current);
    const int32 MaxInt = FMath::RoundToInt(Max);

    UE_LOG(LogPlayerHUD, Log, TEXT("Ammo counter updated: %d/%d (%.0f%%) — Color: R:%.2f G:%.2f B:%.2f"),
        CurrentInt, MaxInt, ClampedRatio * 100.0f, AmmoColor.R, AmmoColor.G, AmmoColor.B);

}

void UPlayerHUD::SetReticleColor(const FLinearColor& NewColor)
{
    if (!Crosshair)
    {
        UE_LOG(LogPlayerHUD, Warning, TEXT("Crosshair widget is null - cannot set color"));
        return;
    }
    Crosshair->SetColorAndOpacity(NewColor);
    UE_LOG(LogPlayerHUD, VeryVerbose, TEXT("Reticle color set to R:%.2f G:%.2f B:%.2f"),
        NewColor.R, NewColor.G, NewColor.B);
}

