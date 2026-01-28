// ============================================================================
// StructuredLoggingSubsystem.cpp
// Developer: Marcus Daley
// Date: January 25, 2026
// Plugin: StructuredLogging
// ============================================================================

#include "StructuredLoggingSubsystem.h"
#include "StructuredLogOutputDevice.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/Paths.h"
#include "Misc/ConfigCacheIni.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogStructuredLoggingSubsystem, Log, All);

// ============================================================================
// Lifecycle
// ============================================================================

void UStructuredLoggingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Load configuration from DefaultStructuredLogging.ini
	LoadConfiguration();

	// Check if logging is enabled
	if (!bEnabled)
	{
		UE_LOG(LogStructuredLoggingSubsystem, Log,
			TEXT("Structured Logging is disabled in configuration"));
		return;
	}

	// Initialize session
	InitializeSession();

	// Create output device for file writing
	if (bWriteToFile)
	{
		const FString LogDirectory = GetSessionLogDirectory();
		OutputDevice = MakeShared<FStructuredLogOutputDevice>(
			LogDirectory,
			MaxLogFileSizeMB,
			MaxLogFileAgeMinutes,
			AsyncWriteQueueSize,
			FlushIntervalSeconds
		);
	}

	// Write session metadata file
	WriteSessionMetadata();

	UE_LOG(LogStructuredLoggingSubsystem, Display,
		TEXT("Structured Logging initialized - Session GUID: %s"),
		*SessionGuid.ToString());
}

void UStructuredLoggingSubsystem::Deinitialize()
{
	if (OutputDevice.IsValid())
	{
		// Flush any pending logs before shutdown
		OutputDevice->Flush();
		OutputDevice.Reset();
	}

	UE_LOG(LogStructuredLoggingSubsystem, Display,
		TEXT("Structured Logging shutdown - Session GUID: %s"),
		*SessionGuid.ToString());

	Super::Deinitialize();
}

// ============================================================================
// Static Accessor
// ============================================================================

UStructuredLoggingSubsystem* UStructuredLoggingSubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	const UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		return nullptr;
	}

	return GameInstance->GetSubsystem<UStructuredLoggingSubsystem>();
}

// ============================================================================
// Primary Logging API
// ============================================================================

void UStructuredLoggingSubsystem::LogEvent(
	const UObject* ContextObject,
	FName Channel,
	FName EventName,
	EStructuredLogVerbosity Verbosity,
	const TMap<FString, FString>& Metadata,
	const FString& SourceFile,
	int32 SourceLine)
{
	// Check if logging is enabled and channel should be logged
	if (!bEnabled || !ShouldLogChannel(Channel, Verbosity))
	{
		return;
	}

	// Construct log entry
	FStructuredLogEntry Entry;
	Entry.Timestamp = StructuredLogUtils::GetISO8601Timestamp();
	Entry.SessionGuid = SessionGuid;
	Entry.EventName = EventName;
	Entry.Channel = Channel;
	Entry.Verbosity = Verbosity;
	Entry.Context = ExtractContext(ContextObject, SourceFile, SourceLine);
	Entry.Metadata = Metadata;

	// Write to output device (async file writing)
	if (OutputDevice.IsValid())
	{
		OutputDevice->WriteEntry(Entry, bFocusModeActive, CurrentFocusFeature, FocusModeChannels);
	}

	// Echo to native UE_LOG if configured
	if (bEchoToNativeLog)
	{
		EchoToNativeLog(Entry);
	}
}

// ============================================================================
// Focus Mode
// ============================================================================

void UStructuredLoggingSubsystem::EnableFocusMode(FName FeatureName, const TArray<FName>& ChannelsToCapture)
{
	bFocusModeActive = true;
	CurrentFocusFeature = FeatureName;
	FocusModeChannels = TSet<FName>(ChannelsToCapture);

	// Create focus mode output file
	if (OutputDevice.IsValid())
	{
		const FString FocusLogPath = FPaths::Combine(
			GetSessionLogDirectory(),
			FString::Printf(TEXT("focus_%s.jsonl"), *FeatureName.ToString())
		);
		OutputDevice->CreateFocusModeFile(FocusLogPath);
	}

	UE_LOG(LogStructuredLoggingSubsystem, Display,
		TEXT("Focus mode enabled - Feature: %s, Channels: %d"),
		*FeatureName.ToString(), ChannelsToCapture.Num());
}

