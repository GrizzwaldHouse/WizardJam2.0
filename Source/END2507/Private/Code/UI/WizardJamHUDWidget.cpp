// ============================================================================
// WizardJamHUDWidget.cpp
// Implementation of the main player HUD widget
//
// Developer: Marcus Daley
// Date: January 26, 2026
// Project: WizardJam2.0 (END2507)
// ============================================================================

#include "Code/UI/WizardJamHUDWidget.h"
#include "Code/UI/WizardJamQuidditchWidget.h"
#include "Code/AC_HealthComponent.h"
#include "Code/Utilities/AC_StaminaComponent.h"
#include "Code/Utilities/AC_SpellCollectionComponent.h"
#include "Code/Flight/AC_BroomComponent.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "Blueprint/WidgetTree.h"

DEFINE_LOG_CATEGORY(LogWizardJamHUD);

// ============================================================================
// LIFECYCLE
// ============================================================================

void UWizardJamHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    UE_LOG(LogWizardJamHUD, Log, TEXT("WizardJamHUDWidget NativeConstruct"));

    // Cache owner actor reference
    OwnerActor = GetOwningPlayerPawn();
    if (!OwnerActor)
    {
        UE_LOG(LogWizardJamHUD, Warning, TEXT("No owning player pawn found - HUD may not function correctly"));
    }

    // Initialize spell slot system from designer config
    InitializeSpellSlotSystem();

    // Initialize Quidditch widget if configured
    InitializeQuidditchWidget();

    // Cache component references and bind delegates
    CacheComponents();
    BindComponentDelegates();

    // Hide warning text initially
    if (OutOfStaminaWarningText)
    {
        OutOfStaminaWarningText->SetVisibility(ESlateVisibility::Collapsed);
    }

    // Hide interaction prompt initially
    if (InteractionPromptPanel)
    {
        InteractionPromptPanel->SetVisibility(ESlateVisibility::Collapsed);
    }

    UE_LOG(LogWizardJamHUD, Log, TEXT("WizardJamHUDWidget initialization complete"));
}

void UWizardJamHUDWidget::NativeDestruct()
{
    // Clean unbind to prevent dangling delegates
    UnbindComponentDelegates();

    // Clear runtime maps
    SpellSlotWidgets.Empty();
    SpellConfigLookup.Empty();

    Super::NativeDestruct();
}

// ============================================================================
// COMPONENT CACHING
// ============================================================================

void UWizardJamHUDWidget::CacheComponents()
{
    if (!OwnerActor)
    {
        UE_LOG(LogWizardJamHUD, Warning, TEXT("CacheComponents: OwnerActor is null"));
        return;
    }

    // Find all relevant components
    HealthComp = OwnerActor->FindComponentByClass<UAC_HealthComponent>();
    StaminaComp = OwnerActor->FindComponentByClass<UAC_StaminaComponent>();
    SpellCollectionComp = OwnerActor->FindComponentByClass<UAC_SpellCollectionComponent>();
    BroomComp = OwnerActor->FindComponentByClass<UAC_BroomComponent>();

    // Log what we found
    UE_LOG(LogWizardJamHUD, Log, TEXT("Component cache: Health=%s, Stamina=%s, SpellCollection=%s, Broom=%s"),
        HealthComp ? TEXT("Found") : TEXT("Missing"),
        StaminaComp ? TEXT("Found") : TEXT("Missing"),
        SpellCollectionComp ? TEXT("Found") : TEXT("Missing"),
        BroomComp ? TEXT("Found") : TEXT("Missing"));
}

// ============================================================================
// DELEGATE BINDING
// ============================================================================

void UWizardJamHUDWidget::BindComponentDelegates()
{
    BindHealthComponentDelegates();
    BindStaminaComponentDelegates();
    BindSpellCollectionDelegates();
    BindBroomComponentDelegates();
}

