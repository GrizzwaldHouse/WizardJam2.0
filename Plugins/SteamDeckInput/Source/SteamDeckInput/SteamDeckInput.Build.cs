using UnrealBuildTool;

public class SteamDeckInput : ModuleRules
{
    public SteamDeckInput(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "EnhancedInput",
            "DeveloperSettings"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Slate",
            "SlateCore",
            "UMG"
        });

        // Uncomment for direct Steam API integration (gyro, trackpad, haptics):
        // PrivateDependencyModuleNames.Add("Steamworks");
        // PrivateDependencyModuleNames.Add("OnlineSubsystemSteam");
    }
}
