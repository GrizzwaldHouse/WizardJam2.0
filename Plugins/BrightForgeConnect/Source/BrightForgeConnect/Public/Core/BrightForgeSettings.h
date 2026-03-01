// ============================================================================
// BrightForgeSettings.h
// Developer: Marcus Daley
// Date: February 2026
// Project: BrightForgeConnect Plugin
// ============================================================================
// Purpose:
// User-configurable settings for BrightForge Connect.
// Exposed through the Project Settings panel under Plugins > BrightForge Connect.
//
// Architecture:
// UDeveloperSettings subclass. Settings are per-project and stored in Config/DefaultEditor.ini.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Core/BrightForgeTypes.h"
#include "BrightForgeSettings.generated.h"

// Log category for settings operations
DECLARE_LOG_CATEGORY_EXTERN(LogBrightForgeSettings, Log, All);

UCLASS(Config = Editor, DefaultConfig, meta = (DisplayName = "BrightForge Connect"))
class BRIGHTFORGECONNECT_API UBrightForgeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UBrightForgeSettings();

	// ========================================================================
	// CONNECTION SETTINGS
	// ========================================================================

	/** URL of the BrightForge REST API server */
	UPROPERTY(Config, EditDefaultsOnly, Category = "Connection",
		meta = (DisplayName = "Server URL"))
	FString ServerUrl;

	// ========================================================================
	// IMPORT SETTINGS
	// ========================================================================

	/** Content Browser path where generated assets will be imported */
	UPROPERTY(Config, EditDefaultsOnly, Category = "Import",
		meta = (DisplayName = "Default Import Path"))
	FDirectoryPath DefaultImportPath;

	/** Automatically apply a default material to imported static meshes */
	UPROPERTY(Config, EditDefaultsOnly, Category = "Import",
		meta = (DisplayName = "Auto-Apply Default Material"))
	bool bAutoApplyDefaultMaterial;

	/** Automatically open the imported asset in the asset editor after import */
	UPROPERTY(Config, EditDefaultsOnly, Category = "Import",
		meta = (DisplayName = "Auto-Open Imported Asset"))
	bool bAutoOpenImportedAsset;

	// ========================================================================
	// GENERATION SETTINGS
	// ========================================================================

	/** Default generation type when opening a new request */
	UPROPERTY(Config, EditDefaultsOnly, Category = "Generation",
		meta = (DisplayName = "Default Generation Type"))
	EBrightForgeGenerationType DefaultGenerationType;

	/** How often to poll the server for generation progress (milliseconds) */
	UPROPERTY(Config, EditDefaultsOnly, Category = "Generation",
		meta = (DisplayName = "Status Polling Interval (ms)", ClampMin = "500", ClampMax = "10000"))
	int32 StatusPollingIntervalMs;

	// ========================================================================
	// METHODS
	// ========================================================================

	/** Get the settings singleton */
	static UBrightForgeSettings* Get();

	// UDeveloperSettings interface
	virtual FName GetContainerName() const override { return TEXT("Project"); }
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
	virtual FName GetSectionName() const override { return TEXT("BrightForge Connect"); }

#if WITH_EDITOR
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