void UWizardJamHUDWidget::UnbindComponentDelegates()
{
    if (HealthComp)
    {
        HealthComp->OnHealthChanged.RemoveDynamic(this, &UWizardJamHUDWidget::HandleHealthChanged);
    }

    if (StaminaComp)
    {
        StaminaComp->OnStaminaChanged.RemoveDynamic(this, &UWizardJamHUDWidget::HandleStaminaChanged);
    }

    if (SpellCollectionComp)
    {
        SpellCollectionComp->OnSpellAdded.RemoveDynamic(this, &UWizardJamHUDWidget::HandleSpellAdded);
        SpellCollectionComp->OnChannelAdded.RemoveDynamic(this, &UWizardJamHUDWidget::HandleChannelAdded);
    }

    if (BroomComp)
    {
        BroomComp->OnFlightStateChanged.RemoveDynamic(this, &UWizardJamHUDWidget::HandleFlightStateChanged);
        BroomComp->OnStaminaVisualUpdate.RemoveDynamic(this, &UWizardJamHUDWidget::HandleStaminaColorChange);
        BroomComp->OnForcedDismount.RemoveDynamic(this, &UWizardJamHUDWidget::HandleForcedDismount);
        BroomComp->OnBoostStateChanged.RemoveDynamic(this, &UWizardJamHUDWidget::HandleBoostChange);
    }
}

void UWizardJamHUDWidget::BindHealthComponentDelegates()
{
    if (!HealthComp)
    {
        UE_LOG(LogWizardJamHUD, Warning, TEXT("HealthComponent not found - health display will not update"));
        return;
    }

    HealthComp->OnHealthChanged.AddDynamic(this, &UWizardJamHUDWidget::HandleHealthChanged);

    // Initialize with current health
    HandleHealthChanged(HealthComp->GetHealthRatio());

    UE_LOG(LogWizardJamHUD, Log, TEXT("Bound to HealthComponent delegates"));
}

void UWizardJamHUDWidget::BindStaminaComponentDelegates()
{
    if (!StaminaComp)
    {
        UE_LOG(LogWizardJamHUD, Warning, TEXT("StaminaComponent not found - stamina display will not update"));
        return;
    }

    StaminaComp->OnStaminaChanged.AddDynamic(this, &UWizardJamHUDWidget::HandleStaminaChanged);

    // Initialize with current stamina
    UpdateStaminaVisual(StaminaComp->GetStaminaPercent());

    UE_LOG(LogWizardJamHUD, Log, TEXT("Bound to StaminaComponent delegates"));
}

void UWizardJamHUDWidget::BindSpellCollectionDelegates()
{
    if (!SpellCollectionComp)
    {
        UE_LOG(LogWizardJamHUD, Warning, TEXT("SpellCollectionComponent not found - spell slots will not update"));
        return;
    }

    SpellCollectionComp->OnSpellAdded.AddDynamic(this, &UWizardJamHUDWidget::HandleSpellAdded);
    SpellCollectionComp->OnChannelAdded.AddDynamic(this, &UWizardJamHUDWidget::HandleChannelAdded);

    // Initialize with current spells
    RefreshAllSpellSlots();

    UE_LOG(LogWizardJamHUD, Log, TEXT("Bound to SpellCollectionComponent delegates"));
}

void UWizardJamHUDWidget::BindBroomComponentDelegates()
{
    if (!BroomComp)
    {
        UE_LOG(LogWizardJamHUD, Warning, TEXT("BroomComponent not found - flight status will not update"));
        return;
    }

    BroomComp->OnFlightStateChanged.AddDynamic(this, &UWizardJamHUDWidget::HandleFlightStateChanged);
    BroomComp->OnStaminaVisualUpdate.AddDynamic(this, &UWizardJamHUDWidget::HandleStaminaColorChange);
    BroomComp->OnForcedDismount.AddDynamic(this, &UWizardJamHUDWidget::HandleForcedDismount);
    BroomComp->OnBoostStateChanged.AddDynamic(this, &UWizardJamHUDWidget::HandleBoostChange);

    // Initialize with current flight state
    HandleFlightStateChanged(BroomComp->IsFlying());

    UE_LOG(LogWizardJamHUD, Log, TEXT("Bound to BroomComponent delegates"));
}

