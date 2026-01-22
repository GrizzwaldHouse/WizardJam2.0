// ============================================================================
// FileChangeDetector.h
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Monitors specified directories for source file changes.
// Uses Unreal's DirectoryWatcher module for cross-platform support.
//
// Architecture:
// Wraps DirectoryWatcher with source file filtering.
// Tracks recent modifications for activity detection.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "IDirectoryWatcher.h"
#include "ExternalActivityMonitor.h"

// Delegate for file change notifications
DECLARE_DELEGATE_OneParam(FOnFileChangeDetected, const FFileChangeEvent&);

// File change detector using Unreal's DirectoryWatcher
class DEVELOPERPRODUCTIVITYTRACKER_API FFileChangeDetector
{
public:
	FFileChangeDetector();
	~FFileChangeDetector();

	// ========================================================================
	// LIFECYCLE
	// ========================================================================

	// Initialize the detector
	bool Initialize();

	// Shutdown and cleanup
	void Shutdown();

	// Update state (clear old modifications, etc.)
	void Update(float DeltaTime);

	// ========================================================================
	// DIRECTORY MANAGEMENT
	// ========================================================================

	// Add a directory to monitor
	bool AddDirectory(const FString& Directory);

	// Remove a monitored directory (named to avoid Windows macro conflict)
	bool RemoveMonitoredDirectory(const FString& Directory);

	// Get all monitored directories
	TArray<FString> GetMonitoredDirectories() const;

	// Clear all monitored directories
	void ClearAllDirectories();

	// ========================================================================
	// CONFIGURATION
	// ========================================================================

	// Set how long a modification is considered "recent"
	void SetRecentThreshold(float Seconds);

	// Get the recent threshold
	float GetRecentThreshold() const { return RecentModificationThresholdSeconds; }

	// Set file extensions to monitor (e.g., ".cpp", ".h")
	void SetMonitoredExtensions(const TArray<FString>& Extensions);

	// Get monitored extensions
	TArray<FString> GetMonitoredExtensions() const { return MonitoredExtensions; }

	// Add a single extension to monitor
	void AddMonitoredExtension(const FString& Extension);

	// ========================================================================
	// STATE QUERIES
	// ========================================================================

	// Check if any files were modified recently
	bool HasRecentModifications() const;

	// Get the most recent modification time
	FDateTime GetLastModificationTime() const { return LastModificationTime; }

	// Get the most recently modified file path
	FString GetLastModifiedFile() const { return LastModifiedFilePath; }

	// Get count of recent modifications
	int32 GetRecentModificationCount() const;

	// ========================================================================
	// CALLBACKS
	// ========================================================================

	// Set callback for file changes
	void SetOnFileChangeCallback(FOnFileChangeDetected Callback);

private:
	// Directory watcher delegates
	void OnDirectoryChanged(const TArray<FFileChangeData>& FileChanges);

	// Check if a file extension should be monitored
	bool ShouldMonitorFile(const FString& FilePath) const;

	// Check if a file is a source code file
	bool IsSourceFile(const FString& FilePath) const;

	// State
	bool bIsInitialized;
	TArray<FString> MonitoredDirectories;
	TArray<FString> MonitoredExtensions;
	float RecentModificationThresholdSeconds;

	// Recent modification tracking
	struct FRecentModification
	{
		FString FilePath;
		FDateTime Timestamp;
	};
	TArray<FRecentModification> RecentModifications;
	FDateTime LastModificationTime;
	FString LastModifiedFilePath;

	// Callback
	FOnFileChangeDetected OnFileChangeCallback;

	// Directory watcher handles
	TMap<FString, FDelegateHandle> DirectoryWatcherHandles;
};
