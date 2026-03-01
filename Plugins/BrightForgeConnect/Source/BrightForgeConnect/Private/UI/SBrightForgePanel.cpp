// ============================================================================
// SBrightForgePanel.cpp
// Developer: Marcus Daley
// Date: February 2026
// Project: BrightForgeConnect Plugin
// ============================================================================
// Purpose:
// Implementation of the dockable BrightForge Connect Slate panel.
// ============================================================================

#include "UI/SBrightForgePanel.h"
#include "Core/BrightForgeClientSubsystem.h"
#include "Core/BrightForgeSettings.h"
#include "Import/BrightForgeImporter.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Views/SListView.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "BrightForgeConnect"

// ============================================================================
// Helpers
// ============================================================================

static FText GenerationTypeToText(EBrightForgeGenerationType Type)
{
	switch (Type)
	{
		case EBrightForgeGenerationType::Full:  return LOCTEXT("TypeFull",  "Full (Text to 3D)");
		case EBrightForgeGenerationType::Mesh:  return LOCTEXT("TypeMesh",  "Mesh Only");
		case EBrightForgeGenerationType::Image: return LOCTEXT("TypeImage", "Image to 3D");
		default:                                return LOCTEXT("TypeUnknown", "Unknown");
	}
}

// ============================================================================
// Construct / Destruct
// ============================================================================