// ============================================================================
// SPELL SLOT SYSTEM
// ============================================================================

void UWizardJamHUDWidget::InitializeSpellSlotSystem()
{
    // Clear any existing mappings
    SpellSlotWidgets.Empty();
    SpellConfigLookup.Empty();

    // Build lookup map from designer's config array
    for (const FSpellSlotConfig& Config : SpellSlotConfigs)
    {
        if (!Config.IsValid())
        {
            continue;
        }

        // Find the widget for this slot index
        UImage* SlotWidget = FindSpellSlotWidget(Config.SlotIndex);
        if (SlotWidget)
        {
            SpellSlotWidgets.Add(Config.SpellTypeName, SlotWidget);
            SpellConfigLookup.Add(Config.SpellTypeName, Config);

            // Initialize as locked
            UpdateSpellSlotVisual(Config.SpellTypeName, false);

            UE_LOG(LogWizardJamHUD, Log, TEXT("Mapped spell '%s' to SpellSlot_%d"),
                *Config.SpellTypeName.ToString(), Config.SlotIndex);
        }
        else
        {
            UE_LOG(LogWizardJamHUD, Warning, TEXT("No widget found for SpellSlot_%d (spell: %s)"),
                Config.SlotIndex, *Config.SpellTypeName.ToString());
        }
    }

    // Update count text
    UpdateSpellCountText();

    UE_LOG(LogWizardJamHUD, Log, TEXT("Spell slot system initialized with %d slots"), SpellSlotWidgets.Num());
}

UImage* UWizardJamHUDWidget::FindSpellSlotWidget(int32 SlotIndex)
{
    // Look for widget named "SpellSlot_X" where X is the index
    FString WidgetName = FString::Printf(TEXT("SpellSlot_%d"), SlotIndex);

    // Search in widget tree
    if (WidgetTree)
    {
        UWidget* FoundWidget = WidgetTree->FindWidget(FName(*WidgetName));
        if (UImage* ImageWidget = Cast<UImage>(FoundWidget))
        {
            return ImageWidget;
        }
    }

    // Fallback: search in SpellSlotContainer's children if it exists
    if (SpellSlotContainer)
    {
        for (int32 i = 0; i < SpellSlotContainer->GetChildrenCount(); ++i)
        {
            UWidget* Child = SpellSlotContainer->GetChildAt(i);
            if (Child && Child->GetName() == WidgetName)
            {
                if (UImage* ImageWidget = Cast<UImage>(Child))
                {
                    return ImageWidget;
                }
            }
        }
    }

    return nullptr;
}

void UWizardJamHUDWidget::UpdateSpellSlotVisual(FName SpellTypeName, bool bIsUnlocked)
{
    UImage** FoundWidget = SpellSlotWidgets.Find(SpellTypeName);
    if (!FoundWidget || !(*FoundWidget))
    {
        return;
    }

    FSpellSlotConfig* FoundConfig = SpellConfigLookup.Find(SpellTypeName);
    if (!FoundConfig)
    {
        return;
    }

    UImage* SlotImage = *FoundWidget;

    // Get appropriate icon and color from config
    UTexture2D* Icon = FoundConfig->GetIcon(bIsUnlocked);
    FLinearColor Tint = FoundConfig->GetColor(bIsUnlocked);

    // Apply texture
    if (Icon)
    {
        SlotImage->SetBrushFromTexture(Icon);
    }

    // Apply color tint
    SlotImage->SetColorAndOpacity(Tint);

    UE_LOG(LogWizardJamHUD, Verbose, TEXT("Updated spell slot '%s' to %s"),
        *SpellTypeName.ToString(), bIsUnlocked ? TEXT("UNLOCKED") : TEXT("LOCKED"));
}

