// ============================================================================
// SecureStorageManager.cpp
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Implementation of secure data storage with tamper detection and export.
// ============================================================================

#include "Core/SecureStorageManager.h"
#include "Core/ProductivityTrackerSettings.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "HAL/PlatformMisc.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

DEFINE_LOG_CATEGORY(LogProductivityStorage);

USecureStorageManager::USecureStorageManager()
	: bIsInitialized(false)
{
}

bool USecureStorageManager::Initialize(const FString& InDataDirectory)
{
	DataDirectory = InDataDirectory;

	// Ensure the data directory exists
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*DataDirectory))
	{
		if (!PlatformFile.CreateDirectoryTree(*DataDirectory))
		{
			UE_LOG(LogProductivityStorage, Error, TEXT("Failed to create data directory: %s"), *DataDirectory);
			return false;
		}
	}

	// Create subdirectories
	FString SessionsDir = FPaths::Combine(DataDirectory, TEXT("Sessions"));
	FString SummariesDir = FPaths::Combine(DataDirectory, TEXT("Summaries"));

	PlatformFile.CreateDirectoryTree(*SessionsDir);
	PlatformFile.CreateDirectoryTree(*SummariesDir);

	// Load or create security components
	LoadOrCreateInstallationSalt();
	LoadOrCreateMachineId();

	bIsInitialized = true;

	UE_LOG(LogProductivityStorage, Log, TEXT("SecureStorageManager initialized at: %s"), *DataDirectory);

	return true;
}

void USecureStorageManager::Shutdown()
{
	UE_LOG(LogProductivityStorage, Log, TEXT("SecureStorageManager shutting down"));
	bIsInitialized = false;
}

// ============================================================================
// SESSION STORAGE
// ============================================================================

bool USecureStorageManager::SaveSession(const FSessionRecord& Session)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogProductivityStorage, Warning, TEXT("Cannot save session - storage not initialized"));
		return false;
	}

	FString FilePath = GetSessionFilePath(Session.SessionId);
	TSharedPtr<FJsonObject> JsonObject = Session.ToJson();

	if (WriteJsonToFile(JsonObject, FilePath))
	{
		UE_LOG(LogProductivityStorage, Log, TEXT("Saved session %s to %s"),
			*Session.SessionId.ToString(), *FilePath);
		return true;
	}

	UE_LOG(LogProductivityStorage, Error, TEXT("Failed to save session %s"), *Session.SessionId.ToString());
	return false;
}

bool USecureStorageManager::LoadSession(const FGuid& SessionId, FSessionRecord& OutSession)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogProductivityStorage, Warning, TEXT("Cannot load session - storage not initialized"));
		return false;
	}

	FString FilePath = GetSessionFilePath(SessionId);

	TSharedPtr<FJsonObject> JsonObject = ReadJsonFromFile(FilePath);
	if (!JsonObject.IsValid())
	{
		UE_LOG(LogProductivityStorage, Warning, TEXT("Failed to read session file: %s"), *FilePath);
		return false;
	}

	if (!FSessionRecord::FromJson(JsonObject, OutSession))
	{
		UE_LOG(LogProductivityStorage, Warning, TEXT("Failed to parse session: %s"), *SessionId.ToString());
		return false;
	}

	// Verify integrity if enabled
	const UProductivityTrackerSettings* Settings = UProductivityTrackerSettings::Get();
	if (Settings && Settings->bEnableChecksumVerification)
	{
		if (!OutSession.VerifyChecksum(InstallationSalt))
		{
			UE_LOG(LogProductivityStorage, Warning, TEXT("Session %s failed checksum verification"),
				*SessionId.ToString());

			if (Settings->bWarnOnTamperDetection)
			{
				OnDataIntegrityWarning.Broadcast(FilePath, EDataIntegrityResult::Modified);
			}
		}
	}

	return true;
}

TArray<FSessionRecord> USecureStorageManager::LoadSessionsInRange(const FDateTime& StartDate, const FDateTime& EndDate)
{
	TArray<FSessionRecord> Results;

	TArray<FGuid> AllIds = GetAllSessionIds();
	for (const FGuid& Id : AllIds)
	{
		FSessionRecord Session;
		if (LoadSession(Id, Session))
		{
			if (Session.StartTime >= StartDate && Session.StartTime <= EndDate)
			{
				Results.Add(Session);
			}
		}
	}

	// Sort by start time
	Results.Sort([](const FSessionRecord& A, const FSessionRecord& B)
	{
		return A.StartTime < B.StartTime;
	});

	UE_LOG(LogProductivityStorage, Log, TEXT("Loaded %d sessions in date range"), Results.Num());
	return Results;
}

