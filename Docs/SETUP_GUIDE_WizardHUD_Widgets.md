# WizardJam HUD Widgets Setup Guide
## Developer: Marcus Daley | Date: January 26, 2026
## Target: Tuesday Demo - Quidditch Match with Full UI

---

## EXECUTIVE SUMMARY

This guide covers setting up three critical UI widgets for the WizardJam Quidditch game:

| Priority | Widget | Purpose | Est. Time |
|----------|--------|---------|-----------|
| 1 | **WizardPlayerHUD** | Player health, stamina, spells, interaction prompts | 2-3 hours |
| 2 | **QuidditchScoreboardWidget** | Match timer, team scores, current role | 1-2 hours |
| 3 | **QuidditchResultsWidget** | End-game results, winner display | 1-2 hours |

**Total Estimated: 4-7 hours**

---

## PHASE 1: WIZARD PLAYER HUD (HIGHEST PRIORITY)

### 1.1 Understanding Current Infrastructure

**Existing C++ Class:** `Source/END2507/Public/Both/PlayerHUD.h`
```cpp
// Current bound widgets:
UProgressBar* HealthBar;      // Health display
UTextBlock* CurrentAmmo;      // Ammo count (repurpose for spells)
UTextBlock* MaxAmmo;          // Max ammo (repurpose)
UImage* Crosshair;            // Reticle

// Current functions:
void UpdateHealthBar(float HealthPercent);
void SetAmmo(float Current, float Max);
void SetReticleColor(const FLinearColor& NewColor);
```

**Decision Point:** Extend existing `UPlayerHUD` or create new `UWizardPlayerHUD`?

**RECOMMENDATION:** Create NEW `UWizardPlayerHUD` that inherits from `UUserWidget` directly. This keeps the original intact for other projects and allows wizard-specific features.

---

### 1.2 Create WizardPlayerHUD C++ Class

**File Location:** `Source/END2507/Public/Code/UI/WizardPlayerHUD.h`

**Required BindWidget Properties:**
```cpp
// HEALTH SYSTEM
UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
UProgressBar* HealthBar;

// STAMINA SYSTEM (for flight)
UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
UProgressBar* StaminaBar;

// SPELL SLOTS (4 elements)
UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
UImage* SpellSlot_Flame;

UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
UImage* SpellSlot_Ice;

UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
UImage* SpellSlot_Lightning;

UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
UImage* SpellSlot_Arcane;

// BROOM/FLIGHT INDICATOR
UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
UImage* BroomStatusIcon;

UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
UTextBlock* FlightStatusText;

// INTERACTION PROMPT (Press E to...)
UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
UCanvasPanel* InteractionPromptPanel;

UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
UTextBlock* InteractionPromptText;

// CROSSHAIR
UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
UImage* Crosshair;

// OBJECTIVE/TASK DISPLAY
UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
UTextBlock* ObjectiveText;
```

**Required Delegate Bindings:**

| Delegate Source | Delegate Name | Handler Function |
|-----------------|---------------|------------------|
| `AC_HealthComponent` | `OnHealthChanged` | `HandleHealthChanged(float HealthRatio)` |
| `AC_StaminaComponent` | `OnStaminaChanged` | `HandleStaminaChanged(AActor*, float, float)` |
| `AC_SpellCollectionComponent` | `OnSpellAdded` | `HandleSpellAdded(FName, int32)` |
| `AC_SpellCollectionComponent` | `OnChannelAdded` | `HandleChannelAdded(FName)` |
| `AC_BroomComponent` | `OnFlightStateChanged` | `HandleFlightStateChanged(bool)` |
| `InteractionComponent` | `OnInteractableTargeted` | `HandleInteractableTargeted(bool)` |

---

### 1.3 WizardPlayerHUD Header File Template

Create file: `Source/END2507/Public/Code/UI/WizardPlayerHUD.h`

