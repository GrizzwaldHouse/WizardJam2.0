// ============================================================================
// SessionTrackingSubsystem.cpp
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Implementation of the session tracking subsystem. Manages work sessions,
// integrates with external monitoring, and handles persistence.
// ============================================================================

#include "Core/SessionTrackingSubsystem.h"
#include "Core/SecureStorageManager.h"
#include "Core/ProductivityTrackerSettings.h"
#include "External/ExternalActivityMonitor.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Editor.h"

DEFINE_LOG_CATEGORY(LogProductivitySession);

// Constants
namespace SessionConstants
{
	const float AutoSaveIntervalSeconds = 60.0f;
	const FString PluginVersion = TEXT("1.0.0");
}

USessionTrackingSubsystem::USessionTrackingSubsystem()
	: bHasActiveSession(false)
	, bIsSessionPaused(false)
	, CurrentActivityState(EActivityState::Away)
	, PreviousActivityState(EActivityState::Away)
	, SnapshotTimer(0.0f)
	, AutoSaveTimer(0.0f)
	, LastInputTime(0.0f)
	, SessionStartRealTime(0.0)
	, PauseStartRealTime(0.0)
	, TotalPausedTime(0.0)
	, StorageManager(nullptr)
	, CachedExternalState(nullptr)
{
}

USessionTrackingSubsystem::~USessionTrackingSubsystem()
{
	// Destructor defined in .cpp to ensure TUniquePtr<IExternalActivityMonitor> has full type
}

// ============================================================================
// USubsystem Interface
// ============================================================================

void USessionTrackingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogProductivitySession, Log, TEXT("SessionTrackingSubsystem initializing..."));

	// Initialize storage
	InitializeStorage();

	// Check for recoverable sessions from crashes
	CheckForRecoverableSession();

	// Initialize external monitoring if enabled
	const UProductivityTrackerSettings* Settings = UProductivityTrackerSettings::Get();
	if (Settings && Settings->bEnableExternalMonitoring)
	{
		InitializeExternalMonitoring();
	}

	// Auto-start session if configured
	if (Settings && Settings->bAutoStartSession)
	{
		StartSession();
	}

	UE_LOG(LogProductivitySession, Log, TEXT("SessionTrackingSubsystem initialized"));
}

void USessionTrackingSubsystem::Deinitialize()
{
	UE_LOG(LogProductivitySession, Log, TEXT("SessionTrackingSubsystem deinitializing..."));

	// End any active session
	if (bHasActiveSession)
	{
		EndSession();
	}

	// Shutdown external monitoring
	ShutdownExternalMonitoring();

	// Shutdown storage
	if (StorageManager)
	{
		StorageManager->Shutdown();
	}

	Super::Deinitialize();
}

// ============================================================================
// FTickableEditorObject Interface
// ============================================================================

void USessionTrackingSubsystem::Tick(float DeltaTime)
{
	if (!bHasActiveSession || bIsSessionPaused)
	{
		return;
	}

	// Update external activity monitor
	if (ExternalActivityMonitor.IsValid())
	{
		ExternalActivityMonitor->Update(DeltaTime);
	}

	// Determine current activity state
	EActivityState NewState = DetermineActivityState();
	if (NewState != CurrentActivityState)
	{
		PreviousActivityState = CurrentActivityState;
		CurrentActivityState = NewState;

		UE_LOG(LogProductivitySession, Verbose, TEXT("Activity state changed: %s -> %s"),
			*UEnum::GetValueAsString(PreviousActivityState),
			*UEnum::GetValueAsString(CurrentActivityState));

		OnActivityStateChanged.Broadcast(CurrentActivityState);
	}

	// Update activity summary
	UpdateActivitySummary(DeltaTime, CurrentActivityState);

	// Snapshot timer
	const UProductivityTrackerSettings* Settings = UProductivityTrackerSettings::Get();
	float SnapshotInterval = Settings ? Settings->SnapshotIntervalSeconds : 30.0f;

	SnapshotTimer += DeltaTime;
	if (SnapshotTimer >= SnapshotInterval)
	{
		CaptureActivitySnapshot();
		SnapshotTimer = 0.0f;
	}

	// Auto-save timer
	AutoSaveTimer += DeltaTime;
	if (AutoSaveTimer >= SessionConstants::AutoSaveIntervalSeconds)
	{
		SaveActiveSessionState();
		AutoSaveTimer = 0.0f;
	}

	// Broadcast tick event
	float ElapsedSeconds = GetElapsedSeconds();
	float ProductiveSeconds = GetProductiveSeconds();
	OnSessionTick.Broadcast(ElapsedSeconds, ProductiveSeconds);
}

