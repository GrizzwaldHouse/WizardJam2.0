// ============================================================================
// FileChangeDetector.cpp
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Implementation of file system monitoring for source code changes.
// Uses Unreal's DirectoryWatcher for cross-platform file monitoring.
// ============================================================================

#include "External/FileChangeDetector.h"
#include "DirectoryWatcherModule.h"
#include "IDirectoryWatcher.h"
#include "Misc/Paths.h"

DECLARE_LOG_CATEGORY_EXTERN(LogFileChangeDetector, Log, All);
DEFINE_LOG_CATEGORY(LogFileChangeDetector);

FFileChangeDetector::FFileChangeDetector()
	: bIsInitialized(false)
	, RecentModificationThresholdSeconds(120.0f)
	, LastModificationTime(FDateTime::MinValue())
{
	// Default source file extensions
	MonitoredExtensions = {
		TEXT(".cpp"),
		TEXT(".h"),
		TEXT(".hpp"),
		TEXT(".c"),
		TEXT(".cc"),
		TEXT(".cxx"),
		TEXT(".inl"),
		TEXT(".cs"),
		TEXT(".py"),
		TEXT(".js"),
		TEXT(".ts"),
		TEXT(".usf"),
		TEXT(".ush")
	};
}

FFileChangeDetector::~FFileChangeDetector()
{
	Shutdown();
}

bool FFileChangeDetector::Initialize()
{
	if (bIsInitialized)
	{
		return true;
	}

	// Verify DirectoryWatcher module is available
	FDirectoryWatcherModule& DirectoryWatcherModule = FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));
	IDirectoryWatcher* DirectoryWatcher = DirectoryWatcherModule.Get();

	if (!DirectoryWatcher)
	{
		UE_LOG(LogFileChangeDetector, Error, TEXT("DirectoryWatcher not available"));
		return false;
	}

	bIsInitialized = true;
	UE_LOG(LogFileChangeDetector, Log, TEXT("FileChangeDetector initialized"));

	return true;
}

void FFileChangeDetector::Shutdown()
{
	if (!bIsInitialized)
	{
		return;
	}

	// Unregister all directory watchers
	ClearAllDirectories();

	bIsInitialized = false;
	UE_LOG(LogFileChangeDetector, Log, TEXT("FileChangeDetector shutdown"));
}

void FFileChangeDetector::Update(float DeltaTime)
{
	if (!bIsInitialized)
	{
		return;
	}

	// Clean up old modifications from the recent list
	FDateTime Cutoff = FDateTime::Now() - FTimespan::FromSeconds(RecentModificationThresholdSeconds);

	RecentModifications.RemoveAll([&Cutoff](const FRecentModification& Mod)
	{
		return Mod.Timestamp < Cutoff;
	});
}

bool FFileChangeDetector::AddDirectory(const FString& Directory)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogFileChangeDetector, Warning, TEXT("Cannot add directory - detector not initialized"));
		return false;
	}

	// Check if directory exists
	if (!FPaths::DirectoryExists(Directory))
	{
		UE_LOG(LogFileChangeDetector, Warning, TEXT("Directory does not exist: %s"), *Directory);
		return false;
	}

	// Check if already monitoring
	if (DirectoryWatcherHandles.Contains(Directory))
	{
		UE_LOG(LogFileChangeDetector, Verbose, TEXT("Already monitoring directory: %s"), *Directory);
		return true;
	}

	// Get directory watcher
	FDirectoryWatcherModule& DirectoryWatcherModule = FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));
	IDirectoryWatcher* DirectoryWatcher = DirectoryWatcherModule.Get();

	if (!DirectoryWatcher)
	{
		return false;
	}

	// Create delegate for this directory
	FDelegateHandle Handle;
	IDirectoryWatcher::FDirectoryChanged Delegate = IDirectoryWatcher::FDirectoryChanged::CreateRaw(
		this, &FFileChangeDetector::OnDirectoryChanged);

	if (DirectoryWatcher->RegisterDirectoryChangedCallback_Handle(
		Directory,
		Delegate,
		Handle,
		IDirectoryWatcher::WatchOptions::IncludeDirectoryChanges))
	{
		DirectoryWatcherHandles.Add(Directory, Handle);
		MonitoredDirectories.Add(Directory);

		UE_LOG(LogFileChangeDetector, Log, TEXT("Now monitoring directory: %s"), *Directory);
		return true;
	}

	UE_LOG(LogFileChangeDetector, Warning, TEXT("Failed to register watcher for: %s"), *Directory);
	return false;
}

bool FFileChangeDetector::RemoveMonitoredDirectory(const FString& Directory)
{
	if (!bIsInitialized)
	{
		return false;
	}

	FDelegateHandle* Handle = DirectoryWatcherHandles.Find(Directory);
	if (!Handle)
	{
		return false;
	}

	FDirectoryWatcherModule& DirectoryWatcherModule = FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));
	IDirectoryWatcher* DirectoryWatcher = DirectoryWatcherModule.Get();

	if (DirectoryWatcher)
	{
		DirectoryWatcher->UnregisterDirectoryChangedCallback_Handle(Directory, *Handle);
	}

	DirectoryWatcherHandles.Remove(Directory);
	MonitoredDirectories.Remove(Directory);

	UE_LOG(LogFileChangeDetector, Log, TEXT("Stopped monitoring directory: %s"), *Directory);
	return true;
}

