// ============================================================================
// StructuredLoggingSubsystem.h
// Developer: Marcus Daley
// Date: January 25, 2026
// Plugin: StructuredLogging
// ============================================================================
// Purpose:
// Game Instance Subsystem providing structured, persistent logging with
// JSON output for human and AI consumption. Auto-initializes per game session.
//
// Usage (C++):
// #include "StructuredLoggingMacros.h"
// SLOG_EVENT(this, "BroomComponent", "BroomMounted", {{"stamina", "85.0"}})
// SLOG_WARNING(this, "AI", "BlackboardKeyNotSet", {{"key", "TargetLocation"}})
//
// Usage (Blueprint):
// UStructuredLogLibrary::LogEvent(WorldContext, "PlayerDeath", Metadata)
//
// Why Subsystem:
// - Per-GameInstance lifecycle (survives level transitions)
// - Automatic initialization without singletons
// - Global access via Get(WorldContextObject)
// - Blueprint accessible
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StructuredLogTypes.h"
#include "StructuredLoggingSubsystem.generated.h"

// Forward declarations
class FStructuredLogOutputDevice;

/**
 * Subsystem that manages structured logging for a game session
 */
UCLASS()
class STRUCTUREDLOGGING_API UStructuredLoggingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ========================================================================
	// Lifecycle
	// ========================================================================

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ========================================================================
	// Static Accessor (follows ElementDatabaseSubsystem pattern)
	// ========================================================================

	/**
	 * Get the Structured Logging subsystem for the given world context.
	 * Returns nullptr if subsystem is not initialized or if structured logging is disabled.
	 */
	UFUNCTION(BlueprintPure, Category = "Structured Logging",
		meta = (WorldContext = "WorldContextObject", DisplayName = "Get Structured Logging Subsystem"))
	static UStructuredLoggingSubsystem* Get(const UObject* WorldContextObject);

	// ========================================================================
	// Primary Logging API
	// ========================================================================

	/**
	 * Log a structured event with metadata.
	 * Context (actor, world, file, line) is auto-populated.
	 *
	 * @param ContextObject - Object providing context (usually 'this')
	 * @param Channel - System/feature name (e.g., "AI", "Perception")
	 * @param EventName - What happened (e.g., "BroomMounted", "BlackboardKeyNotSet")
	 * @param Verbosity - Log level (Display, Warning, Error, Fatal, Verbose)
	 * @param Metadata - User-provided key-value pairs
	 * @param SourceFile - Source file path (__FILE__ in C++)
	 * @param SourceLine - Line number (__LINE__ in C++)
	 */
	void LogEvent(
		const UObject* ContextObject,
		FName Channel,
		FName EventName,
		EStructuredLogVerbosity Verbosity,
		const TMap<FString, FString>& Metadata,
		const FString& SourceFile = TEXT(""),
		int32 SourceLine = 0
	);

	// ========================================================================
	// Focus Mode (for testing specific features)
	// ========================================================================

	/**
	 * Enable focus mode - only logs from specified channels are written to a separate file.
	 * Useful for testing specific systems without noise from other channels.
	 */
	UFUNCTION(BlueprintCallable, Category = "Structured Logging")
	void EnableFocusMode(FName FeatureName, const TArray<FName>& ChannelsToCapture);

	/**
	 * Disable focus mode and return to normal logging.
	 */
	UFUNCTION(BlueprintCallable, Category = "Structured Logging")
	void DisableFocusMode();

	/**
	 * Check if focus mode is currently active.
	 */
	UFUNCTION(BlueprintPure, Category = "Structured Logging")
	bool IsFocusModeActive() const { return bFocusModeActive; }

	/**
	 * Get the current focus feature name (empty if focus mode disabled).
	 */
	UFUNCTION(BlueprintPure, Category = "Structured Logging")
	FName GetCurrentFocusFeature() const { return CurrentFocusFeature; }

	// ========================================================================
	// Channel Filtering
	// ========================================================================

	/**
	 * Set verbosity level for a specific channel at runtime.
	 * Only logs at or above this level will be written.
	 */
	UFUNCTION(BlueprintCallable, Category = "Structured Logging")
	void SetChannelVerbosity(FName Channel, EStructuredLogVerbosity Verbosity);

	/**
	 * Get the verbosity level for a channel.
	 */
	UFUNCTION(BlueprintPure, Category = "Structured Logging")
	EStructuredLogVerbosity GetChannelVerbosity(FName Channel) const;

	// ========================================================================
	// Session Info
	// ========================================================================

	/**
	 * Get the unique session GUID for this game session.
	 */
	UFUNCTION(BlueprintPure, Category = "Structured Logging")
	FGuid GetSessionGuid() const { return SessionGuid; }

	/**
	 * Get the directory where logs for this session are stored.
	 */
	UFUNCTION(BlueprintPure, Category = "Structured Logging")
	FString GetSessionLogDirectory() const;

	/**
	 * Get the session metadata (start time, project name, etc.)
	 */
	UFUNCTION(BlueprintPure, Category = "Structured Logging")
	FStructuredLogSessionMetadata GetSessionMetadata() const { return SessionMetadata; }

	// ========================================================================
	// Configuration
	// ========================================================================

	/**
	 * Check if structured logging is enabled globally.
	 */
	UFUNCTION(BlueprintPure, Category = "Structured Logging")
	bool IsLoggingEnabled() const { return bEnabled; }

	/**
	 * Enable or disable structured logging at runtime.
	 */
	UFUNCTION(BlueprintCallable, Category = "Structured Logging")
	void SetLoggingEnabled(bool bInEnabled);

	/**
	 * Flush all pending log entries to disk immediately.
	 */
	UFUNCTION(BlueprintCallable, Category = "Structured Logging")
	void FlushLogs();

private:
	// ========================================================================
	// Internal Implementation
	// ========================================================================

	// Output device for file writing (shared pointer for thread safety)
	TSharedPtr<FStructuredLogOutputDevice> OutputDevice;

	// Session tracking
	FGuid SessionGuid;
	FStructuredLogSessionMetadata SessionMetadata;

	// Focus mode state
	bool bFocusModeActive;
	FName CurrentFocusFeature;
	TSet<FName> FocusModeChannels;

	// Channel verbosity overrides (loaded from config and runtime changes)
	TMap<FName, EStructuredLogVerbosity> ChannelVerbosities;

	// Configuration (loaded from DefaultStructuredLogging.ini)
	bool bEnabled;
	bool bEnabledInShipping;
	bool bWriteToFile;
	bool bEchoToNativeLog;
	int32 MaxLogFileSizeMB;
	int32 MaxLogFileAgeMinutes;
	int32 RetentionDays;
	int32 MinSessionsToKeep;
	int32 AsyncWriteQueueSize;
	float FlushIntervalSeconds;
	bool bEnablePerformanceProfiling;

	// Helper functions
	void LoadConfiguration();
	void InitializeSession();
	void WriteSessionMetadata();
	bool ShouldLogChannel(FName Channel, EStructuredLogVerbosity Verbosity) const;
	FStructuredLogContext ExtractContext(const UObject* ContextObject,
		const FString& SourceFile, int32 SourceLine) const;
	void EchoToNativeLog(const FStructuredLogEntry& Entry) const;
};
