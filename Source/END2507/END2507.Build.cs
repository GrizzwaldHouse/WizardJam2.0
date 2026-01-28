// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class END2507 : ModuleRules
{
	public END2507(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] {  "NavigationSystem",
    "GameplayTasks" ,"Core", "CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AnimGraphRuntime",
			"UMG",
			"AIModule",
			"AnimGraphRuntime",
			"UMG",
			"GameplayTasks",  // For BT tasks
			"NavigationSystem"  // For AI pathfinding
		});      
     


        PrivateDependencyModuleNames.AddRange(new string[] { "StructuredLogging" });


        
        //PublicIncludePaths.AddRange(new string[] {
        //    ModuleDirectory,                                    // Source/END2507/
        //    ModuleDirectory + "/WizardJam",                     // Source/END2507/WizardJam/
        //    ModuleDirectory + "/WizardJam/Public",              // Source/END2507/WizardJam/Public/
        //    ModuleDirectory + "/WizardJam/Public/Actors",       // Source/END2507/WizardJam/Public/Actors/
        //    ModuleDirectory + "/WizardJam/Public/Utility",      // Source/END2507/WizardJam/Public/Utility/
        //    ModuleDirectory + "/WizardJam/Public/UI",           // Source/END2507/WizardJam/Public/UI/
        //    ModuleDirectory + "/WizardJam/Public/Data",         // Source/END2507/WizardJam/Public/Data/
        //    ModuleDirectory + "/WizardJam/Public/GameModes",    // Source/END2507/WizardJam/Public/GameModes/
        //});
        // Uncomment if you are using Slate UI
        PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Productivity Sky System - Niagara for visual effects
        PublicDependencyModuleNames.AddRange(new string[] {
            "Niagara",
            "ProceduralMeshComponent"
        });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
