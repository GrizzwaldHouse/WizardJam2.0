// ============================================================================
// ExternalActivityMonitor.cpp
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Factory implementation for creating platform-specific external monitors.
// Includes implementation of the known applications factory.
// ============================================================================

#include "External/ExternalActivityMonitor.h"

// Forward declare the Windows implementation
#if PLATFORM_WINDOWS
class FWindowsExternalActivityMonitor;
#endif

// ============================================================================
// Factory Method
// ============================================================================

TUniquePtr<IExternalActivityMonitor> IExternalActivityMonitor::Create()
{
#if PLATFORM_WINDOWS
	// Forward declaration - actual class defined in WindowsExternalActivityMonitor.cpp
	extern TUniquePtr<IExternalActivityMonitor> CreateWindowsExternalActivityMonitor();
	return CreateWindowsExternalActivityMonitor();
#else
	UE_LOG(LogTemp, Warning, TEXT("External activity monitoring not supported on this platform"));
	return nullptr;
#endif
}

// ============================================================================
// Known Applications Factory
// ============================================================================

TArray<FKnownApplication> FKnownApplicationsFactory::GetDefaultApplications()
{
	TArray<FKnownApplication> AllApps;

	// Combine all categories
	AllApps.Append(CreateIDEApplications());
	AllApps.Append(CreateVersionControlApplications());
	AllApps.Append(CreateAssetCreationApplications());
	AllApps.Append(CreateCommunicationApplications());
	AllApps.Append(CreateProjectManagementApplications());
	AllApps.Append(CreateTerminalApplications());
	AllApps.Append(CreateGameEngineApplications());

	return AllApps;
}

TArray<FKnownApplication> FKnownApplicationsFactory::GetApplicationsByCategory(EExternalAppCategory Category)
{
	TArray<FKnownApplication> AllApps = GetDefaultApplications();
	TArray<FKnownApplication> Filtered;

	for (const FKnownApplication& App : AllApps)
	{
		if (App.Category == Category)
		{
			Filtered.Add(App);
		}
	}

	return Filtered;
}

FString FKnownApplicationsFactory::GetCategoryDisplayName(EExternalAppCategory Category)
{
	switch (Category)
	{
	case EExternalAppCategory::IDE:
		return TEXT("Integrated Development Environments");
	case EExternalAppCategory::VersionControl:
		return TEXT("Version Control");
	case EExternalAppCategory::Documentation:
		return TEXT("Documentation");
	case EExternalAppCategory::Communication:
		return TEXT("Communication");
	case EExternalAppCategory::AssetCreation:
		return TEXT("Asset Creation");
	case EExternalAppCategory::OtherGameEngine:
		return TEXT("Other Game Engines");
	case EExternalAppCategory::ProjectManagement:
		return TEXT("Project Management");
	case EExternalAppCategory::BuildTools:
		return TEXT("Build Tools");
	case EExternalAppCategory::WebBrowser:
		return TEXT("Web Browser");
	case EExternalAppCategory::Terminal:
		return TEXT("Terminal");
	case EExternalAppCategory::GameEngine:
		return TEXT("Game Engines");
	default:
		return TEXT("Unknown");
	}
}