TStatId USessionTrackingSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USessionTrackingSubsystem, STATGROUP_Tickables);
}

ETickableTickType USessionTrackingSubsystem::GetTickableTickType() const
{
	return ETickableTickType::Always;
}

// ============================================================================
// SESSION CONTROL
// ============================================================================

bool USessionTrackingSubsystem::StartSession()
{
	if (bHasActiveSession)
	{
		UE_LOG(LogProductivitySession, Warning, TEXT("Cannot start session - session already active"));
		return false;
	}

	// Create new session record
	CurrentSession = FSessionRecord();
	CurrentSession.StartTime = FDateTime::Now();
	CurrentSession.MachineId = MachineIdentifier;
	CurrentSession.PluginVersion = SessionConstants::PluginVersion;

	// Reset timing
	SessionStartRealTime = FApp::GetCurrentTime();
	TotalPausedTime = 0.0;
	SnapshotTimer = 0.0f;
	AutoSaveTimer = 0.0f;

	// Reset activity state
	CurrentActivityState = EActivityState::Active;
	PreviousActivityState = EActivityState::Active;

	bHasActiveSession = true;
	bIsSessionPaused = false;

	// Save initial state for crash recovery
	SaveActiveSessionState();

	UE_LOG(LogProductivitySession, Log, TEXT("Session started: %s"), *CurrentSession.SessionId.ToString());

	OnSessionStarted.Broadcast();

	return true;
}

bool USessionTrackingSubsystem::EndSession()
{
	if (!bHasActiveSession)
	{
		UE_LOG(LogProductivitySession, Warning, TEXT("Cannot end session - no active session"));
		return false;
	}

	// Finalize and save the session
	FinalizeAndSaveSession();

	// Broadcast completion
	OnSessionEnded.Broadcast(CurrentSession);

	// Clear state
	bHasActiveSession = false;
	bIsSessionPaused = false;

	// Clear recovery file
	if (StorageManager)
	{
		StorageManager->ClearActiveSessionState();
	}

	UE_LOG(LogProductivitySession, Log, TEXT("Session ended: %s (%.1f seconds)"),
		*CurrentSession.SessionId.ToString(), CurrentSession.TotalElapsedSeconds);

	return true;
}

void USessionTrackingSubsystem::PauseSession()
{
	if (!bHasActiveSession || bIsSessionPaused)
	{
		return;
	}

	bIsSessionPaused = true;
	PauseStartRealTime = FApp::GetCurrentTime();
	CurrentActivityState = EActivityState::Paused;

	UE_LOG(LogProductivitySession, Log, TEXT("Session paused"));

	OnActivityStateChanged.Broadcast(CurrentActivityState);
}

void USessionTrackingSubsystem::ResumeSession()
{
	if (!bHasActiveSession || !bIsSessionPaused)
	{
		return;
	}

	// Track total paused time
	double PauseDuration = FApp::GetCurrentTime() - PauseStartRealTime;
	TotalPausedTime += PauseDuration;

	bIsSessionPaused = false;
	CurrentActivityState = EActivityState::Active;

	UE_LOG(LogProductivitySession, Log, TEXT("Session resumed (paused %.1f seconds)"), PauseDuration);

	OnActivityStateChanged.Broadcast(CurrentActivityState);
}

void USessionTrackingSubsystem::ToggleSession()
{
	if (!bHasActiveSession)
	{
		StartSession();
	}
	else if (bIsSessionPaused)
	{
		ResumeSession();
	}
	else
	{
		PauseSession();
	}
}

// ============================================================================
// SESSION QUERIES
// ============================================================================

float USessionTrackingSubsystem::GetElapsedSeconds() const
{
	if (!bHasActiveSession)
	{
		return 0.0f;
	}

	double CurrentTime = FApp::GetCurrentTime();
	double TotalElapsed = CurrentTime - SessionStartRealTime;

	// Subtract paused time
	if (bIsSessionPaused)
	{
		TotalElapsed -= (CurrentTime - PauseStartRealTime);
	}
	TotalElapsed -= TotalPausedTime;

	return static_cast<float>(FMath::Max(0.0, TotalElapsed));
}

float USessionTrackingSubsystem::GetProductiveSeconds() const
{
	if (!bHasActiveSession)
	{
		return 0.0f;
	}

	return CurrentSession.ActivitySummary.GetProductiveSeconds();
}

