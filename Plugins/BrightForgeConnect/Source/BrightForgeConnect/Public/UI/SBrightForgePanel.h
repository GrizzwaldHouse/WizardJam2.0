// ============================================================================
// SBrightForgePanel.h
// Developer: Marcus Daley
// Date: February 2026
// Project: BrightForgeConnect Plugin
// ============================================================================
// Purpose:
// Dockable Slate panel for the BrightForge Connect plugin.
// Provides UI for server status, asset generation, progress, and import.
//
// Architecture:
// SCompoundWidget registered as a NomadTab via FGlobalTabmanager.
// Binds to UBrightForgeClientSubsystem delegates for reactive updates.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Core/BrightForgeTypes.h"

class UBrightForgeClientSubsystem;
class UBrightForgeImporter;

class BRIGHTFORGECONNECT_API SBrightForgePanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SBrightForgePanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SBrightForgePanel();

private:
	// ========================================================================
	// SUBSYSTEM DELEGATE HANDLERS
	// ========================================================================

	void OnConnectionStateChanged(EBrightForgeConnectionState NewState);
	void OnGenerationComplete(const FBrightForgeGenerationStatus& Status);
	void OnGenerationProgress(const FBrightForgeGenerationStatus& Status);
	void OnGenerationFailed(const FString& SessionId, const FString& ErrorMessage);

	// ========================================================================
	// BUTTON / UI CALLBACKS
	// ========================================================================

	FReply OnGenerateClicked();
	FReply OnImportClicked(FString SessionId);
	FReply OnRefreshConnectionClicked();

	// ========================================================================
	// ATTRIBUTE GETTERS (Slate bindings)
	// ========================================================================

	FText GetConnectionStatusText() const;
	FSlateColor GetConnectionStatusColor() const;
	FText GetStatusMessageText() const;
	TOptional<float> GetProgressValue() const;
	EVisibility GetProgressBarVisibility() const;
	EVisibility GetGenerateButtonVisibility() const;
	bool IsGenerateButtonEnabled() const;

	// Generation type combo
	TSharedRef<SWidget> MakeGenerationTypeComboEntry(TSharedPtr<EBrightForgeGenerationType> InType);
	void OnGenerationTypeSelected(TSharedPtr<EBrightForgeGenerationType> NewType, ESelectInfo::Type SelectInfo);
	FText GetSelectedGenerationTypeText() const;

	// Recent generation list
	TSharedRef<ITableRow> OnGenerateListRow(
		TSharedPtr<FBrightForgeGenerationStatus> Item,
		const TSharedRef<STableViewBase>& OwnerTable);

	// ========================================================================
	// MEMBER DATA
	// ========================================================================

	/** Pointer to the editor subsystem — not owned, lifetime managed by GEditor */
	UBrightForgeClientSubsystem* Subsystem;

	/** Importer object for FBX download + Content Browser import */
	UBrightForgeImporter* Importer;

	// Prompt text
	TSharedPtr<class SEditableTextBox> PromptTextBox;

	// Type combo
	TArray<TSharedPtr<EBrightForgeGenerationType>> GenerationTypeOptions;
	TSharedPtr<EBrightForgeGenerationType> SelectedGenerationType;
	TSharedPtr<class SComboBox<TSharedPtr<EBrightForgeGenerationType>>> TypeComboBox;

	// Recent generations list
	TArray<TSharedPtr<FBrightForgeGenerationStatus>> RecentGenerations;
	TSharedPtr<SListView<TSharedPtr<FBrightForgeGenerationStatus>>> GenerationListView;

	// Current panel state
	EBrightForgeConnectionState CurrentConnectionState;
	float CurrentProgress;
	FString StatusMessage;
	bool bIsGenerating;

	// Delegate handles (for cleanup on destruction)
	FDelegateHandle ConnectionStateHandle;
	FDelegateHandle GenerationCompleteHandle;
	FDelegateHandle GenerationProgressHandle;
	FDelegateHandle GenerationFailedHandle;
};