```cpp
// WizardPlayerHUD.h
// HUD widget for wizard player displaying health, stamina, spells, and flight status
// Developer: Marcus Daley
// Date: January 26, 2026

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WizardPlayerHUD.generated.h"

class UProgressBar;
class UTextBlock;
class UImage;
class UCanvasPanel;
class UAC_HealthComponent;
class UAC_StaminaComponent;
class UAC_SpellCollectionComponent;
class UAC_BroomComponent;

UCLASS()
class END2507_API UWizardPlayerHUD : public UUserWidget
{
    GENERATED_BODY()

public:
    // Initialize HUD and bind to player components
    UFUNCTION(BlueprintCallable, Category = "WizardHUD")
    void InitializeForPlayer(APlayerController* OwningPlayer);

    // ========================================================================
    // HEALTH DISPLAY
    // ========================================================================

    UFUNCTION(BlueprintCallable, Category = "WizardHUD|Health")
    void UpdateHealthBar(float HealthRatio);

    // ========================================================================
    // STAMINA DISPLAY
    // ========================================================================

    UFUNCTION(BlueprintCallable, Category = "WizardHUD|Stamina")
    void UpdateStaminaBar(float StaminaPercent);

    UFUNCTION(BlueprintCallable, Category = "WizardHUD|Stamina")
    void SetStaminaBarColor(const FLinearColor& NewColor);

    // ========================================================================
    // SPELL SLOTS
    // ========================================================================

    UFUNCTION(BlueprintCallable, Category = "WizardHUD|Spells")
    void SetSpellSlotUnlocked(FName SpellChannel, bool bUnlocked);

    UFUNCTION(BlueprintCallable, Category = "WizardHUD|Spells")
    void SetActiveSpellSlot(FName SpellChannel);

    // ========================================================================
    // FLIGHT STATUS
    // ========================================================================

    UFUNCTION(BlueprintCallable, Category = "WizardHUD|Flight")
    void SetFlightActive(bool bIsFlying);

    UFUNCTION(BlueprintCallable, Category = "WizardHUD|Flight")
    void SetBoostActive(bool bIsBoosting);

    // ========================================================================
    // INTERACTION PROMPT
    // ========================================================================

    UFUNCTION(BlueprintCallable, Category = "WizardHUD|Interaction")
    void ShowInteractionPrompt(const FText& PromptText);

    UFUNCTION(BlueprintCallable, Category = "WizardHUD|Interaction")
    void HideInteractionPrompt();

    // ========================================================================
    // OBJECTIVE DISPLAY
    // ========================================================================

    UFUNCTION(BlueprintCallable, Category = "WizardHUD|Objectives")
    void SetObjectiveText(const FText& NewObjective);

    // ========================================================================
    // CROSSHAIR
    // ========================================================================

    UFUNCTION(BlueprintCallable, Category = "WizardHUD|Crosshair")
    void SetCrosshairColor(const FLinearColor& NewColor);

protected:
    virtual void NativeConstruct() override;

    // ========================================================================
    // BOUND WIDGETS (Must match UMG widget names EXACTLY)
    // ========================================================================

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UProgressBar* HealthBar;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UProgressBar* StaminaBar;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UImage* SpellSlot_Flame;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UImage* SpellSlot_Ice;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UImage* SpellSlot_Lightning;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UImage* SpellSlot_Arcane;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UImage* BroomStatusIcon;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* FlightStatusText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UCanvasPanel* InteractionPromptPanel;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* InteractionPromptText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UImage* Crosshair;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* ObjectiveText;

    // ========================================================================
    // CONFIGURATION (Designer sets in Blueprint)
    // ========================================================================

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WizardHUD|Config")
    UTexture2D* SpellIcon_Flame_Locked;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WizardHUD|Config")
    UTexture2D* SpellIcon_Flame_Unlocked;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WizardHUD|Config")
    UTexture2D* SpellIcon_Ice_Locked;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WizardHUD|Config")
    UTexture2D* SpellIcon_Ice_Unlocked;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WizardHUD|Config")
    UTexture2D* SpellIcon_Lightning_Locked;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WizardHUD|Config")
    UTexture2D* SpellIcon_Lightning_Unlocked;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WizardHUD|Config")
    UTexture2D* SpellIcon_Arcane_Locked;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WizardHUD|Config")
    UTexture2D* SpellIcon_Arcane_Unlocked;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WizardHUD|Config")
    UTexture2D* BroomIcon_Grounded;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WizardHUD|Config")
    UTexture2D* BroomIcon_Flying;

private:
    // ========================================================================
    // DELEGATE HANDLERS
    // ========================================================================

    UFUNCTION()
    void HandleHealthChanged(float HealthRatio);

    UFUNCTION()
    void HandleStaminaChanged(AActor* Owner, float NewStamina, float Delta);

    UFUNCTION()
    void HandleSpellAdded(FName SpellType, int32 TotalCount);

    UFUNCTION()
    void HandleChannelAdded(FName Channel);

    UFUNCTION()
    void HandleFlightStateChanged(bool bIsFlying);

    UFUNCTION()
    void HandleBoostStateChanged(bool bIsBoosting);

    // ========================================================================
    // COMPONENT REFERENCES
    // ========================================================================

    UPROPERTY()
    UAC_HealthComponent* HealthComponent;

    UPROPERTY()
    UAC_StaminaComponent* StaminaComponent;

    UPROPERTY()
    UAC_SpellCollectionComponent* SpellComponent;

    UPROPERTY()
    UAC_BroomComponent* BroomComponent;

    // Helper to get spell slot image by channel name
    UImage* GetSpellSlotImage(FName Channel) const;
};
```

---

### 1.4 WizardPlayerHUD Implementation File Template

Create file: `Source/END2507/Private/Code/UI/WizardPlayerHUD.cpp`

