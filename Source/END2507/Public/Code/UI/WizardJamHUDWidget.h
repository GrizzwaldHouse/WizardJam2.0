// ============================================================================
// WizardJamHUDWidget.h
// Main player HUD widget with health, stamina, spell slots, and Quidditch integration
//
// Developer: Marcus Daley
// Date: January 26, 2026
// Project: WizardJam2.0 (END2507)
//
// PURPOSE:
// Complete player HUD displaying all vital information for wizard gameplay.
// Uses observer pattern - binds to component delegates, never polls.
//
// DESIGNER WORKFLOW:
// 1. Create WBP_WizardJamHUD Blueprint inheriting from this class
// 2. Add widgets with EXACT names matching BindWidgetOptional properties
// 3. Configure SpellSlotConfigs array for each spell type
// 4. Assign QuidditchWidgetClass for embedded scoreboard
//
// WIDGET NAMING CONVENTION:
// - HealthProgressBar, StaminaProgressBar (Progress Bars)
// - HealthText, StaminaText, SpellCountText (Text Blocks)
// - SpellSlotContainer (Panel containing spell slots)
// - SpellSlot_0, SpellSlot_1, etc. (Image widgets for spell icons)
// - BroomIcon, BoostIndicatorImage (Flight status images)
// - OutOfStaminaWarningText (Warning text)
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Code/Spells/SpellSlotConfig.h"
#include "WizardJamHUDWidget.generated.h"

// Forward declarations - avoid heavy includes in header
class UAC_HealthComponent;
class UAC_StaminaComponent;
class UAC_SpellCollectionComponent;
class UAC_BroomComponent;
class UProgressBar;
class UImage;
class UTextBlock;
class UPanelWidget;
class UWizardJamQuidditchWidget;

DECLARE_LOG_CATEGORY_EXTERN(LogWizardJamHUD, Log, All);

UCLASS()
class END2507_API UWizardJamHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // ========================================================================
    // DESIGNER CONFIGURATION - Spell Slots
    // Each entry defines one spell's visual properties
    // Designer adds elements in Blueprint Details panel
    // ========================================================================

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spell Slots")
    TArray<FSpellSlotConfig> SpellSlotConfigs;

    // ========================================================================
    // DESIGNER CONFIGURATION - Quidditch Integration
    // Optional embedded Quidditch scoreboard widget
    // ========================================================================

    // Widget class to spawn for Quidditch UI
    // Set to WBP_WizardJamQuidditchWidget in Blueprint
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quidditch")
    TSubclassOf<UUserWidget> QuidditchWidgetClass;

    // Should Quidditch UI be visible at start?
    // Set true for Quidditch matches, false for regular gameplay
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quidditch")
    bool bShowQuidditchOnStart;

    // ========================================================================
    // PUBLIC API - Quidditch Controls
    // Called by GameMode or level Blueprint to toggle scoreboard
    // ========================================================================

    UFUNCTION(BlueprintCallable, Category = "Quidditch")
    void ShowQuidditchUI();

    UFUNCTION(BlueprintCallable, Category = "Quidditch")
    void HideQuidditchUI();

    UFUNCTION(BlueprintPure, Category = "Quidditch")
    bool IsQuidditchUIVisible() const;

    // ========================================================================
    // PUBLIC API - Manual Refresh
    // Normally not needed - delegates handle updates automatically
    // ========================================================================

    UFUNCTION(BlueprintCallable, Category = "Spell Slots")
    void RefreshAllSpellSlots();

    // Get the embedded Quidditch widget for direct access
    UFUNCTION(BlueprintPure, Category = "Quidditch")
    UWizardJamQuidditchWidget* GetQuidditchWidget() const { return QuidditchWidget; }

