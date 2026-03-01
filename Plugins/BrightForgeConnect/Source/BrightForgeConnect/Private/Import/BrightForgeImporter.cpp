// ============================================================================
// BrightForgeImporter.cpp
// Developer: Marcus Daley
// Date: February 2026
// Project: BrightForgeConnect Plugin
// ============================================================================
// Purpose:
// Implementation of FBX download and Content Browser import for BrightForge assets.
// ============================================================================

#include "Import/BrightForgeImporter.h"
#include "Core/BrightForgeClientSubsystem.h"
#include "Core/BrightForgeSettings.h"
#include "AssetToolsModule.h"
#include "AssetImportTask.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxStaticMeshImportData.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Editor.h"

DEFINE_LOG_CATEGORY(LogBrightForgeImporter);

UBrightForgeImporter::UBrightForgeImporter()
{
}

// ============================================================================
// PUBLIC API
// ============================================================================

void UBrightForgeImporter::DownloadAndImport(const FString& SessionId, const FString& AssetName)
{
	UE_LOG(LogBrightForgeImporter, Log, TEXT("DownloadAndImport: SessionId=%s AssetName=%s"), *SessionId, *AssetName);

	PendingSessionId = SessionId;
	PendingAssetName = AssetName;

	// Get the client subsystem to trigger the download
	if (!GEditor)
	{
		UE_LOG(LogBrightForgeImporter, Error, TEXT("GEditor is null — cannot initiate download"));
		OnImportFailed.Broadcast(TEXT("Editor not available"));
		return;
	}

	UBrightForgeClientSubsystem* Subsystem = GEditor->GetEditorSubsystem<UBrightForgeClientSubsystem>();
	if (!Subsystem)
	{
		UE_LOG(LogBrightForgeImporter, Error, TEXT("BrightForgeClientSubsystem not available"));
		OnImportFailed.Broadcast(TEXT("BrightForge subsystem not available"));
		return;
	}

	// Check for an already-staged file (avoid duplicate download)
	const FString ExpectedPath = GetStagingDirectory() / FString::Printf(TEXT("BF_%s.fbx"), *SessionId);
	if (FPaths::FileExists(ExpectedPath))
	{
		UE_LOG(LogBrightForgeImporter, Log, TEXT("FBX already staged at: %s — skipping download"), *ExpectedPath);

		const UBrightForgeSettings* Settings = UBrightForgeSettings::Get();
		const FString DestPath = Settings ? Settings->DefaultImportPath.Path : TEXT("/Game/BrightForge/Generated");

		const bool bSuccess = ImportFbxAsset(ExpectedPath, DestPath, AssetName);
		if (!bSuccess)
		{
			OnImportFailed.Broadcast(TEXT("FBX import failed"));
		}
		return;
	}

	// Request download; the subsystem will save the file to staging
	// We rely on the file being present after the download completes.
	// Subscribe to subsystem's existing download path by triggering it.
	Subsystem->DownloadFbx(SessionId);

	// Note: after DownloadFbx completes (async), the caller is responsible for
	// calling ImportFbxAsset with the staged file path. The panel triggers this flow.
}

bool UBrightForgeImporter::ImportFbxAsset(const FString& FbxFilePath, const FString& DestPath, const FString& AssetName)
{
	UE_LOG(LogBrightForgeImporter, Log, TEXT("Importing FBX: %s -> %s as SM_BF_%s"), *FbxFilePath, *DestPath, *AssetName);

	if (!FPaths::FileExists(FbxFilePath))
	{
		UE_LOG(LogBrightForgeImporter, Error, TEXT("FBX file does not exist: %s"), *FbxFilePath);
		OnImportFailed.Broadcast(FString::Printf(TEXT("File not found: %s"), *FbxFilePath));
		return false;
	}

	// Build import task
	UAssetImportTask* ImportTask = NewObject<UAssetImportTask>();
	ImportTask->Filename = FbxFilePath;
	ImportTask->DestinationPath = DestPath;
	ImportTask->DestinationName = FString::Printf(TEXT("SM_BF_%s"), *AssetName);
	ImportTask->bReplaceExisting = true;
	ImportTask->bAutomated = true;
	ImportTask->bSave = true;

	// Configure FBX import options
	UFbxImportUI* ImportUI = NewObject<UFbxImportUI>();
	ImportUI->bImportMesh = true;
	ImportUI->bImportAnimations = false;
	ImportUI->bImportMaterials = false;
	ImportUI->bImportTextures = false;

	if (ImportUI->StaticMeshImportData)
	{
		ImportUI->StaticMeshImportData->bAutoGenerateCollision = true;
		ImportUI->StaticMeshImportData->bCombineMeshes = true;
	}

	ImportTask->Options = ImportUI;

	// Run the import via AssetTools
	if (!FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		UE_LOG(LogBrightForgeImporter, Error, TEXT("AssetTools module is not loaded"));
		OnImportFailed.Broadcast(TEXT("AssetTools module unavailable"));
		return false;
	}

	FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
	TArray<UAssetImportTask*> Tasks;
	Tasks.Add(ImportTask);
	AssetToolsModule.Get().ImportAssetTasks(Tasks);

	// Verify import succeeded by checking imported objects
	if (ImportTask->ImportedObjectPaths.Num() == 0)
	{
		UE_LOG(LogBrightForgeImporter, Error, TEXT("Import produced no objects — likely an FBX parse error"));
		OnImportFailed.Broadcast(TEXT("FBX import produced no assets"));
		return false;
	}

	const FString ImportedPath = ImportTask->ImportedObjectPaths[0];
	UE_LOG(LogBrightForgeImporter, Log, TEXT("Import successful: %s"), *ImportedPath);

	// Open the imported asset if settings request it
	const UBrightForgeSettings* Settings = UBrightForgeSettings::Get();
	if (Settings && Settings->bAutoOpenImportedAsset && GEditor)
	{
		// AssetPath is in the form /Game/Path/AssetName.AssetName — load it
		UObject* LoadedAsset = StaticLoadObject(UObject::StaticClass(), nullptr, *ImportedPath);
		if (LoadedAsset)
		{
			GEditor->EditObject(LoadedAsset);
		}
	}

	OnImportComplete.Broadcast(ImportedPath);
	return true;
}

FString UBrightForgeImporter::GetStagingDirectory()
{
	return FPaths::ProjectIntermediateDir() / TEXT("BrightForge");
}