```cpp
// WizardPlayerHUD.cpp
// Implementation of wizard player HUD widget
// Developer: Marcus Daley
// Date: January 26, 2026

#include "Code/UI/WizardPlayerHUD.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Code/AC_HealthComponent.h"
#include "Code/Utilities/AC_StaminaComponent.h"
#include "Code/Utilities/AC_SpellCollectionComponent.h"
#include "Code/Flight/AC_BroomComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogWizardHUD, Log, All);

void UWizardPlayerHUD::NativeConstruct()
{
    Super::NativeConstruct();

    // Hide interaction prompt by default
    if (InteractionPromptPanel)
    {
        InteractionPromptPanel->SetVisibility(ESlateVisibility::Collapsed);
    }

    // Set default stamina bar color (green)
    if (StaminaBar)
    {
        StaminaBar->SetFillColorAndOpacity(FLinearColor::Green);
    }

    UE_LOG(LogWizardHUD, Log, TEXT("WizardPlayerHUD constructed"));
}

void UWizardPlayerHUD::InitializeForPlayer(APlayerController* OwningPlayer)
{
    if (!OwningPlayer)
    {
        UE_LOG(LogWizardHUD, Warning, TEXT("InitializeForPlayer called with null controller"));
        return;
    }

    APawn* PlayerPawn = OwningPlayer->GetPawn();
    if (!PlayerPawn)
    {
        UE_LOG(LogWizardHUD, Warning, TEXT("Player pawn is null"));
        return;
    }

    // Find and bind to Health Component
    HealthComponent = PlayerPawn->FindComponentByClass<UAC_HealthComponent>();
    if (HealthComponent)
    {
        HealthComponent->OnHealthChanged.AddDynamic(this, &UWizardPlayerHUD::HandleHealthChanged);
        // Initialize with current health
        HandleHealthChanged(HealthComponent->GetHealthRatio());
        UE_LOG(LogWizardHUD, Log, TEXT("Bound to HealthComponent"));
    }

    // Find and bind to Stamina Component
    StaminaComponent = PlayerPawn->FindComponentByClass<UAC_StaminaComponent>();
    if (StaminaComponent)
    {
        StaminaComponent->OnStaminaChanged.AddDynamic(this, &UWizardPlayerHUD::HandleStaminaChanged);
        // Initialize with current stamina
        UpdateStaminaBar(StaminaComponent->GetStaminaPercent());
        UE_LOG(LogWizardHUD, Log, TEXT("Bound to StaminaComponent"));
    }

    // Find and bind to Spell Collection Component
    SpellComponent = PlayerPawn->FindComponentByClass<UAC_SpellCollectionComponent>();
    if (SpellComponent)
    {
        SpellComponent->OnSpellAdded.AddDynamic(this, &UWizardPlayerHUD::HandleSpellAdded);
        SpellComponent->OnChannelAdded.AddDynamic(this, &UWizardPlayerHUD::HandleChannelAdded);

        // Initialize with current spells
        TArray<FName> CurrentSpells = SpellComponent->GetAllSpells();
        for (const FName& Spell : CurrentSpells)
        {
            SetSpellSlotUnlocked(Spell, true);
        }
        UE_LOG(LogWizardHUD, Log, TEXT("Bound to SpellCollectionComponent"));
    }

    // Find and bind to Broom Component
    BroomComponent = PlayerPawn->FindComponentByClass<UAC_BroomComponent>();
    if (BroomComponent)
    {
        BroomComponent->OnFlightStateChanged.AddDynamic(this, &UWizardPlayerHUD::HandleFlightStateChanged);
        BroomComponent->OnBoostStateChanged.AddDynamic(this, &UWizardPlayerHUD::HandleBoostStateChanged);

        // Initialize with current flight state
        SetFlightActive(BroomComponent->IsFlying());
        UE_LOG(LogWizardHUD, Log, TEXT("Bound to BroomComponent"));
    }

    UE_LOG(LogWizardHUD, Log, TEXT("WizardPlayerHUD initialized for player"));
}

// ============================================================================
// HEALTH DISPLAY
// ============================================================================

void UWizardPlayerHUD::UpdateHealthBar(float HealthRatio)
{
    if (!HealthBar) return;

    HealthBar->SetPercent(HealthRatio);

    // Color gradient: Green > Orange > Red
    FLinearColor HealthColor = (HealthRatio <= 0.2f) ? FLinearColor::Red :
        (HealthRatio <= 0.5f) ? FLinearColor(1.0f, 0.5f, 0.0f, 1.0f) :
        FLinearColor::Green;

    HealthBar->SetFillColorAndOpacity(HealthColor);
}

void UWizardPlayerHUD::HandleHealthChanged(float HealthRatio)
{
    UpdateHealthBar(HealthRatio);
}

// ============================================================================
// STAMINA DISPLAY
// ============================================================================

void UWizardPlayerHUD::UpdateStaminaBar(float StaminaPercent)
{
    if (!StaminaBar) return;

    StaminaBar->SetPercent(StaminaPercent);

    // Update color based on stamina level
    FLinearColor StaminaColor = (StaminaPercent <= 0.2f) ? FLinearColor::Red :
        (StaminaPercent <= 0.5f) ? FLinearColor(1.0f, 0.5f, 0.0f, 1.0f) :
        FLinearColor::Green;

    StaminaBar->SetFillColorAndOpacity(StaminaColor);
}

void UWizardPlayerHUD::SetStaminaBarColor(const FLinearColor& NewColor)
{
    if (StaminaBar)
    {
        StaminaBar->SetFillColorAndOpacity(NewColor);
    }
}

void UWizardPlayerHUD::HandleStaminaChanged(AActor* Owner, float NewStamina, float Delta)
{
    if (StaminaComponent)
    {
        UpdateStaminaBar(StaminaComponent->GetStaminaPercent());
    }
}

// ============================================================================
// SPELL SLOTS
// ============================================================================

UImage* UWizardPlayerHUD::GetSpellSlotImage(FName Channel) const
{
    if (Channel == TEXT("Flame")) return SpellSlot_Flame;
    if (Channel == TEXT("Ice")) return SpellSlot_Ice;
    if (Channel == TEXT("Lightning")) return SpellSlot_Lightning;
    if (Channel == TEXT("Arcane")) return SpellSlot_Arcane;
    return nullptr;
}

void UWizardPlayerHUD::SetSpellSlotUnlocked(FName SpellChannel, bool bUnlocked)
{
    UImage* SlotImage = GetSpellSlotImage(SpellChannel);
    if (!SlotImage) return;

    // Get the appropriate texture
    UTexture2D* NewTexture = nullptr;

    if (SpellChannel == TEXT("Flame"))
    {
        NewTexture = bUnlocked ? SpellIcon_Flame_Unlocked : SpellIcon_Flame_Locked;
    }
    else if (SpellChannel == TEXT("Ice"))
    {
        NewTexture = bUnlocked ? SpellIcon_Ice_Unlocked : SpellIcon_Ice_Locked;
    }
    else if (SpellChannel == TEXT("Lightning"))
    {
        NewTexture = bUnlocked ? SpellIcon_Lightning_Unlocked : SpellIcon_Lightning_Locked;
    }
    else if (SpellChannel == TEXT("Arcane"))
    {
        NewTexture = bUnlocked ? SpellIcon_Arcane_Unlocked : SpellIcon_Arcane_Locked;
    }

    if (NewTexture)
    {
        SlotImage->SetBrushFromTexture(NewTexture);
    }

    UE_LOG(LogWizardHUD, Log, TEXT("Spell slot %s set to %s"),
        *SpellChannel.ToString(), bUnlocked ? TEXT("UNLOCKED") : TEXT("LOCKED"));
}

void UWizardPlayerHUD::SetActiveSpellSlot(FName SpellChannel)
{
    // Reset all slots to normal tint
    const FLinearColor NormalTint = FLinearColor::White;
    const FLinearColor ActiveTint = FLinearColor(1.0f, 1.0f, 0.5f, 1.0f); // Slight yellow highlight

    if (SpellSlot_Flame) SpellSlot_Flame->SetColorAndOpacity(NormalTint);
    if (SpellSlot_Ice) SpellSlot_Ice->SetColorAndOpacity(NormalTint);
    if (SpellSlot_Lightning) SpellSlot_Lightning->SetColorAndOpacity(NormalTint);
    if (SpellSlot_Arcane) SpellSlot_Arcane->SetColorAndOpacity(NormalTint);

    // Highlight the active slot
    UImage* ActiveSlot = GetSpellSlotImage(SpellChannel);
    if (ActiveSlot)
    {
        ActiveSlot->SetColorAndOpacity(ActiveTint);
    }
}

void UWizardPlayerHUD::HandleSpellAdded(FName SpellType, int32 TotalCount)
{
    SetSpellSlotUnlocked(SpellType, true);
}

void UWizardPlayerHUD::HandleChannelAdded(FName Channel)
{
    // Channels and spells use same unlock system
    SetSpellSlotUnlocked(Channel, true);
}

// ============================================================================
// FLIGHT STATUS
// ============================================================================

void UWizardPlayerHUD::SetFlightActive(bool bIsFlying)
{
    if (BroomStatusIcon)
    {
        UTexture2D* Icon = bIsFlying ? BroomIcon_Flying : BroomIcon_Grounded;
        if (Icon)
        {
            BroomStatusIcon->SetBrushFromTexture(Icon);
        }
        BroomStatusIcon->SetVisibility(ESlateVisibility::Visible);
    }

    if (FlightStatusText)
    {
        FlightStatusText->SetText(bIsFlying ?
            FText::FromString(TEXT("FLYING")) :
            FText::FromString(TEXT("GROUNDED")));
    }
}

void UWizardPlayerHUD::SetBoostActive(bool bIsBoosting)
{
    // Change stamina bar color to indicate boost
    if (StaminaBar)
    {
        if (bIsBoosting)
        {
            StaminaBar->SetFillColorAndOpacity(FLinearColor(1.0f, 0.5f, 0.0f, 1.0f)); // Orange
        }
        else if (StaminaComponent)
        {
            // Restore normal color based on percentage
            UpdateStaminaBar(StaminaComponent->GetStaminaPercent());
        }
    }
}

void UWizardPlayerHUD::HandleFlightStateChanged(bool bIsFlying)
{
    SetFlightActive(bIsFlying);
}

void UWizardPlayerHUD::HandleBoostStateChanged(bool bIsBoosting)
{
    SetBoostActive(bIsBoosting);
}

// ============================================================================
// INTERACTION PROMPT
// ============================================================================

void UWizardPlayerHUD::ShowInteractionPrompt(const FText& PromptText)
{
    if (InteractionPromptPanel)
    {
        InteractionPromptPanel->SetVisibility(ESlateVisibility::Visible);
    }
    if (InteractionPromptText)
    {
        InteractionPromptText->SetText(PromptText);
    }
}

void UWizardPlayerHUD::HideInteractionPrompt()
{
    if (InteractionPromptPanel)
    {
        InteractionPromptPanel->SetVisibility(ESlateVisibility::Collapsed);
    }
}

// ============================================================================
// OBJECTIVE DISPLAY
// ============================================================================

void UWizardPlayerHUD::SetObjectiveText(const FText& NewObjective)
{
    if (ObjectiveText)
    {
        ObjectiveText->SetText(NewObjective);
    }
}

// ============================================================================
// CROSSHAIR
// ============================================================================

void UWizardPlayerHUD::SetCrosshairColor(const FLinearColor& NewColor)
{
    if (Crosshair)
    {
        Crosshair->SetColorAndOpacity(NewColor);
    }
}
```

