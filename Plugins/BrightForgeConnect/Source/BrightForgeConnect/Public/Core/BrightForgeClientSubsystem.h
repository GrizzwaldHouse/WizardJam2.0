// ============================================================================
// BrightForgeClientSubsystem.h
// Developer: Marcus Daley
// Date: February 2026
// Project: BrightForgeConnect Plugin
// ============================================================================
// Purpose:
// Editor Subsystem that owns all HTTP communication with the BrightForge REST API.
// Provides delegates that other systems (UI, importer) subscribe to.
//
// Architecture:
// UEditorSubsystem for editor-lifetime persistence.
// Timer-based polling via GEditor->GetTimerManager() (NOT FTickableEditorObject).
// All HTTP calls are fully async via delegates — editor thread is never blocked.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "Core/BrightForgeTypes.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "BrightForgeClientSubsystem.generated.h"

// Log category
DECLARE_LOG_CATEGORY_EXTERN(LogBrightForgeClient, Log, All);

UCLASS()
class BRIGHTFORGECONNECT_API UBrightForgeClientSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	UBrightForgeClientSubsystem();

	// ========================================================================
	// USubsystem Interface
	// ========================================================================

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ========================================================================
	// API METHODS
	// ========================================================================

	/** GET /api/health — check server availability, broadcasts OnConnectionStateChanged */
	void CheckServerHealth();

	/**
	 * POST /api/forge3d/generate — start a generation job.
	 * @param Prompt    Text description for asset generation
	 * @param Type      Generation type (Full / Mesh / Image)
	 * @param ProjectId Optional project ID to associate with
	 * @return The session ID if the request was sent (empty on failure)
	 */
	FString GenerateAsset(const FString& Prompt, EBrightForgeGenerationType Type, const FString& ProjectId = TEXT(""));

	/** GET /api/forge3d/status/:id — begin timer-based polling for generation progress */
	void StartPolling(const FString& SessionId);

	/** Stop any active polling timer */
	void StopPolling();

	/** GET /api/forge3d/download/:id?format=fbx — save file to Intermediate/BrightForge/ */
	void DownloadFbx(const FString& SessionId);

	/** GET /api/forge3d/projects — list available BrightForge projects */
	void ListProjects();

	/** GET /api/forge3d/fbx-status — check FBX converter availability on the server */
	void GetFbxStatus();

	// ========================================================================
	// STATE QUERIES
	// ========================================================================

	/** Returns the current connection state */
	EBrightForgeConnectionState GetConnectionState() const { return ConnectionState; }

	/** Returns true if a generation is currently in progress */
	bool IsGenerating() const { return bIsGenerating; }

	/** Returns the ID of the currently active generation session (empty if none) */
	FString GetActiveSessionId() const { return ActiveSessionId; }

	/** Returns the list of projects fetched by the last ListProjects() call */
	TArray<FBrightForgeProject> GetCachedProjects() const { return CachedProjects; }

	/** Returns whether the FBX converter is available on the server */
	bool IsFbxConverterAvailable() const { return bFbxConverterAvailable; }

	// ========================================================================
	// DELEGATES
	// ========================================================================

	/** Broadcast whenever the server connection state changes */
	FOnConnectionStateChanged OnConnectionStateChanged;

	/** Broadcast when a generation job completes successfully */
	FOnGenerationComplete OnGenerationComplete;

	/** Broadcast with progress updates during generation */
	FOnGenerationProgress OnGenerationProgress;

	/** Broadcast when a generation job fails */
	FOnGenerationFailed OnGenerationFailed;

private:
	// ========================================================================
	// HTTP RESPONSE HANDLERS
	// ========================================================================

	void OnHealthResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void OnGenerateResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void OnStatusPollResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void OnDownloadFbxResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void OnProjectsResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void OnFbxStatusResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

	// ========================================================================
	// POLLING
	// ========================================================================

	/** Called by the repeating timer to poll generation status */
	void PollStatus();

	FTimerHandle PollingTimerHandle;

	// ========================================================================
	// HELPERS
	// ========================================================================

	/** Build a fully-qualified URL from the configured server base URL and a path */
	FString BuildUrl(const FString& Path) const;

	/** Set connection state and broadcast if it changed */
	void SetConnectionState(EBrightForgeConnectionState NewState);

	/** Parse a FBrightForgeGenerationStatus from a JSON object */
	bool ParseGenerationStatus(TSharedPtr<class FJsonObject> JsonObject, FBrightForgeGenerationStatus& OutStatus) const;

	/** Save raw FBX bytes to the staging directory and return the file path */
	FString SaveFbxToStaging(const FString& SessionId, const TArray<uint8>& Data) const;

	// ========================================================================
	// STATE
	// ========================================================================

	EBrightForgeConnectionState ConnectionState;
	bool bIsGenerating;
	FString ActiveSessionId;
	TArray<FBrightForgeProject> CachedProjects;
	bool bFbxConverterAvailable;

	// Pending download tracking (SessionId -> expected download)
	FString PendingDownloadSessionId;
};
