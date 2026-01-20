// Fill out your copyright notice in the Description page of Project Settings.


#include "Code/ButtonWidgetComponent.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
//#include "ButtonWidgetComponent.h"
#include "../END2507.h"

DEFINE_LOG_CATEGORY_STATIC(LogButtonWidget, Log, All);
void UButtonWidgetComponent::SetButtonText(const FText& NewText)
{// Validate text block exists before attempting to update
	if (!InformationText)
	{
		UE_LOG(LogButtonWidget, Error, TEXT("SetButtonText failed — InformationText is null!"));
		return;
	}

	// Update text display
	InformationText->SetText(NewText);

	UE_LOG(LogButtonWidget, Log, TEXT(" Button text updated: %s"), *NewText.ToString());
}

void UButtonWidgetComponent::NativeConstruct()
{
	Super::NativeConstruct();

	// Validate button component exists (auto-bound via BindWidget meta)
	if (!BackgroundButton)
	{
		UE_LOG(LogButtonWidget, Error, TEXT("BackgroundButton is null — Designer widget name must be 'BackgroundButton'!"));
		return;
	}

	// Validate text component exists
	if (!InformationText)
	{
		UE_LOG(LogButtonWidget, Error, TEXT("InformationText is null — Designer widget name must be 'InformationText'!"));
		return;
	}

	// Bind button's OnClicked event to internal handler
	BackgroundButton->OnClicked.AddDynamic(this, &UButtonWidgetComponent::HandleClicked);

	UE_LOG(LogButtonWidget, Log, TEXT(" Button widget constructed — Click delegate bound"));

}

void UButtonWidgetComponent::HandleClicked()
{
	// Button was clicked - broadcast to all listeners
	UE_LOG(LogButtonWidget, Log, TEXT(" Button clicked — Broadcasting OnClicked delegate"));

	// Broadcast delegate (calls all bound functions in parent widgets)
	OnClickedEvent.Broadcast();
}