void SBrightForgePanel::Construct(const FArguments& InArgs)
{
	CurrentConnectionState = EBrightForgeConnectionState::Disconnected;
	CurrentProgress        = 0.0f;
	bIsGenerating          = false;
	StatusMessage          = TEXT("Ready");

	// Build generation type combo options
	GenerationTypeOptions.Add(MakeShareable(new EBrightForgeGenerationType(EBrightForgeGenerationType::Full)));
	GenerationTypeOptions.Add(MakeShareable(new EBrightForgeGenerationType(EBrightForgeGenerationType::Mesh)));
	GenerationTypeOptions.Add(MakeShareable(new EBrightForgeGenerationType(EBrightForgeGenerationType::Image)));
	SelectedGenerationType = GenerationTypeOptions[0];

	// Get the subsystem
	Subsystem = GEditor ? GEditor->GetEditorSubsystem<UBrightForgeClientSubsystem>() : nullptr;

	// Create the importer
	Importer = NewObject<UBrightForgeImporter>(GetTransientPackage());
	Importer->AddToRoot(); // Prevent GC

	// Bind to subsystem delegates
	if (Subsystem)
	{
		ConnectionStateHandle = Subsystem->OnConnectionStateChanged.AddSP(
			this, &SBrightForgePanel::OnConnectionStateChanged);

		GenerationCompleteHandle = Subsystem->OnGenerationComplete.AddSP(
			this, &SBrightForgePanel::OnGenerationComplete);

		GenerationProgressHandle = Subsystem->OnGenerationProgress.AddSP(
			this, &SBrightForgePanel::OnGenerationProgress);

		GenerationFailedHandle = Subsystem->OnGenerationFailed.AddSP(
			this, &SBrightForgePanel::OnGenerationFailed);
	}

	// ========================================================================
	// UI Layout
	// ========================================================================

	ChildSlot
	[
		SNew(SVerticalBox)

		// ----------------------------------------------------------------
		// HEADER: Title + Connection Status
		// ----------------------------------------------------------------
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f, 8.0f, 8.0f, 4.0f)
		[
			SNew(SHorizontalBox)

			// Title
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("PanelTitle", "BrightForge Connect"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
			]

			// Refresh button
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(4.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("RefreshBtn", "Ping"))
				.ToolTipText(LOCTEXT("RefreshTooltip", "Check BrightForge server connection"))
				.OnClicked(this, &SBrightForgePanel::OnRefreshConnectionClicked)
			]
		]

		// Connection status row
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f, 2.0f, 8.0f, 4.0f)
		[
			SNew(SHorizontalBox)

			// Coloured status dot (SImage with tint)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("StatusDot", "\u25CF"))  // Unicode filled circle
				.ColorAndOpacity(this, &SBrightForgePanel::GetConnectionStatusColor)
			]

			// Status text
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SBrightForgePanel::GetConnectionStatusText)
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f, 0.0f)
		[
			SNew(SSeparator)
		]

		// ----------------------------------------------------------------
		// PROMPT INPUT
		// ----------------------------------------------------------------
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f, 8.0f, 8.0f, 4.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("PromptLabel", "Asset Description"))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f, 0.0f, 8.0f, 8.0f)
		[
			SAssignNew(PromptTextBox, SEditableTextBox)
			.HintText(LOCTEXT("PromptHint", "e.g. medieval stone fortress wall, weathered"))
			.IsEnabled(this, &SBrightForgePanel::IsGenerateButtonEnabled)
		]

		// ----------------------------------------------------------------
		// GENERATION TYPE DROPDOWN
		// ----------------------------------------------------------------
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f, 0.0f, 8.0f, 4.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("TypeLabel", "Generation Type"))
			.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f, 0.0f, 8.0f, 8.0f)
		[
			SAssignNew(TypeComboBox, SComboBox<TSharedPtr<EBrightForgeGenerationType>>)
			.OptionsSource(&GenerationTypeOptions)
			.OnGenerateWidget(this, &SBrightForgePanel::MakeGenerationTypeComboEntry)
			.OnSelectionChanged(this, &SBrightForgePanel::OnGenerationTypeSelected)
			.InitiallySelectedItem(SelectedGenerationType)
			.IsEnabled(this, &SBrightForgePanel::IsGenerateButtonEnabled)
			[
				SNew(STextBlock)
				.Text(this, &SBrightForgePanel::GetSelectedGenerationTypeText)
			]
		]

		// ----------------------------------------------------------------
		// GENERATE BUTTON
		// ----------------------------------------------------------------
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f, 0.0f, 8.0f, 8.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("GenerateBtn", "Generate 3D Asset"))
			.ToolTipText(LOCTEXT("GenerateBtnTooltip", "Send request to BrightForge for AI 3D asset generation"))
			.HAlign(HAlign_Center)
			.OnClicked(this, &SBrightForgePanel::OnGenerateClicked)
			.IsEnabled(this, &SBrightForgePanel::IsGenerateButtonEnabled)
			.Visibility(this, &SBrightForgePanel::GetGenerateButtonVisibility)
		]

		// ----------------------------------------------------------------
		// PROGRESS BAR (visible during generation)
		// ----------------------------------------------------------------
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f, 0.0f, 8.0f, 4.0f)
		[
			SNew(SProgressBar)
			.Percent(this, &SBrightForgePanel::GetProgressValue)
			.Visibility(this, &SBrightForgePanel::GetProgressBarVisibility)
		]

		// Status text
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f, 2.0f, 8.0f, 8.0f)
		[
			SNew(STextBlock)
			.Text(this, &SBrightForgePanel::GetStatusMessageText)
			.ColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f)))
			.Visibility(this, &SBrightForgePanel::GetProgressBarVisibility)
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f, 0.0f)
		[
			SNew(SSeparator)
		]

		// ----------------------------------------------------------------
		// RECENT GENERATIONS LIST
		// ----------------------------------------------------------------
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f, 8.0f, 8.0f, 4.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("RecentLabel", "Recent Generations"))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(8.0f, 0.0f, 8.0f, 8.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SAssignNew(GenerationListView, SListView<TSharedPtr<FBrightForgeGenerationStatus>>)
				.ListItemsSource(&RecentGenerations)
				.OnGenerateRow(this, &SBrightForgePanel::OnGenerateListRow)
				.SelectionMode(ESelectionMode::None)
			]
		]

		// ----------------------------------------------------------------
		// FOOTER: Server info
		// ----------------------------------------------------------------
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f, 4.0f)
		[
			SNew(SSeparator)
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(8.0f, 2.0f, 8.0f, 6.0f)
		[
			SNew(STextBlock)
			.Text_Lambda([this]()
			{
				const UBrightForgeSettings* Settings = UBrightForgeSettings::Get();
				const FString Url = Settings ? Settings->ServerUrl : TEXT("http://localhost:3847");
				const bool bFbx = Subsystem ? Subsystem->IsFbxConverterAvailable() : false;
				return FText::Format(
					LOCTEXT("FooterFmt", "Server: {0}  |  FBX Converter: {1}"),
					FText::FromString(Url),
					bFbx ? LOCTEXT("FbxYes", "Available") : LOCTEXT("FbxNo", "Unavailable")
				);
			})
			.Font(FCoreStyle::GetDefaultFontStyle("Italic", 8))
			.ColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)))
		]
	];
}

