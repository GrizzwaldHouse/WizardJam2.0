// ============================================================================
// KnownApplications.h
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Defines recognized applications for productivity tracking.
// Different categories have different productivity weights.
//
// Architecture:
// Pure data definitions with no behavioral code.
// Factory function provides default application database.
// ============================================================================

#pragma once

#include "CoreMinimal.h"

// Application category for activity tracking
enum class EExternalAppCategory : uint8
{
	Unknown,           // Not in detection list
	IDE,               // Visual Studio, VS Code, Rider, CLion
	VersionControl,    // Git GUI, P4V, SourceTree
	Documentation,     // Confluence, Notion, web browsers on docs
	Communication,     // Slack, Discord, Teams
	AssetCreation,     // Photoshop, Blender, Maya, Substance
	OtherGameEngine,   // Unity, Godot
	ProjectManagement, // Jira, Trello, Asana
	BuildTools,        // Jenkins, build monitors
	WebBrowser,        // General web browsing
	Terminal           // Command line tools
};

// Known application definition
struct DEVELOPERPRODUCTIVITYTRACKER_API FKnownApplication
{
	// Display name for UI
	FString DisplayName;

	// Process names to detect (e.g., "devenv.exe", "Code.exe")
	TArray<FString> ProcessNames;

	// Window title patterns for additional matching
	TArray<FString> WindowTitlePatterns;

	// Application category
	EExternalAppCategory Category;

	// Whether this app is considered productive for development
	bool bIsProductiveApp;

	// Weight multiplier for productivity calculation (0.0 - 1.0)
	float ProductivityWeight;

	FKnownApplication()
		: Category(EExternalAppCategory::Unknown)
		, bIsProductiveApp(false)
		, ProductivityWeight(1.0f)
	{
	}

	// Constructor for easy initialization
	FKnownApplication(
		const FString& InDisplayName,
		const TArray<FString>& InProcessNames,
		EExternalAppCategory InCategory,
		bool InIsProductive,
		float InWeight)
		: DisplayName(InDisplayName)
		, ProcessNames(InProcessNames)
		, Category(InCategory)
		, bIsProductiveApp(InIsProductive)
		, ProductivityWeight(InWeight)
	{
	}
};

// Current external activity state
struct DEVELOPERPRODUCTIVITYTRACKER_API FExternalActivityState
{
	// Is a development application currently focused?
	bool bDevelopmentAppFocused;

	// Name of the focused application
	FString FocusedAppName;

	// Category of the focused application
	EExternalAppCategory FocusedAppCategory;

	// Is the focused app considered productive?
	bool bFocusedAppIsProductive;

	// Productivity weight of focused app
	float FocusedAppProductivityWeight;

	// List of running development applications
	TArray<FString> RunningDevApps;

	// Time since last external activity
	float SecondsSinceExternalActivity;

	// Were source files modified recently?
	bool bSourceFilesModifiedRecently;

	// Path of last modified source file
	FString LastModifiedSourceFile;

	// When was the last source modification?
	FDateTime LastSourceModificationTime;

	// Is a build currently in progress?
	bool bBuildInProgress;

	// When was this state last updated?
	FDateTime LastUpdateTime;

	FExternalActivityState()
		: bDevelopmentAppFocused(false)
		, FocusedAppCategory(EExternalAppCategory::Unknown)
		, bFocusedAppIsProductive(false)
		, FocusedAppProductivityWeight(0.0f)
		, SecondsSinceExternalActivity(0.0f)
		, bSourceFilesModifiedRecently(false)
		, LastSourceModificationTime(FDateTime::MinValue())
		, bBuildInProgress(false)
		, LastUpdateTime(FDateTime::Now())
	{
	}

	// Check if there's any external development activity
	bool HasExternalActivity() const
	{
		return bDevelopmentAppFocused || bSourceFilesModifiedRecently || bBuildInProgress;
	}

	// Check if the external activity is productive
	bool IsExternallyProductive() const
	{
		return (bDevelopmentAppFocused && bFocusedAppIsProductive) ||
			   bSourceFilesModifiedRecently ||
			   bBuildInProgress;
	}

	// Get a description of the current activity
	FString GetActivityDescription() const
	{
		if (bBuildInProgress)
		{
			return TEXT("Building...");
		}
		if (bSourceFilesModifiedRecently)
		{
			return FString::Printf(TEXT("Editing: %s"), *FPaths::GetCleanFilename(LastModifiedSourceFile));
		}
		if (bDevelopmentAppFocused)
		{
			return FocusedAppName;
		}
		return TEXT("No external activity");
	}
};

// Factory class for creating default application definitions
class DEVELOPERPRODUCTIVITYTRACKER_API FKnownApplicationsFactory
{
public:
	// Get the default list of known applications
	static TArray<FKnownApplication> GetDefaultApplications();

	// Get applications by category
	static TArray<FKnownApplication> GetApplicationsByCategory(EExternalAppCategory Category);

	// Get category display name
	static FString GetCategoryDisplayName(EExternalAppCategory Category);

private:
	// Individual category builders
	static TArray<FKnownApplication> CreateIDEApplications();
	static TArray<FKnownApplication> CreateVersionControlApplications();
	static TArray<FKnownApplication> CreateAssetCreationApplications();
	static TArray<FKnownApplication> CreateCommunicationApplications();
	static TArray<FKnownApplication> CreateProjectManagementApplications();
	static TArray<FKnownApplication> CreateTerminalApplications();
};