TArray<FString> FFileChangeDetector::GetMonitoredDirectories() const
{
	return MonitoredDirectories;
}

void FFileChangeDetector::ClearAllDirectories()
{
	FDirectoryWatcherModule& DirectoryWatcherModule = FModuleManager::LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));
	IDirectoryWatcher* DirectoryWatcher = DirectoryWatcherModule.Get();

	if (DirectoryWatcher)
	{
		for (const auto& Pair : DirectoryWatcherHandles)
		{
			DirectoryWatcher->UnregisterDirectoryChangedCallback_Handle(Pair.Key, Pair.Value);
		}
	}

	DirectoryWatcherHandles.Empty();
	MonitoredDirectories.Empty();

	UE_LOG(LogFileChangeDetector, Log, TEXT("Cleared all monitored directories"));
}

void FFileChangeDetector::SetRecentThreshold(float Seconds)
{
	RecentModificationThresholdSeconds = FMath::Max(30.0f, Seconds);
}

void FFileChangeDetector::SetMonitoredExtensions(const TArray<FString>& Extensions)
{
	MonitoredExtensions = Extensions;

	// Ensure all extensions start with a dot
	for (FString& Ext : MonitoredExtensions)
	{
		if (!Ext.StartsWith(TEXT(".")))
		{
			Ext = TEXT(".") + Ext;
		}
		Ext = Ext.ToLower();
	}
}

void FFileChangeDetector::AddMonitoredExtension(const FString& Extension)
{
	FString Ext = Extension;
	if (!Ext.StartsWith(TEXT(".")))
	{
		Ext = TEXT(".") + Ext;
	}
	Ext = Ext.ToLower();

	MonitoredExtensions.AddUnique(Ext);
}

bool FFileChangeDetector::HasRecentModifications() const
{
	return RecentModifications.Num() > 0;
}

int32 FFileChangeDetector::GetRecentModificationCount() const
{
	return RecentModifications.Num();
}

void FFileChangeDetector::SetOnFileChangeCallback(FOnFileChangeDetected Callback)
{
	OnFileChangeCallback = Callback;
}

void FFileChangeDetector::OnDirectoryChanged(const TArray<FFileChangeData>& FileChanges)
{
	for (const FFileChangeData& Change : FileChanges)
	{
		// Filter by extension
		if (!ShouldMonitorFile(Change.Filename))
		{
			continue;
		}

		// Create file change event
		FFileChangeEvent Event;
		Event.FilePath = Change.Filename;
		Event.Timestamp = FDateTime::Now();
		Event.bIsSourceFile = IsSourceFile(Change.Filename);

		switch (Change.Action)
		{
		case FFileChangeData::FCA_Added:
			Event.ChangeType = FFileChangeEvent::EChangeType::Added;
			break;
		case FFileChangeData::FCA_Modified:
			Event.ChangeType = FFileChangeEvent::EChangeType::Modified;
			break;
		case FFileChangeData::FCA_Removed:
			Event.ChangeType = FFileChangeEvent::EChangeType::Removed;
			break;
		}

		// Track as recent modification
		FRecentModification RecentMod;
		RecentMod.FilePath = Change.Filename;
		RecentMod.Timestamp = FDateTime::Now();
		RecentModifications.Add(RecentMod);

		// Update last modification tracking
		LastModificationTime = FDateTime::Now();
		LastModifiedFilePath = Change.Filename;

		UE_LOG(LogFileChangeDetector, Verbose, TEXT("File changed: %s (Type: %d)"),
			*Change.Filename, static_cast<int32>(Event.ChangeType));

		// Fire callback
		if (OnFileChangeCallback.IsBound())
		{
			OnFileChangeCallback.Execute(Event);
		}
	}
}

bool FFileChangeDetector::ShouldMonitorFile(const FString& FilePath) const
{
	FString Extension = FPaths::GetExtension(FilePath, true).ToLower();

	for (const FString& MonitoredExt : MonitoredExtensions)
	{
		if (Extension.Equals(MonitoredExt, ESearchCase::IgnoreCase))
		{
			return true;
		}
	}

	return false;
}

bool FFileChangeDetector::IsSourceFile(const FString& FilePath) const
{
	FString Extension = FPaths::GetExtension(FilePath, true).ToLower();

	// Common source file extensions
	static const TArray<FString> SourceExtensions = {
		TEXT(".cpp"), TEXT(".c"), TEXT(".cc"), TEXT(".cxx"),
		TEXT(".h"), TEXT(".hpp"), TEXT(".hh"), TEXT(".hxx"),
		TEXT(".inl"), TEXT(".cs")
	};

	return SourceExtensions.Contains(Extension);
}
