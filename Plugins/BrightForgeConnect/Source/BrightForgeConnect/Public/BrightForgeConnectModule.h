// ============================================================================
// BrightForgeConnectModule.h
// Developer: Marcus Daley
// Date: February 2026
// Project: BrightForgeConnect Plugin
// ============================================================================
// Purpose:
// Module interface for the BrightForge Connect plugin.
// Handles plugin startup, shutdown, toolbar integration, and tab spawning.
//
// Architecture:
// IModuleInterface implementation for Unreal's module system.
// Registers editor toolbar extensions, menu items, and dockable tab spawner.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// Log category for all BrightForge plugin operations
DECLARE_LOG_CATEGORY_EXTERN(LogBrightForge, Log, All);

class FToolBarBuilder;
class FMenuBuilder;
class FExtender;

class FBrightForgeConnectModule : public IModuleInterface
{
public:
	// ========================================================================
	// IModuleInterface Implementation
	// ========================================================================

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	virtual bool IsGameModule() const override { return false; }

	// ========================================================================
	// MODULE ACCESS
	// ========================================================================

	/** Get the module singleton */
	static FBrightForgeConnectModule& Get();

	/** Check if module is loaded and ready */
	static bool IsAvailable();

	/** Get the toolbar extender */
	TSharedPtr<FExtender> GetToolBarExtender() const { return ToolBarExtender; }

private:
	// Toolbar extension handles
	TSharedPtr<FExtender> ToolBarExtender;
	TSharedPtr<const FExtensionBase> ToolBarExtension;

	// Menu command list
	TSharedPtr<FUICommandList> PluginCommands;

	// ========================================================================
	// REGISTRATION
	// ========================================================================

	void RegisterCommands();
	void RegisterToolbarExtension();
	void UnregisterToolbarExtension();
	void RegisterMenus();
	void RegisterTabSpawner();
	void UnregisterTabSpawner();

	// ========================================================================
	// BUILDERS
	// ========================================================================

	void AddToolbarButton(FToolBarBuilder& Builder);
	void AddMenuExtension(FMenuBuilder& Builder);

	// ========================================================================
	// COMMAND HANDLERS
	// ========================================================================

	void OnOpenBrightForgePanel();
	void OnOpenSettings();

	bool CanExecuteCommands() const;

	// ========================================================================
	// TAB SPAWNER
	// ========================================================================

	TSharedRef<SDockTab> SpawnBrightForgeTab(const FSpawnTabArgs& SpawnTabArgs);
};