void UWizardJamHUDWidget::UpdateSpellCountText()
{
    if (!SpellCountText) return;

    int32 UnlockedCount = 0;
    int32 TotalCount = SpellSlotConfigs.Num();

    // Count unlocked spells
    if (SpellCollectionComp)
    {
        for (const FSpellSlotConfig& Config : SpellSlotConfigs)
        {
            if (Config.IsValid() && SpellCollectionComp->HasSpell(Config.SpellTypeName))
            {
                UnlockedCount++;
            }
        }
    }

    // Update text (e.g., "2/4 Spells")
    FString CountStr = FString::Printf(TEXT("%d/%d Spells"), UnlockedCount, TotalCount);
    SpellCountText->SetText(FText::FromString(CountStr));
}

void UWizardJamHUDWidget::RefreshAllSpellSlots()
{
    if (!SpellCollectionComp)
    {
        return;
    }

    // Update each configured spell slot
    for (const FSpellSlotConfig& Config : SpellSlotConfigs)
    {
        if (Config.IsValid())
        {
            bool bIsUnlocked = SpellCollectionComp->HasSpell(Config.SpellTypeName);
            UpdateSpellSlotVisual(Config.SpellTypeName, bIsUnlocked);
        }
    }

    UpdateSpellCountText();
}

// ============================================================================
// QUIDDITCH WIDGET
// ============================================================================

void UWizardJamHUDWidget::InitializeQuidditchWidget()
{
    if (!QuidditchWidgetClass)
    {
        UE_LOG(LogWizardJamHUD, Log, TEXT("No QuidditchWidgetClass set - Quidditch UI disabled"));
        return;
    }

    // Create the Quidditch widget
    QuidditchWidget = CreateWidget<UWizardJamQuidditchWidget>(GetOwningPlayer(), QuidditchWidgetClass);
    if (QuidditchWidget)
    {
        QuidditchWidget->AddToViewport(1); // Layer above main HUD

        // Set initial visibility
        if (bShowQuidditchOnStart)
        {
            ShowQuidditchUI();
        }
        else
        {
            HideQuidditchUI();
        }

        UE_LOG(LogWizardJamHUD, Log, TEXT("Quidditch widget created and added to viewport"));
    }
}

void UWizardJamHUDWidget::ShowQuidditchUI()
{
    if (QuidditchWidget)
    {
        QuidditchWidget->SetVisibility(ESlateVisibility::Visible);
    }
}

void UWizardJamHUDWidget::HideQuidditchUI()
{
    if (QuidditchWidget)
    {
        QuidditchWidget->SetVisibility(ESlateVisibility::Collapsed);
    }
}

bool UWizardJamHUDWidget::IsQuidditchUIVisible() const
{
    return QuidditchWidget && QuidditchWidget->IsVisible();
}

// ============================================================================
// VISUAL UPDATE FUNCTIONS
// ============================================================================

void UWizardJamHUDWidget::UpdateHealthVisual(float HealthRatio)
{
    // Clamp to valid range
    float ClampedRatio = FMath::Clamp(HealthRatio, 0.0f, 1.0f);

    // Update progress bar
    if (HealthProgressBar)
    {
        HealthProgressBar->SetPercent(ClampedRatio);

        // Color gradient: Green > Orange > Red
        FLinearColor HealthColor;
        if (ClampedRatio <= 0.2f)
        {
            HealthColor = FLinearColor::Red;
        }
        else if (ClampedRatio <= 0.5f)
        {
            HealthColor = FLinearColor(1.0f, 0.5f, 0.0f, 1.0f); // Orange
        }
        else
        {
            HealthColor = FLinearColor::Green;
        }

        HealthProgressBar->SetFillColorAndOpacity(HealthColor);
    }

    // Update text if present
    if (HealthText)
    {
        int32 HealthPercent = FMath::RoundToInt(ClampedRatio * 100.0f);
        HealthText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), HealthPercent)));
    }
}

