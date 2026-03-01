// ============================================================================
// BrightForgeConnect.Build.cs
// Developer: Marcus Daley
// Date: February 2026
// Project: BrightForgeConnect Plugin
// ============================================================================
// Purpose:
// Build configuration for the BrightForge Connect plugin module.
// Defines all module dependencies for editor integration and HTTP communication.
// ============================================================================

using UnrealBuildTool;
using System.IO;

public class BrightForgeConnect : ModuleRules
{
	public BrightForgeConnect(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Public include paths
		PublicIncludePaths.AddRange(
			new string[]
			{
				Path.Combine(ModuleDirectory, "Public"),
				Path.Combine(ModuleDirectory, "Public/Core"),
				Path.Combine(ModuleDirectory, "Public/Import"),
				Path.Combine(ModuleDirectory, "Public/UI")
			}
		);

		// Private include paths
		PrivateIncludePaths.AddRange(
			new string[]
			{
				Path.Combine(ModuleDirectory, "Private"),
				Path.Combine(ModuleDirectory, "Private/Core"),
				Path.Combine(ModuleDirectory, "Private/Import"),
				Path.Combine(ModuleDirectory, "Private/UI")
			}
		);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
			"HTTP",
			"Json",
			"JsonUtilities",
			"DeveloperSettings"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UnrealEd",
			"EditorSubsystem",
			"ToolMenus",
			"EditorFramework",
			"InputCore",
			"Projects",
			"WorkspaceMenuStructure",
			"ContentBrowser",
			"AssetTools"
		});
	}
}
