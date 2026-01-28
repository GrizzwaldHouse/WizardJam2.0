// ============================================================================
// StructuredLogOutputDevice.cpp
// Developer: Marcus Daley
// Date: January 25, 2026
// Plugin: StructuredLogging
// ============================================================================

#include "StructuredLogOutputDevice.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/RunnableThread.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogStructuredLogOutput, Log, All);

// ============================================================================
// Constructor / Destructor
// ============================================================================

FStructuredLogOutputDevice::FStructuredLogOutputDevice(
	const FString& InLogDirectory,
	int32 InMaxFileSizeMB,
	int32 InMaxFileAgeMinutes,
	int32 InQueueSize,
	float InFlushIntervalSeconds)
	: LogDirectory(InLogDirectory)
	, MaxFileSizeMB(InMaxFileSizeMB)
	, MaxFileAgeMinutes(InMaxFileAgeMinutes)
	, QueueSize(InQueueSize)
	, FlushIntervalSeconds(InFlushIntervalSeconds)
	, bShuttingDown(false)
	, bHasFocusModeFile(false)
{
	// Create directory structure
	CreateDirectoryStructure();

	// Initialize first log file
	CurrentLogFilePath = FPaths::Combine(LogDirectory, GetTimestampedFileName());
	CurrentFileStartTime = FDateTime::Now();

	// Start background write thread
	WriteThread = MakeUnique<FWriteThread>(this);
	RunnableThread = TUniquePtr<FRunnableThread>(
		FRunnableThread::Create(WriteThread.Get(), TEXT("StructuredLogWriteThread"))
	);

	UE_LOG(LogStructuredLogOutput, Log,
		TEXT("Output device initialized - Directory: %s"), *LogDirectory);
}

FStructuredLogOutputDevice::~FStructuredLogOutputDevice()
{
	// Signal shutdown
	bShuttingDown = true;

	// Flush remaining logs
	Flush();

	// Stop write thread
	if (WriteThread.IsValid())
	{
		WriteThread->Stop();
	}

	if (RunnableThread.IsValid())
	{
		RunnableThread->WaitForCompletion();
		RunnableThread.Reset();
	}

	WriteThread.Reset();

	UE_LOG(LogStructuredLogOutput, Log, TEXT("Output device shutdown"));
}

// ============================================================================
// Public API
// ============================================================================

void FStructuredLogOutputDevice::WriteEntry(
	const FStructuredLogEntry& Entry,
	bool bFocusMode,
	FName FocusFeature,
	const TSet<FName>& FocusChannels)
{
	if (bShuttingDown)
	{
		return;
	}

	// Add to main queue
	if (!PendingEntries.Enqueue(Entry))
	{
		UE_LOG(LogStructuredLogOutput, Warning,
			TEXT("Queue full - dropping log entry: %s"), *Entry.EventName.ToString());
	}

	// Add to focus mode queue if applicable
	if (bFocusMode && bHasFocusModeFile && FocusChannels.Contains(Entry.Channel))
	{
		FocusModePendingEntries.Enqueue(Entry);
	}
}

void FStructuredLogOutputDevice::Flush()
{
	// Process all pending entries
	ProcessWriteQueue();
}

void FStructuredLogOutputDevice::WriteSessionMetadata(
	const FStructuredLogSessionMetadata& Metadata,
	const FString& FilePath)
{
	const FString JSONContent = FormatSessionMetadataAsJSON(Metadata);
	WriteToFile(FilePath, JSONContent, false);  // Overwrite mode for metadata
}

void FStructuredLogOutputDevice::CreateFocusModeFile(const FString& FilePath)
{
	FocusModeLogFilePath = FilePath;
	bHasFocusModeFile = true;

	UE_LOG(LogStructuredLogOutput, Log,
		TEXT("Focus mode file created: %s"), *FilePath);
}

void FStructuredLogOutputDevice::CloseFocusModeFile()
{
	// Flush focus mode entries
	ProcessWriteQueue();

	bHasFocusModeFile = false;
	FocusModeLogFilePath.Empty();

	UE_LOG(LogStructuredLogOutput, Log, TEXT("Focus mode file closed"));
}

int32 FStructuredLogOutputDevice::GetQueueSize() const
{
	// Note: TQueue doesn't provide size(), this is an approximation
	return 0;  // TODO: Implement proper queue size tracking if needed for profiling
}

// ============================================================================
// File Management
// ============================================================================

