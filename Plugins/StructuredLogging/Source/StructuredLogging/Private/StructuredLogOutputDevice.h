// ============================================================================
// StructuredLogOutputDevice.h
// Developer: Marcus Daley
// Date: January 25, 2026
// Plugin: StructuredLogging
// ============================================================================
// Purpose:
// Private class that handles async file writing, JSON formatting, and
// log file rotation. Uses background thread with lock-free queue for
// performance (< 1μs main thread overhead per log).
//
// This is NOT a UObject - it's a plain C++ class managed by the subsystem.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "StructuredLogTypes.h"
#include "HAL/Runnable.h"
#include "Containers/Queue.h"

/**
 * Non-UObject class that handles file I/O for structured logging.
 * Runs a background thread to write logs asynchronously.
 */
class FStructuredLogOutputDevice : public TSharedFromThis<FStructuredLogOutputDevice>
{
public:
	/**
	 * Constructor
	 * @param InLogDirectory - Directory where logs will be written (session-specific folder)
	 * @param InMaxFileSizeMB - Maximum file size before rotation (MB)
	 * @param InMaxFileAgeMinutes - Maximum file age before rotation (minutes)
	 * @param InQueueSize - Size of async write queue
	 * @param InFlushIntervalSeconds - Interval between disk flushes
	 */
	FStructuredLogOutputDevice(
		const FString& InLogDirectory,
		int32 InMaxFileSizeMB,
		int32 InMaxFileAgeMinutes,
		int32 InQueueSize,
		float InFlushIntervalSeconds
	);

	/**
	 * Destructor - flushes remaining logs and shuts down write thread
	 */
	~FStructuredLogOutputDevice();

	/**
	 * Write a log entry to the queue (main thread, non-blocking)
	 * @param Entry - Log entry to write
	 * @param bFocusMode - Whether focus mode is active
	 * @param FocusFeature - Focus feature name
	 * @param FocusChannels - Set of channels to capture in focus mode
	 */
	void WriteEntry(
		const FStructuredLogEntry& Entry,
		bool bFocusMode,
		FName FocusFeature,
		const TSet<FName>& FocusChannels
	);

	/**
	 * Flush all pending entries to disk immediately (blocking)
	 */
	void Flush();

	/**
	 * Write session metadata to JSON file
	 */
	void WriteSessionMetadata(const FStructuredLogSessionMetadata& Metadata, const FString& FilePath);

	/**
	 * Create focus mode output file
	 */
	void CreateFocusModeFile(const FString& FilePath);

	/**
	 * Close focus mode output file
	 */
	void CloseFocusModeFile();

	/**
	 * Get current queue size (for monitoring)
	 */
	int32 GetQueueSize() const;

private:
	// ========================================================================
	// File Management
	// ========================================================================

	// Create log directory if it doesn't exist
	void CreateDirectoryStructure();

	// Rotate log file if needed (size or age threshold exceeded)
	void RotateLogFileIfNeeded();

	// Get timestamp-based filename for rotating logs
	FString GetTimestampedFileName() const;

	// ========================================================================
	// JSON Formatting
	// ========================================================================

	// Format log entry as JSON string (single line for .jsonl format)
	FString FormatEntryAsJSON(const FStructuredLogEntry& Entry) const;

	// Format session metadata as JSON
	FString FormatSessionMetadataAsJSON(const FStructuredLogSessionMetadata& Metadata) const;

	// Escape string for JSON output
	FString EscapeJSONString(const FString& Input) const;

	// ========================================================================
	// Async Writing
	// ========================================================================

	// Background thread that processes write queue
	class FWriteThread : public FRunnable
	{
	public:
		FWriteThread(FStructuredLogOutputDevice* InOwner);
		virtual ~FWriteThread();

		// FRunnable interface
		virtual bool Init() override;
		virtual uint32 Run() override;
		virtual void Stop() override;
		virtual void Exit() override;

	private:
		FStructuredLogOutputDevice* Owner;
		FThreadSafeBool bShouldStop;
	};

	// Process entries from queue and write to disk
	void ProcessWriteQueue();

	// Write string to file
	void WriteToFile(const FString& FilePath, const FString& Content, bool bAppend = true);

	// ========================================================================
	// Member Variables
	// ========================================================================

	// Log directory (session-specific)
	FString LogDirectory;

	// Current main log file path
	FString CurrentLogFilePath;

	// Focus mode log file path
	FString FocusModeLogFilePath;

	// File rotation settings
	int32 MaxFileSizeMB;
	int32 MaxFileAgeMinutes;
	FDateTime CurrentFileStartTime;

	// Async queue and threading
	TQueue<FStructuredLogEntry, EQueueMode::Mpsc> PendingEntries;  // Lock-free multi-producer single-consumer queue
	TUniquePtr<FWriteThread> WriteThread;
	TUniquePtr<FRunnableThread> RunnableThread;
	int32 QueueSize;
	float FlushIntervalSeconds;

	// Thread synchronization
	FCriticalSection FileLock;  // Protects file writes
	FThreadSafeBool bShuttingDown;

	// Focus mode tracking
	TQueue<FStructuredLogEntry, EQueueMode::Mpsc> FocusModePendingEntries;
	bool bHasFocusModeFile;
};
