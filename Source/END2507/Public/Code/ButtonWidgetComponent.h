// Author: Marcus Daley
// Date: 10/13/2025
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ButtonWidgetComponent.generated.h"
class UButton;
class UTextBlock;
// Delegate signature for button click events
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnButtonClicked);

// Reusable button component used across multiple UI screens.
// Designer contains: Background Button + Information Text Block.
// C++ handles: Click events, text updates, delegate broadcasting.
UCLASS()
class END2507_API UButtonWidgetComponent : public UUserWidget
{
	GENERATED_BODY()
	
public:
	// Delegate that fires when button is clicked
	// Other widgets bind to this to handle button press
	UPROPERTY(BlueprintCallable, Category = "Button Events")
	FOnButtonClicked OnClickedEvent;

	UFUNCTION(BlueprintCallable, Category = "Button")
	void SetButtonText(const FText& NewText);

protected:
	// Called after widget is constructed and before added to viewport
	virtual void NativeConstruct() override;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UButton* BackgroundButton;

	// Text display component - BlueprintReadOnly for BindWidget compatibility
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UTextBlock* InformationText;
private:

	UFUNCTION()
	void HandleClicked();
	
};