---

### 1.5 UMG Blueprint Setup (WBP_WizardPlayerHUD)

1. **Create Widget Blueprint:**
   - Content Browser > Right-click > User Interface > Widget Blueprint
   - Parent Class: `WizardPlayerHUD`
   - Name: `WBP_WizardPlayerHUD`
   - Save to: `Content/UI/`

2. **Widget Layout (Designer Tab):**

```
Canvas Panel (Root)
├── HealthBar (Progress Bar) - Top Left
│   └── Anchors: Top-Left, Position: (20, 20), Size: (200, 25)
│
├── StaminaBar (Progress Bar) - Below Health
│   └── Anchors: Top-Left, Position: (20, 55), Size: (200, 20)
│
├── SpellSlotContainer (Horizontal Box) - Bottom Center
│   ├── SpellSlot_Flame (Image) - Size: 64x64
│   ├── SpellSlot_Ice (Image) - Size: 64x64
│   ├── SpellSlot_Lightning (Image) - Size: 64x64
│   └── SpellSlot_Arcane (Image) - Size: 64x64
│
├── FlightStatusContainer (Vertical Box) - Top Right
│   ├── BroomStatusIcon (Image) - Size: 48x48
│   └── FlightStatusText (Text Block)
│
├── InteractionPromptPanel (Canvas Panel) - Center Screen
│   └── InteractionPromptText (Text Block)
│       └── Text: "[E] Equip Broom"
│
├── Crosshair (Image) - Center Screen
│   └── Anchors: Center, Size: 32x32
│
└── ObjectiveText (Text Block) - Top Center
    └── Anchors: Top-Center
```