bool USecureStorageManager::DeleteSession(const FGuid& SessionId)
{
	if (!bIsInitialized)
	{
		return false;
	}

	FString FilePath = GetSessionFilePath(SessionId);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	if (PlatformFile.DeleteFile(*FilePath))
	{
		UE_LOG(LogProductivityStorage, Log, TEXT("Deleted session: %s"), *SessionId.ToString());
		return true;
	}

	UE_LOG(LogProductivityStorage, Warning, TEXT("Failed to delete session: %s"), *SessionId.ToString());
	return false;
}

TArray<FGuid> USecureStorageManager::GetAllSessionIds()
{
	TArray<FGuid> SessionIds;

	if (!bIsInitialized)
	{
		return SessionIds;
	}

	FString SessionsDir = FPaths::Combine(DataDirectory, TEXT("Sessions"));
	TArray<FString> SessionFiles;
	IFileManager::Get().FindFiles(SessionFiles, *FPaths::Combine(SessionsDir, TEXT("*.json")), true, false);

	for (const FString& FileName : SessionFiles)
	{
		FString IdString = FPaths::GetBaseFilename(FileName);
		FGuid Id;
		if (FGuid::Parse(IdString, Id))
		{
			SessionIds.Add(Id);
		}
	}

	return SessionIds;
}

// ============================================================================
// ACTIVE SESSION PERSISTENCE
// ============================================================================

bool USecureStorageManager::SaveActiveSessionState(const FSessionRecord& Session)
{
	if (!bIsInitialized)
	{
		return false;
	}

	FString FilePath = GetActiveSessionFilePath();
	TSharedPtr<FJsonObject> JsonObject = Session.ToJson();

	return WriteJsonToFile(JsonObject, FilePath);
}

bool USecureStorageManager::LoadActiveSessionState(FSessionRecord& OutSession)
{
	if (!bIsInitialized)
	{
		return false;
	}

	FString FilePath = GetActiveSessionFilePath();

	TSharedPtr<FJsonObject> JsonObject = ReadJsonFromFile(FilePath);
	if (!JsonObject.IsValid())
	{
		return false;
	}

	if (!FSessionRecord::FromJson(JsonObject, OutSession))
	{
		return false;
	}

	OutSession.bWasRecovered = true;
	UE_LOG(LogProductivityStorage, Log, TEXT("Recovered active session: %s"), *OutSession.SessionId.ToString());
	return true;
}

void USecureStorageManager::ClearActiveSessionState()
{
	if (!bIsInitialized)
	{
		return;
	}

	FString FilePath = GetActiveSessionFilePath();
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.DeleteFile(*FilePath);
}

bool USecureStorageManager::HasRecoverableSession() const
{
	if (!bIsInitialized)
	{
		return false;
	}

	FString FilePath = GetActiveSessionFilePath();
	return FPaths::FileExists(FilePath);
}

// ============================================================================
// DAILY SUMMARY STORAGE
// ============================================================================

bool USecureStorageManager::SaveDailySummary(const FDailySummary& Summary)
{
	if (!bIsInitialized)
	{
		return false;
	}

	FString FilePath = GetDailySummaryFilePath(Summary.Date);
	TSharedPtr<FJsonObject> JsonObject = Summary.ToJson();

	return WriteJsonToFile(JsonObject, FilePath);
}

bool USecureStorageManager::LoadDailySummary(const FDateTime& Date, FDailySummary& OutSummary)
{
	if (!bIsInitialized)
	{
		return false;
	}

	FString FilePath = GetDailySummaryFilePath(Date);

	TSharedPtr<FJsonObject> JsonObject = ReadJsonFromFile(FilePath);
	if (!JsonObject.IsValid())
	{
		return false;
	}

	return FDailySummary::FromJson(JsonObject, OutSummary);
}