void FStructuredLogOutputDevice::CreateDirectoryStructure()
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	if (!PlatformFile.DirectoryExists(*LogDirectory))
	{
		PlatformFile.CreateDirectoryTree(*LogDirectory);
		UE_LOG(LogStructuredLogOutput, Log,
			TEXT("Created log directory: %s"), *LogDirectory);
	}
}

void FStructuredLogOutputDevice::RotateLogFileIfNeeded()
{
	// Check file size
	const int64 CurrentFileSize = IFileManager::Get().FileSize(*CurrentLogFilePath);
	const int64 MaxFileSizeBytes = static_cast<int64>(MaxFileSizeMB) * 1024 * 1024;

	const bool bExceedsSize = CurrentFileSize > MaxFileSizeBytes;

	// Check file age
	const FTimespan FileAge = FDateTime::Now() - CurrentFileStartTime;
	const bool bExceedsAge = FileAge.GetTotalMinutes() > MaxFileAgeMinutes;

	if (bExceedsSize || bExceedsAge)
	{
		// Create new log file
		CurrentLogFilePath = FPaths::Combine(LogDirectory, GetTimestampedFileName());
		CurrentFileStartTime = FDateTime::Now();

		UE_LOG(LogStructuredLogOutput, Log,
			TEXT("Log file rotated - New file: %s"), *CurrentLogFilePath);
	}
}

FString FStructuredLogOutputDevice::GetTimestampedFileName() const
{
	const FDateTime Now = FDateTime::Now();
	return FString::Printf(TEXT("events_%04d%02d%02d_%02d%02d%02d.jsonl"),
		Now.GetYear(), Now.GetMonth(), Now.GetDay(),
		Now.GetHour(), Now.GetMinute(), Now.GetSecond());
}

// ============================================================================
// JSON Formatting
// ============================================================================

FString FStructuredLogOutputDevice::FormatEntryAsJSON(const FStructuredLogEntry& Entry) const
{
	// Build JSON manually for performance (avoid TJsonWriter overhead for simple structure)
	FString JSON = TEXT("{");

	// Timestamp
	JSON += FString::Printf(TEXT("\"timestamp\":\"%s\","), *Entry.Timestamp);

	// Session GUID
	JSON += FString::Printf(TEXT("\"session_guid\":\"%s\","), *Entry.SessionGuid.ToString());

	// Event name
	JSON += FString::Printf(TEXT("\"event_name\":\"%s\","), *Entry.EventName.ToString());

	// Channel
	JSON += FString::Printf(TEXT("\"channel\":\"%s\","), *Entry.Channel.ToString());

	// Verbosity
	JSON += FString::Printf(TEXT("\"verbosity\":\"%s\","),
		*StructuredLogUtils::VerbosityToString(Entry.Verbosity));

	// Context
	JSON += TEXT("\"context\":{");
	JSON += FString::Printf(TEXT("\"actor\":%s,"),
		Entry.Context.ActorName.IsEmpty() ? TEXT("null") :
		*FString::Printf(TEXT("\"%s\""), *EscapeJSONString(Entry.Context.ActorName)));
	JSON += FString::Printf(TEXT("\"actor_class\":%s,"),
		Entry.Context.ActorClass.IsEmpty() ? TEXT("null") :
		*FString::Printf(TEXT("\"%s\""), *EscapeJSONString(Entry.Context.ActorClass)));
	JSON += FString::Printf(TEXT("\"world\":%s,"),
		Entry.Context.WorldName.IsEmpty() ? TEXT("null") :
		*FString::Printf(TEXT("\"%s\""), *EscapeJSONString(Entry.Context.WorldName)));
	JSON += FString::Printf(TEXT("\"subsystem\":%s,"),
		Entry.Context.SubsystemName.IsEmpty() ? TEXT("null") :
		*FString::Printf(TEXT("\"%s\""), *EscapeJSONString(Entry.Context.SubsystemName)));
	JSON += FString::Printf(TEXT("\"source_file\":%s,"),
		Entry.Context.SourceFile.IsEmpty() ? TEXT("null") :
		*FString::Printf(TEXT("\"%s\""), *EscapeJSONString(Entry.Context.SourceFile)));
	JSON += FString::Printf(TEXT("\"source_line\":%d"),
		Entry.Context.SourceLine);
	JSON += TEXT("},");

	// Metadata
	JSON += TEXT("\"metadata\":{");
	bool bFirstMetadata = true;
	for (const auto& Pair : Entry.Metadata)
	{
		if (!bFirstMetadata)
		{
			JSON += TEXT(",");
		}
		JSON += FString::Printf(TEXT("\"%s\":\"%s\""),
			*EscapeJSONString(Pair.Key),
			*EscapeJSONString(Pair.Value));
		bFirstMetadata = false;
	}
	JSON += TEXT("}");

	JSON += TEXT("}");

	return JSON;
}

