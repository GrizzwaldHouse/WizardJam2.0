// ============================================================================
// BrightForgeTypes.h
// Developer: Marcus Daley
// Date: February 2026
// Project: BrightForgeConnect Plugin
// ============================================================================
// Purpose:
// Shared types, enums, structs, and delegate declarations for BrightForge Connect.
// All other plugin files include this header for common definitions.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BrightForgeTypes.generated.h"

// ============================================================================
// ENUMS
// ============================================================================

/** The type of 3D asset generation to request from BrightForge */
UENUM(BlueprintType)
enum class EBrightForgeGenerationType : uint8
{
	/** Generate mesh from text prompt using full pipeline */
	Full    UMETA(DisplayName = "Full (Text to 3D)"),
	/** Generate only the mesh geometry */
	Mesh    UMETA(DisplayName = "Mesh Only"),
	/** Generate from an image reference */
	Image   UMETA(DisplayName = "Image to 3D")
};

/** Current connection state to the BrightForge server */
UENUM(BlueprintType)
enum class EBrightForgeConnectionState : uint8
{
	/** No connection has been attempted or server is unreachable */
	Disconnected    UMETA(DisplayName = "Disconnected"),
	/** Health check request is in flight */
	Connecting      UMETA(DisplayName = "Connecting"),
	/** Server responded healthy */
	Connected       UMETA(DisplayName = "Connected"),
	/** Server responded with an error or timed out */
	Error           UMETA(DisplayName = "Error")
};

// ============================================================================
// STRUCTS
// ============================================================================

/** Tracks the state of an active or completed generation session */
USTRUCT(BlueprintType)
struct BRIGHTFORGECONNECT_API FBrightForgeGenerationStatus
{
	GENERATED_BODY()

	FBrightForgeGenerationStatus()
		: State(TEXT("pending"))
		, Type(EBrightForgeGenerationType::Full)
		, Progress(0.0f)
		, GenerationTimeMs(0)
	{
	}

	/** Unique identifier for this generation session */
	UPROPERTY(BlueprintReadOnly, Category = "BrightForge")
	FString SessionId;

	/** Current state string from server (pending, processing, complete, failed) */
	UPROPERTY(BlueprintReadOnly, Category = "BrightForge")
	FString State;

	/** Generation type requested */
	UPROPERTY(BlueprintReadOnly, Category = "BrightForge")
	EBrightForgeGenerationType Type;

	/** Progress from 0.0 to 1.0 */
	UPROPERTY(BlueprintReadOnly, Category = "BrightForge")
	float Progress;

	/** Error message if State == "failed" */
	UPROPERTY(BlueprintReadOnly, Category = "BrightForge")
	FString Error;

	/** Total generation time in milliseconds */
	UPROPERTY(BlueprintReadOnly, Category = "BrightForge")
	int32 GenerationTimeMs;

	/** Human-readable prompt used to generate this asset */
	UPROPERTY(BlueprintReadOnly, Category = "BrightForge")
	FString Prompt;
};

/** A BrightForge project entry */
USTRUCT(BlueprintType)
struct BRIGHTFORGECONNECT_API FBrightForgeProject
{
	GENERATED_BODY()

	FBrightForgeProject()
		: AssetCount(0)
	{
	}

	/** Project identifier */
	UPROPERTY(BlueprintReadOnly, Category = "BrightForge")
	FString Id;

	/** Display name for this project */
	UPROPERTY(BlueprintReadOnly, Category = "BrightForge")
	FString Name;

	/** Number of generated assets in this project */
	UPROPERTY(BlueprintReadOnly, Category = "BrightForge")
	int32 AssetCount;
};

// ============================================================================
// DELEGATES
// ============================================================================

/** Broadcast when the server connection state changes */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnConnectionStateChanged, EBrightForgeConnectionState);

/** Broadcast when a generation completes successfully; carries the session status */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnGenerationComplete, const FBrightForgeGenerationStatus&);

/** Broadcast during generation with progress updates */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnGenerationProgress, const FBrightForgeGenerationStatus&);

/** Broadcast when a generation fails */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnGenerationFailed, const FString& /*SessionId*/, const FString& /*ErrorMessage*/);
