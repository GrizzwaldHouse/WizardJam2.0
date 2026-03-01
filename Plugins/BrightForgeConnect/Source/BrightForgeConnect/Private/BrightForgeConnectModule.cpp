// ============================================================================
// BrightForgeConnectModule.cpp
// Developer: Marcus Daley
// Date: February 2026
// Project: BrightForgeConnect Plugin
// ============================================================================
// Purpose:
// Module implementation handling startup, shutdown, toolbar, and tab registration.
// ============================================================================

#include "BrightForgeConnectModule.h"
#include "Core/BrightForgeClientSubsystem.h"
#include "Core/BrightForgeSettings.h"
#include "UI/SBrightForgePanel.h"
#include "LevelEditor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "ISettingsModule.h"

#define LOCTEXT_NAMESPACE "BrightForgeConnect"

DEFINE_LOG_CATEGORY(LogBrightForge);

// Unique tab identifier for the BrightForge panel
static const FName BrightForgeTabName("BrightForgeConnect");

IMPLEMENT_MODULE(FBrightForgeConnectModule, BrightForgeConnect)

// ============================================================================
// IModuleInterface
// ============================================================================

void FBrightForgeConnectModule::StartupModule()
{
	UE_LOG(LogBrightForge, Log, TEXT("BrightForge Connect module starting up..."));

	RegisterCommands();
	RegisterToolbarExtension();
	RegisterTabSpawner();

	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FBrightForgeConnectModule::RegisterMenus)
	);

	UE_LOG(LogBrightForge, Log, TEXT("BrightForge Connect module started successfully"));
}

void FBrightForgeConnectModule::ShutdownModule()
{
	UE_LOG(LogBrightForge, Log, TEXT("BrightForge Connect module shutting down..."));

	UnregisterTabSpawner();
	UnregisterToolbarExtension();

	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	UE_LOG(LogBrightForge, Log, TEXT("BrightForge Connect module shutdown complete"));
}

// ============================================================================
// MODULE ACCESS
// ============================================================================

FBrightForgeConnectModule& FBrightForgeConnectModule::Get()
{
	return FModuleManager::LoadModuleChecked<FBrightForgeConnectModule>("BrightForgeConnect");
}

bool FBrightForgeConnectModule::IsAvailable()
{
	return FModuleManager::Get().IsModuleLoaded("BrightForgeConnect");
}

// ============================================================================
// REGISTRATION
// ============================================================================

void FBrightForgeConnectModule::RegisterCommands()
{
	PluginCommands = MakeShareable(new FUICommandList);
}

void FBrightForgeConnectModule::RegisterToolbarExtension()
{
	ToolBarExtender = MakeShareable(new FExtender);

	ToolBarExtension = ToolBarExtender->AddToolBarExtension(
		"Play",
		EExtensionHook::After,
		PluginCommands,
		FToolBarExtensionDelegate::CreateRaw(this, &FBrightForgeConnectModule::AddToolbarButton)
	);

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolBarExtender);
}

void FBrightForgeConnectModule::UnregisterToolbarExtension()
{
	if (ToolBarExtender.IsValid() && FModuleManager::Get().IsModuleLoaded("LevelEditor"))
	{
		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		LevelEditorModule.GetToolBarExtensibilityManager()->RemoveExtender(ToolBarExtender);
	}

	ToolBarExtender.Reset();
}

void FBrightForgeConnectModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	// Add entry under Window menu
	UToolMenu* WindowMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
	if (WindowMenu)
	{
		FToolMenuSection& Section = WindowMenu->FindOrAddSection("WindowLocalTabSpawners");
		Section.AddMenuEntry(
			"BrightForgeConnect",
			LOCTEXT("BrightForgeMenuLabel", "BrightForge Connect"),
			LOCTEXT("BrightForgeMenuTooltip", "Open the BrightForge Connect panel for AI 3D asset generation"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.BulletPoint"),
			FUIAction(FExecuteAction::CreateRaw(this, &FBrightForgeConnectModule::OnOpenBrightForgePanel))
		);
	}
}

void FBrightForgeConnectModule::RegisterTabSpawner()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		BrightForgeTabName,
		FOnSpawnTab::CreateRaw(this, &FBrightForgeConnectModule::SpawnBrightForgeTab)
	)
	.SetDisplayName(LOCTEXT("BrightForgeTabTitle", "BrightForge Connect"))
	.SetTooltipText(LOCTEXT("BrightForgeTabTooltip", "AI-powered 3D asset generation via BrightForge"))
	.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.BulletPoint"))
	.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());
}

void FBrightForgeConnectModule::UnregisterTabSpawner()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(BrightForgeTabName);
}

// ============================================================================
// BUILDERS
// ============================================================================

void FBrightForgeConnectModule::AddToolbarButton(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateRaw(this, &FBrightForgeConnectModule::OnOpenBrightForgePanel),
			FCanExecuteAction::CreateRaw(this, &FBrightForgeConnectModule::CanExecuteCommands)
		),
		NAME_None,
		LOCTEXT("BrightForgeButton", "BrightForge"),
		LOCTEXT("BrightForgeButtonTooltip", "Open BrightForge Connect — AI 3D asset generation"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.BulletPoint")
	);

	Builder.AddComboButton(
		FUIAction(),
		FOnGetContent::CreateLambda([this]()
		{
			FMenuBuilder MenuBuilder(true, PluginCommands);
			AddMenuExtension(MenuBuilder);
			return MenuBuilder.MakeWidget();
		}),
		LOCTEXT("BrightForgeOptions", "Options"),
		LOCTEXT("BrightForgeOptionsTooltip", "BrightForge Connect options"),
		FSlateIcon(),
		true
	);
}

void FBrightForgeConnectModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.BeginSection("BrightForgeActions", LOCTEXT("ActionsSection", "Actions"));
	{
		Builder.AddMenuEntry(
			LOCTEXT("OpenPanel", "Open Panel"),
			LOCTEXT("OpenPanelTooltip", "Open the BrightForge Connect panel"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FBrightForgeConnectModule::OnOpenBrightForgePanel))
		);
	}
	Builder.EndSection();

	Builder.BeginSection("BrightForgeSettings", LOCTEXT("SettingsSection", "Settings"));
	{
		Builder.AddMenuEntry(
			LOCTEXT("OpenSettings", "Settings..."),
			LOCTEXT("OpenSettingsTooltip", "Open BrightForge Connect settings"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FBrightForgeConnectModule::OnOpenSettings))
		);
	}
	Builder.EndSection();
}

// ============================================================================
// COMMAND HANDLERS
// ============================================================================

void FBrightForgeConnectModule::OnOpenBrightForgePanel()
{
	UE_LOG(LogBrightForge, Log, TEXT("Opening BrightForge Connect panel..."));
	FGlobalTabmanager::Get()->TryInvokeTab(BrightForgeTabName);
}

void FBrightForgeConnectModule::OnOpenSettings()
{
	FModuleManager::LoadModuleChecked<ISettingsModule>("Settings")
		.ShowViewer("Project", "Plugins", "BrightForge Connect");
}

bool FBrightForgeConnectModule::CanExecuteCommands() const
{
	return GEditor != nullptr;
}

// ============================================================================
// TAB SPAWNER
// ============================================================================

TSharedRef<SDockTab> FBrightForgeConnectModule::SpawnBrightForgeTab(const FSpawnTabArgs& SpawnTabArgs)
{
	UE_LOG(LogBrightForge, Log, TEXT("Spawning BrightForge Connect tab"));

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SBrightForgePanel)
		];
}

#undef LOCTEXT_NAMESPACE
