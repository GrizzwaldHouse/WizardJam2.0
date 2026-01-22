// ============================================================================
// SessionData.cpp
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Implementation of core session data structures including serialization,
// checksum calculation, and tamper detection.
// ============================================================================

#include "Core/SessionData.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Dom/JsonObject.h"

DEFINE_LOG_CATEGORY(LogProductivitySessionData);

// ============================================================================
// FActivitySnapshot Implementation
// ============================================================================

FString FActivitySnapshot::CalculateChecksum(const FString& Salt) const
{
	// Build a deterministic string from snapshot data
	FString DataString = FString::Printf(
		TEXT("%s|%d|%.2f|%d|%d|%s|%d|%.2f|%s"),
		*Timestamp.ToString(),
		static_cast<int32>(State),
		SecondsSinceLastInput,
		bEditorFocused ? 1 : 0,
		bExternalAppFocused ? 1 : 0,
		*FocusedExternalApp,
		bSourceFilesModified ? 1 : 0,
		ProductivityWeight,
		*Salt
	);

	// Generate MD5 hash for tamper detection
	return FMD5::HashAnsiString(*DataString);
}

bool FActivitySnapshot::VerifyChecksum(const FString& Salt) const
{
	FString CalculatedChecksum = CalculateChecksum(Salt);
	return SnapshotChecksum.Equals(CalculatedChecksum, ESearchCase::IgnoreCase);
}

TSharedPtr<FJsonObject> FActivitySnapshot::ToJson() const
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());

	JsonObject->SetStringField(TEXT("Timestamp"), Timestamp.ToIso8601());
	JsonObject->SetNumberField(TEXT("State"), static_cast<int32>(State));
	JsonObject->SetNumberField(TEXT("SecondsSinceLastInput"), SecondsSinceLastInput);
	JsonObject->SetBoolField(TEXT("bEditorFocused"), bEditorFocused);
	JsonObject->SetBoolField(TEXT("bPlayInEditorActive"), bPlayInEditorActive);
	JsonObject->SetBoolField(TEXT("bExternalAppFocused"), bExternalAppFocused);
	JsonObject->SetStringField(TEXT("FocusedExternalApp"), FocusedExternalApp);
	JsonObject->SetBoolField(TEXT("bSourceFilesModified"), bSourceFilesModified);
	JsonObject->SetNumberField(TEXT("ProductivityWeight"), ProductivityWeight);
	JsonObject->SetStringField(TEXT("SnapshotChecksum"), SnapshotChecksum);

	return JsonObject;
}

bool FActivitySnapshot::FromJson(const TSharedPtr<FJsonObject>& JsonObject, FActivitySnapshot& OutSnapshot)
{
	if (!JsonObject.IsValid())
	{
		UE_LOG(LogProductivitySessionData, Warning, TEXT("Invalid JSON object for FActivitySnapshot"));
		return false;
	}

	FString TimestampString;
	if (JsonObject->TryGetStringField(TEXT("Timestamp"), TimestampString))
	{
		FDateTime::ParseIso8601(*TimestampString, OutSnapshot.Timestamp);
	}

	int32 StateValue = 0;
	if (JsonObject->TryGetNumberField(TEXT("State"), StateValue))
	{
		OutSnapshot.State = static_cast<EActivityState>(StateValue);
	}

	JsonObject->TryGetNumberField(TEXT("SecondsSinceLastInput"), OutSnapshot.SecondsSinceLastInput);
	JsonObject->TryGetBoolField(TEXT("bEditorFocused"), OutSnapshot.bEditorFocused);
	JsonObject->TryGetBoolField(TEXT("bPlayInEditorActive"), OutSnapshot.bPlayInEditorActive);
	JsonObject->TryGetBoolField(TEXT("bExternalAppFocused"), OutSnapshot.bExternalAppFocused);
	JsonObject->TryGetStringField(TEXT("FocusedExternalApp"), OutSnapshot.FocusedExternalApp);
	JsonObject->TryGetBoolField(TEXT("bSourceFilesModified"), OutSnapshot.bSourceFilesModified);
	JsonObject->TryGetNumberField(TEXT("ProductivityWeight"), OutSnapshot.ProductivityWeight);
	JsonObject->TryGetStringField(TEXT("SnapshotChecksum"), OutSnapshot.SnapshotChecksum);

	return true;
}

// ============================================================================
// FActivitySummary Implementation
// ============================================================================

void FActivitySummary::AddTimeForState(EActivityState State, float Seconds)
{
	TotalSeconds += Seconds;

	switch (State)
	{
	case EActivityState::Active:
		ActiveSeconds += Seconds;
		break;
	case EActivityState::Thinking:
		ThinkingSeconds += Seconds;
		break;
	case EActivityState::Away:
		AwaySeconds += Seconds;
		break;
	case EActivityState::Paused:
		PausedSeconds += Seconds;
		break;
	}
}