3. **CRITICAL: Widget Names Must Match EXACTLY:**
   - `HealthBar`
   - `StaminaBar`
   - `SpellSlot_Flame`
   - `SpellSlot_Ice`
   - `SpellSlot_Lightning`
   - `SpellSlot_Arcane`
   - `BroomStatusIcon`
   - `FlightStatusText`
   - `InteractionPromptPanel`
   - `InteractionPromptText`
   - `Crosshair`
   - `ObjectiveText`

---

## PHASE 2: QUIDDITCH SCOREBOARD WIDGET

### 2.1 Create QuidditchScoreboardWidget C++ Class

**File Location:** `Source/END2507/Public/Code/UI/QuidditchScoreboardWidget.h`

**Required Bindings:**
```cpp
// TEAM SCORES
UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
UTextBlock* TeamAScore;

UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
UTextBlock* TeamBScore;

UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
UTextBlock* TeamAName;

UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
UTextBlock* TeamBName;

// MATCH TIMER
UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
UTextBlock* MatchTimerText;

// PLAYER ROLE
UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
UTextBlock* CurrentRoleText;

UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
UImage* RoleIcon;

// SNITCH STATUS
UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
UImage* SnitchStatusIcon;

UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
UTextBlock* SnitchStatusText;
```

**Required Delegate Bindings:**