TArray<FKnownApplication> FKnownApplicationsFactory::CreateIDEApplications()
{
	TArray<FKnownApplication> Apps;

	// Visual Studio
	Apps.Add(FKnownApplication(
		TEXT("Visual Studio"),
		{ TEXT("devenv.exe") },
		EExternalAppCategory::IDE,
		true,
		1.0f
	));

	// Visual Studio Code
	Apps.Add(FKnownApplication(
		TEXT("Visual Studio Code"),
		{ TEXT("Code.exe"), TEXT("Code - Insiders.exe") },
		EExternalAppCategory::IDE,
		true,
		1.0f
	));

	// JetBrains Rider
	Apps.Add(FKnownApplication(
		TEXT("JetBrains Rider"),
		{ TEXT("rider64.exe"), TEXT("rider.exe") },
		EExternalAppCategory::IDE,
		true,
		1.0f
	));

	// JetBrains CLion
	Apps.Add(FKnownApplication(
		TEXT("JetBrains CLion"),
		{ TEXT("clion64.exe"), TEXT("clion.exe") },
		EExternalAppCategory::IDE,
		true,
		1.0f
	));

	// JetBrains IntelliJ IDEA
	Apps.Add(FKnownApplication(
		TEXT("IntelliJ IDEA"),
		{ TEXT("idea64.exe"), TEXT("idea.exe") },
		EExternalAppCategory::IDE,
		true,
		1.0f
	));

	// Sublime Text
	Apps.Add(FKnownApplication(
		TEXT("Sublime Text"),
		{ TEXT("sublime_text.exe") },
		EExternalAppCategory::IDE,
		true,
		0.95f
	));

	// Notepad++
	Apps.Add(FKnownApplication(
		TEXT("Notepad++"),
		{ TEXT("notepad++.exe") },
		EExternalAppCategory::IDE,
		true,
		0.9f
	));

	// Vim/Neovim (typically run in terminals, but standalone GUIs exist)
	Apps.Add(FKnownApplication(
		TEXT("Neovim"),
		{ TEXT("nvim.exe"), TEXT("nvim-qt.exe") },
		EExternalAppCategory::IDE,
		true,
		1.0f
	));

	// Atom (legacy but some still use it)
	Apps.Add(FKnownApplication(
		TEXT("Atom"),
		{ TEXT("atom.exe") },
		EExternalAppCategory::IDE,
		true,
		0.95f
	));

	return Apps;
}

TArray<FKnownApplication> FKnownApplicationsFactory::CreateVersionControlApplications()
{
	TArray<FKnownApplication> Apps;

	// Perforce P4V
	Apps.Add(FKnownApplication(
		TEXT("Perforce P4V"),
		{ TEXT("p4v.exe") },
		EExternalAppCategory::VersionControl,
		true,
		0.9f
	));

	// Git GUI clients
	Apps.Add(FKnownApplication(
		TEXT("SourceTree"),
		{ TEXT("SourceTree.exe") },
		EExternalAppCategory::VersionControl,
		true,
		0.9f
	));

	Apps.Add(FKnownApplication(
		TEXT("GitKraken"),
		{ TEXT("gitkraken.exe") },
		EExternalAppCategory::VersionControl,
		true,
		0.9f
	));

	Apps.Add(FKnownApplication(
		TEXT("GitHub Desktop"),
		{ TEXT("GitHubDesktop.exe") },
		EExternalAppCategory::VersionControl,
		true,
		0.9f
	));

	Apps.Add(FKnownApplication(
		TEXT("Fork"),
		{ TEXT("Fork.exe") },
		EExternalAppCategory::VersionControl,
		true,
		0.9f
	));

	Apps.Add(FKnownApplication(
		TEXT("SmartGit"),
		{ TEXT("smartgit.exe"), TEXT("smartgit64.exe") },
		EExternalAppCategory::VersionControl,
		true,
		0.9f
	));

	Apps.Add(FKnownApplication(
		TEXT("TortoiseGit"),
		{ TEXT("TortoiseGitProc.exe") },
		EExternalAppCategory::VersionControl,
		true,
		0.9f
	));

	Apps.Add(FKnownApplication(
		TEXT("TortoiseSVN"),
		{ TEXT("TortoiseProc.exe") },
		EExternalAppCategory::VersionControl,
		true,
		0.9f
	));

	return Apps;
}