void UStructuredLoggingSubsystem::DisableFocusMode()
{
	if (!bFocusModeActive)
	{
		return;
	}

	UE_LOG(LogStructuredLoggingSubsystem, Display,
		TEXT("Focus mode disabled - Feature: %s"),
		*CurrentFocusFeature.ToString());

	// Close focus mode file
	if (OutputDevice.IsValid())
	{
		OutputDevice->CloseFocusModeFile();
	}

	bFocusModeActive = false;
	CurrentFocusFeature = NAME_None;
	FocusModeChannels.Empty();
}

// ============================================================================
// Channel Filtering
// ============================================================================

void UStructuredLoggingSubsystem::SetChannelVerbosity(FName Channel, EStructuredLogVerbosity Verbosity)
{
	ChannelVerbosities.Add(Channel, Verbosity);

	UE_LOG(LogStructuredLoggingSubsystem, Log,
		TEXT("Channel verbosity set - Channel: %s, Verbosity: %s"),
		*Channel.ToString(),
		*StructuredLogUtils::VerbosityToString(Verbosity));
}

EStructuredLogVerbosity UStructuredLoggingSubsystem::GetChannelVerbosity(FName Channel) const
{
	if (const EStructuredLogVerbosity* Found = ChannelVerbosities.Find(Channel))
	{
		return *Found;
	}

	// Default verbosity if not configured
	return EStructuredLogVerbosity::Display;
}

// ============================================================================
// Session Info
// ============================================================================

FString UStructuredLoggingSubsystem::GetSessionLogDirectory() const
{
	const FString ProjectSavedDir = FPaths::ProjectSavedDir();
	const FString StructuredLogsDir = FPaths::Combine(ProjectSavedDir, TEXT("Logs"), TEXT("Structured"));
	const FString SessionDir = FPaths::Combine(StructuredLogsDir, SessionGuid.ToString());
	return SessionDir;
}

// ============================================================================
// Configuration
// ============================================================================

void UStructuredLoggingSubsystem::SetLoggingEnabled(bool bInEnabled)
{
	bEnabled = bInEnabled;

	UE_LOG(LogStructuredLoggingSubsystem, Display,
		TEXT("Structured Logging %s"),
		bEnabled ? TEXT("enabled") : TEXT("disabled"));
}

void UStructuredLoggingSubsystem::FlushLogs()
{
	if (OutputDevice.IsValid())
	{
		OutputDevice->Flush();
		UE_LOG(LogStructuredLoggingSubsystem, Log, TEXT("Logs flushed to disk"));
	}
}

// ============================================================================
// Internal Implementation
// ============================================================================

