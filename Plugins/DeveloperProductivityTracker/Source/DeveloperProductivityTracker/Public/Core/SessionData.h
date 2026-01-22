// ============================================================================
// SessionData.h
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Core data structures for session tracking and activity snapshots.
// All timestamps use FDateTime for cross-platform compatibility.
// Checksums prevent tampering with recorded time data.
//
// Architecture:
// Pure data structures with no behavioral dependencies.
// Designed for JSON serialization and secure storage.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "SessionData.generated.h"

// Log category for session data operations
DECLARE_LOG_CATEGORY_EXTERN(LogProductivitySessionData, Log, All);

// Activity state for the developer
UENUM(BlueprintType)
enum class EActivityState : uint8
{
	Active     UMETA(DisplayName = "Active"),     // Developer actively working
	Thinking   UMETA(DisplayName = "Thinking"),   // No input but productive apps open
	Away       UMETA(DisplayName = "Away"),       // Extended absence detected
	Paused     UMETA(DisplayName = "Paused")      // Manual pause by user
};

// Snapshot of activity at a point in time
USTRUCT(BlueprintType)
struct DEVELOPERPRODUCTIVITYTRACKER_API FActivitySnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Activity")
	FDateTime Timestamp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Activity")
	EActivityState State;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Activity")
	float SecondsSinceLastInput;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Activity")
	bool bEditorFocused;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Activity")
	bool bPlayInEditorActive;

	// External activity data
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "External")
	bool bExternalAppFocused;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "External")
	FString FocusedExternalApp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "External")
	bool bSourceFilesModified;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Productivity")
	float ProductivityWeight;

	// Tamper detection
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Security")
	FString SnapshotChecksum;

	FActivitySnapshot()
		: Timestamp(FDateTime::MinValue())
		, State(EActivityState::Away)
		, SecondsSinceLastInput(0.0f)
		, bEditorFocused(false)
		, bPlayInEditorActive(false)
		, bExternalAppFocused(false)
		, bSourceFilesModified(false)
		, ProductivityWeight(1.0f)
	{
	}

	// Calculate checksum for tamper detection
	FString CalculateChecksum(const FString& Salt) const;

	// Verify checksum matches
	bool VerifyChecksum(const FString& Salt) const;

	// Serialize to JSON object
	TSharedPtr<FJsonObject> ToJson() const;

	// Deserialize from JSON object
	static bool FromJson(const TSharedPtr<FJsonObject>& JsonObject, FActivitySnapshot& OutSnapshot);
};

// Summary of activity during a session
USTRUCT(BlueprintType)
struct DEVELOPERPRODUCTIVITYTRACKER_API FActivitySummary
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Summary")
	float TotalSeconds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Summary")
	float ActiveSeconds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Summary")
	float ThinkingSeconds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Summary")
	float AwaySeconds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Summary")
	float PausedSeconds;

	// Breakdown by application
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Summary")
	TMap<FString, float> SecondsByApplication;

	FActivitySummary()
		: TotalSeconds(0.0f)
		, ActiveSeconds(0.0f)
		, ThinkingSeconds(0.0f)
		, AwaySeconds(0.0f)
		, PausedSeconds(0.0f)
	{
	}

	// Calculate percentage of time spent actively working
	float GetActivePercentage() const
	{
		return TotalSeconds > 0.0f ? (ActiveSeconds / TotalSeconds) * 100.0f : 0.0f;
	}

	// Calculate productive time (Active + Thinking)
	float GetProductiveSeconds() const
	{
		return ActiveSeconds + ThinkingSeconds;
	}

	// Calculate productive percentage
	float GetProductivePercentage() const
	{
		return TotalSeconds > 0.0f ? (GetProductiveSeconds() / TotalSeconds) * 100.0f : 0.0f;
	}

	// Add time for a specific activity state
	void AddTimeForState(EActivityState State, float Seconds);

	// Add time for an external application
	void AddTimeForApplication(const FString& AppName, float Seconds);

	// Serialize to JSON object
	TSharedPtr<FJsonObject> ToJson() const;

	// Deserialize from JSON object
	static bool FromJson(const TSharedPtr<FJsonObject>& JsonObject, FActivitySummary& OutSummary);
};

// Complete session record
USTRUCT(BlueprintType)
struct DEVELOPERPRODUCTIVITYTRACKER_API FSessionRecord
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Session")
	FGuid SessionId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Session")
	FDateTime StartTime;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Session")
	FDateTime EndTime;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Session")
	float TotalElapsedSeconds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Session")
	FActivitySummary ActivitySummary;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Session")
	TArray<FActivitySnapshot> ActivitySnapshots;

	// External task linkage for project management integration
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Integration")
	FString LinkedTaskId;

	// Machine identifier for multi-device tracking
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Security")
	FString MachineId;

	// Tamper detection for entire record
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Security")
	FString RecordChecksum;

	// Plugin version that created this record
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Metadata")
	FString PluginVersion;

	// Whether the session was properly closed or recovered
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Metadata")
	bool bWasRecovered;

	FSessionRecord()
		: SessionId(FGuid::NewGuid())
		, StartTime(FDateTime::MinValue())
		, EndTime(FDateTime::MinValue())
		, TotalElapsedSeconds(0.0f)
		, bWasRecovered(false)
	{
	}

	// Check if session is currently active (no end time set)
	bool IsActive() const
	{
		return EndTime == FDateTime::MinValue();
	}

	// Get duration as FTimespan
	FTimespan GetDuration() const;

	// Calculate checksum for tamper detection
	FString CalculateChecksum(const FString& Salt) const;

	// Verify checksum matches
	bool VerifyChecksum(const FString& Salt) const;

	// Finalize the session with end time and checksums
	void Finalize(const FString& Salt);

	// Serialize to JSON object
	TSharedPtr<FJsonObject> ToJson() const;

	// Deserialize from JSON object
	static bool FromJson(const TSharedPtr<FJsonObject>& JsonObject, FSessionRecord& OutRecord);

	// Serialize to JSON string
	FString ToJsonString() const;

	// Deserialize from JSON string
	static bool FromJsonString(const FString& JsonString, FSessionRecord& OutRecord);
};

// Daily summary aggregating multiple sessions
USTRUCT(BlueprintType)
struct DEVELOPERPRODUCTIVITYTRACKER_API FDailySummary
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Daily")
	FDateTime Date;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Daily")
	int32 SessionCount;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Daily")
	FActivitySummary AggregatedSummary;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Daily")
	TArray<FGuid> SessionIds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Daily")
	float LongestSessionSeconds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Daily")
	float AverageSessionSeconds;

	FDailySummary()
		: Date(FDateTime::MinValue())
		, SessionCount(0)
		, LongestSessionSeconds(0.0f)
		, AverageSessionSeconds(0.0f)
	{
	}

	// Add a session record to the daily summary
	void AddSession(const FSessionRecord& Session);

	// Serialize to JSON object
	TSharedPtr<FJsonObject> ToJson() const;

	// Deserialize from JSON object
	static bool FromJson(const TSharedPtr<FJsonObject>& JsonObject, FDailySummary& OutSummary);
};