TArray<FKnownApplication> FKnownApplicationsFactory::CreateAssetCreationApplications()
{
	TArray<FKnownApplication> Apps;

	// 3D Modeling
	Apps.Add(FKnownApplication(
		TEXT("Blender"),
		{ TEXT("blender.exe") },
		EExternalAppCategory::AssetCreation,
		true,
		1.0f
	));

	Apps.Add(FKnownApplication(
		TEXT("Autodesk Maya"),
		{ TEXT("maya.exe") },
		EExternalAppCategory::AssetCreation,
		true,
		1.0f
	));

	Apps.Add(FKnownApplication(
		TEXT("Autodesk 3ds Max"),
		{ TEXT("3dsmax.exe") },
		EExternalAppCategory::AssetCreation,
		true,
		1.0f
	));

	Apps.Add(FKnownApplication(
		TEXT("Cinema 4D"),
		{ TEXT("Cinema 4D.exe") },
		EExternalAppCategory::AssetCreation,
		true,
		1.0f
	));

	Apps.Add(FKnownApplication(
		TEXT("ZBrush"),
		{ TEXT("ZBrush.exe") },
		EExternalAppCategory::AssetCreation,
		true,
		1.0f
	));

	// 2D/Texture
	Apps.Add(FKnownApplication(
		TEXT("Adobe Photoshop"),
		{ TEXT("Photoshop.exe") },
		EExternalAppCategory::AssetCreation,
		true,
		1.0f
	));

	Apps.Add(FKnownApplication(
		TEXT("Adobe Illustrator"),
		{ TEXT("Illustrator.exe") },
		EExternalAppCategory::AssetCreation,
		true,
		1.0f
	));

	Apps.Add(FKnownApplication(
		TEXT("GIMP"),
		{ TEXT("gimp-2.10.exe"), TEXT("gimp.exe") },
		EExternalAppCategory::AssetCreation,
		true,
		1.0f
	));

	Apps.Add(FKnownApplication(
		TEXT("Krita"),
		{ TEXT("krita.exe") },
		EExternalAppCategory::AssetCreation,
		true,
		1.0f
	));

	// Substance
	Apps.Add(FKnownApplication(
		TEXT("Substance Painter"),
		{ TEXT("Substance Painter.exe"), TEXT("Adobe Substance 3D Painter.exe") },
		EExternalAppCategory::AssetCreation,
		true,
		1.0f
	));

	Apps.Add(FKnownApplication(
		TEXT("Substance Designer"),
		{ TEXT("Substance Designer.exe"), TEXT("Adobe Substance 3D Designer.exe") },
		EExternalAppCategory::AssetCreation,
		true,
		1.0f
	));

	// Audio
	Apps.Add(FKnownApplication(
		TEXT("Audacity"),
		{ TEXT("Audacity.exe") },
		EExternalAppCategory::AssetCreation,
		true,
		0.9f
	));

	Apps.Add(FKnownApplication(
		TEXT("FMOD Studio"),
		{ TEXT("FMOD Studio.exe") },
		EExternalAppCategory::AssetCreation,
		true,
		1.0f
	));

	Apps.Add(FKnownApplication(
		TEXT("Wwise"),
		{ TEXT("Wwise.exe") },
		EExternalAppCategory::AssetCreation,
		true,
		1.0f
	));

	return Apps;
}

TArray<FKnownApplication> FKnownApplicationsFactory::CreateCommunicationApplications()
{
	TArray<FKnownApplication> Apps;

	Apps.Add(FKnownApplication(
		TEXT("Slack"),
		{ TEXT("slack.exe") },
		EExternalAppCategory::Communication,
		false,
		0.5f
	));

	Apps.Add(FKnownApplication(
		TEXT("Discord"),
		{ TEXT("Discord.exe") },
		EExternalAppCategory::Communication,
		false,
		0.5f
	));

	Apps.Add(FKnownApplication(
		TEXT("Microsoft Teams"),
		{ TEXT("Teams.exe"), TEXT("ms-teams.exe") },
		EExternalAppCategory::Communication,
		false,
		0.5f
	));

	Apps.Add(FKnownApplication(
		TEXT("Zoom"),
		{ TEXT("Zoom.exe") },
		EExternalAppCategory::Communication,
		false,
		0.6f
	));

	Apps.Add(FKnownApplication(
		TEXT("Skype"),
		{ TEXT("Skype.exe") },
		EExternalAppCategory::Communication,
		false,
		0.5f
	));

	return Apps;
}