SBrightForgePanel::~SBrightForgePanel()
{
	// Unbind all delegates to prevent use-after-free
	if (Subsystem)
	{
		Subsystem->OnConnectionStateChanged.Remove(ConnectionStateHandle);
		Subsystem->OnGenerationComplete.Remove(GenerationCompleteHandle);
		Subsystem->OnGenerationProgress.Remove(GenerationProgressHandle);
		Subsystem->OnGenerationFailed.Remove(GenerationFailedHandle);
	}

	// Release the importer from root to allow GC
	if (Importer && Importer->IsRooted())
	{
		Importer->RemoveFromRoot();
	}
}

// ============================================================================
// SUBSYSTEM DELEGATE HANDLERS
// ============================================================================

void SBrightForgePanel::OnConnectionStateChanged(EBrightForgeConnectionState NewState)
{
	CurrentConnectionState = NewState;
}

void SBrightForgePanel::OnGenerationComplete(const FBrightForgeGenerationStatus& Status)
{
	bIsGenerating   = false;
	CurrentProgress = 1.0f;
	StatusMessage   = FString::Printf(TEXT("Complete! Session: %s"), *Status.SessionId);

	// Add to recent list
	TSharedPtr<FBrightForgeGenerationStatus> Entry = MakeShareable(new FBrightForgeGenerationStatus(Status));
	RecentGenerations.Insert(Entry, 0);
	if (GenerationListView.IsValid())
	{
		GenerationListView->RequestListRefresh();
	}
}

void SBrightForgePanel::OnGenerationProgress(const FBrightForgeGenerationStatus& Status)
{
	bIsGenerating   = true;
	CurrentProgress = Status.Progress;
	StatusMessage   = FString::Printf(TEXT("Generating... %.0f%%"), Status.Progress * 100.0f);
}

void SBrightForgePanel::OnGenerationFailed(const FString& SessionId, const FString& ErrorMessage)
{
	bIsGenerating   = false;
	CurrentProgress = 0.0f;
	StatusMessage   = FString::Printf(TEXT("Failed: %s"), *ErrorMessage);
}

// ============================================================================
// BUTTON / UI CALLBACKS
// ============================================================================

FReply SBrightForgePanel::OnGenerateClicked()
{
	if (!Subsystem)
	{
		return FReply::Handled();
	}

	const FString Prompt = PromptTextBox.IsValid() ? PromptTextBox->GetText().ToString() : FString();
	if (Prompt.IsEmpty())
	{
		StatusMessage = TEXT("Please enter a prompt before generating");
		return FReply::Handled();
	}

	const EBrightForgeGenerationType Type = SelectedGenerationType.IsValid()
		? *SelectedGenerationType
		: EBrightForgeGenerationType::Full;

	StatusMessage   = TEXT("Sending generation request...");
	bIsGenerating   = true;
	CurrentProgress = 0.0f;

	Subsystem->GenerateAsset(Prompt, Type);
	return FReply::Handled();
}

FReply SBrightForgePanel::OnImportClicked(FString SessionId)
{
	if (!Importer || SessionId.IsEmpty())
	{
		return FReply::Handled();
	}

	// Derive a sanitised asset name from the session ID (last 8 chars)
	const FString AssetName = SessionId.Right(8).Replace(TEXT("-"), TEXT("_"));
	Importer->DownloadAndImport(SessionId, AssetName);
	return FReply::Handled();
}

FReply SBrightForgePanel::OnRefreshConnectionClicked()
{
	if (Subsystem)
	{
		StatusMessage = TEXT("Checking server connection...");
		Subsystem->CheckServerHealth();
		Subsystem->GetFbxStatus();
	}
	return FReply::Handled();
}

