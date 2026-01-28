// ============================================================================
// StructuredLogTypes.h
// Developer: Marcus Daley
// Date: January 25, 2026
// Plugin: StructuredLogging
// ============================================================================
// Purpose:
// Core type definitions for the Structured Logging system. Defines log entry
// structure, verbosity levels, and metadata containers.
//
// Usage:
// Include this header when working with structured log data structures.
// Most developers will use StructuredLoggingMacros.h instead for logging.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "StructuredLogTypes.generated.h"

// ============================================================================
// Verbosity Levels (mirrors UE's ELogVerbosity but Blueprint-friendly)
// ============================================================================

UENUM(BlueprintType)
enum class EStructuredLogVerbosity : uint8
{
	// Normal successful events
	Display UMETA(DisplayName = "Display"),

	// Unexpected but recoverable conditions
	Warning UMETA(DisplayName = "Warning"),

	// Critical failures requiring attention
	Error UMETA(DisplayName = "Error"),

	// Fatal errors (will be logged but won't crash)
	Fatal UMETA(DisplayName = "Fatal"),

	// Detailed tracing (high verbosity, disabled by default)
	Verbose UMETA(DisplayName = "Verbose")
};

// ============================================================================
// Log Entry Context (auto-populated by subsystem)
// ============================================================================

USTRUCT(BlueprintType)
struct STRUCTUREDLOGGING_API FStructuredLogContext
{
	GENERATED_BODY()

	// Actor that initiated the log (if applicable)
	UPROPERTY(BlueprintReadOnly, Category = "Structured Logging")
	FString ActorName;

	// Actor's class name
	UPROPERTY(BlueprintReadOnly, Category = "Structured Logging")
	FString ActorClass;

	// World/level name
	UPROPERTY(BlueprintReadOnly, Category = "Structured Logging")
	FString WorldName;

	// Subsystem name (if logged from subsystem)
	UPROPERTY(BlueprintReadOnly, Category = "Structured Logging")
	FString SubsystemName;

	// Source file (populated via __FILE__ macro in C++)
	UPROPERTY(BlueprintReadOnly, Category = "Structured Logging")
	FString SourceFile;

	// Source line number (populated via __LINE__ macro in C++)
	UPROPERTY(BlueprintReadOnly, Category = "Structured Logging")
	int32 SourceLine;

	FStructuredLogContext()
		: SourceLine(0)
	{
	}
};

// ============================================================================
// Log Entry (complete log event with metadata)
// ============================================================================

USTRUCT(BlueprintType)
struct STRUCTUREDLOGGING_API FStructuredLogEntry
{
	GENERATED_BODY()

	// ISO 8601 timestamp with milliseconds
	UPROPERTY(BlueprintReadOnly, Category = "Structured Logging")
	FString Timestamp;

	// Session GUID (links all logs from same game session)
	UPROPERTY(BlueprintReadOnly, Category = "Structured Logging")
	FGuid SessionGuid;

	// Event name (what happened, e.g., "BroomMounted", "BlackboardKeyNotSet")
	UPROPERTY(BlueprintReadOnly, Category = "Structured Logging")
	FName EventName;

	// Channel/category (e.g., "AI", "Perception", "BroomComponent")
	UPROPERTY(BlueprintReadOnly, Category = "Structured Logging")
	FName Channel;

	// Verbosity level
	UPROPERTY(BlueprintReadOnly, Category = "Structured Logging")
	EStructuredLogVerbosity Verbosity;

	// Auto-populated context
	UPROPERTY(BlueprintReadOnly, Category = "Structured Logging")
	FStructuredLogContext Context;

	// User-provided metadata (key-value pairs, all values stored as strings)
	UPROPERTY(BlueprintReadOnly, Category = "Structured Logging")
	TMap<FString, FString> Metadata;

	FStructuredLogEntry()
		: Verbosity(EStructuredLogVerbosity::Display)
	{
	}
};

// ============================================================================
// Session Metadata (written to session_metadata.json)
// ============================================================================

USTRUCT(BlueprintType)
struct STRUCTUREDLOGGING_API FStructuredLogSessionMetadata
{
	GENERATED_BODY()

	// Unique session identifier
	UPROPERTY(BlueprintReadOnly, Category = "Structured Logging")
	FGuid SessionGuid;

	// Session start time (ISO 8601)
	UPROPERTY(BlueprintReadOnly, Category = "Structured Logging")
	FString SessionStartTime;

	// Project name
	UPROPERTY(BlueprintReadOnly, Category = "Structured Logging")
	FString ProjectName;

	// Engine version
	UPROPERTY(BlueprintReadOnly, Category = "Structured Logging")
	FString EngineVersion;

	// Build configuration (Development, Editor, Shipping)
	UPROPERTY(BlueprintReadOnly, Category = "Structured Logging")
	FString BuildConfiguration;

	// Platform (Win64, Mac, Linux)
	UPROPERTY(BlueprintReadOnly, Category = "Structured Logging")
	FString Platform;

	FStructuredLogSessionMetadata()
	{
	}
};

// ============================================================================
// Helper Functions
// ============================================================================

namespace StructuredLogUtils
{
	// Convert ELogVerbosity::Type (UE native) to EStructuredLogVerbosity (Blueprint-friendly)
	inline EStructuredLogVerbosity ConvertVerbosity(ELogVerbosity::Type NativeVerbosity)
	{
		switch (NativeVerbosity)
		{
		case ELogVerbosity::Warning:
			return EStructuredLogVerbosity::Warning;
		case ELogVerbosity::Error:
			return EStructuredLogVerbosity::Error;
		case ELogVerbosity::Fatal:
			return EStructuredLogVerbosity::Fatal;
		case ELogVerbosity::Verbose:
		case ELogVerbosity::VeryVerbose:
			return EStructuredLogVerbosity::Verbose;
		default:
			return EStructuredLogVerbosity::Display;
		}
	}

	// Convert EStructuredLogVerbosity back to native UE verbosity
	inline ELogVerbosity::Type ConvertToNativeVerbosity(EStructuredLogVerbosity Verbosity)
	{
		switch (Verbosity)
		{
		case EStructuredLogVerbosity::Warning:
			return ELogVerbosity::Warning;
		case EStructuredLogVerbosity::Error:
			return ELogVerbosity::Error;
		case EStructuredLogVerbosity::Fatal:
			return ELogVerbosity::Fatal;
		case EStructuredLogVerbosity::Verbose:
			return ELogVerbosity::Verbose;
		default:
			return ELogVerbosity::Display;
		}
	}

	// Get verbosity as string for JSON output
	inline FString VerbosityToString(EStructuredLogVerbosity Verbosity)
	{
		switch (Verbosity)
		{
		case EStructuredLogVerbosity::Warning:
			return TEXT("Warning");
		case EStructuredLogVerbosity::Error:
			return TEXT("Error");
		case EStructuredLogVerbosity::Fatal:
			return TEXT("Fatal");
		case EStructuredLogVerbosity::Verbose:
			return TEXT("Verbose");
		default:
			return TEXT("Display");
		}
	}

	// Get current timestamp in ISO 8601 format with milliseconds
	inline FString GetISO8601Timestamp()
	{
		const FDateTime Now = FDateTime::UtcNow();
		return FString::Printf(TEXT("%04d-%02d-%02dT%02d:%02d:%02d.%03dZ"),
			Now.GetYear(), Now.GetMonth(), Now.GetDay(),
			Now.GetHour(), Now.GetMinute(), Now.GetSecond(),
			Now.GetMillisecond());
	}
}
