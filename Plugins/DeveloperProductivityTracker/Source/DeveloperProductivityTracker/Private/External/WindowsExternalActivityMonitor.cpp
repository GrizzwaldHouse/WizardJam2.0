// ============================================================================
// WindowsExternalActivityMonitor.cpp
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Windows-specific implementation of external activity monitoring.
// Uses Windows API for process enumeration and window focus detection.
//
// Architecture:
// Implements IExternalActivityMonitor interface.
// Uses Win32 API: EnumProcesses, GetForegroundWindow, etc.
// ============================================================================

#include "External/ExternalActivityMonitor.h"
#include "External/FileChangeDetector.h"
#include "Core/ProductivityTrackerSettings.h"

#if PLATFORM_WINDOWS

#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include <Psapi.h>
#include <TlHelp32.h>
#include "Windows/HideWindowsPlatformTypes.h"

DECLARE_LOG_CATEGORY_EXTERN(LogExternalMonitor, Log, All);
DEFINE_LOG_CATEGORY(LogExternalMonitor);

// Windows implementation of the external activity monitor
class FWindowsExternalActivityMonitor : public IExternalActivityMonitor
{
public:
	FWindowsExternalActivityMonitor()
		: bIsRunning(false)
		, bFileMonitoringEnabled(false)
		, ProcessScanInterval(5.0f)
		, RecentModificationThreshold(120.0f)
		, ProcessScanTimer(0.0f)
	{
	}

	virtual ~FWindowsExternalActivityMonitor()
	{
		Shutdown();
	}

	// ========================================================================
	// IExternalActivityMonitor Interface
	// ========================================================================

	virtual bool Initialize() override
	{
		if (bIsRunning)
		{
			return true;
		}

		// Load default applications
		KnownApplications = FKnownApplicationsFactory::GetDefaultApplications();

		// Build process name lookup map for fast matching
		RebuildProcessLookup();

		// Initialize file change detector
		FileChangeDetector = MakeUnique<FFileChangeDetector>();
		if (FileChangeDetector->Initialize())
		{
			FileChangeDetector->SetOnFileChangeCallback(
				FOnFileChangeDetected::CreateRaw(this, &FWindowsExternalActivityMonitor::HandleFileChange));

			// Set default source extensions
			FileChangeDetector->SetMonitoredExtensions({
				TEXT(".cpp"), TEXT(".h"), TEXT(".hpp"), TEXT(".c"),
				TEXT(".cs"), TEXT(".inl"), TEXT(".generated.h")
			});
		}

		// Load settings
		const UProductivityTrackerSettings* Settings = UProductivityTrackerSettings::Get();
		if (Settings)
		{
			ProcessScanInterval = Settings->ProcessScanIntervalSeconds;
			RecentModificationThreshold = Settings->RecentModificationThresholdSeconds;
		}

		bIsRunning = true;
		ProcessScanTimer = ProcessScanInterval; // Scan immediately on first update

		UE_LOG(LogExternalMonitor, Log, TEXT("Windows External Activity Monitor initialized with %d known applications"),
			KnownApplications.Num());

		return true;
	}

	virtual void Shutdown() override
	{
		if (!bIsRunning)
		{
			return;
		}

		if (FileChangeDetector.IsValid())
		{
			FileChangeDetector->Shutdown();
			FileChangeDetector.Reset();
		}

		bIsRunning = false;
		UE_LOG(LogExternalMonitor, Log, TEXT("Windows External Activity Monitor shutdown"));
	}

	virtual void Update(float DeltaTime) override
	{
		if (!bIsRunning)
		{
			return;
		}

		// Update file change detector
		if (FileChangeDetector.IsValid() && bFileMonitoringEnabled)
		{
			FileChangeDetector->Update(DeltaTime);
		}

		// Periodic process scan
		ProcessScanTimer += DeltaTime;
		if (ProcessScanTimer >= ProcessScanInterval)
		{
			ProcessScanTimer = 0.0f;
			ScanRunningProcesses();
		}

		// Always update focused window (fast operation)
		UpdateFocusedWindow();

		// Update recent modification status
		UpdateRecentModificationStatus();

		CurrentState.LastUpdateTime = FDateTime::Now();

		// Check if state changed significantly
		if (HasStateChanged())
		{
			PreviousState = CurrentState;
			if (OnActivityChanged.IsBound())
			{
				OnActivityChanged.Execute(CurrentState);
			}
		}
	}