| Delegate Source | Delegate Name | Handler |
|-----------------|---------------|---------|
| `QuidditchGameMode` | `OnQuidditchTeamScored` | `HandleTeamScored` |
| `QuidditchGameMode` | `OnSnitchCaught` | `HandleSnitchCaught` |
| `QuidditchGameMode` | `OnQuidditchRoleAssigned` | `HandleRoleAssigned` |
| `WorldSignalEmitter` | Signal: `QuidditchMatchStart` | `HandleMatchStart` |

---

### 2.2 QuidditchScoreboardWidget Header Template

Create file: `Source/END2507/Public/Code/UI/QuidditchScoreboardWidget.h`

```cpp
// QuidditchScoreboardWidget.h
// Scoreboard widget for Quidditch matches displaying scores, timer, and roles
// Developer: Marcus Daley
// Date: January 26, 2026

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Code/GameModes/QuidditchGameMode.h"
#include "QuidditchScoreboardWidget.generated.h"

class UTextBlock;
class UImage;

UCLASS()
class END2507_API UQuidditchScoreboardWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // Initialize and bind to GameMode delegates
    UFUNCTION(BlueprintCallable, Category = "Scoreboard")
    void InitializeScoreboard();

    // ========================================================================
    // SCORE DISPLAY
    // ========================================================================

    UFUNCTION(BlueprintCallable, Category = "Scoreboard|Scores")
    void UpdateTeamScore(EQuidditchTeam Team, int32 NewScore);

    UFUNCTION(BlueprintCallable, Category = "Scoreboard|Scores")
    void SetTeamNames(const FString& TeamA, const FString& TeamB);

    // ========================================================================
    // TIMER
    // ========================================================================

    UFUNCTION(BlueprintCallable, Category = "Scoreboard|Timer")
    void StartMatchTimer(float MatchDurationSeconds);

    UFUNCTION(BlueprintCallable, Category = "Scoreboard|Timer")
    void StopMatchTimer();

    UFUNCTION(BlueprintCallable, Category = "Scoreboard|Timer")
    void UpdateTimerDisplay(float RemainingSeconds);

    // ========================================================================
    // ROLE DISPLAY
    // ========================================================================

    UFUNCTION(BlueprintCallable, Category = "Scoreboard|Role")
    void SetPlayerRole(EQuidditchRole Role);

    // ========================================================================
    // SNITCH STATUS
    // ========================================================================

    UFUNCTION(BlueprintCallable, Category = "Scoreboard|Snitch")
    void SetSnitchActive(bool bIsActive);

    UFUNCTION(BlueprintCallable, Category = "Scoreboard|Snitch")
    void SetSnitchCaught(EQuidditchTeam CatchingTeam);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // ========================================================================
    // BOUND WIDGETS
    // ========================================================================

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* TeamAScore;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* TeamBScore;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* TeamAName;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* TeamBName;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* MatchTimerText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* CurrentRoleText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UImage* RoleIcon;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UImage* SnitchStatusIcon;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* SnitchStatusText;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scoreboard|Config")
    UTexture2D* Icon_Seeker;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scoreboard|Config")
    UTexture2D* Icon_Chaser;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scoreboard|Config")
    UTexture2D* Icon_Beater;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scoreboard|Config")
    UTexture2D* Icon_Keeper;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scoreboard|Config")
    UTexture2D* Icon_SnitchActive;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Scoreboard|Config")
    UTexture2D* Icon_SnitchCaught;

private:
    // ========================================================================
    // DELEGATE HANDLERS
    // ========================================================================

    UFUNCTION()
    void HandleTeamScored(EQuidditchTeam ScoringTeam, int32 Points, int32 TotalScore);

    UFUNCTION()
    void HandleSnitchCaught(APawn* CatchingSeeker, EQuidditchTeam WinningTeam);

    UFUNCTION()
    void HandleRoleAssigned(APawn* Agent, EQuidditchTeam Team, EQuidditchRole Role);

    // ========================================================================
    // TIMER STATE
    // ========================================================================

    bool bTimerRunning;
    float MatchTimeRemaining;
    float MatchDuration;

    // GameMode reference
    UPROPERTY()
    AQuidditchGameMode* QuidditchGameMode;
};
```

---

### 2.3 UMG Blueprint Layout (WBP_QuidditchScoreboard)

```
Canvas Panel (Root)
├── ScoreContainer (Horizontal Box) - Top Center
│   ├── TeamAPanel (Vertical Box)
│   │   ├── TeamAName (Text Block) - "WIZARDS"
│   │   └── TeamAScore (Text Block) - "0"
│   │
│   ├── VsText (Text Block) - "VS"
│   │
│   └── TeamBPanel (Vertical Box)
│       ├── TeamBName (Text Block) - "SERPENTS"
│       └── TeamBScore (Text Block) - "0"
│
├── MatchTimerText (Text Block) - Below Scores
│   └── Text: "10:00"
│
├── RoleContainer (Horizontal Box) - Top Left
│   ├── RoleIcon (Image) - Size: 32x32
│   └── CurrentRoleText (Text Block) - "SEEKER"
│
└── SnitchContainer (Horizontal Box) - Top Right
    ├── SnitchStatusIcon (Image) - Size: 32x32
    └── SnitchStatusText (Text Block) - "SNITCH ACTIVE"
```

