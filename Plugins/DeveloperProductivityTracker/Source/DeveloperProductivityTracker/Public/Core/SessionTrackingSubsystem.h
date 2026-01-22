// ============================================================================
// SessionTrackingSubsystem.h
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Editor Subsystem that tracks developer work sessions.
// Persists across level transitions and editor restarts.
// Integrates with External Activity Monitor for comprehensive tracking.
//
// Architecture:
// UEditorSubsystem for editor-lifetime persistence.
// FTickableEditorObject for frame-rate independent updates.
// Observer pattern via delegates for loose coupling with other systems.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "Tickable.h"
#include "SessionData.h"
#include "External/ExternalActivityMonitor.h"
#include "SessionTrackingSubsystem.generated.h"

// Forward declarations (IExternalActivityMonitor fully included above)
class USecureStorageManager;
struct FExternalActivityState;
struct FFileChangeEvent;

// Log category
DECLARE_LOG_CATEGORY_EXTERN(LogProductivitySession, Log, All);

// Delegate declarations
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSessionStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionEnded, const FSessionRecord&, CompletedSession);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActivityStateChanged, EActivityState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSessionTick, float, ElapsedSeconds, float, ProductiveSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionRecovered, const FSessionRecord&, RecoveredSession);

UCLASS()
class DEVELOPERPRODUCTIVITYTRACKER_API USessionTrackingSubsystem : public UEditorSubsystem, public FTickableEditorObject
{
	GENERATED_BODY()

public:
	USessionTrackingSubsystem();
	virtual ~USessionTrackingSubsystem();

	// ========================================================================
	// USubsystem Interface
	// ========================================================================

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ========================================================================
	// FTickableEditorObject Interface
	// ========================================================================

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return true; }
	virtual ETickableTickType GetTickableTickType() const override;

	// ========================================================================
	// SESSION CONTROL
	// ========================================================================

	// Start a new tracking session
	UFUNCTION(BlueprintCallable, Category = "Productivity|Session")
	bool StartSession();

	// End the current tracking session
	UFUNCTION(BlueprintCallable, Category = "Productivity|Session")
	bool EndSession();

	// Temporarily pause the current session
	UFUNCTION(BlueprintCallable, Category = "Productivity|Session")
	void PauseSession();

	// Resume a paused session
	UFUNCTION(BlueprintCallable, Category = "Productivity|Session")
	void ResumeSession();

	// Toggle session state (start/end or pause/resume)
	UFUNCTION(BlueprintCallable, Category = "Productivity|Session")
	void ToggleSession();

	// ========================================================================
	// SESSION QUERIES
	// ========================================================================

	// Check if a session is currently active
	UFUNCTION(BlueprintPure, Category = "Productivity|Session")
	bool IsSessionActive() const { return bHasActiveSession; }

	// Check if the current session is paused
	UFUNCTION(BlueprintPure, Category = "Productivity|Session")
	bool IsSessionPaused() const { return bIsSessionPaused; }

	// Get total elapsed time in seconds
	UFUNCTION(BlueprintPure, Category = "Productivity|Session")
	float GetElapsedSeconds() const;

	// Get productive time (active + thinking) in seconds
	UFUNCTION(BlueprintPure, Category = "Productivity|Session")
	float GetProductiveSeconds() const;

	// Get current activity state
	UFUNCTION(BlueprintPure, Category = "Productivity|Session")
	EActivityState GetCurrentActivityState() const { return CurrentActivityState; }

	// Get the current session record
	UFUNCTION(BlueprintPure, Category = "Productivity|Session")
	FSessionRecord GetCurrentSessionRecord() const { return CurrentSession; }

	// Get formatted elapsed time string (HH:MM:SS)
	UFUNCTION(BlueprintPure, Category = "Productivity|Session")
	FString GetFormattedElapsedTime() const;

	// Get activity state as display string
	UFUNCTION(BlueprintPure, Category = "Productivity|Session")
	FString GetActivityStateDisplayString() const;

	// ========================================================================
	// TASK LINKING
	// ========================================================================

	// Link current session to a task ID (Jira, Trello, etc.)
	UFUNCTION(BlueprintCallable, Category = "Productivity|Session")
	void LinkToTask(const FString& TaskId);

	// Get the linked task ID for the current session
	UFUNCTION(BlueprintPure, Category = "Productivity|Session")
	FString GetLinkedTaskId() const;

	// Clear the task link
	UFUNCTION(BlueprintCallable, Category = "Productivity|Session")
	void ClearTaskLink();

	// ========================================================================
	// HISTORY QUERIES
	// ========================================================================

	// Get sessions from the last N days
	UFUNCTION(BlueprintCallable, Category = "Productivity|History")
	TArray<FSessionRecord> GetRecentSessions(int32 DayCount);

	// Get daily summary for a specific date
	UFUNCTION(BlueprintCallable, Category = "Productivity|History")
	bool GetDailySummary(const FDateTime& Date, FDailySummary& OutSummary);

	// Get total time tracked today
	UFUNCTION(BlueprintPure, Category = "Productivity|History")
	float GetTodayTotalSeconds() const;

	// ========================================================================
	// EXTERNAL ACTIVITY
	// ========================================================================

	// Get the current external activity state
	UFUNCTION(BlueprintPure, Category = "Productivity|External")
	bool IsExternallyProductive() const;

	// Get the name of the currently focused external app
	UFUNCTION(BlueprintPure, Category = "Productivity|External")
	FString GetFocusedExternalApp() const;

	// ========================================================================
	// STORAGE ACCESS
	// ========================================================================

	// Get the storage manager for direct data operations
	UFUNCTION(BlueprintPure, Category = "Productivity|Storage")
	USecureStorageManager* GetStorageManager() const { return StorageManager; }

	// ========================================================================
	// DELEGATES
	// ========================================================================

	// Broadcast when a session starts
	UPROPERTY(BlueprintAssignable, Category = "Productivity|Events")
	FOnSessionStarted OnSessionStarted;

	// Broadcast when a session ends with the completed record
	UPROPERTY(BlueprintAssignable, Category = "Productivity|Events")
	FOnSessionEnded OnSessionEnded;

	// Broadcast when activity state changes
	UPROPERTY(BlueprintAssignable, Category = "Productivity|Events")
	FOnActivityStateChanged OnActivityStateChanged;

	// Broadcast every tick with elapsed and productive time
	UPROPERTY(BlueprintAssignable, Category = "Productivity|Events")
	FOnSessionTick OnSessionTick;

	// Broadcast when a crashed session is recovered
	UPROPERTY(BlueprintAssignable, Category = "Productivity|Events")
	FOnSessionRecovered OnSessionRecovered;