	virtual FExternalActivityState GetCurrentState() const override
	{
		return CurrentState;
	}

	virtual bool IsRunning() const override
	{
		return bIsRunning;
	}

	virtual void SetOnActivityChangedCallback(FOnExternalActivityChanged Callback) override
	{
		OnActivityChanged = Callback;
	}

	virtual void SetOnSourceFileChangedCallback(FOnSourceFileChanged Callback) override
	{
		OnSourceFileChanged = Callback;
	}

	virtual void AddKnownApplication(const FKnownApplication& App) override
	{
		KnownApplications.Add(App);
		RebuildProcessLookup();
		UE_LOG(LogExternalMonitor, Log, TEXT("Added known application: %s"), *App.DisplayName);
	}

	virtual void RemoveKnownApplication(const FString& DisplayName) override
	{
		KnownApplications.RemoveAll([&DisplayName](const FKnownApplication& App)
		{
			return App.DisplayName.Equals(DisplayName, ESearchCase::IgnoreCase);
		});
		RebuildProcessLookup();
		UE_LOG(LogExternalMonitor, Log, TEXT("Removed known application: %s"), *DisplayName);
	}

	virtual TArray<FKnownApplication> GetKnownApplications() const override
	{
		return KnownApplications;
	}

	virtual void ResetToDefaultApplications() override
	{
		KnownApplications = FKnownApplicationsFactory::GetDefaultApplications();
		RebuildProcessLookup();
	}

	virtual void SetSourceDirectory(const FString& Directory) override
	{
		if (FileChangeDetector.IsValid())
		{
			FileChangeDetector->ClearAllDirectories();
			FileChangeDetector->AddDirectory(Directory);
			UE_LOG(LogExternalMonitor, Log, TEXT("Set source directory: %s"), *Directory);
		}
	}

	virtual void AddSourceDirectory(const FString& Directory) override
	{
		if (FileChangeDetector.IsValid())
		{
			FileChangeDetector->AddDirectory(Directory);
		}
	}

	virtual void RemoveSourceDirectory(const FString& Directory) override
	{
		if (FileChangeDetector.IsValid())
		{
			FileChangeDetector->RemoveMonitoredDirectory(Directory);
		}
	}

	virtual TArray<FString> GetMonitoredDirectories() const override
	{
		if (FileChangeDetector.IsValid())
		{
			return FileChangeDetector->GetMonitoredDirectories();
		}
		return TArray<FString>();
	}

	virtual void SetFileMonitoringEnabled(bool bEnabled) override
	{
		bFileMonitoringEnabled = bEnabled;
	}

	virtual bool IsFileMonitoringEnabled() const override
	{
		return bFileMonitoringEnabled;
	}

	virtual void SetProcessScanInterval(float Seconds) override
	{
		ProcessScanInterval = FMath::Max(1.0f, Seconds);
	}

	virtual void SetRecentModificationThreshold(float Seconds) override
	{
		RecentModificationThreshold = FMath::Max(30.0f, Seconds);
		if (FileChangeDetector.IsValid())
		{
			FileChangeDetector->SetRecentThreshold(RecentModificationThreshold);
		}
	}

private:
	// State
	bool bIsRunning;
	bool bFileMonitoringEnabled;
	float ProcessScanInterval;
	float RecentModificationThreshold;
	float ProcessScanTimer;

	FExternalActivityState CurrentState;
	FExternalActivityState PreviousState;

	// Known applications
	TArray<FKnownApplication> KnownApplications;
	TMap<FString, const FKnownApplication*> ProcessNameLookup;

	// File monitoring
	TUniquePtr<FFileChangeDetector> FileChangeDetector;

	// Callbacks
	FOnExternalActivityChanged OnActivityChanged;
	FOnSourceFileChanged OnSourceFileChanged;

	// ========================================================================
	// Private Methods
	// ========================================================================

	void RebuildProcessLookup()
	{
		ProcessNameLookup.Empty();
		for (const FKnownApplication& App : KnownApplications)
		{
			for (const FString& ProcessName : App.ProcessNames)
			{
				ProcessNameLookup.Add(ProcessName.ToLower(), &App);
			}
		}
	}

	void ScanRunningProcesses()
	{
		CurrentState.RunningDevApps.Empty();

		// Get snapshot of all processes
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnapshot == INVALID_HANDLE_VALUE)
		{
			UE_LOG(LogExternalMonitor, Warning, TEXT("Failed to create process snapshot"));
			return;
		}