---

## PHASE 3: QUIDDITCH RESULTS WIDGET

### 3.1 Create QuidditchResultsWidget C++ Class

**File Location:** `Source/END2507/Public/Code/UI/QuidditchResultsWidget.h`

**Extends existing ResultsWidget pattern with Quidditch-specific elements:**

```cpp
// QuidditchResultsWidget.h
// Results screen for Quidditch matches showing final scores and winner
// Developer: Marcus Daley
// Date: January 26, 2026

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Code/GameModes/QuidditchGameMode.h"
#include "QuidditchResultsWidget.generated.h"

class UTextBlock;
class UImage;
class UWidgetSwitcher;
class UVerticalBox;
class UButtonWidgetComponent;

UCLASS()
class END2507_API UQuidditchResultsWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UQuidditchResultsWidget(const FObjectInitializer& ObjectInitializer);

    // Configure for match end
    UFUNCTION(BlueprintCallable, Category = "Results")
    void ShowMatchResults(EQuidditchTeam WinningTeam, int32 TeamAFinalScore,
        int32 TeamBFinalScore, APawn* SnitchCatcher);

protected:
    virtual void NativeConstruct() override;

    // ========================================================================
    // BOUND WIDGETS
    // ========================================================================

    // Result state switcher (Victory/Defeat screens)
    UPROPERTY(meta = (BindWidget))
    UWidgetSwitcher* ResultsSwitch;

    // Winner display
    UPROPERTY(meta = (BindWidget))
    UTextBlock* WinnerTeamText;

    UPROPERTY(meta = (BindWidget))
    UImage* WinnerTeamIcon;

    // Final scores
    UPROPERTY(meta = (BindWidget))
    UTextBlock* FinalScoreTeamA;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* FinalScoreTeamB;

    // Snitch catcher highlight
    UPROPERTY(meta = (BindWidget))
    UTextBlock* SnitchCatcherText;

    UPROPERTY(meta = (BindWidget))
    UImage* SnitchCatcherIcon;

    // Button area
    UPROPERTY(meta = (BindWidget))
    UVerticalBox* ButtonArea;

    UPROPERTY(meta = (BindWidget))
    UButtonWidgetComponent* PlayAgainButton;

    UPROPERTY(meta = (BindWidget))
    UButtonWidgetComponent* MainMenuButton;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Results|Config")
    UTexture2D* TeamAIcon;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Results|Config")
    UTexture2D* TeamBIcon;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Results|Config")
    float AutoReturnDelay;

private:
    UFUNCTION()
    void OnPlayAgainClicked();

    UFUNCTION()
    void OnMainMenuClicked();

    void AutoReturnToMenu();

    FTimerHandle AutoReturnTimerHandle;
};
```

---

### 3.2 UMG Blueprint Layout (WBP_QuidditchResults)

```
Canvas Panel (Root)
├── ResultsSwitch (Widget Switcher)
│   ├── [Index 0] DefeatPanel
│   │   └── DefeatText - "DEFEAT"
│   │
│   └── [Index 1] VictoryPanel
│       └── VictoryText - "VICTORY!"
│
├── WinnerContainer (Vertical Box) - Center
│   ├── WinnerTeamIcon (Image) - Size: 128x128
│   └── WinnerTeamText (Text Block) - "WIZARDS WIN!"
│
├── ScoreContainer (Horizontal Box) - Below Winner
│   ├── FinalScoreTeamA (Text Block) - "150"
│   ├── DashText (Text Block) - " - "
│   └── FinalScoreTeamB (Text Block) - "30"
│
├── SnitchCatcherContainer (Horizontal Box) - Below Scores
│   ├── SnitchCatcherIcon (Image) - Golden Snitch icon
│   └── SnitchCatcherText (Text Block) - "Caught by: Harry"
│
└── ButtonArea (Vertical Box) - Bottom Center
    ├── PlayAgainButton (ButtonWidgetComponent)
    └── MainMenuButton (ButtonWidgetComponent)
```

---

## PHASE 4: INTEGRATION STEPS

### 4.1 Add to Build.cs

Ensure `SlateCore` module is included for FSlateBrush (already should be there from Day 8):

**File:** `Source/END2507/END2507.Build.cs`
```cpp
PublicDependencyModuleNames.AddRange(new string[] {
    "Core",
    "CoreUObject",
    "Engine",
    "InputCore",
    "UMG",
    "Slate",
    "SlateCore",  // Required for FSlateBrush
    "EnhancedInput",
    "AIModule",
    "GameplayTasks",
    "NavigationSystem"
});
```

### 4.2 Create HUD in PlayerController or GameMode

