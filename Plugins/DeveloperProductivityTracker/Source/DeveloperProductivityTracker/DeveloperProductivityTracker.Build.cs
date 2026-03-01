// ============================================================================
// DeveloperProductivityTracker.Build.cs
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Build configuration for the Developer Productivity Tracker plugin module.
// Defines all module dependencies and platform-specific settings.
//
// Architecture:
// Editor-only module that integrates with Unreal's editor subsystem framework.
// Uses Windows-specific APIs for process and window monitoring.
// ============================================================================

using UnrealBuildTool;
using System.IO;

public class DeveloperProductivityTracker : ModuleRules
{
	public DeveloperProductivityTracker(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Enable RTTI for dynamic casting in activity monitor implementations
		bUseRTTI = false;

		// Disable exceptions for performance
		bEnableExceptions = false;

		// Public include paths
		PublicIncludePaths.AddRange(
			new string[]
			{
				Path.Combine(ModuleDirectory, "Public"),
				Path.Combine(ModuleDirectory, "Public/Core"),
				Path.Combine(ModuleDirectory, "Public/External"),
				Path.Combine(ModuleDirectory, "Public/Wellness"),
				Path.Combine(ModuleDirectory, "Public/Visualization"),
				Path.Combine(ModuleDirectory, "Public/UI")
			}
		);

		// Private include paths
		PrivateIncludePaths.AddRange(
			new string[]
			{
				Path.Combine(ModuleDirectory, "Private"),
				Path.Combine(ModuleDirectory, "Private/Core"),
				Path.Combine(ModuleDirectory, "Private/External"),
				Path.Combine(ModuleDirectory, "Private/Wellness"),
				Path.Combine(ModuleDirectory, "Private/Visualization"),
				Path.Combine(ModuleDirectory, "Private/UI")
			}
		);

		// Engine module dependencies
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"Slate",
				"SlateCore",
				"UMG",
				"EditorSubsystem",
				"UnrealEd",
				"DirectoryWatcher",
				"Json",
				"JsonUtilities",
				"DeveloperSettings"
			}
		);

		// Private module dependencies
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"EditorStyle",
				"ToolMenus",
				"EditorFramework",
				"WorkspaceMenuStructure",
				"DesktopPlatform",
				"ApplicationCore",
				"Blutility",
				"UMGEditor",
				"HTTPServer"
			}
		);

		// Platform-specific dependencies and definitions
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			// Windows-specific libraries for process and window monitoring
			PublicSystemLibraries.AddRange(
				new string[]
				{
					"User32.lib",
					"Psapi.lib",
					"Advapi32.lib"
				}
			);

			// Define Windows platform macro for conditional compilation
			PublicDefinitions.Add("PRODUCTIVITY_TRACKER_WINDOWS=1");
		}
		else
		{
			PublicDefinitions.Add("PRODUCTIVITY_TRACKER_WINDOWS=0");
		}

		// Enable logging in all builds for diagnostics
		PublicDefinitions.Add("PRODUCTIVITY_TRACKER_LOGGING_ENABLED=1");

		// Development-only features
		if (Target.Configuration == UnrealTargetConfiguration.Debug ||
			Target.Configuration == UnrealTargetConfiguration.Development)
		{
			PublicDefinitions.Add("PRODUCTIVITY_TRACKER_DEV_FEATURES=1");
		}
		else
		{
			PublicDefinitions.Add("PRODUCTIVITY_TRACKER_DEV_FEATURES=0");
		}
	}
}