		PROCESSENTRY32 ProcessEntry;
		ProcessEntry.dwSize = sizeof(PROCESSENTRY32);

		if (Process32First(hSnapshot, &ProcessEntry))
		{
			do
			{
				FString ProcessName = FString(ProcessEntry.szExeFile).ToLower();

				if (const FKnownApplication** FoundApp = ProcessNameLookup.Find(ProcessName))
				{
					CurrentState.RunningDevApps.AddUnique((*FoundApp)->DisplayName);
				}
			} while (Process32Next(hSnapshot, &ProcessEntry));
		}

		CloseHandle(hSnapshot);

		UE_LOG(LogExternalMonitor, Verbose, TEXT("Found %d running development apps"),
			CurrentState.RunningDevApps.Num());
	}

	void UpdateFocusedWindow()
	{
		HWND ForegroundWindow = GetForegroundWindow();
		if (!ForegroundWindow)
		{
			CurrentState.bDevelopmentAppFocused = false;
			CurrentState.FocusedAppName.Empty();
			CurrentState.FocusedAppCategory = EExternalAppCategory::Unknown;
			CurrentState.bFocusedAppIsProductive = false;
			CurrentState.FocusedAppProductivityWeight = 0.0f;
			return;
		}

		// Get the process ID of the foreground window
		DWORD ProcessId = 0;
		GetWindowThreadProcessId(ForegroundWindow, &ProcessId);

		if (ProcessId == 0)
		{
			return;
		}

		// Get the process name
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, 0, ProcessId);
		if (!hProcess)
		{
			return;
		}

		TCHAR ProcessPath[MAX_PATH];
		DWORD PathSize = MAX_PATH;

		FString ProcessName;
		if (QueryFullProcessImageName(hProcess, 0, ProcessPath, &PathSize))
		{
			ProcessName = FPaths::GetCleanFilename(FString(ProcessPath)).ToLower();
		}

		CloseHandle(hProcess);

		// Look up the process
		if (const FKnownApplication** FoundApp = ProcessNameLookup.Find(ProcessName))
		{
			CurrentState.bDevelopmentAppFocused = true;
			CurrentState.FocusedAppName = (*FoundApp)->DisplayName;
			CurrentState.FocusedAppCategory = (*FoundApp)->Category;
			CurrentState.bFocusedAppIsProductive = (*FoundApp)->bIsProductiveApp;
			CurrentState.FocusedAppProductivityWeight = (*FoundApp)->ProductivityWeight;
			CurrentState.SecondsSinceExternalActivity = 0.0f;
		}
		else
		{
			CurrentState.bDevelopmentAppFocused = false;
			CurrentState.FocusedAppName.Empty();
			CurrentState.FocusedAppCategory = EExternalAppCategory::Unknown;
			CurrentState.bFocusedAppIsProductive = false;
			CurrentState.FocusedAppProductivityWeight = 0.0f;
		}
	}

	void UpdateRecentModificationStatus()
	{
		if (!FileChangeDetector.IsValid() || !bFileMonitoringEnabled)
		{
			CurrentState.bSourceFilesModifiedRecently = false;
			return;
		}

		CurrentState.bSourceFilesModifiedRecently = FileChangeDetector->HasRecentModifications();
		CurrentState.LastModifiedSourceFile = FileChangeDetector->GetLastModifiedFile();
		CurrentState.LastSourceModificationTime = FileChangeDetector->GetLastModificationTime();
	}

	void HandleFileChange(const FFileChangeEvent& Event)
	{
		UE_LOG(LogExternalMonitor, Verbose, TEXT("File change detected: %s"), *Event.FilePath);

		if (OnSourceFileChanged.IsBound())
		{
			OnSourceFileChanged.Execute(Event);
		}
	}

	bool HasStateChanged() const
	{
		// Check for significant state changes
		if (CurrentState.bDevelopmentAppFocused != PreviousState.bDevelopmentAppFocused)
		{
			return true;
		}
		if (CurrentState.FocusedAppName != PreviousState.FocusedAppName)
		{
			return true;
		}
		if (CurrentState.bSourceFilesModifiedRecently != PreviousState.bSourceFilesModifiedRecently)
		{
			return true;
		}
		return false;
	}
};

// Factory function
TUniquePtr<IExternalActivityMonitor> CreateWindowsExternalActivityMonitor()
{
	return MakeUnique<FWindowsExternalActivityMonitor>();
}

#endif // PLATFORM_WINDOWS
