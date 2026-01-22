// ============================================================================
// ExternalActivityMonitor.h
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Interface for platform-specific external activity monitoring.
// Detects when developer is working in Visual Studio, VS Code, etc.
// Also monitors source file changes to catch coding activity.
//
// Detection Methods:
// 1. Window Focus - Which application is the user actively using?
// 2. Process Detection - Which development tools are running?
// 3. File System Monitoring - Are source files being modified?
// 4. Build Detection - Did Unreal just compile code?
//
// Architecture:
// Abstract interface with platform-specific implementations.
// Uses factory pattern for runtime platform selection.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "KnownApplications.h"

// File change event data
struct DEVELOPERPRODUCTIVITYTRACKER_API FFileChangeEvent
{
	// Full path to the changed file
	FString FilePath;

	// When the change was detected
	FDateTime Timestamp;

	// Is this a source code file?
	bool bIsSourceFile;

	// Type of change (Added, Modified, Removed)
	enum class EChangeType : uint8
	{
		Added,
		Modified,
		Removed
	};
	EChangeType ChangeType;

	FFileChangeEvent()
		: Timestamp(FDateTime::MinValue())
		, bIsSourceFile(false)
		, ChangeType(EChangeType::Modified)
	{
	}

	FFileChangeEvent(const FString& InPath, bool InIsSource)
		: FilePath(InPath)
		, Timestamp(FDateTime::Now())
		, bIsSourceFile(InIsSource)
		, ChangeType(EChangeType::Modified)
	{
	}
};

// Delegate types for activity callbacks
DECLARE_DELEGATE_OneParam(FOnExternalActivityChanged, const FExternalActivityState&);
DECLARE_DELEGATE_OneParam(FOnSourceFileChanged, const FFileChangeEvent&);

// Abstract interface for external activity monitoring
class DEVELOPERPRODUCTIVITYTRACKER_API IExternalActivityMonitor
{
public:
	virtual ~IExternalActivityMonitor() = default;

	// ========================================================================
	// LIFECYCLE
	// ========================================================================

	// Initialize the monitor and start background operations
	virtual bool Initialize() = 0;

	// Shutdown the monitor and cleanup resources
	virtual void Shutdown() = 0;

	// Update the monitor (call each frame or at regular intervals)
	virtual void Update(float DeltaTime) = 0;

	// ========================================================================
	// STATE QUERIES
	// ========================================================================

	// Get the current external activity state
	virtual FExternalActivityState GetCurrentState() const = 0;

	// Check if the monitor is running
	virtual bool IsRunning() const = 0;

	// ========================================================================
	// CALLBACKS
	// ========================================================================

	// Set callback for activity state changes
	virtual void SetOnActivityChangedCallback(FOnExternalActivityChanged Callback) = 0;

	// Set callback for source file changes
	virtual void SetOnSourceFileChangedCallback(FOnSourceFileChanged Callback) = 0;

	// ========================================================================
	// APPLICATION MANAGEMENT
	// ========================================================================

	// Add a new known application to monitor
	virtual void AddKnownApplication(const FKnownApplication& App) = 0;

	// Remove a known application by display name
	virtual void RemoveKnownApplication(const FString& DisplayName) = 0;

	// Get all currently configured known applications
	virtual TArray<FKnownApplication> GetKnownApplications() const = 0;

	// Reset to default application list
	virtual void ResetToDefaultApplications() = 0;

	// ========================================================================
	// FILE MONITORING
	// ========================================================================

	// Set the primary source directory to monitor
	virtual void SetSourceDirectory(const FString& Directory) = 0;

	// Add an additional directory to monitor
	virtual void AddSourceDirectory(const FString& Directory) = 0;

	// Remove a monitored directory
	virtual void RemoveSourceDirectory(const FString& Directory) = 0;

	// Get all monitored directories
	virtual TArray<FString> GetMonitoredDirectories() const = 0;

	// Enable or disable file monitoring
	virtual void SetFileMonitoringEnabled(bool bEnabled) = 0;

	// Check if file monitoring is enabled
	virtual bool IsFileMonitoringEnabled() const = 0;

	// ========================================================================
	// CONFIGURATION
	// ========================================================================

	// Set the interval between process scans (in seconds)
	virtual void SetProcessScanInterval(float Seconds) = 0;

	// Set how long a file change is considered "recent" (in seconds)
	virtual void SetRecentModificationThreshold(float Seconds) = 0;

	// ========================================================================
	// FACTORY METHOD
	// ========================================================================

	// Create the appropriate platform-specific implementation
	static TUniquePtr<IExternalActivityMonitor> Create();
};
