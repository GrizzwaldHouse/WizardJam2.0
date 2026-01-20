// TooltipWidget.h
// Custom widget class for displaying interactive tooltips
// Developer: Marcus Daley
// Date: December 31, 2025

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TooltipWidget.generated.h"

class UTextBlock;

// Base C++ class for tooltip widget
// Designer creates Blueprint child (WBP_TooltipWidget) to design UI layout
UCLASS()
class END2507_API UTooltipWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // Sets the main display text 
    UFUNCTION(BlueprintCallable, Category = "Tooltip")
    void SetDisplayText(const FText& NewText);

    // Sets the interaction prompt text (e.g., "Press E to Mount")
    UFUNCTION(BlueprintCallable, Category = "Tooltip")
    void SetInteractionPrompt(const FText& NewText);

protected:
    virtual void NativeConstruct() override;

    // Main display text widget 
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* DisplayText;

    // Interaction prompt text widget 
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* InteractionPromptText;

private:
    // Validates that required widgets are bound
    bool ValidateWidgets() const;
};