TArray<FKnownApplication> FKnownApplicationsFactory::CreateProjectManagementApplications()
{
	TArray<FKnownApplication> Apps;

	// Most PM tools are web-based, but some have desktop apps
	Apps.Add(FKnownApplication(
		TEXT("Notion"),
		{ TEXT("Notion.exe") },
		EExternalAppCategory::ProjectManagement,
		true,
		0.8f
	));

	Apps.Add(FKnownApplication(
		TEXT("Obsidian"),
		{ TEXT("Obsidian.exe") },
		EExternalAppCategory::ProjectManagement,
		true,
		0.8f
	));

	Apps.Add(FKnownApplication(
		TEXT("Trello"),
		{ TEXT("Trello.exe") },
		EExternalAppCategory::ProjectManagement,
		true,
		0.8f
	));

	return Apps;
}

TArray<FKnownApplication> FKnownApplicationsFactory::CreateTerminalApplications()
{
	TArray<FKnownApplication> Apps;

	Apps.Add(FKnownApplication(
		TEXT("Windows Terminal"),
		{ TEXT("WindowsTerminal.exe"), TEXT("wt.exe") },
		EExternalAppCategory::Terminal,
		true,
		0.95f
	));

	Apps.Add(FKnownApplication(
		TEXT("Command Prompt"),
		{ TEXT("cmd.exe") },
		EExternalAppCategory::Terminal,
		true,
		0.9f
	));

	Apps.Add(FKnownApplication(
		TEXT("PowerShell"),
		{ TEXT("powershell.exe"), TEXT("pwsh.exe") },
		EExternalAppCategory::Terminal,
		true,
		0.95f
	));

	Apps.Add(FKnownApplication(
		TEXT("Git Bash"),
		{ TEXT("git-bash.exe"), TEXT("bash.exe") },
		EExternalAppCategory::Terminal,
		true,
		0.95f
	));

	Apps.Add(FKnownApplication(
		TEXT("ConEmu"),
		{ TEXT("ConEmu64.exe"), TEXT("ConEmu.exe") },
		EExternalAppCategory::Terminal,
		true,
		0.95f
	));

	Apps.Add(FKnownApplication(
		TEXT("Cmder"),
		{ TEXT("Cmder.exe") },
		EExternalAppCategory::Terminal,
		true,
		0.95f
	));

	return Apps;
}

TArray<FKnownApplication> FKnownApplicationsFactory::CreateGameEngineApplications()
{
	TArray<FKnownApplication> Apps;

	// Unreal Engine - all common executable variants
	Apps.Add(FKnownApplication(
		TEXT("Unreal Engine"),
		{
			TEXT("UnrealEditor.exe"),
			TEXT("UnrealEditor-Win64-Debug.exe"),
			TEXT("UnrealEditor-Win64-DebugGame.exe"),
			TEXT("UnrealEditor-Win64-Development.exe"),
			TEXT("UnrealEditor-Cmd.exe"),
			TEXT("UE4Editor.exe"),
			TEXT("UE4Editor-Cmd.exe")
		},
		EExternalAppCategory::GameEngine,
		true,
		1.0f
	));

	// Unity Editor
	Apps.Add(FKnownApplication(
		TEXT("Unity Editor"),
		{ TEXT("Unity.exe"), TEXT("Unity Hub.exe") },
		EExternalAppCategory::GameEngine,
		true,
		1.0f
	));

	// Godot Engine
	Apps.Add(FKnownApplication(
		TEXT("Godot Engine"),
		{ TEXT("Godot.exe"), TEXT("Godot_v4.exe"), TEXT("Godot_v4.2.exe") },
		EExternalAppCategory::GameEngine,
		true,
		1.0f
	));

	return Apps;
}