**Option A: PlayerController (Recommended for Player HUD)**

In your PlayerController's BeginPlay:
```cpp
void AMyPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (IsLocalController() && WizardHUDClass)
    {
        WizardHUD = CreateWidget<UWizardPlayerHUD>(this, WizardHUDClass);
        if (WizardHUD)
        {
            WizardHUD->AddToViewport();
            WizardHUD->InitializeForPlayer(this);
        }
    }
}
```

**Option B: GameMode (For Scoreboard and Results)**

In QuidditchGameMode's BeginPlay:
```cpp
void AQuidditchGameMode::BeginPlay()
{
    Super::BeginPlay();

    // Create scoreboard for all local players
    if (ScoreboardWidgetClass)
    {
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            APlayerController* PC = It->Get();
            if (PC && PC->IsLocalController())
            {
                UQuidditchScoreboardWidget* Scoreboard = CreateWidget<UQuidditchScoreboardWidget>(PC, ScoreboardWidgetClass);
                if (Scoreboard)
                {
                    Scoreboard->AddToViewport();
                    Scoreboard->InitializeScoreboard();
                }
            }
        }
    }
}
```

---

## PHASE 5: TESTING CHECKLIST

### Pre-Demo Verification (Tuesday)

**WizardPlayerHUD:**
- [ ] Health bar updates when player takes damage
- [ ] Stamina bar drains during flight
- [ ] Spell slots light up when collected
- [ ] "Press E" prompt shows near broom
- [ ] Broom icon changes when flying
- [ ] Crosshair visible and centered

**QuidditchScoreboard:**
- [ ] Timer starts on WorldSignal QuidditchMatchStart
- [ ] Timer counts down correctly
- [ ] Scores update when goals are scored
- [ ] Role icon matches player's assigned role
- [ ] Snitch status shows "ACTIVE" at match start

**QuidditchResults:**
- [ ] Shows correct winning team
- [ ] Displays final scores
- [ ] Shows snitch catcher name
- [ ] Play Again button works
- [ ] Main Menu button works
- [ ] Auto-return after delay works

---

## QUESTIONS FOR MARCUS

Before implementation, please clarify:

1. **Spell Casting Verification:**
   - Is the current spell casting system working?
   - Which class handles projectile spawning? (WizardPlayer? BasePlayer?)
   - Do you want the wand to be a separate actor or part of the character mesh?

2. **World Meter / Timer Source:**
   - Should the timer start from a WorldSignalEmitter in the level?
   - What's the default match duration? (10 minutes standard Quidditch?)
   - Should the timer pause when snitch is caught or keep running?

3. **Existing Blueprint Widgets:**
   - WBP_QuidditchHUD.uasset already exists - should we extend it or replace?
   - WBP_WizardJamResults.uasset exists - extend or create new?

4. **Team Configuration:**
   - Are team names fixed ("Wizards" vs "Serpents") or configurable?
   - Team icons: Use T_Team_Serpents.uasset and T_Team_Wizards.uasset?

5. **Interaction System:**
   - Is the TooltipWidget already hooked up to InteractionComponent?
   - Or should the HUD handle interaction prompts directly?

6. **Wand Implementation:**
   - Socket attachment point on character skeleton?
   - Should AI wizards also have wands?
   - Wand affects spell projectile spawn location?

---

## FILE CREATION SUMMARY

**New C++ Files to Create:**
1. `Source/END2507/Public/Code/UI/WizardPlayerHUD.h`
2. `Source/END2507/Private/Code/UI/WizardPlayerHUD.cpp`
3. `Source/END2507/Public/Code/UI/QuidditchScoreboardWidget.h`
4. `Source/END2507/Private/Code/UI/QuidditchScoreboardWidget.cpp`
5. `Source/END2507/Public/Code/UI/QuidditchResultsWidget.h`
6. `Source/END2507/Private/Code/UI/QuidditchResultsWidget.cpp`

**New Blueprint Widgets to Create:**
1. `Content/UI/WBP_WizardPlayerHUD.uasset` (Parent: WizardPlayerHUD)
2. `Content/UI/WBP_QuidditchScoreboard.uasset` (Parent: QuidditchScoreboardWidget)
3. `Content/UI/WBP_QuidditchResults.uasset` (Parent: QuidditchResultsWidget)

**Existing Files to Potentially Modify:**
1. `QuidditchGameMode.cpp` - Add widget creation
2. Player Controller - Add WizardPlayerHUD creation
3. `END2507.Build.cs` - Verify SlateCore module

---

## NEXT STEPS

1. **Answer the clarifying questions above**
2. **Create the C++ header/source files**
3. **Compile and verify no errors**
4. **Create UMG Blueprint widgets**
5. **Hook up delegate bindings**
6. **Test each widget individually**
7. **Integration test full match flow**

---

*Guide generated for WizardJam2.0 - January 26, 2026*
*Based on existing codebase analysis and Nick Penney AAA Coding Standards*