TArray<FDailySummary> USecureStorageManager::LoadDailySummariesInRange(const FDateTime& StartDate, const FDateTime& EndDate)
{
	TArray<FDailySummary> Results;

	FDateTime CurrentDate = StartDate;
	while (CurrentDate <= EndDate)
	{
		FDailySummary Summary;
		if (LoadDailySummary(CurrentDate, Summary))
		{
			Results.Add(Summary);
		}
		CurrentDate += FTimespan::FromDays(1);
	}

	return Results;
}

// ============================================================================
// DATA INTEGRITY
// ============================================================================

EDataIntegrityResult USecureStorageManager::VerifySessionIntegrity(const FGuid& SessionId)
{
	FString FilePath = GetSessionFilePath(SessionId);

	if (!FPaths::FileExists(FilePath))
	{
		return EDataIntegrityResult::Missing;
	}

	TSharedPtr<FJsonObject> JsonObject = ReadJsonFromFile(FilePath);
	if (!JsonObject.IsValid())
	{
		return EDataIntegrityResult::Corrupted;
	}

	FSessionRecord Session;
	if (!FSessionRecord::FromJson(JsonObject, Session))
	{
		return EDataIntegrityResult::Corrupted;
	}

	if (!Session.VerifyChecksum(InstallationSalt))
	{
		return EDataIntegrityResult::Modified;
	}

	return EDataIntegrityResult::Valid;
}

TMap<FGuid, EDataIntegrityResult> USecureStorageManager::VerifyAllDataIntegrity()
{
	TMap<FGuid, EDataIntegrityResult> Results;

	TArray<FGuid> AllIds = GetAllSessionIds();
	for (const FGuid& Id : AllIds)
	{
		Results.Add(Id, VerifySessionIntegrity(Id));
	}

	int32 ValidCount = 0;
	int32 IssueCount = 0;
	for (const auto& Pair : Results)
	{
		if (Pair.Value == EDataIntegrityResult::Valid)
		{
			ValidCount++;
		}
		else
		{
			IssueCount++;
		}
	}

	UE_LOG(LogProductivityStorage, Log, TEXT("Integrity check complete. Valid: %d, Issues: %d"),
		ValidCount, IssueCount);

	return Results;
}

// ============================================================================
// DATA EXPORT
// ============================================================================

bool USecureStorageManager::ExportSessions(const TArray<FGuid>& SessionIds, const FString& FilePath, EExportFormat Format)
{
	TArray<FSessionRecord> Sessions;
	for (const FGuid& Id : SessionIds)
	{
		FSessionRecord Session;
		if (LoadSession(Id, Session))
		{
			Sessions.Add(Session);
		}
	}

	switch (Format)
	{
	case EExportFormat::JSON:
		return ExportToJson(Sessions, FilePath);
	case EExportFormat::CSV:
		return ExportToCsv(Sessions, FilePath);
	case EExportFormat::Markdown:
		return ExportToMarkdown(Sessions, FilePath);
	default:
		return false;
	}
}

bool USecureStorageManager::ExportDateRange(const FDateTime& StartDate, const FDateTime& EndDate,
	const FString& FilePath, EExportFormat Format)
{
	TArray<FSessionRecord> Sessions = LoadSessionsInRange(StartDate, EndDate);

	switch (Format)
	{
	case EExportFormat::JSON:
		return ExportToJson(Sessions, FilePath);
	case EExportFormat::CSV:
		return ExportToCsv(Sessions, FilePath);
	case EExportFormat::Markdown:
		return ExportToMarkdown(Sessions, FilePath);
	default:
		return false;
	}
}

bool USecureStorageManager::ExportAllUserData(const FString& FilePath)
{
	TArray<FGuid> AllIds = GetAllSessionIds();
	TArray<FSessionRecord> AllSessions;

	for (const FGuid& Id : AllIds)
	{
		FSessionRecord Session;
		if (LoadSession(Id, Session))
		{
			AllSessions.Add(Session);
		}
	}

	return ExportToJson(AllSessions, FilePath);
}

// ============================================================================
// DATA CLEANUP
// ============================================================================

int32 USecureStorageManager::CleanupOldData(int32 RetentionDays)
{
	FDateTime CutoffDate = FDateTime::Now() - FTimespan::FromDays(RetentionDays);

	int32 DeletedCount = 0;
	TArray<FGuid> AllIds = GetAllSessionIds();

	for (const FGuid& Id : AllIds)
	{
		FSessionRecord Session;
		if (LoadSession(Id, Session))
		{
			if (Session.EndTime < CutoffDate)
			{
				if (DeleteSession(Id))
				{
					DeletedCount++;
				}
			}
		}
	}

	UE_LOG(LogProductivityStorage, Log, TEXT("Cleanup complete. Deleted %d sessions older than %d days"),
		DeletedCount, RetentionDays);

	return DeletedCount;
}

