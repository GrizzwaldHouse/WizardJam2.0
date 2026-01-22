// ============================================================================
// DeveloperProductivityTrackerModule.cpp
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Module implementation handling startup, shutdown, and editor integration.
// ============================================================================

#include "DeveloperProductivityTrackerModule.h"
#include "Core/SessionTrackingSubsystem.h"
#include "Core/ProductivityTrackerSettings.h"
#include "LevelEditor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "ISettingsModule.h"

#define LOCTEXT_NAMESPACE "FDeveloperProductivityTrackerModule"

DEFINE_LOG_CATEGORY(LogProductivityTracker);

// Module implementation
IMPLEMENT_MODULE(FDeveloperProductivityTrackerModule, DeveloperProductivityTracker)

void FDeveloperProductivityTrackerModule::StartupModule()
{
	UE_LOG(LogProductivityTracker, Log, TEXT("Developer Productivity Tracker module starting up..."));

	// Register commands
	RegisterCommands();

	// Register toolbar extension
	RegisterToolbarExtension();

	// Register menus
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FDeveloperProductivityTrackerModule::RegisterMenus));

	UE_LOG(LogProductivityTracker, Log, TEXT("Developer Productivity Tracker module started successfully"));
}

void FDeveloperProductivityTrackerModule::ShutdownModule()
{
	UE_LOG(LogProductivityTracker, Log, TEXT("Developer Productivity Tracker module shutting down..."));

	// Unregister toolbar extension
	UnregisterToolbarExtension();

	// Unregister menus
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	UE_LOG(LogProductivityTracker, Log, TEXT("Developer Productivity Tracker module shutdown complete"));
}

FDeveloperProductivityTrackerModule& FDeveloperProductivityTrackerModule::Get()
{
	return FModuleManager::LoadModuleChecked<FDeveloperProductivityTrackerModule>("DeveloperProductivityTracker");
}

bool FDeveloperProductivityTrackerModule::IsAvailable()
{
	return FModuleManager::Get().IsModuleLoaded("DeveloperProductivityTracker");
}

void FDeveloperProductivityTrackerModule::RegisterToolbarExtension()
{
	// Create toolbar extender
	ToolBarExtender = MakeShareable(new FExtender);

	ToolBarExtension = ToolBarExtender->AddToolBarExtension(
		"Play",
		EExtensionHook::After,
		PluginCommands,
		FToolBarExtensionDelegate::CreateRaw(this, &FDeveloperProductivityTrackerModule::AddToolbarButton)
	);

	// Add to level editor toolbar
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolBarExtender);
}

void FDeveloperProductivityTrackerModule::UnregisterToolbarExtension()
{
	if (ToolBarExtender.IsValid() && FModuleManager::Get().IsModuleLoaded("LevelEditor"))
	{
		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		LevelEditorModule.GetToolBarExtensibilityManager()->RemoveExtender(ToolBarExtender);
	}

	ToolBarExtender.Reset();
}

void FDeveloperProductivityTrackerModule::RegisterMenus()
{
	// Owner for menu registration
	FToolMenuOwnerScoped OwnerScoped(this);

	// Add to Window menu
	UToolMenu* WindowMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
	if (WindowMenu)
	{
		FToolMenuSection& Section = WindowMenu->FindOrAddSection("WindowLocalTabSpawners");
		Section.AddMenuEntry(
			"ProductivityTracker",
			LOCTEXT("ProductivityTrackerMenu", "Productivity Tracker"),
			LOCTEXT("ProductivityTrackerMenuTooltip", "Open the Productivity Tracker dashboard"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FDeveloperProductivityTrackerModule::OnOpenDashboard))
		);
	}
}

void FDeveloperProductivityTrackerModule::RegisterCommands()
{
	PluginCommands = MakeShareable(new FUICommandList);

	// Commands would be registered here if using a command class
}

void FDeveloperProductivityTrackerModule::AddToolbarButton(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateRaw(this, &FDeveloperProductivityTrackerModule::OnToggleSession),
			FCanExecuteAction::CreateRaw(this, &FDeveloperProductivityTrackerModule::CanExecuteCommands)
		),
		NAME_None,
		LOCTEXT("ProductivityButton", "Productivity"),
		LOCTEXT("ProductivityButtonTooltip", "Toggle productivity tracking session"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Clock")
	);

	Builder.AddComboButton(
		FUIAction(),
		FOnGetContent::CreateLambda([this]()
		{
			FMenuBuilder MenuBuilder(true, PluginCommands);
			AddMenuExtension(MenuBuilder);
			return MenuBuilder.MakeWidget();
		}),
		LOCTEXT("ProductivityOptions", "Options"),
		LOCTEXT("ProductivityOptionsTooltip", "Productivity Tracker options"),
		FSlateIcon(),
		true
	);
}

void FDeveloperProductivityTrackerModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.BeginSection("ProductivitySession", LOCTEXT("SessionSection", "Session"));
	{
		Builder.AddMenuEntry(
			LOCTEXT("ToggleSession", "Toggle Session"),
			LOCTEXT("ToggleSessionTooltip", "Start or end the current tracking session"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FDeveloperProductivityTrackerModule::OnToggleSession))
		);

		Builder.AddMenuEntry(
			LOCTEXT("OpenDashboard", "Open Dashboard"),
			LOCTEXT("OpenDashboardTooltip", "Open the productivity dashboard"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FDeveloperProductivityTrackerModule::OnOpenDashboard))
		);
	}
	Builder.EndSection();

	Builder.BeginSection("ProductivitySettings", LOCTEXT("SettingsSection", "Settings"));
	{
		Builder.AddMenuEntry(
			LOCTEXT("OpenSettings", "Settings..."),
			LOCTEXT("OpenSettingsTooltip", "Open productivity tracker settings"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FDeveloperProductivityTrackerModule::OnOpenSettings))
		);
	}
	Builder.EndSection();
}

void FDeveloperProductivityTrackerModule::OnOpenDashboard()
{
	UE_LOG(LogProductivityTracker, Log, TEXT("Opening productivity dashboard..."));

	// The dashboard would be opened via EditorUtilityWidget
	// For now, log that it was requested
}

void FDeveloperProductivityTrackerModule::OnToggleSession()
{
	if (!GEditor)
	{
		return;
	}

	USessionTrackingSubsystem* SessionSubsystem = GEditor->GetEditorSubsystem<USessionTrackingSubsystem>();
	if (SessionSubsystem)
	{
		SessionSubsystem->ToggleSession();

		UE_LOG(LogProductivityTracker, Log, TEXT("Session toggled: %s"),
			SessionSubsystem->IsSessionActive() ? TEXT("Active") : TEXT("Inactive"));
	}
}

void FDeveloperProductivityTrackerModule::OnOpenSettings()
{
	// Open project settings to productivity tracker section
	FModuleManager::LoadModuleChecked<ISettingsModule>("Settings")
		.ShowViewer("Project", "Plugins", "Developer Productivity Tracker");
}

bool FDeveloperProductivityTrackerModule::CanExecuteCommands() const
{
	return GEditor != nullptr;
}

#undef LOCTEXT_NAMESPACE