FString USessionTrackingSubsystem::GetFormattedElapsedTime() const
{
	float TotalSeconds = GetElapsedSeconds();
	int32 Hours = FMath::FloorToInt(TotalSeconds / 3600.0f);
	int32 Minutes = FMath::FloorToInt(FMath::Fmod(TotalSeconds, 3600.0f) / 60.0f);
	int32 Seconds = FMath::FloorToInt(FMath::Fmod(TotalSeconds, 60.0f));

	return FString::Printf(TEXT("%02d:%02d:%02d"), Hours, Minutes, Seconds);
}

FString USessionTrackingSubsystem::GetActivityStateDisplayString() const
{
	switch (CurrentActivityState)
	{
	case EActivityState::Active:
		return TEXT("Active");
	case EActivityState::Thinking:
		return TEXT("Thinking");
	case EActivityState::Away:
		return TEXT("Away");
	case EActivityState::Paused:
		return TEXT("Paused");
	default:
		return TEXT("Unknown");
	}
}

// ============================================================================
// TASK LINKING
// ============================================================================

void USessionTrackingSubsystem::LinkToTask(const FString& TaskId)
{
	if (bHasActiveSession)
	{
		CurrentSession.LinkedTaskId = TaskId;
		UE_LOG(LogProductivitySession, Log, TEXT("Session linked to task: %s"), *TaskId);
	}
}

FString USessionTrackingSubsystem::GetLinkedTaskId() const
{
	return CurrentSession.LinkedTaskId;
}

void USessionTrackingSubsystem::ClearTaskLink()
{
	CurrentSession.LinkedTaskId.Empty();
}

// ============================================================================
// HISTORY QUERIES
// ============================================================================

TArray<FSessionRecord> USessionTrackingSubsystem::GetRecentSessions(int32 DayCount)
{
	if (!StorageManager)
	{
		return TArray<FSessionRecord>();
	}

	FDateTime EndDate = FDateTime::Now();
	FDateTime StartDate = EndDate - FTimespan::FromDays(DayCount);

	return StorageManager->LoadSessionsInRange(StartDate, EndDate);
}

bool USessionTrackingSubsystem::GetDailySummary(const FDateTime& Date, FDailySummary& OutSummary)
{
	if (!StorageManager)
	{
		return false;
	}

	return StorageManager->LoadDailySummary(Date, OutSummary);
}

float USessionTrackingSubsystem::GetTodayTotalSeconds() const
{
	if (!StorageManager)
	{
		return GetElapsedSeconds();
	}

	FDateTime Today = FDateTime::Today();
	FDailySummary Summary;

	float TodaySeconds = 0.0f;
	if (StorageManager->LoadDailySummary(Today, Summary))
	{
		TodaySeconds = Summary.AggregatedSummary.TotalSeconds;
	}

	// Add current session if active
	if (bHasActiveSession)
	{
		TodaySeconds += GetElapsedSeconds();
	}

	return TodaySeconds;
}

// ============================================================================
// EXTERNAL ACTIVITY
// ============================================================================

bool USessionTrackingSubsystem::IsExternallyProductive() const
{
	if (ExternalActivityMonitor.IsValid())
	{
		return ExternalActivityMonitor->GetCurrentState().IsExternallyProductive();
	}
	return false;
}

FString USessionTrackingSubsystem::GetFocusedExternalApp() const
{
	if (ExternalActivityMonitor.IsValid())
	{
		return ExternalActivityMonitor->GetCurrentState().FocusedAppName;
	}
	return FString();
}

// ============================================================================
// PRIVATE METHODS
// ============================================================================

EActivityState USessionTrackingSubsystem::DetermineActivityState() const
{
	const UProductivityTrackerSettings* Settings = UProductivityTrackerSettings::Get();
	float ThinkingThreshold = Settings ? Settings->ThinkingThresholdSeconds : 120.0f;
	float AwayThreshold = Settings ? Settings->AwayThresholdSeconds : 300.0f;

	float SecondsSinceInput = GetSecondsSinceLastInput();

	// Check for away status first
	if (SecondsSinceInput > AwayThreshold)
	{
		return EActivityState::Away;
	}

	// Check if externally productive (coding in IDE)
	if (ExternalActivityMonitor.IsValid())
	{
		const FExternalActivityState& ExternalState = ExternalActivityMonitor->GetCurrentState();
		if (ExternalState.IsExternallyProductive())
		{
			return EActivityState::Active;
		}
	}

	// Check editor focus
	if (IsEditorFocused())
	{
		if (SecondsSinceInput < ThinkingThreshold)
		{
			return EActivityState::Active;
		}
		return EActivityState::Thinking;
	}

	// Not focused on editor and not externally productive
	if (SecondsSinceInput > ThinkingThreshold)
	{
		return EActivityState::Away;
	}

	return EActivityState::Thinking;
}

