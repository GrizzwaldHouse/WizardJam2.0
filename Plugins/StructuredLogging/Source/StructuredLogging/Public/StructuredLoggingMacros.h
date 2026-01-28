// ============================================================================
// StructuredLoggingMacros.h
// Developer: Marcus Daley
// Date: January 25, 2026
// Plugin: StructuredLogging
// ============================================================================
// Purpose:
// C++ macro API for structured logging. Provides convenient, UE_LOG-like
// syntax that automatically captures source file and line number.
//
// Usage:
// #include "StructuredLoggingMacros.h"
//
// SLOG_EVENT(this, "AI", "ControllerPossessed", {{"pawn", "BP_Agent_3"}})
// SLOG_WARNING(this, "Perception", "ActorLost", {{"actor", TargetName}})
// SLOG_ERROR(this, "Blackboard", "KeyNotSet", {{"key", "TargetLocation"}})
//
// Shipping builds: Macros expand to no-ops (zero runtime cost)
// ============================================================================

#pragma once

#include "StructuredLoggingSubsystem.h"

// ============================================================================
// Configuration Check
// ============================================================================

#ifndef STRUCTURED_LOGGING_ENABLED
	// Default to enabled in non-shipping builds
	#if UE_BUILD_SHIPPING
		#define STRUCTURED_LOGGING_ENABLED 0
	#else
		#define STRUCTURED_LOGGING_ENABLED 1
	#endif
#endif

// ============================================================================
// Core Logging Macros
// ============================================================================

#if STRUCTURED_LOGGING_ENABLED

/**
 * Log a successful event (Display verbosity)
 *
 * @param ContextObject - Object providing context (usually 'this')
 * @param Channel - System/feature name (e.g., "AI", "Perception")
 * @param EventName - What happened (e.g., "BroomMounted", "ActorPerceived")
 * @param Metadata - TMap<FString, FString> key-value pairs (optional)
 *
 * Example:
 * SLOG_EVENT(this, "BroomComponent", "BroomMounted", {
 *     {"stamina", FString::SanitizeFloat(CurrentStamina)},
 *     {"broom_type", "Combat"}
 * })
 */
#define SLOG_EVENT(ContextObject, Channel, EventName, ...) \
	{ \
		if (UStructuredLoggingSubsystem* SLogSubsystem = UStructuredLoggingSubsystem::Get(ContextObject)) \
		{ \
			TMap<FString, FString> Metadata; \
			__VA_ARGS__; \
			SLogSubsystem->LogEvent( \
				ContextObject, \
				FName(Channel), \
				FName(EventName), \
				EStructuredLogVerbosity::Display, \
				Metadata, \
				FString(__FILE__), \
				__LINE__ \
			); \
		} \
	}

/**
 * Log a warning (unexpected but recoverable condition)
 *
 * Use for validation failures, performance concerns, deprecated code paths.
 *
 * Example:
 * SLOG_WARNING(this, "AI", "BlackboardKeyNotSet", {
 *     {"key_name", "TargetLocation"},
 *     {"node_name", "BTService_FindCollectible"}
 * })
 */
#define SLOG_WARNING(ContextObject, Channel, EventName, ...) \
	{ \
		if (UStructuredLoggingSubsystem* SLogSubsystem = UStructuredLoggingSubsystem::Get(ContextObject)) \
		{ \
			TMap<FString, FString> Metadata; \
			__VA_ARGS__; \
			SLogSubsystem->LogEvent( \
				ContextObject, \
				FName(Channel), \
				FName(EventName), \
				EStructuredLogVerbosity::Warning, \
				Metadata, \
				FString(__FILE__), \
				__LINE__ \
			); \
		} \
	}

/**
 * Log an error (critical failure requiring attention)
 *
 * Use for operations that should succeed but failed, invalid state indicating bugs.
 *
 * Example:
 * SLOG_ERROR(this, "AI", "BehaviorTreeStartFailed", {
 *     {"tree_name", BTAsset->GetName()},
 *     {"controller", GetName()}
 * })
 */
#define SLOG_ERROR(ContextObject, Channel, EventName, ...) \
	{ \
		if (UStructuredLoggingSubsystem* SLogSubsystem = UStructuredLoggingSubsystem::Get(ContextObject)) \
		{ \
			TMap<FString, FString> Metadata; \
			__VA_ARGS__; \
			SLogSubsystem->LogEvent( \
				ContextObject, \
				FName(Channel), \
				FName(EventName), \
				EStructuredLogVerbosity::Error, \
				Metadata, \
				FString(__FILE__), \
				__LINE__ \
			); \
		} \
	}

/**
 * Log a fatal error (unrecoverable state)
 *
 * Note: This logs the error but doesn't crash. Use native ensure()/check() for actual crashes.
 *
 * Example:
 * SLOG_FATAL(this, "Core", "SubsystemInitializationFailed", {
 *     {"subsystem", "ElementDatabase"}
 * })
 */
#define SLOG_FATAL(ContextObject, Channel, EventName, ...) \
	{ \
		if (UStructuredLoggingSubsystem* SLogSubsystem = UStructuredLoggingSubsystem::Get(ContextObject)) \
		{ \
			TMap<FString, FString> Metadata; \
			__VA_ARGS__; \
			SLogSubsystem->LogEvent( \
				ContextObject, \
				FName(Channel), \
				FName(EventName), \
				EStructuredLogVerbosity::Fatal, \
				Metadata, \
				FString(__FILE__), \
				__LINE__ \
			); \
		} \
	}

