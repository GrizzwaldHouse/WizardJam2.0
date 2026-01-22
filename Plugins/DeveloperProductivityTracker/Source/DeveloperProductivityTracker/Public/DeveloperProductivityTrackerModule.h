// ============================================================================
// DeveloperProductivityTrackerModule.h
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Module interface for the Developer Productivity Tracker plugin.
// Handles plugin startup, shutdown, and toolbar integration.
//
// Architecture:
// IModuleInterface implementation for Unreal's module system.
// Registers editor toolbar extensions and menu items.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

// Log category for module operations
DECLARE_LOG_CATEGORY_EXTERN(LogProductivityTracker, Log, All);

class FToolBarBuilder;
class FMenuBuilder;
class FExtender;

class FDeveloperProductivityTrackerModule : public IModuleInterface
{
public:
	// ========================================================================
	// IModuleInterface Implementation
	// ========================================================================

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	// Check if module is loaded and ready
	virtual bool IsGameModule() const override { return false; }

	// ========================================================================
	// MODULE ACCESS
	// ========================================================================

	// Get module singleton
	static FDeveloperProductivityTrackerModule& Get();

	// Check if module is available
	static bool IsAvailable();

	// ========================================================================
	// TOOLBAR INTEGRATION
	// ========================================================================

	// Get the toolbar extender
	TSharedPtr<FExtender> GetToolBarExtender() const { return ToolBarExtender; }

private:
	// Toolbar extension
	TSharedPtr<FExtender> ToolBarExtender;
	TSharedPtr<const FExtensionBase> ToolBarExtension;

	// Menu commands
	TSharedPtr<FUICommandList> PluginCommands;

	// Registration methods
	void RegisterToolbarExtension();
	void UnregisterToolbarExtension();
	void RegisterMenus();
	void RegisterCommands();

	// Toolbar builders
	void AddToolbarButton(FToolBarBuilder& Builder);
	void AddMenuExtension(FMenuBuilder& Builder);

	// Command handlers
	void OnOpenDashboard();
	void OnToggleSession();
	void OnOpenSettings();

	// Validation
	bool CanExecuteCommands() const;
};
