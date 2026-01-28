// ============================================================================
// StructuredLogLibrary.cpp
// Developer: Marcus Daley
// Date: January 25, 2026
// Plugin: StructuredLogging
// ============================================================================

#include "StructuredLogLibrary.h"
#include "StructuredLoggingSubsystem.h"
#include "GameFramework/Actor.h"

// ============================================================================
// Primary Logging Functions
// ============================================================================

void UStructuredLogLibrary::LogEvent(
	UObject* WorldContextObject,
	const FString& Channel,
	const FString& EventName,
	const TMap<FString, FString>& Metadata,
	EStructuredLogVerbosity Verbosity)
{
	if (UStructuredLoggingSubsystem* SLog = UStructuredLoggingSubsystem::Get(WorldContextObject))
	{
		SLog->LogEvent(
			WorldContextObject,
			FName(*Channel),
			FName(*EventName),
			Verbosity,
			Metadata
		);
	}
}

void UStructuredLogLibrary::LogWarning(
	UObject* WorldContextObject,
	const FString& Channel,
	const FString& EventName,
	const TMap<FString, FString>& Metadata)
{
	LogEvent(WorldContextObject, Channel, EventName, Metadata, EStructuredLogVerbosity::Warning);
}

void UStructuredLogLibrary::LogError(
	UObject* WorldContextObject,
	const FString& Channel,
	const FString& EventName,
	const TMap<FString, FString>& Metadata)
{
	LogEvent(WorldContextObject, Channel, EventName, Metadata, EStructuredLogVerbosity::Error);
}

// ============================================================================
// Metadata Helpers
// ============================================================================

TMap<FString, FString> UStructuredLogLibrary::MakeMetadata()
{
	return TMap<FString, FString>();
}

TMap<FString, FString> UStructuredLogLibrary::AddMetadataString(
	const TMap<FString, FString>& Metadata,
	const FString& Key,
	const FString& Value)
{
	TMap<FString, FString> NewMetadata = Metadata;
	NewMetadata.Add(Key, Value);
	return NewMetadata;
}

TMap<FString, FString> UStructuredLogLibrary::AddMetadataFloat(
	const TMap<FString, FString>& Metadata,
	const FString& Key,
	float Value)
{
	TMap<FString, FString> NewMetadata = Metadata;
	NewMetadata.Add(Key, FString::SanitizeFloat(Value));
	return NewMetadata;
}

TMap<FString, FString> UStructuredLogLibrary::AddMetadataInt(
	const TMap<FString, FString>& Metadata,
	const FString& Key,
	int32 Value)
{
	TMap<FString, FString> NewMetadata = Metadata;
	NewMetadata.Add(Key, FString::FromInt(Value));
	return NewMetadata;
}

TMap<FString, FString> UStructuredLogLibrary::AddMetadataBool(
	const TMap<FString, FString>& Metadata,
	const FString& Key,
	bool Value)
{
	TMap<FString, FString> NewMetadata = Metadata;
	NewMetadata.Add(Key, Value ? TEXT("true") : TEXT("false"));
	return NewMetadata;
}

TMap<FString, FString> UStructuredLogLibrary::AddMetadataActor(
	const TMap<FString, FString>& Metadata,
	const FString& Key,
	AActor* Actor)
{
	TMap<FString, FString> NewMetadata = Metadata;
	if (Actor)
	{
		NewMetadata.Add(Key, Actor->GetName());
	}
	else
	{
		NewMetadata.Add(Key, TEXT("NULL"));
	}
	return NewMetadata;
}