/**
 * Log a verbose trace (high verbosity, disabled by default)
 *
 * Use for detailed debugging of specific systems. Enable per-channel via config.
 *
 * Example:
 * SLOG_VERBOSE(this, "Blackboard", "KeyValueRead", {
 *     {"key", "TargetLocation"},
 *     {"value", Location.ToString()}
 * })
 */
#define SLOG_VERBOSE(ContextObject, Channel, EventName, ...) \
	{ \
		if (UStructuredLoggingSubsystem* SLogSubsystem = UStructuredLoggingSubsystem::Get(ContextObject)) \
		{ \
			TMap<FString, FString> Metadata; \
			__VA_ARGS__; \
			SLogSubsystem->LogEvent( \
				ContextObject, \
				FName(Channel), \
				FName(EventName), \
				EStructuredLogVerbosity::Verbose, \
				Metadata, \
				FString(__FILE__), \
				__LINE__ \
			); \
		} \
	}

// ============================================================================
// Utility Macros
// ============================================================================

/**
 * Log metadata helper - simplifies creating metadata maps
 *
 * Example:
 * SLOG_METADATA_BEGIN(Metadata)
 *     SLOG_METADATA_ADD(Metadata, "stamina", FString::SanitizeFloat(Stamina))
 *     SLOG_METADATA_ADD(Metadata, "broom_type", "Combat")
 * SLOG_METADATA_END(Metadata)
 * SLogSubsystem->LogEvent(this, "Flight", "BroomMounted", EStructuredLogVerbosity::Display, Metadata);
 */
#define SLOG_METADATA_BEGIN(VarName) \
	TMap<FString, FString> VarName;

#define SLOG_METADATA_ADD(VarName, Key, Value) \
	VarName.Add(FString(Key), FString(Value));

#define SLOG_METADATA_END(VarName)

/**
 * Scoped timer for performance profiling
 *
 * Logs event with duration_ms metadata when scope exits.
 *
 * Example:
 * void ExpensiveFunction()
 * {
 *     SLOG_SCOPE_TIMER(this, "Performance", "ExpensiveFunctionDuration");
 *     // ... complex logic ...
 * }  // Auto-logs ScopeTimerEnd with duration_ms
 */
#define SLOG_SCOPE_TIMER(ContextObject, Channel, ScopeName) \
	FScopedStructuredLogTimer ANONYMOUS_VARIABLE(SLogTimer_)( \
		ContextObject, FName(Channel), FName(ScopeName), FString(__FILE__), __LINE__);

// ============================================================================
// Scoped Timer Implementation (RAII pattern)
// ============================================================================

/**
 * RAII helper class for scoped timing
 */
class FScopedStructuredLogTimer
{
public:
	FScopedStructuredLogTimer(
		const UObject* InContextObject,
		FName InChannel,
		FName InScopeName,
		const FString& InSourceFile,
		int32 InSourceLine)
		: ContextObject(InContextObject)
		, Channel(InChannel)
		, ScopeName(InScopeName)
		, SourceFile(InSourceFile)
		, SourceLine(InSourceLine)
		, StartTime(FPlatformTime::Seconds())
	{
		// Log scope start
		if (UStructuredLoggingSubsystem* SLog = UStructuredLoggingSubsystem::Get(ContextObject))
		{
			TMap<FString, FString> Metadata;
			Metadata.Add(TEXT("scope_name"), ScopeName.ToString());
			SLog->LogEvent(
				ContextObject,
				Channel,
				FName(TEXT("ScopeTimerStart")),
				EStructuredLogVerbosity::Verbose,
				Metadata,
				SourceFile,
				SourceLine
			);
		}
	}

	~FScopedStructuredLogTimer()
	{
		// Log scope end with duration
		const double EndTime = FPlatformTime::Seconds();
		const double DurationMs = (EndTime - StartTime) * 1000.0;

		if (UStructuredLoggingSubsystem* SLog = UStructuredLoggingSubsystem::Get(ContextObject))
		{
			TMap<FString, FString> Metadata;
			Metadata.Add(TEXT("scope_name"), ScopeName.ToString());
			Metadata.Add(TEXT("duration_ms"), FString::SanitizeFloat(DurationMs));
			SLog->LogEvent(
				ContextObject,
				Channel,
				FName(TEXT("ScopeTimerEnd")),
				EStructuredLogVerbosity::Display,
				Metadata,
				SourceFile,
				SourceLine
			);
		}
	}

private:
	const UObject* ContextObject;
	FName Channel;
	FName ScopeName;
	FString SourceFile;
	int32 SourceLine;
	double StartTime;
};

#else  // STRUCTURED_LOGGING_ENABLED == 0

// ============================================================================
// Shipping Build Stubs (zero runtime cost)
// ============================================================================

#define SLOG_EVENT(ContextObject, Channel, EventName, ...)
#define SLOG_WARNING(ContextObject, Channel, EventName, ...)
#define SLOG_ERROR(ContextObject, Channel, EventName, ...)
#define SLOG_FATAL(ContextObject, Channel, EventName, ...)
#define SLOG_VERBOSE(ContextObject, Channel, EventName, ...)
#define SLOG_METADATA_BEGIN(VarName)
#define SLOG_METADATA_ADD(VarName, Key, Value)
#define SLOG_METADATA_END(VarName)
#define SLOG_SCOPE_TIMER(ContextObject, Channel, ScopeName)

#endif  // STRUCTURED_LOGGING_ENABLED