float USessionTrackingSubsystem::CalculateProductivityWeight() const
{
	// Base weight based on activity state
	float Weight = 1.0f;

	switch (CurrentActivityState)
	{
	case EActivityState::Active:
		Weight = 1.0f;
		break;
	case EActivityState::Thinking:
		Weight = 0.75f;
		break;
	case EActivityState::Away:
	case EActivityState::Paused:
		Weight = 0.0f;
		break;
	}

	// Modify based on external app if focused
	if (ExternalActivityMonitor.IsValid())
	{
		const FExternalActivityState& ExternalState = ExternalActivityMonitor->GetCurrentState();
		if (ExternalState.bDevelopmentAppFocused)
		{
			Weight *= ExternalState.FocusedAppProductivityWeight;
		}
	}

	return FMath::Clamp(Weight, 0.0f, 1.0f);
}

void USessionTrackingSubsystem::CaptureActivitySnapshot()
{
	FActivitySnapshot Snapshot;
	Snapshot.Timestamp = FDateTime::Now();
	Snapshot.State = CurrentActivityState;
	Snapshot.SecondsSinceLastInput = GetSecondsSinceLastInput();
	Snapshot.bEditorFocused = IsEditorFocused();
	Snapshot.bPlayInEditorActive = IsPlayInEditorActive();
	Snapshot.ProductivityWeight = CalculateProductivityWeight();

	// External activity data
	if (ExternalActivityMonitor.IsValid())
	{
		const FExternalActivityState& ExternalState = ExternalActivityMonitor->GetCurrentState();
		Snapshot.bExternalAppFocused = ExternalState.bDevelopmentAppFocused;
		Snapshot.FocusedExternalApp = ExternalState.FocusedAppName;
		Snapshot.bSourceFilesModified = ExternalState.bSourceFilesModifiedRecently;
	}

	// Calculate and store checksum
	Snapshot.SnapshotChecksum = Snapshot.CalculateChecksum(InstallationSalt);

	CurrentSession.ActivitySnapshots.Add(Snapshot);

	UE_LOG(LogProductivitySession, Verbose, TEXT("Captured snapshot - State: %s, Weight: %.2f"),
		*UEnum::GetValueAsString(Snapshot.State), Snapshot.ProductivityWeight);
}

void USessionTrackingSubsystem::UpdateActivitySummary(float DeltaTime, EActivityState State)
{
	CurrentSession.ActivitySummary.AddTimeForState(State, DeltaTime);

	// Track time by external application
	if (ExternalActivityMonitor.IsValid())
	{
		const FExternalActivityState& ExternalState = ExternalActivityMonitor->GetCurrentState();
		if (ExternalState.bDevelopmentAppFocused && !ExternalState.FocusedAppName.IsEmpty())
		{
			const UProductivityTrackerSettings* Settings = UProductivityTrackerSettings::Get();
			if (Settings && Settings->bStoreApplicationNames)
			{
				CurrentSession.ActivitySummary.AddTimeForApplication(ExternalState.FocusedAppName, DeltaTime);
			}
		}
	}
}

void USessionTrackingSubsystem::HandleExternalActivityChanged(const FExternalActivityState& NewState)
{
	UE_LOG(LogProductivitySession, Verbose, TEXT("External activity changed - App: %s, Productive: %s"),
		*NewState.FocusedAppName,
		NewState.IsExternallyProductive() ? TEXT("Yes") : TEXT("No"));
}

void USessionTrackingSubsystem::HandleSourceFileChanged(const FFileChangeEvent& FileEvent)
{
	UE_LOG(LogProductivitySession, Verbose, TEXT("Source file changed: %s"), *FileEvent.FilePath);
}

void USessionTrackingSubsystem::InitializeStorage()
{
	// Create storage manager
	StorageManager = NewObject<USecureStorageManager>(this);

	// Determine data directory
	FString DataDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ProductivityTracker"));

	if (StorageManager->Initialize(DataDir))
	{
		InstallationSalt = StorageManager->GetInstallationSalt();
		MachineIdentifier = StorageManager->GenerateMachineIdentifier();

		UE_LOG(LogProductivitySession, Log, TEXT("Storage initialized at: %s"), *DataDir);
	}
	else
	{
		UE_LOG(LogProductivitySession, Error, TEXT("Failed to initialize storage"));
	}
}