protected:
    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // ========================================================================
    // COMPONENT REFERENCES
    // Cached at NativeConstruct via FindComponentByClass
    // ========================================================================

    UPROPERTY()
    AActor* OwnerActor;

    UPROPERTY()
    UAC_HealthComponent* HealthComp;

    UPROPERTY()
    UAC_StaminaComponent* StaminaComp;

    UPROPERTY()
    UAC_SpellCollectionComponent* SpellCollectionComp;

    UPROPERTY()
    UAC_BroomComponent* BroomComp;

    // ========================================================================
    // WIDGET REFERENCES - Core HUD Elements
    // Names MUST match exactly in UMG Blueprint
    // Using BindWidgetOptional allows partial HUD setups
    // ========================================================================

    // Health display
    UPROPERTY(meta = (BindWidgetOptional))
    UProgressBar* HealthProgressBar;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* HealthText;

    // Stamina display
    UPROPERTY(meta = (BindWidgetOptional))
    UProgressBar* StaminaProgressBar;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* StaminaText;

    // Spell slots container - holds SpellSlot_X images
    UPROPERTY(meta = (BindWidgetOptional))
    UPanelWidget* SpellSlotContainer;

    // Optional spell count text (e.g., "3/4 Spells")
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* SpellCountText;

    // Flight status indicators
    UPROPERTY(meta = (BindWidgetOptional))
    UImage* BroomIcon;

    UPROPERTY(meta = (BindWidgetOptional))
    UImage* BoostIndicatorImage;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* OutOfStaminaWarningText;

    // Interaction prompt (Press E to...)
    UPROPERTY(meta = (BindWidgetOptional))
    UPanelWidget* InteractionPromptPanel;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* InteractionPromptText;

    // ========================================================================
    // RUNTIME STATE - Spell Slot Tracking
    // Built at NativeConstruct from SpellSlotConfigs array
    // ========================================================================

    // Maps spell type name to its UImage widget for quick lookup
    UPROPERTY()
    TMap<FName, UImage*> SpellSlotWidgets;

    // Maps spell type name to its config for icon/color lookup
    TMap<FName, FSpellSlotConfig> SpellConfigLookup;

    // ========================================================================
    // RUNTIME STATE - Quidditch Widget
    // ========================================================================

    UPROPERTY()
    UWizardJamQuidditchWidget* QuidditchWidget;

    // ========================================================================
    // INITIALIZATION FUNCTIONS
    // ========================================================================

    // Find and cache component references from owner actor
    void CacheComponents();

    // Bind to all component delegates
    void BindComponentDelegates();
    void UnbindComponentDelegates();

    // Individual component binding (for clarity)
    void BindHealthComponentDelegates();
    void BindStaminaComponentDelegates();
    void BindSpellCollectionDelegates();
    void BindBroomComponentDelegates();

    // Build spell slot widget mapping from config array
    void InitializeSpellSlotSystem();

    // Find SpellSlot_X widget by index
    UImage* FindSpellSlotWidget(int32 SlotIndex);

    // Create Quidditch widget if class is set
    void InitializeQuidditchWidget();

    // ========================================================================
    // VISUAL UPDATE FUNCTIONS
    // ========================================================================

    // Update a single spell slot's visual (icon + color)
    void UpdateSpellSlotVisual(FName SpellTypeName, bool bIsUnlocked);

    // Update the "X/Y Spells" counter text
    void UpdateSpellCountText();

    // Update health bar percentage and color
    void UpdateHealthVisual(float HealthRatio);

    // Update stamina bar percentage and color
    void UpdateStaminaVisual(float StaminaPercent);

    // ========================================================================
    // DELEGATE HANDLERS
    // These are bound to component events at NativeConstruct
    // ========================================================================

    // Health component handler
    // Signature: FOnHealthChanged(float HealthRatio)
    UFUNCTION()
    void HandleHealthChanged(float HealthRatio);

    // Stamina component handler
    // Signature: FOnStaminaChanged(AActor* Owner, float NewStamina, float Delta)
    UFUNCTION()
    void HandleStaminaChanged(AActor* Owner, float NewStamina, float Delta);

    // Spell collection handler - new spell collected
    // Signature: FOnSpellAdded(FName SpellType, int32 TotalSpellCount)
    UFUNCTION()
    void HandleSpellAdded(FName SpellType, int32 TotalSpellCount);

    // Spell collection handler - channel unlocked
    // Signature: FOnChannelAdded(FName Channel)
    UFUNCTION()
    void HandleChannelAdded(FName Channel);

    // Broom component handlers
    // Signature: FOnFlightStateChanged(bool bIsFlying)
    UFUNCTION()
    void HandleFlightStateChanged(bool bIsFlying);

    // Signature: FOnBroomStaminaVisualUpdate(FLinearColor NewColor)
    UFUNCTION()
    void HandleStaminaColorChange(FLinearColor NewColor);

    // Signature: FOnForcedDismount()
    UFUNCTION()
    void HandleForcedDismount();

    // Signature: FOnBoostStateChanged(bool bIsBoosting)
    UFUNCTION()
    void HandleBoostChange(bool bIsBoosting);
};