void FActivitySummary::AddTimeForApplication(const FString& AppName, float Seconds)
{
	if (AppName.IsEmpty())
	{
		return;
	}

	if (float* ExistingTime = SecondsByApplication.Find(AppName))
	{
		*ExistingTime += Seconds;
	}
	else
	{
		SecondsByApplication.Add(AppName, Seconds);
	}
}

TSharedPtr<FJsonObject> FActivitySummary::ToJson() const
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());

	JsonObject->SetNumberField(TEXT("TotalSeconds"), TotalSeconds);
	JsonObject->SetNumberField(TEXT("ActiveSeconds"), ActiveSeconds);
	JsonObject->SetNumberField(TEXT("ThinkingSeconds"), ThinkingSeconds);
	JsonObject->SetNumberField(TEXT("AwaySeconds"), AwaySeconds);
	JsonObject->SetNumberField(TEXT("PausedSeconds"), PausedSeconds);

	// Serialize application time breakdown
	TSharedPtr<FJsonObject> AppTimeObject = MakeShareable(new FJsonObject());
	for (const auto& Pair : SecondsByApplication)
	{
		AppTimeObject->SetNumberField(Pair.Key, Pair.Value);
	}
	JsonObject->SetObjectField(TEXT("SecondsByApplication"), AppTimeObject);

	return JsonObject;
}

bool FActivitySummary::FromJson(const TSharedPtr<FJsonObject>& JsonObject, FActivitySummary& OutSummary)
{
	if (!JsonObject.IsValid())
	{
		UE_LOG(LogProductivitySessionData, Warning, TEXT("Invalid JSON object for FActivitySummary"));
		return false;
	}

	JsonObject->TryGetNumberField(TEXT("TotalSeconds"), OutSummary.TotalSeconds);
	JsonObject->TryGetNumberField(TEXT("ActiveSeconds"), OutSummary.ActiveSeconds);
	JsonObject->TryGetNumberField(TEXT("ThinkingSeconds"), OutSummary.ThinkingSeconds);
	JsonObject->TryGetNumberField(TEXT("AwaySeconds"), OutSummary.AwaySeconds);
	JsonObject->TryGetNumberField(TEXT("PausedSeconds"), OutSummary.PausedSeconds);

	// Deserialize application time breakdown
	const TSharedPtr<FJsonObject>* AppTimeObject = nullptr;
	if (JsonObject->TryGetObjectField(TEXT("SecondsByApplication"), AppTimeObject) && AppTimeObject->IsValid())
	{
		for (const auto& Pair : (*AppTimeObject)->Values)
		{
			double TimeValue = 0.0;
			if (Pair.Value->TryGetNumber(TimeValue))
			{
				OutSummary.SecondsByApplication.Add(Pair.Key, static_cast<float>(TimeValue));
			}
		}
	}

	return true;
}

// ============================================================================
// FSessionRecord Implementation
// ============================================================================

FTimespan FSessionRecord::GetDuration() const
{
	if (EndTime != FDateTime::MinValue())
	{
		return EndTime - StartTime;
	}
	return FDateTime::Now() - StartTime;
}

FString FSessionRecord::CalculateChecksum(const FString& Salt) const
{
	// Build a deterministic string from session data
	FString DataString = FString::Printf(
		TEXT("%s|%s|%s|%.2f|%s|%s|%d|%s"),
		*SessionId.ToString(),
		*StartTime.ToString(),
		*EndTime.ToString(),
		TotalElapsedSeconds,
		*LinkedTaskId,
		*MachineId,
		ActivitySnapshots.Num(),
		*Salt
	);

	// Include summary data in checksum
	DataString += FString::Printf(
		TEXT("|%.2f|%.2f|%.2f|%.2f"),
		ActivitySummary.ActiveSeconds,
		ActivitySummary.ThinkingSeconds,
		ActivitySummary.AwaySeconds,
		ActivitySummary.PausedSeconds
	);

	return FMD5::HashAnsiString(*DataString);
}

bool FSessionRecord::VerifyChecksum(const FString& Salt) const
{
	FString CalculatedChecksum = CalculateChecksum(Salt);
	return RecordChecksum.Equals(CalculatedChecksum, ESearchCase::IgnoreCase);
}