private:
	// ========================================================================
	// SESSION STATE
	// ========================================================================

	bool bHasActiveSession;
	bool bIsSessionPaused;
	FSessionRecord CurrentSession;
	EActivityState CurrentActivityState;
	EActivityState PreviousActivityState;

	// ========================================================================
	// TIMING
	// ========================================================================

	float SnapshotTimer;
	float AutoSaveTimer;
	float LastInputTime;
	double SessionStartRealTime;
	double PauseStartRealTime;
	double TotalPausedTime;

	// ========================================================================
	// COMPONENTS
	// ========================================================================

	TUniquePtr<IExternalActivityMonitor> ExternalActivityMonitor;

	UPROPERTY()
	USecureStorageManager* StorageManager;

	// Cached external state
	FExternalActivityState* CachedExternalState;

	// ========================================================================
	// SECURITY
	// ========================================================================

	FString InstallationSalt;
	FString MachineIdentifier;

	// ========================================================================
	// PRIVATE METHODS
	// ========================================================================

	// Determine activity state from all inputs
	EActivityState DetermineActivityState() const;

	// Calculate current productivity weight based on activity
	float CalculateProductivityWeight() const;

	// Capture an activity snapshot
	void CaptureActivitySnapshot();

	// Update the activity summary with delta time
	void UpdateActivitySummary(float DeltaTime, EActivityState State);

	// External activity callbacks
	void HandleExternalActivityChanged(const FExternalActivityState& NewState);
	void HandleSourceFileChanged(const FFileChangeEvent& FileEvent);

	// Persistence operations
	void InitializeStorage();
	void CheckForRecoverableSession();
	void SaveActiveSessionState();
	void FinalizeAndSaveSession();

	// Input detection
	float GetSecondsSinceLastInput() const;
	bool IsEditorFocused() const;
	bool IsPlayInEditorActive() const;

	// Initialize external monitoring
	void InitializeExternalMonitoring();
	void ShutdownExternalMonitoring();
};