bool USecureStorageManager::DeleteAllData()
{
	if (!bIsInitialized)
	{
		return false;
	}

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	// Delete sessions directory
	FString SessionsDir = FPaths::Combine(DataDirectory, TEXT("Sessions"));
	if (!PlatformFile.DeleteDirectoryRecursively(*SessionsDir))
	{
		UE_LOG(LogProductivityStorage, Warning, TEXT("Failed to delete sessions directory"));
	}

	// Delete summaries directory
	FString SummariesDir = FPaths::Combine(DataDirectory, TEXT("Summaries"));
	if (!PlatformFile.DeleteDirectoryRecursively(*SummariesDir))
	{
		UE_LOG(LogProductivityStorage, Warning, TEXT("Failed to delete summaries directory"));
	}

	// Clear active session
	ClearActiveSessionState();

	// Recreate directories
	PlatformFile.CreateDirectoryTree(*SessionsDir);
	PlatformFile.CreateDirectoryTree(*SummariesDir);

	UE_LOG(LogProductivityStorage, Log, TEXT("All user data deleted"));
	return true;
}

// ============================================================================
// SECURITY
// ============================================================================

FString USecureStorageManager::GetInstallationSalt()
{
	return InstallationSalt;
}

FString USecureStorageManager::GenerateMachineIdentifier()
{
	// Combine multiple system identifiers for uniqueness
	FString MacAddress = FPlatformMisc::GetMacAddressString();
	FString LoginId = FPlatformMisc::GetLoginId();

	FString Combined = FString::Printf(TEXT("%s|%s"), *MacAddress, *LoginId);
	return FMD5::HashAnsiString(*Combined);
}

// ============================================================================
// PRIVATE HELPERS
// ============================================================================

FString USecureStorageManager::GetSessionFilePath(const FGuid& SessionId) const
{
	return FPaths::Combine(DataDirectory, TEXT("Sessions"), SessionId.ToString() + TEXT(".json"));
}

FString USecureStorageManager::GetDailySummaryFilePath(const FDateTime& Date) const
{
	FString DateString = Date.ToString(TEXT("%Y-%m-%d"));
	return FPaths::Combine(DataDirectory, TEXT("Summaries"), DateString + TEXT(".json"));
}

FString USecureStorageManager::GetActiveSessionFilePath() const
{
	return FPaths::Combine(DataDirectory, TEXT("active_session.json"));
}

FString USecureStorageManager::GetSaltFilePath() const
{
	return FPaths::Combine(DataDirectory, TEXT(".salt"));
}

FString USecureStorageManager::GetMachineIdFilePath() const
{
	return FPaths::Combine(DataDirectory, TEXT(".machine"));
}

bool USecureStorageManager::WriteJsonToFile(const TSharedPtr<FJsonObject>& JsonObject, const FString& FilePath)
{
	FString OutputString;
	TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&OutputString);

	if (!FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer))
	{
		return false;
	}

	return FFileHelper::SaveStringToFile(OutputString, *FilePath);
}

TSharedPtr<FJsonObject> USecureStorageManager::ReadJsonFromFile(const FString& FilePath)
{
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		return nullptr;
	}

	return JsonObject;
}

void USecureStorageManager::LoadOrCreateInstallationSalt()
{
	FString SaltPath = GetSaltFilePath();

	if (FFileHelper::LoadFileToString(InstallationSalt, *SaltPath))
	{
		UE_LOG(LogProductivityStorage, Verbose, TEXT("Loaded existing installation salt"));
		return;
	}

	// Generate new salt
	InstallationSalt = GenerateRandomSalt();
	FFileHelper::SaveStringToFile(InstallationSalt, *SaltPath);

	UE_LOG(LogProductivityStorage, Log, TEXT("Generated new installation salt"));
}

void USecureStorageManager::LoadOrCreateMachineId()
{
	FString MachineIdPath = GetMachineIdFilePath();

	if (FFileHelper::LoadFileToString(MachineId, *MachineIdPath))
	{
		return;
	}

	MachineId = GenerateMachineIdentifier();
	FFileHelper::SaveStringToFile(MachineId, *MachineIdPath);
}

