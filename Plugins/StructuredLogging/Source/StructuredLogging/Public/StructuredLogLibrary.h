// ============================================================================
// StructuredLogLibrary.h
// Developer: Marcus Daley
// Date: January 25, 2026
// Plugin: StructuredLogging
// ============================================================================
// Purpose:
// Blueprint Function Library providing structured logging for designers
// and Blueprint-only projects. Exposes subsystem functionality to Blueprints.
//
// Usage (Blueprint):
// 1. Create metadata map: Make Metadata
// 2. Add values: Add Metadata String/Float/Int/Bool
// 3. Log event: Log Event (channel, event name, metadata)
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "StructuredLogTypes.h"
#include "StructuredLogLibrary.generated.h"

/**
 * Blueprint Function Library for Structured Logging
 */
UCLASS()
class STRUCTUREDLOGGING_API UStructuredLogLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ========================================================================
	// Primary Logging Functions
	// ========================================================================

	/**
	 * Log an event with metadata
	 *
	 * @param WorldContextObject - World context (usually Self in Blueprint)
	 * @param Channel - System/feature name (e.g., "Gameplay", "UI")
	 * @param EventName - What happened (e.g., "PlayerDied", "ButtonClicked")
	 * @param Metadata - Key-value pairs (create with Make Metadata helpers)
	 * @param Verbosity - Log level (Display, Warning, Error)
	 */
	UFUNCTION(BlueprintCallable, Category = "Structured Logging",
		meta = (WorldContext = "WorldContextObject", DisplayName = "Log Event"))
	static void LogEvent(
		UObject* WorldContextObject,
		const FString& Channel,
		const FString& EventName,
		const TMap<FString, FString>& Metadata,
		EStructuredLogVerbosity Verbosity = EStructuredLogVerbosity::Display
	);

	/**
	 * Log a warning event
	 */
	UFUNCTION(BlueprintCallable, Category = "Structured Logging",
		meta = (WorldContext = "WorldContextObject", DisplayName = "Log Warning"))
	static void LogWarning(
		UObject* WorldContextObject,
		const FString& Channel,
		const FString& EventName,
		const TMap<FString, FString>& Metadata
	);

	/**
	 * Log an error event
	 */
	UFUNCTION(BlueprintCallable, Category = "Structured Logging",
		meta = (WorldContext = "WorldContextObject", DisplayName = "Log Error"))
	static void LogError(
		UObject* WorldContextObject,
		const FString& Channel,
		const FString& EventName,
		const TMap<FString, FString>& Metadata
	);

	// ========================================================================
	// Metadata Helpers
	// ========================================================================

	/**
	 * Create an empty metadata map
	 */
	UFUNCTION(BlueprintPure, Category = "Structured Logging",
		meta = (DisplayName = "Make Metadata"))
	static TMap<FString, FString> MakeMetadata();

	/**
	 * Add a string value to metadata
	 */
	UFUNCTION(BlueprintPure, Category = "Structured Logging",
		meta = (DisplayName = "Add Metadata String"))
	static TMap<FString, FString> AddMetadataString(
		const TMap<FString, FString>& Metadata,
		const FString& Key,
		const FString& Value
	);

	/**
	 * Add a float value to metadata
	 */
	UFUNCTION(BlueprintPure, Category = "Structured Logging",
		meta = (DisplayName = "Add Metadata Float"))
	static TMap<FString, FString> AddMetadataFloat(
		const TMap<FString, FString>& Metadata,
		const FString& Key,
		float Value
	);

	/**
	 * Add an integer value to metadata
	 */
	UFUNCTION(BlueprintPure, Category = "Structured Logging",
		meta = (DisplayName = "Add Metadata Int"))
	static TMap<FString, FString> AddMetadataInt(
		const TMap<FString, FString>& Metadata,
		const FString& Key,
		int32 Value
	);

	/**
	 * Add a boolean value to metadata
	 */
	UFUNCTION(BlueprintPure, Category = "Structured Logging",
		meta = (DisplayName = "Add Metadata Bool"))
	static TMap<FString, FString> AddMetadataBool(
		const TMap<FString, FString>& Metadata,
		const FString& Key,
		bool Value
	);

	/**
	 * Add an actor name to metadata (automatically extracts actor name)
	 */
	UFUNCTION(BlueprintPure, Category = "Structured Logging",
		meta = (DisplayName = "Add Metadata Actor"))
	static TMap<FString, FString> AddMetadataActor(
		const TMap<FString, FString>& Metadata,
		const FString& Key,
		AActor* Actor
	);
};
