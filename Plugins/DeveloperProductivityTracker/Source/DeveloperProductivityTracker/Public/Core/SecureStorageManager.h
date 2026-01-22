// ============================================================================
// SecureStorageManager.h
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Manages secure storage of session data with tamper detection.
// Handles data persistence, encryption, checksum verification, and data export.
//
// Architecture:
// File-based storage using JSON format with optional encryption.
// Installation-specific salt prevents cross-machine tampering.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "SessionData.h"
#include "SecureStorageManager.generated.h"

// Log category for storage operations
DECLARE_LOG_CATEGORY_EXTERN(LogProductivityStorage, Log, All);

// Result of a data integrity verification
UENUM(BlueprintType)
enum class EDataIntegrityResult : uint8
{
	Valid           UMETA(DisplayName = "Valid"),           // Data is intact and verified
	Modified        UMETA(DisplayName = "Modified"),        // Data was modified externally
	Corrupted       UMETA(DisplayName = "Corrupted"),       // Data is unreadable
	Missing         UMETA(DisplayName = "Missing"),         // File not found
	VersionMismatch UMETA(DisplayName = "Version Mismatch") // Incompatible data version
};

// Export format options
UENUM(BlueprintType)
enum class EExportFormat : uint8
{
	JSON       UMETA(DisplayName = "JSON"),
	CSV        UMETA(DisplayName = "CSV"),
	Markdown   UMETA(DisplayName = "Markdown Report")
};

// Delegate for integrity warnings
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDataIntegrityWarning, const FString&, FilePath, EDataIntegrityResult, Result);

UCLASS()
class DEVELOPERPRODUCTIVITYTRACKER_API USecureStorageManager : public UObject
{
	GENERATED_BODY()

public:
	USecureStorageManager();

	// ========================================================================
	// INITIALIZATION
	// ========================================================================

	// Initialize the storage manager with the data directory
	bool Initialize(const FString& InDataDirectory);

	// Shutdown and flush any pending writes
	void Shutdown();

	// Check if the manager is ready for operations
	bool IsInitialized() const { return bIsInitialized; }

	// ========================================================================
	// SESSION STORAGE
	// ========================================================================

	// Save a session record to storage
	UFUNCTION(BlueprintCallable, Category = "Productivity|Storage")
	bool SaveSession(const FSessionRecord& Session);

	// Load a session record by ID
	UFUNCTION(BlueprintCallable, Category = "Productivity|Storage")
	bool LoadSession(const FGuid& SessionId, FSessionRecord& OutSession);

	// Load all sessions within a date range
	UFUNCTION(BlueprintCallable, Category = "Productivity|Storage")
	TArray<FSessionRecord> LoadSessionsInRange(const FDateTime& StartDate, const FDateTime& EndDate);

	// Delete a session record
	UFUNCTION(BlueprintCallable, Category = "Productivity|Storage")
	bool DeleteSession(const FGuid& SessionId);

	// Get all session IDs
	UFUNCTION(BlueprintCallable, Category = "Productivity|Storage")
	TArray<FGuid> GetAllSessionIds();

	// ========================================================================
	// ACTIVE SESSION PERSISTENCE
	// ========================================================================

	// Save the current active session state (for crash recovery)
	bool SaveActiveSessionState(const FSessionRecord& Session);

	// Load a crashed session for recovery
	bool LoadActiveSessionState(FSessionRecord& OutSession);

	// Clear the active session state file
	void ClearActiveSessionState();

	// Check if there's a recoverable session
	bool HasRecoverableSession() const;

	// ========================================================================
	// DAILY SUMMARY STORAGE
	// ========================================================================

	// Save a daily summary
	UFUNCTION(BlueprintCallable, Category = "Productivity|Storage")
	bool SaveDailySummary(const FDailySummary& Summary);

	// Load a daily summary for a specific date
	UFUNCTION(BlueprintCallable, Category = "Productivity|Storage")
	bool LoadDailySummary(const FDateTime& Date, FDailySummary& OutSummary);

	// Get summaries for a date range
	UFUNCTION(BlueprintCallable, Category = "Productivity|Storage")
	TArray<FDailySummary> LoadDailySummariesInRange(const FDateTime& StartDate, const FDateTime& EndDate);

	// ========================================================================
	// DATA INTEGRITY
	// ========================================================================

	// Verify integrity of a specific session
	UFUNCTION(BlueprintCallable, Category = "Productivity|Security")
	EDataIntegrityResult VerifySessionIntegrity(const FGuid& SessionId);

	// Verify integrity of all stored data
	UFUNCTION(BlueprintCallable, Category = "Productivity|Security")
	TMap<FGuid, EDataIntegrityResult> VerifyAllDataIntegrity();

	// ========================================================================
	// DATA EXPORT
	// ========================================================================

	// Export sessions to a file
	UFUNCTION(BlueprintCallable, Category = "Productivity|Export")
	bool ExportSessions(const TArray<FGuid>& SessionIds, const FString& FilePath, EExportFormat Format);

	// Export all data for a date range
	UFUNCTION(BlueprintCallable, Category = "Productivity|Export")
	bool ExportDateRange(const FDateTime& StartDate, const FDateTime& EndDate, const FString& FilePath, EExportFormat Format);

	// Export for user data request (GDPR compliance)
	UFUNCTION(BlueprintCallable, Category = "Productivity|Export")
	bool ExportAllUserData(const FString& FilePath);

	// ========================================================================
	// DATA CLEANUP
	// ========================================================================

	// Delete data older than the retention period
	UFUNCTION(BlueprintCallable, Category = "Productivity|Storage")
	int32 CleanupOldData(int32 RetentionDays);

	// Delete all stored data (user-initiated)
	UFUNCTION(BlueprintCallable, Category = "Productivity|Storage")
	bool DeleteAllData();

	// ========================================================================
	// SECURITY
	// ========================================================================

	// Get the installation salt (creates if not exists)
	FString GetInstallationSalt();

	// Generate a unique machine identifier
	FString GenerateMachineIdentifier();

	// ========================================================================
	// DELEGATES
	// ========================================================================

	UPROPERTY(BlueprintAssignable, Category = "Productivity|Events")
	FOnDataIntegrityWarning OnDataIntegrityWarning;

private:
	// State
	bool bIsInitialized;
	FString DataDirectory;
	FString InstallationSalt;
	FString MachineId;

	// File paths
	FString GetSessionFilePath(const FGuid& SessionId) const;
	FString GetDailySummaryFilePath(const FDateTime& Date) const;
	FString GetActiveSessionFilePath() const;
	FString GetSaltFilePath() const;
	FString GetMachineIdFilePath() const;

	// Internal helpers
	bool WriteJsonToFile(const TSharedPtr<FJsonObject>& JsonObject, const FString& FilePath);
	TSharedPtr<FJsonObject> ReadJsonFromFile(const FString& FilePath);
	void LoadOrCreateInstallationSalt();
	void LoadOrCreateMachineId();
	FString GenerateRandomSalt(int32 Length = 32);

	// Export format implementations
	bool ExportToJson(const TArray<FSessionRecord>& Sessions, const FString& FilePath);
	bool ExportToCsv(const TArray<FSessionRecord>& Sessions, const FString& FilePath);
	bool ExportToMarkdown(const TArray<FSessionRecord>& Sessions, const FString& FilePath);
};