void UWizardJamHUDWidget::UpdateStaminaVisual(float StaminaPercent)
{
    // Clamp to valid range
    float ClampedPercent = FMath::Clamp(StaminaPercent, 0.0f, 1.0f);

    // Update progress bar
    if (StaminaProgressBar)
    {
        StaminaProgressBar->SetPercent(ClampedPercent);

        // Color gradient: Green > Orange > Red
        FLinearColor StaminaColor;
        if (ClampedPercent <= 0.2f)
        {
            StaminaColor = FLinearColor::Red;
        }
        else if (ClampedPercent <= 0.5f)
        {
            StaminaColor = FLinearColor(1.0f, 0.5f, 0.0f, 1.0f); // Orange
        }
        else
        {
            StaminaColor = FLinearColor::Green;
        }

        StaminaProgressBar->SetFillColorAndOpacity(StaminaColor);
    }

    // Update text if present
    if (StaminaText)
    {
        int32 StaminaPercentInt = FMath::RoundToInt(ClampedPercent * 100.0f);
        StaminaText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), StaminaPercentInt)));
    }
}

// ============================================================================
// DELEGATE HANDLERS
// ============================================================================

void UWizardJamHUDWidget::HandleHealthChanged(float HealthRatio)
{
    UpdateHealthVisual(HealthRatio);
}

void UWizardJamHUDWidget::HandleStaminaChanged(AActor* Owner, float NewStamina, float Delta)
{
    // Get percent from component if available
    if (StaminaComp)
    {
        UpdateStaminaVisual(StaminaComp->GetStaminaPercent());
    }
}

void UWizardJamHUDWidget::HandleSpellAdded(FName SpellType, int32 TotalSpellCount)
{
    UE_LOG(LogWizardJamHUD, Log, TEXT("Spell added: %s (Total: %d)"), *SpellType.ToString(), TotalSpellCount);

    // Update the specific spell slot to unlocked
    UpdateSpellSlotVisual(SpellType, true);

    // Update count text
    UpdateSpellCountText();
}

void UWizardJamHUDWidget::HandleChannelAdded(FName Channel)
{
    UE_LOG(LogWizardJamHUD, Log, TEXT("Channel added: %s"), *Channel.ToString());

    // Channels also unlock spell visuals (same system)
    UpdateSpellSlotVisual(Channel, true);
    UpdateSpellCountText();
}

void UWizardJamHUDWidget::HandleFlightStateChanged(bool bIsFlying)
{
    UE_LOG(LogWizardJamHUD, Log, TEXT("Flight state changed: %s"), bIsFlying ? TEXT("FLYING") : TEXT("GROUNDED"));

    // Update broom icon visibility/state
    if (BroomIcon)
    {
        // Could swap textures or change tint based on flight state
        FLinearColor IconColor = bIsFlying ? FLinearColor::White : FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);
        BroomIcon->SetColorAndOpacity(IconColor);
    }

    // Hide stamina warning when grounded
    if (!bIsFlying && OutOfStaminaWarningText)
    {
        OutOfStaminaWarningText->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UWizardJamHUDWidget::HandleStaminaColorChange(FLinearColor NewColor)
{
    // Override stamina bar color (used during flight for special states)
    if (StaminaProgressBar)
    {
        StaminaProgressBar->SetFillColorAndOpacity(NewColor);
    }
}

void UWizardJamHUDWidget::HandleForcedDismount()
{
    UE_LOG(LogWizardJamHUD, Log, TEXT("Forced dismount - stamina depleted"));

    // Show warning text briefly
    if (OutOfStaminaWarningText)
    {
        OutOfStaminaWarningText->SetText(FText::FromString(TEXT("OUT OF STAMINA!")));
        OutOfStaminaWarningText->SetVisibility(ESlateVisibility::Visible);

        // Hide after delay (would need timer or animation in full implementation)
    }
}

void UWizardJamHUDWidget::HandleBoostChange(bool bIsBoosting)
{
    UE_LOG(LogWizardJamHUD, Verbose, TEXT("Boost state changed: %s"), bIsBoosting ? TEXT("ON") : TEXT("OFF"));

    // Update boost indicator
    if (BoostIndicatorImage)
    {
        FLinearColor BoostColor = bIsBoosting ?
            FLinearColor(1.0f, 0.5f, 0.0f, 1.0f) : // Orange when boosting
            FLinearColor(0.3f, 0.3f, 0.3f, 1.0f);  // Gray when not boosting

        BoostIndicatorImage->SetColorAndOpacity(BoostColor);
    }
}