void UStructuredLoggingSubsystem::LoadConfiguration()
{
	const FString ConfigSection = TEXT("/Script/StructuredLogging.StructuredLoggingSubsystem");

	// Load boolean settings
	GConfig->GetBool(*ConfigSection, TEXT("bEnabled"), bEnabled, GGameIni);
	GConfig->GetBool(*ConfigSection, TEXT("bEnabledInShipping"), bEnabledInShipping, GGameIni);
	GConfig->GetBool(*ConfigSection, TEXT("bWriteToFile"), bWriteToFile, GGameIni);
	GConfig->GetBool(*ConfigSection, TEXT("bEchoToNativeLog"), bEchoToNativeLog, GGameIni);
	GConfig->GetBool(*ConfigSection, TEXT("bEnablePerformanceProfiling"), bEnablePerformanceProfiling, GGameIni);

	// Load numeric settings
	GConfig->GetInt(*ConfigSection, TEXT("MaxLogFileSizeMB"), MaxLogFileSizeMB, GGameIni);
	GConfig->GetInt(*ConfigSection, TEXT("MaxLogFileAgeMinutes"), MaxLogFileAgeMinutes, GGameIni);
	GConfig->GetInt(*ConfigSection, TEXT("RetentionDays"), RetentionDays, GGameIni);
	GConfig->GetInt(*ConfigSection, TEXT("MinSessionsToKeep"), MinSessionsToKeep, GGameIni);
	GConfig->GetInt(*ConfigSection, TEXT("AsyncWriteQueueSize"), AsyncWriteQueueSize, GGameIni);
	GConfig->GetFloat(*ConfigSection, TEXT("FlushIntervalSeconds"), FlushIntervalSeconds, GGameIni);

	// Load channel verbosities
	// Format: +ChannelVerbosities=(Channel="AI",Verbosity=Display)
	TArray<FString> ChannelVerbosityStrings;
	GConfig->GetArray(*ConfigSection, TEXT("ChannelVerbosities"), ChannelVerbosityStrings, GGameIni);

	for (const FString& VerbosityString : ChannelVerbosityStrings)
	{
		// Parse: Channel="AI",Verbosity=Display
		FString ChannelName, VerbosityName;
		if (VerbosityString.Split(TEXT("Channel=\""), nullptr, &ChannelName))
		{
			ChannelName.Split(TEXT("\""), &ChannelName, nullptr);

			if (VerbosityString.Split(TEXT("Verbosity="), nullptr, &VerbosityName))
			{
				VerbosityName.Split(TEXT(")"), &VerbosityName, nullptr);
				VerbosityName.TrimStartAndEndInline();

				// Convert verbosity string to enum
				EStructuredLogVerbosity Verbosity = EStructuredLogVerbosity::Display;
				if (VerbosityName == TEXT("Warning"))
				{
					Verbosity = EStructuredLogVerbosity::Warning;
				}
				else if (VerbosityName == TEXT("Error"))
				{
					Verbosity = EStructuredLogVerbosity::Error;
				}
				else if (VerbosityName == TEXT("Fatal"))
				{
					Verbosity = EStructuredLogVerbosity::Fatal;
				}
				else if (VerbosityName == TEXT("Verbose"))
				{
					Verbosity = EStructuredLogVerbosity::Verbose;
				}

				ChannelVerbosities.Add(FName(*ChannelName), Verbosity);
			}
		}
	}

	UE_LOG(LogStructuredLoggingSubsystem, Log,
		TEXT("Configuration loaded - Channels configured: %d"), ChannelVerbosities.Num());
}

void UStructuredLoggingSubsystem::InitializeSession()
{
	// Generate unique session GUID
	SessionGuid = FGuid::NewGuid();

	// Populate session metadata
	SessionMetadata.SessionGuid = SessionGuid;
	SessionMetadata.SessionStartTime = StructuredLogUtils::GetISO8601Timestamp();
	SessionMetadata.ProjectName = FApp::GetProjectName();
	SessionMetadata.EngineVersion = FEngineVersion::Current().ToString();

	// Build configuration
#if UE_BUILD_SHIPPING
	SessionMetadata.BuildConfiguration = TEXT("Shipping");
#elif UE_BUILD_TEST
	SessionMetadata.BuildConfiguration = TEXT("Test");
#elif UE_BUILD_DEVELOPMENT
	SessionMetadata.BuildConfiguration = TEXT("Development");
#else
	SessionMetadata.BuildConfiguration = TEXT("Unknown");
#endif

	// Platform
	SessionMetadata.Platform = FPlatformProperties::IniPlatformName();

	// Focus mode defaults
	bFocusModeActive = false;
	CurrentFocusFeature = NAME_None;
}

void UStructuredLoggingSubsystem::WriteSessionMetadata()
{
	if (!bWriteToFile || !OutputDevice.IsValid())
	{
		return;
	}

	const FString MetadataPath = FPaths::Combine(
		GetSessionLogDirectory(),
		TEXT("session_metadata.json")
	);

	OutputDevice->WriteSessionMetadata(SessionMetadata, MetadataPath);
}