void USessionTrackingSubsystem::CheckForRecoverableSession()
{
	if (!StorageManager || !StorageManager->HasRecoverableSession())
	{
		return;
	}

	const UProductivityTrackerSettings* Settings = UProductivityTrackerSettings::Get();
	if (!Settings || !Settings->bAutoRecoverSessions)
	{
		StorageManager->ClearActiveSessionState();
		return;
	}

	FSessionRecord RecoveredSession;
	if (StorageManager->LoadActiveSessionState(RecoveredSession))
	{
		// Finalize the recovered session
		RecoveredSession.EndTime = RecoveredSession.ActivitySnapshots.Num() > 0
			? RecoveredSession.ActivitySnapshots.Last().Timestamp
			: FDateTime::Now();
		RecoveredSession.bWasRecovered = true;
		RecoveredSession.Finalize(InstallationSalt);

		// Save the recovered session
		StorageManager->SaveSession(RecoveredSession);
		StorageManager->ClearActiveSessionState();

		UE_LOG(LogProductivitySession, Log, TEXT("Recovered session %s from crash"),
			*RecoveredSession.SessionId.ToString());

		OnSessionRecovered.Broadcast(RecoveredSession);
	}
}

void USessionTrackingSubsystem::SaveActiveSessionState()
{
	if (!bHasActiveSession || !StorageManager)
	{
		return;
	}

	// Update elapsed time before saving
	CurrentSession.TotalElapsedSeconds = GetElapsedSeconds();

	StorageManager->SaveActiveSessionState(CurrentSession);
}

void USessionTrackingSubsystem::FinalizeAndSaveSession()
{
	if (!StorageManager)
	{
		return;
	}

	// Set end time and finalize
	CurrentSession.EndTime = FDateTime::Now();
	CurrentSession.TotalElapsedSeconds = GetElapsedSeconds();
	CurrentSession.Finalize(InstallationSalt);

	// Save to permanent storage
	StorageManager->SaveSession(CurrentSession);

	// Update daily summary
	FDateTime Today = FDateTime::Today();
	FDailySummary Summary;
	StorageManager->LoadDailySummary(Today, Summary);
	Summary.Date = Today;
	Summary.AddSession(CurrentSession);
	StorageManager->SaveDailySummary(Summary);
}

float USessionTrackingSubsystem::GetSecondsSinceLastInput() const
{
	// Use Slate application for input timing
	return FSlateApplication::IsInitialized()
		? static_cast<float>(FSlateApplication::Get().GetLastUserInteractionTime())
		: 0.0f;
}

bool USessionTrackingSubsystem::IsEditorFocused() const
{
	return FSlateApplication::IsInitialized()
		&& FSlateApplication::Get().IsActive();
}

bool USessionTrackingSubsystem::IsPlayInEditorActive() const
{
	return GEditor && GEditor->IsPlayingSessionInEditor();
}

void USessionTrackingSubsystem::InitializeExternalMonitoring()
{
	ExternalActivityMonitor = IExternalActivityMonitor::Create();

	if (ExternalActivityMonitor.IsValid())
	{
		if (ExternalActivityMonitor->Initialize())
		{
			// Set up callbacks
			ExternalActivityMonitor->SetOnActivityChangedCallback(
				FOnExternalActivityChanged::CreateUObject(this, &USessionTrackingSubsystem::HandleExternalActivityChanged));
			ExternalActivityMonitor->SetOnSourceFileChangedCallback(
				FOnSourceFileChanged::CreateUObject(this, &USessionTrackingSubsystem::HandleSourceFileChanged));

			// Configure file monitoring
			const UProductivityTrackerSettings* Settings = UProductivityTrackerSettings::Get();
			if (Settings && Settings->bEnableFileMonitoring)
			{
				FString SourceDir = FPaths::Combine(FPaths::ProjectDir(), TEXT("Source"));
				ExternalActivityMonitor->SetSourceDirectory(SourceDir);
				ExternalActivityMonitor->SetFileMonitoringEnabled(true);
			}

			UE_LOG(LogProductivitySession, Log, TEXT("External activity monitoring initialized"));
		}
		else
		{
			UE_LOG(LogProductivitySession, Warning, TEXT("Failed to initialize external activity monitor"));
			ExternalActivityMonitor.Reset();
		}
	}
}

void USessionTrackingSubsystem::ShutdownExternalMonitoring()
{
	if (ExternalActivityMonitor.IsValid())
	{
		ExternalActivityMonitor->Shutdown();
		ExternalActivityMonitor.Reset();
	}
}