// ============================================================================
// ATTRIBUTE GETTERS
// ============================================================================

FText SBrightForgePanel::GetConnectionStatusText() const
{
	switch (CurrentConnectionState)
	{
		case EBrightForgeConnectionState::Connected:    return LOCTEXT("StateConnected",    "Connected");
		case EBrightForgeConnectionState::Connecting:   return LOCTEXT("StateConnecting",   "Connecting...");
		case EBrightForgeConnectionState::Error:        return LOCTEXT("StateError",        "Error");
		case EBrightForgeConnectionState::Disconnected: return LOCTEXT("StateDisconnected", "Disconnected");
		default:                                        return LOCTEXT("StateUnknown",      "Unknown");
	}
}

FSlateColor SBrightForgePanel::GetConnectionStatusColor() const
{
	switch (CurrentConnectionState)
	{
		case EBrightForgeConnectionState::Connected:    return FLinearColor(0.0f, 0.8f, 0.0f); // Green
		case EBrightForgeConnectionState::Connecting:   return FLinearColor(1.0f, 0.8f, 0.0f); // Yellow
		case EBrightForgeConnectionState::Error:        return FLinearColor(0.9f, 0.1f, 0.1f); // Red
		case EBrightForgeConnectionState::Disconnected: return FLinearColor(0.5f, 0.5f, 0.5f); // Grey
		default:                                        return FLinearColor::White;
	}
}

FText SBrightForgePanel::GetStatusMessageText() const
{
	return FText::FromString(StatusMessage);
}

TOptional<float> SBrightForgePanel::GetProgressValue() const
{
	if (bIsGenerating)
	{
		return TOptional<float>(CurrentProgress);
	}
	return TOptional<float>(); // Indeterminate
}

EVisibility SBrightForgePanel::GetProgressBarVisibility() const
{
	return bIsGenerating ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SBrightForgePanel::GetGenerateButtonVisibility() const
{
	return bIsGenerating ? EVisibility::Collapsed : EVisibility::Visible;
}

bool SBrightForgePanel::IsGenerateButtonEnabled() const
{
	return !bIsGenerating &&
		CurrentConnectionState == EBrightForgeConnectionState::Connected;
}

// ============================================================================
// COMBO BOX
// ============================================================================

TSharedRef<SWidget> SBrightForgePanel::MakeGenerationTypeComboEntry(TSharedPtr<EBrightForgeGenerationType> InType)
{
	return SNew(STextBlock)
		.Text(InType.IsValid() ? GenerationTypeToText(*InType) : FText::GetEmpty())
		.Margin(FMargin(4.0f, 2.0f));
}

void SBrightForgePanel::OnGenerationTypeSelected(TSharedPtr<EBrightForgeGenerationType> NewType, ESelectInfo::Type /*SelectInfo*/)
{
	SelectedGenerationType = NewType;
}

FText SBrightForgePanel::GetSelectedGenerationTypeText() const
{
	return SelectedGenerationType.IsValid()
		? GenerationTypeToText(*SelectedGenerationType)
		: LOCTEXT("TypeNone", "Select type...");
}

// ============================================================================
// LIST VIEW ROW
// ============================================================================

TSharedRef<ITableRow> SBrightForgePanel::OnGenerateListRow(
	TSharedPtr<FBrightForgeGenerationStatus> Item,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FBrightForgeGenerationStatus>>, OwnerTable)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SHorizontalBox)

			// Session info
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(Item->Prompt.IsEmpty() ? Item->SessionId : Item->Prompt))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("State: %s  |  ID: %s"), *Item->State, *Item->SessionId.Left(8))))
					.Font(FCoreStyle::GetDefaultFontStyle("Italic", 8))
					.ColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)))
				]
			]

			// Import button (only for completed assets)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(4.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("ImportBtn", "Import"))
				.ToolTipText(LOCTEXT("ImportBtnTooltip", "Download FBX and import into Content Browser"))
				.IsEnabled(Item->State == TEXT("complete") || Item->State == TEXT("success"))
				.OnClicked_Lambda([this, Item]()
				{
					return OnImportClicked(Item->SessionId);
				})
			]
		];
}

#undef LOCTEXT_NAMESPACE