FString FStructuredLogOutputDevice::FormatSessionMetadataAsJSON(const FStructuredLogSessionMetadata& Metadata) const
{
	FString JSON = TEXT("{\n");

	JSON += FString::Printf(TEXT("  \"session_guid\": \"%s\",\n"), *Metadata.SessionGuid.ToString());
	JSON += FString::Printf(TEXT("  \"session_start_time\": \"%s\",\n"), *Metadata.SessionStartTime);
	JSON += FString::Printf(TEXT("  \"project_name\": \"%s\",\n"), *Metadata.ProjectName);
	JSON += FString::Printf(TEXT("  \"engine_version\": \"%s\",\n"), *Metadata.EngineVersion);
	JSON += FString::Printf(TEXT("  \"build_configuration\": \"%s\",\n"), *Metadata.BuildConfiguration);
	JSON += FString::Printf(TEXT("  \"platform\": \"%s\"\n"), *Metadata.Platform);

	JSON += TEXT("}");

	return JSON;
}

FString FStructuredLogOutputDevice::EscapeJSONString(const FString& Input) const
{
	FString Output = Input;

	// Escape special JSON characters
	Output.ReplaceInline(TEXT("\\"), TEXT("\\\\"));
	Output.ReplaceInline(TEXT("\""), TEXT("\\\""));
	Output.ReplaceInline(TEXT("\n"), TEXT("\\n"));
	Output.ReplaceInline(TEXT("\r"), TEXT("\\r"));
	Output.ReplaceInline(TEXT("\t"), TEXT("\\t"));

	return Output;
}

// ============================================================================
// Async Writing
// ============================================================================

void FStructuredLogOutputDevice::ProcessWriteQueue()
{
	FScopeLock Lock(&FileLock);

	// Process main queue
	FStructuredLogEntry Entry;
	while (PendingEntries.Dequeue(Entry))
	{
		const FString JSONLine = FormatEntryAsJSON(Entry) + TEXT("\n");
		WriteToFile(CurrentLogFilePath, JSONLine, true);
	}

	// Process focus mode queue
	if (bHasFocusModeFile)
	{
		while (FocusModePendingEntries.Dequeue(Entry))
		{
			const FString JSONLine = FormatEntryAsJSON(Entry) + TEXT("\n");
			WriteToFile(FocusModeLogFilePath, JSONLine, true);
		}
	}

	// Check if rotation needed
	RotateLogFileIfNeeded();
}

void FStructuredLogOutputDevice::WriteToFile(const FString& FilePath, const FString& Content, bool bAppend)
{
	if (bAppend)
	{
		FFileHelper::SaveStringToFile(Content, *FilePath,
			FFileHelper::EEncodingOptions::AutoDetect,
			&IFileManager::Get(),
			FILEWRITE_Append);
	}
	else
	{
		FFileHelper::SaveStringToFile(Content, *FilePath);
	}
}

// ============================================================================
// FWriteThread Implementation
// ============================================================================

FStructuredLogOutputDevice::FWriteThread::FWriteThread(FStructuredLogOutputDevice* InOwner)
	: Owner(InOwner)
	, bShouldStop(false)
{
}

FStructuredLogOutputDevice::FWriteThread::~FWriteThread()
{
}

bool FStructuredLogOutputDevice::FWriteThread::Init()
{
	return true;
}

uint32 FStructuredLogOutputDevice::FWriteThread::Run()
{
	while (!bShouldStop)
	{
		// Process write queue at regular intervals
		Owner->ProcessWriteQueue();

		// Sleep for flush interval
		FPlatformProcess::Sleep(Owner->FlushIntervalSeconds);
	}

	// Final flush before exit
	Owner->ProcessWriteQueue();

	return 0;
}

void FStructuredLogOutputDevice::FWriteThread::Stop()
{
	bShouldStop = true;
}

void FStructuredLogOutputDevice::FWriteThread::Exit()
{
}