bool UStructuredLoggingSubsystem::ShouldLogChannel(FName Channel, EStructuredLogVerbosity Verbosity) const
{
	// Check if channel has verbosity override
	if (const EStructuredLogVerbosity* ChannelMinVerbosity = ChannelVerbosities.Find(Channel))
	{
		// Only log if verbosity is at or above the channel's minimum
		return static_cast<uint8>(Verbosity) >= static_cast<uint8>(*ChannelMinVerbosity);
	}

	// Default: log everything at Display or above
	return true;
}

FStructuredLogContext UStructuredLoggingSubsystem::ExtractContext(
	const UObject* ContextObject,
	const FString& SourceFile,
	int32 SourceLine) const
{
	FStructuredLogContext Context;

	if (ContextObject)
	{
		// Extract actor context
		if (const AActor* Actor = Cast<AActor>(ContextObject))
		{
			Context.ActorName = Actor->GetName();
			Context.ActorClass = Actor->GetClass()->GetName();
		}
		else if (const AActor* OwnerActor = ContextObject->GetTypedOuter<AActor>())
		{
			// If context object is a component, get owning actor
			Context.ActorName = OwnerActor->GetName();
			Context.ActorClass = OwnerActor->GetClass()->GetName();
		}

		// Extract world context
		if (const UWorld* World = ContextObject->GetWorld())
		{
			Context.WorldName = World->GetName();
		}

		// Check if context is a subsystem
		if (ContextObject->IsA<UGameInstanceSubsystem>() ||
			ContextObject->IsA<UWorldSubsystem>())
		{
			Context.SubsystemName = ContextObject->GetClass()->GetName();
		}
	}

	// Source file and line (populated from macros)
	Context.SourceFile = SourceFile;
	Context.SourceLine = SourceLine;

	return Context;
}

void UStructuredLoggingSubsystem::EchoToNativeLog(const FStructuredLogEntry& Entry) const
{
	// Format: [Channel] EventName: actor=ActorName, metadata...
	FString MetadataString;
	for (const auto& Pair : Entry.Metadata)
	{
		if (!MetadataString.IsEmpty())
		{
			MetadataString += TEXT(", ");
		}
		MetadataString += FString::Printf(TEXT("%s=%s"), *Pair.Key, *Pair.Value);
	}

	const FString ActorContext = Entry.Context.ActorName.IsEmpty()
		? TEXT("")
		: FString::Printf(TEXT(" [%s]"), *Entry.Context.ActorName);

	const FString LogMessage = FString::Printf(
		TEXT("[%s]%s %s: %s"),
		*Entry.Channel.ToString(),
		*ActorContext,
		*Entry.EventName.ToString(),
		*MetadataString
	);

	// Log to native UE_LOG with appropriate verbosity
	const ELogVerbosity::Type NativeVerbosity =
		StructuredLogUtils::ConvertToNativeVerbosity(Entry.Verbosity);

	// Use a switch to call UE_LOG with the correct verbosity level
	// (UE_LOG macro requires compile-time verbosity, can't use variable)
	switch (Entry.Verbosity)
	{
	case EStructuredLogVerbosity::Warning:
		UE_LOG(LogStructuredLoggingSubsystem, Warning, TEXT("%s"), *LogMessage);
		break;
	case EStructuredLogVerbosity::Error:
		UE_LOG(LogStructuredLoggingSubsystem, Error, TEXT("%s"), *LogMessage);
		break;
	case EStructuredLogVerbosity::Fatal:
		UE_LOG(LogStructuredLoggingSubsystem, Fatal, TEXT("%s"), *LogMessage);
		break;
	case EStructuredLogVerbosity::Verbose:
		UE_LOG(LogStructuredLoggingSubsystem, Verbose, TEXT("%s"), *LogMessage);
		break;
	default: // Display
		UE_LOG(LogStructuredLoggingSubsystem, Display, TEXT("%s"), *LogMessage);
		break;
	}
}