FString USecureStorageManager::GenerateRandomSalt(int32 Length)
{
	const FString Characters = TEXT("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
	FString Salt;
	Salt.Reserve(Length);

	for (int32 i = 0; i < Length; i++)
	{
		int32 Index = FMath::RandRange(0, Characters.Len() - 1);
		Salt.AppendChar(Characters[Index]);
	}

	return Salt;
}

// ============================================================================
// EXPORT IMPLEMENTATIONS
// ============================================================================

bool USecureStorageManager::ExportToJson(const TArray<FSessionRecord>& Sessions, const FString& FilePath)
{
	TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject());

	TArray<TSharedPtr<FJsonValue>> SessionsArray;
	for (const FSessionRecord& Session : Sessions)
	{
		SessionsArray.Add(MakeShareable(new FJsonValueObject(Session.ToJson())));
	}

	RootObject->SetArrayField(TEXT("Sessions"), SessionsArray);
	RootObject->SetStringField(TEXT("ExportedAt"), FDateTime::Now().ToIso8601());
	RootObject->SetNumberField(TEXT("SessionCount"), Sessions.Num());

	return WriteJsonToFile(RootObject, FilePath);
}

bool USecureStorageManager::ExportToCsv(const TArray<FSessionRecord>& Sessions, const FString& FilePath)
{
	FString CsvContent;

	// Header row
	CsvContent += TEXT("SessionId,StartTime,EndTime,TotalSeconds,ActiveSeconds,ThinkingSeconds,AwaySeconds,PausedSeconds,ActivePercentage\n");

	for (const FSessionRecord& Session : Sessions)
	{
		CsvContent += FString::Printf(TEXT("%s,%s,%s,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f\n"),
			*Session.SessionId.ToString(),
			*Session.StartTime.ToIso8601(),
			*Session.EndTime.ToIso8601(),
			Session.TotalElapsedSeconds,
			Session.ActivitySummary.ActiveSeconds,
			Session.ActivitySummary.ThinkingSeconds,
			Session.ActivitySummary.AwaySeconds,
			Session.ActivitySummary.PausedSeconds,
			Session.ActivitySummary.GetActivePercentage()
		);
	}

	return FFileHelper::SaveStringToFile(CsvContent, *FilePath);
}

bool USecureStorageManager::ExportToMarkdown(const TArray<FSessionRecord>& Sessions, const FString& FilePath)
{
	FString MdContent;

	MdContent += TEXT("# Productivity Report\n\n");
	MdContent += FString::Printf(TEXT("Generated: %s\n\n"), *FDateTime::Now().ToString());
	MdContent += FString::Printf(TEXT("Total Sessions: %d\n\n"), Sessions.Num());

	// Calculate totals
	float TotalActive = 0.0f;
	float TotalTime = 0.0f;

	for (const FSessionRecord& Session : Sessions)
	{
		TotalActive += Session.ActivitySummary.ActiveSeconds;
		TotalTime += Session.TotalElapsedSeconds;
	}

	MdContent += TEXT("## Summary\n\n");
	MdContent += FString::Printf(TEXT("- **Total Time Tracked**: %.1f hours\n"), TotalTime / 3600.0f);
	MdContent += FString::Printf(TEXT("- **Active Time**: %.1f hours\n"), TotalActive / 3600.0f);
	MdContent += FString::Printf(TEXT("- **Overall Active Percentage**: %.1f%%\n\n"),
		TotalTime > 0.0f ? (TotalActive / TotalTime) * 100.0f : 0.0f);

	MdContent += TEXT("## Session Details\n\n");
	MdContent += TEXT("| Date | Duration | Active % | Productive % |\n");
	MdContent += TEXT("|------|----------|----------|-------------|\n");

	for (const FSessionRecord& Session : Sessions)
	{
		MdContent += FString::Printf(TEXT("| %s | %.1f hrs | %.1f%% | %.1f%% |\n"),
			*Session.StartTime.ToString(TEXT("%Y-%m-%d %H:%M")),
			Session.TotalElapsedSeconds / 3600.0f,
			Session.ActivitySummary.GetActivePercentage(),
			Session.ActivitySummary.GetProductivePercentage()
		);
	}

	return FFileHelper::SaveStringToFile(MdContent, *FilePath);
}
