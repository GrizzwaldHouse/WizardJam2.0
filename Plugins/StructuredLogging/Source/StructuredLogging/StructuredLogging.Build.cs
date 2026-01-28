// ============================================================================
// StructuredLogging.Build.cs
// Developer: Marcus Daley
// Date: January 25, 2026
// Plugin: StructuredLogging
// ============================================================================
// Purpose:
// Build configuration for the Structured Logging plugin. Defines module
// dependencies for JSON serialization, threading, and core Unreal systems.
//
// Module Dependencies:
// - Core, CoreUObject, Engine: Standard UE subsystem functionality
// - Json, JsonUtilities: JSON formatting for structured log output
// - DeveloperSettings: Plugin configuration UI (optional)
//
// Build Configurations:
// - Development/Editor: Full logging enabled
// - Shipping: Macros expand to no-ops (zero runtime cost)
// ============================================================================

using UnrealBuildTool;

public class StructuredLogging : ModuleRules
{
	public StructuredLogging(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Json",
				"JsonUtilities"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"DeveloperSettings"
			}
		);

		// Conditional compilation: disable structured logging in shipping builds
		if (Target.Configuration == UnrealTargetConfiguration.Shipping)
		{
			PublicDefinitions.Add("STRUCTURED_LOGGING_ENABLED=0");
		}
		else
		{
			PublicDefinitions.Add("STRUCTURED_LOGGING_ENABLED=1");
		}
	}
}