void FSessionRecord::Finalize(const FString& Salt)
{
	if (EndTime == FDateTime::MinValue())
	{
		EndTime = FDateTime::Now();
	}

	// Recalculate total elapsed time
	TotalElapsedSeconds = static_cast<float>((EndTime - StartTime).GetTotalSeconds());

	// Calculate and store the record checksum
	RecordChecksum = CalculateChecksum(Salt);

	UE_LOG(LogProductivitySessionData, Log,
		TEXT("Session %s finalized. Duration: %.1f seconds, Active: %.1f%%"),
		*SessionId.ToString(),
		TotalElapsedSeconds,
		ActivitySummary.GetActivePercentage());
}

TSharedPtr<FJsonObject> FSessionRecord::ToJson() const
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());

	JsonObject->SetStringField(TEXT("SessionId"), SessionId.ToString());
	JsonObject->SetStringField(TEXT("StartTime"), StartTime.ToIso8601());
	JsonObject->SetStringField(TEXT("EndTime"), EndTime.ToIso8601());
	JsonObject->SetNumberField(TEXT("TotalElapsedSeconds"), TotalElapsedSeconds);
	JsonObject->SetStringField(TEXT("LinkedTaskId"), LinkedTaskId);
	JsonObject->SetStringField(TEXT("MachineId"), MachineId);
	JsonObject->SetStringField(TEXT("RecordChecksum"), RecordChecksum);
	JsonObject->SetStringField(TEXT("PluginVersion"), PluginVersion);
	JsonObject->SetBoolField(TEXT("bWasRecovered"), bWasRecovered);

	// Serialize activity summary
	JsonObject->SetObjectField(TEXT("ActivitySummary"), ActivitySummary.ToJson());

	// Serialize activity snapshots
	TArray<TSharedPtr<FJsonValue>> SnapshotsArray;
	for (const FActivitySnapshot& Snapshot : ActivitySnapshots)
	{
		SnapshotsArray.Add(MakeShareable(new FJsonValueObject(Snapshot.ToJson())));
	}
	JsonObject->SetArrayField(TEXT("ActivitySnapshots"), SnapshotsArray);

	return JsonObject;
}

bool FSessionRecord::FromJson(const TSharedPtr<FJsonObject>& JsonObject, FSessionRecord& OutRecord)
{
	if (!JsonObject.IsValid())
	{
		UE_LOG(LogProductivitySessionData, Warning, TEXT("Invalid JSON object for FSessionRecord"));
		return false;
	}

	FString SessionIdString;
	if (JsonObject->TryGetStringField(TEXT("SessionId"), SessionIdString))
	{
		FGuid::Parse(SessionIdString, OutRecord.SessionId);
	}

	FString StartTimeString;
	if (JsonObject->TryGetStringField(TEXT("StartTime"), StartTimeString))
	{
		FDateTime::ParseIso8601(*StartTimeString, OutRecord.StartTime);
	}

	FString EndTimeString;
	if (JsonObject->TryGetStringField(TEXT("EndTime"), EndTimeString))
	{
		FDateTime::ParseIso8601(*EndTimeString, OutRecord.EndTime);
	}

	JsonObject->TryGetNumberField(TEXT("TotalElapsedSeconds"), OutRecord.TotalElapsedSeconds);
	JsonObject->TryGetStringField(TEXT("LinkedTaskId"), OutRecord.LinkedTaskId);
	JsonObject->TryGetStringField(TEXT("MachineId"), OutRecord.MachineId);
	JsonObject->TryGetStringField(TEXT("RecordChecksum"), OutRecord.RecordChecksum);
	JsonObject->TryGetStringField(TEXT("PluginVersion"), OutRecord.PluginVersion);
	JsonObject->TryGetBoolField(TEXT("bWasRecovered"), OutRecord.bWasRecovered);

	// Deserialize activity summary
	const TSharedPtr<FJsonObject>* SummaryObject = nullptr;
	if (JsonObject->TryGetObjectField(TEXT("ActivitySummary"), SummaryObject))
	{
		FActivitySummary::FromJson(*SummaryObject, OutRecord.ActivitySummary);
	}

	// Deserialize activity snapshots
	const TArray<TSharedPtr<FJsonValue>>* SnapshotsArray = nullptr;
	if (JsonObject->TryGetArrayField(TEXT("ActivitySnapshots"), SnapshotsArray))
	{
		for (const TSharedPtr<FJsonValue>& SnapshotValue : *SnapshotsArray)
		{
			const TSharedPtr<FJsonObject>* SnapshotObject = nullptr;
			if (SnapshotValue->TryGetObject(SnapshotObject))
			{
				FActivitySnapshot Snapshot;
				if (FActivitySnapshot::FromJson(*SnapshotObject, Snapshot))
				{
					OutRecord.ActivitySnapshots.Add(Snapshot);
				}
			}
		}
	}

	return true;
}

