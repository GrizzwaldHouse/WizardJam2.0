// ============================================================================
// BrightForgeImporter.h
// Developer: Marcus Daley
// Date: February 2026
// Project: BrightForgeConnect Plugin
// ============================================================================
// Purpose:
// Handles downloading FBX data from BrightForge and importing it into the
// Unreal Content Browser via UAssetImportTask + FAssetToolsModule.
//
// Architecture:
// UObject (not a subsystem — it is instantiated on demand by the panel or
// the client subsystem). Talks to UBrightForgeClientSubsystem for downloads.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BrightForgeImporter.generated.h"

// Log category
DECLARE_LOG_CATEGORY_EXTERN(LogBrightForgeImporter, Log, All);

/** Broadcast on successful import, carrying the UObject path of the imported asset */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnImportComplete, const FString& /*AssetPath*/);

/** Broadcast when import fails */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnImportFailed, const FString& /*ErrorMessage*/);

UCLASS()
class BRIGHTFORGECONNECT_API UBrightForgeImporter : public UObject
{
	GENERATED_BODY()

public:
	UBrightForgeImporter();

	// ========================================================================
	// PUBLIC API
	// ========================================================================

	/**
	 * Download the FBX for the given session from BrightForge, save it to the
	 * staging directory, then import it into the Content Browser.
	 *
	 * @param SessionId  The generation session to download
	 * @param AssetName  Name to give the imported static mesh (without prefix)
	 */
	void DownloadAndImport(const FString& SessionId, const FString& AssetName);

	/**
	 * Import an FBX file that is already on disk into the Content Browser.
	 *
	 * @param FbxFilePath  Absolute path to the FBX file on disk
	 * @param DestPath     Content Browser destination path (e.g. /Game/BrightForge/Generated)
	 * @param AssetName    Name for the imported asset (without prefix; SM_BF_ will be prepended)
	 * @return             True on success
	 */
	bool ImportFbxAsset(const FString& FbxFilePath, const FString& DestPath, const FString& AssetName);

	/**
	 * Returns the staging directory where downloaded FBX files are cached before import.
	 * Typically: {ProjectIntermediateDir}/BrightForge/
	 */
	static FString GetStagingDirectory();

	// ========================================================================
	// DELEGATES
	// ========================================================================

	/** Broadcast when an import completes successfully */
	FOnImportComplete OnImportComplete;

	/** Broadcast when an import fails */
	FOnImportFailed OnImportFailed;

private:
	// Pending import data (held between download callback and import call)
	FString PendingAssetName;
	FString PendingSessionId;
};