FString FSessionRecord::ToJsonString() const
{
	TSharedPtr<FJsonObject> JsonObject = ToJson();

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	return OutputString;
}

bool FSessionRecord::FromJsonString(const FString& JsonString, FSessionRecord& OutRecord)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogProductivitySessionData, Warning, TEXT("Failed to parse JSON string for FSessionRecord"));
		return false;
	}

	return FromJson(JsonObject, OutRecord);
}

// ============================================================================
// FDailySummary Implementation
// ============================================================================

void FDailySummary::AddSession(const FSessionRecord& Session)
{
	SessionCount++;
	SessionIds.Add(Session.SessionId);

	// Aggregate activity summary
	AggregatedSummary.TotalSeconds += Session.ActivitySummary.TotalSeconds;
	AggregatedSummary.ActiveSeconds += Session.ActivitySummary.ActiveSeconds;
	AggregatedSummary.ThinkingSeconds += Session.ActivitySummary.ThinkingSeconds;
	AggregatedSummary.AwaySeconds += Session.ActivitySummary.AwaySeconds;
	AggregatedSummary.PausedSeconds += Session.ActivitySummary.PausedSeconds;

	// Aggregate application time
	for (const auto& Pair : Session.ActivitySummary.SecondsByApplication)
	{
		AggregatedSummary.AddTimeForApplication(Pair.Key, Pair.Value);
	}

	// Track longest session
	if (Session.TotalElapsedSeconds > LongestSessionSeconds)
	{
		LongestSessionSeconds = Session.TotalElapsedSeconds;
	}

	// Recalculate average
	AverageSessionSeconds = AggregatedSummary.TotalSeconds / static_cast<float>(SessionCount);

	UE_LOG(LogProductivitySessionData, Verbose,
		TEXT("Daily summary updated. Sessions: %d, Total: %.1f hours"),
		SessionCount,
		AggregatedSummary.TotalSeconds / 3600.0f);
}

TSharedPtr<FJsonObject> FDailySummary::ToJson() const
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());

	JsonObject->SetStringField(TEXT("Date"), Date.ToIso8601());
	JsonObject->SetNumberField(TEXT("SessionCount"), SessionCount);
	JsonObject->SetNumberField(TEXT("LongestSessionSeconds"), LongestSessionSeconds);
	JsonObject->SetNumberField(TEXT("AverageSessionSeconds"), AverageSessionSeconds);

	// Serialize aggregated summary
	JsonObject->SetObjectField(TEXT("AggregatedSummary"), AggregatedSummary.ToJson());

	// Serialize session IDs
	TArray<TSharedPtr<FJsonValue>> SessionIdsArray;
	for (const FGuid& Id : SessionIds)
	{
		SessionIdsArray.Add(MakeShareable(new FJsonValueString(Id.ToString())));
	}
	JsonObject->SetArrayField(TEXT("SessionIds"), SessionIdsArray);

	return JsonObject;
}

bool FDailySummary::FromJson(const TSharedPtr<FJsonObject>& JsonObject, FDailySummary& OutSummary)
{
	if (!JsonObject.IsValid())
	{
		UE_LOG(LogProductivitySessionData, Warning, TEXT("Invalid JSON object for FDailySummary"));
		return false;
	}

	FString DateString;
	if (JsonObject->TryGetStringField(TEXT("Date"), DateString))
	{
		FDateTime::ParseIso8601(*DateString, OutSummary.Date);
	}

	JsonObject->TryGetNumberField(TEXT("SessionCount"), OutSummary.SessionCount);
	JsonObject->TryGetNumberField(TEXT("LongestSessionSeconds"), OutSummary.LongestSessionSeconds);
	JsonObject->TryGetNumberField(TEXT("AverageSessionSeconds"), OutSummary.AverageSessionSeconds);

	// Deserialize aggregated summary
	const TSharedPtr<FJsonObject>* SummaryObject = nullptr;
	if (JsonObject->TryGetObjectField(TEXT("AggregatedSummary"), SummaryObject))
	{
		FActivitySummary::FromJson(*SummaryObject, OutSummary.AggregatedSummary);
	}

	// Deserialize session IDs
	const TArray<TSharedPtr<FJsonValue>>* SessionIdsArray = nullptr;
	if (JsonObject->TryGetArrayField(TEXT("SessionIds"), SessionIdsArray))
	{
		for (const TSharedPtr<FJsonValue>& IdValue : *SessionIdsArray)
		{
			FString IdString;
			if (IdValue->TryGetString(IdString))
			{
				FGuid Id;
				if (FGuid::Parse(IdString, Id))
				{
					OutSummary.SessionIds.Add(Id);
				}
			}
		}
	}

	return true;
}
