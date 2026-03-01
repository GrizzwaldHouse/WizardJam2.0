// ============================================================================
// BrightForgeClientSubsystem.cpp
// Developer: Marcus Daley
// Date: February 2026
// Project: BrightForgeConnect Plugin
// ============================================================================
// Purpose:
// HTTP client subsystem for all BrightForge REST API communication.
// Uses timer-based polling (never FTickableEditorObject) for generation status.
// ============================================================================

#include "Core/BrightForgeClientSubsystem.h"
#include "Core/BrightForgeSettings.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Editor.h"

DEFINE_LOG_CATEGORY(LogBrightForgeClient);

// ============================================================================
// Constants
// ============================================================================

namespace BrightForgeClientConstants
{
	const FString GenerationTypeToString(EBrightForgeGenerationType Type)
	{
		switch (Type)
		{
			case EBrightForgeGenerationType::Full:  return TEXT("full");
			case EBrightForgeGenerationType::Mesh:  return TEXT("mesh");
			case EBrightForgeGenerationType::Image: return TEXT("image");
			default:                                return TEXT("full");
		}
	}
}

// ============================================================================
// Construction / Subsystem lifecycle
// ============================================================================

UBrightForgeClientSubsystem::UBrightForgeClientSubsystem()
	: ConnectionState(EBrightForgeConnectionState::Disconnected)
	, bIsGenerating(false)
	, bFbxConverterAvailable(false)
{
}

void UBrightForgeClientSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogBrightForgeClient, Log, TEXT("BrightForgeClientSubsystem initializing..."));

	// Perform an initial health check so the UI has a starting connection state
	CheckServerHealth();

	UE_LOG(LogBrightForgeClient, Log, TEXT("BrightForgeClientSubsystem initialized"));
}

void UBrightForgeClientSubsystem::Deinitialize()
{
	UE_LOG(LogBrightForgeClient, Log, TEXT("BrightForgeClientSubsystem deinitializing..."));
	StopPolling();
	Super::Deinitialize();
}

// ============================================================================
// API METHODS
// ============================================================================

void UBrightForgeClientSubsystem::CheckServerHealth()
{
	UE_LOG(LogBrightForgeClient, Log, TEXT("Checking BrightForge server health..."));
	SetConnectionState(EBrightForgeConnectionState::Connecting);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(BuildUrl(TEXT("/api/health")));
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->OnProcessRequestComplete().BindUObject(this, &UBrightForgeClientSubsystem::OnHealthResponse);
	Request->ProcessRequest();
}

FString UBrightForgeClientSubsystem::GenerateAsset(const FString& Prompt, EBrightForgeGenerationType Type, const FString& ProjectId)
{
	if (bIsGenerating)
	{
		UE_LOG(LogBrightForgeClient, Warning, TEXT("GenerateAsset called while generation already in progress — ignoring"));
		return FString();
	}

	UE_LOG(LogBrightForgeClient, Log, TEXT("Requesting generation: prompt='%s' type='%s'"),
		*Prompt, *BrightForgeClientConstants::GenerationTypeToString(Type));

	// Build JSON body
	TSharedPtr<FJsonObject> Body = MakeShareable(new FJsonObject);
	Body->SetStringField(TEXT("type"), BrightForgeClientConstants::GenerationTypeToString(Type));
	Body->SetStringField(TEXT("prompt"), Prompt);
	if (!ProjectId.IsEmpty())
	{
		Body->SetStringField(TEXT("projectId"), ProjectId);
	}

	FString BodyString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
	FJsonSerializer::Serialize(Body.ToSharedRef(), Writer);

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(BuildUrl(TEXT("/api/forge3d/generate")));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(BodyString);
	Request->OnProcessRequestComplete().BindUObject(this, &UBrightForgeClientSubsystem::OnGenerateResponse);
	Request->ProcessRequest();

	bIsGenerating = true;
	return FString(); // Session ID will be returned via OnGenerateResponse
}

void UBrightForgeClientSubsystem::StartPolling(const FString& SessionId)
{
	StopPolling();

	ActiveSessionId = SessionId;
	UE_LOG(LogBrightForgeClient, Log, TEXT("Starting status polling for session: %s"), *SessionId);

	const UBrightForgeSettings* Settings = UBrightForgeSettings::Get();
	const float IntervalSeconds = Settings
		? FMath::Clamp(Settings->StatusPollingIntervalMs / 1000.0f, 0.5f, 10.0f)
		: 2.0f;

	if (GEditor)
	{
		GEditor->GetTimerManager()->SetTimer(
			PollingTimerHandle,
			FTimerDelegate::CreateUObject(this, &UBrightForgeClientSubsystem::PollStatus),
			IntervalSeconds,
			true  // Repeating
		);
	}
}

void UBrightForgeClientSubsystem::StopPolling()
{
	if (GEditor && PollingTimerHandle.IsValid())
	{
		GEditor->GetTimerManager()->ClearTimer(PollingTimerHandle);
		UE_LOG(LogBrightForgeClient, Log, TEXT("Status polling stopped"));
	}

	PollingTimerHandle.Invalidate();
}

void UBrightForgeClientSubsystem::DownloadFbx(const FString& SessionId)
{
	UE_LOG(LogBrightForgeClient, Log, TEXT("Downloading FBX for session: %s"), *SessionId);

	PendingDownloadSessionId = SessionId;

	const FString Url = BuildUrl(FString::Printf(TEXT("/api/forge3d/download/%s?format=fbx"), *SessionId));

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(TEXT("GET"));
	Request->OnProcessRequestComplete().BindUObject(this, &UBrightForgeClientSubsystem::OnDownloadFbxResponse);
	Request->ProcessRequest();
}

void UBrightForgeClientSubsystem::ListProjects()
{
	UE_LOG(LogBrightForgeClient, Log, TEXT("Fetching BrightForge project list..."));

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(BuildUrl(TEXT("/api/forge3d/projects")));
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->OnProcessRequestComplete().BindUObject(this, &UBrightForgeClientSubsystem::OnProjectsResponse);
	Request->ProcessRequest();
}

void UBrightForgeClientSubsystem::GetFbxStatus()
{
	UE_LOG(LogBrightForgeClient, Log, TEXT("Checking FBX converter status..."));

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(BuildUrl(TEXT("/api/forge3d/fbx-status")));
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->OnProcessRequestComplete().BindUObject(this, &UBrightForgeClientSubsystem::OnFbxStatusResponse);
	Request->ProcessRequest();
}

// ============================================================================
// HTTP RESPONSE HANDLERS
// ============================================================================

void UBrightForgeClientSubsystem::OnHealthResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	if (!bConnectedSuccessfully || !Response.IsValid())
	{
		UE_LOG(LogBrightForgeClient, Warning, TEXT("Health check failed — could not reach server"));
		SetConnectionState(EBrightForgeConnectionState::Error);
		return;
	}

	const int32 ResponseCode = Response->GetResponseCode();
	if (ResponseCode == 200)
	{
		UE_LOG(LogBrightForgeClient, Log, TEXT("BrightForge server is healthy (HTTP 200)"));
		SetConnectionState(EBrightForgeConnectionState::Connected);
	}
	else
	{
		UE_LOG(LogBrightForgeClient, Warning, TEXT("Health check returned HTTP %d"), ResponseCode);
		SetConnectionState(EBrightForgeConnectionState::Error);
	}
}

void UBrightForgeClientSubsystem::OnGenerateResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	if (!bConnectedSuccessfully || !Response.IsValid())
	{
		UE_LOG(LogBrightForgeClient, Error, TEXT("Generate request failed — no response"));
		bIsGenerating = false;
		OnGenerationFailed.Broadcast(FString(), TEXT("Failed to connect to BrightForge server"));
		return;
	}

	const int32 ResponseCode = Response->GetResponseCode();
	const FString ResponseBody = Response->GetContentAsString();

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogBrightForgeClient, Error, TEXT("Failed to parse generate response JSON"));
		bIsGenerating = false;
		OnGenerationFailed.Broadcast(FString(), TEXT("Invalid JSON response from server"));
		return;
	}

	if (ResponseCode != 200 && ResponseCode != 201)
	{
		FString ErrorMessage;
		JsonObject->TryGetStringField(TEXT("error"), ErrorMessage);
		UE_LOG(LogBrightForgeClient, Error, TEXT("Generate returned HTTP %d: %s"), ResponseCode, *ErrorMessage);
		bIsGenerating = false;
		OnGenerationFailed.Broadcast(FString(), ErrorMessage);
		return;
	}

	// Extract session ID from response
	FString SessionId;
	if (!JsonObject->TryGetStringField(TEXT("id"), SessionId) &&
		!JsonObject->TryGetStringField(TEXT("sessionId"), SessionId))
	{
		UE_LOG(LogBrightForgeClient, Error, TEXT("Generate response missing session ID field"));
		bIsGenerating = false;
		OnGenerationFailed.Broadcast(FString(), TEXT("Server response missing session ID"));
		return;
	}

	UE_LOG(LogBrightForgeClient, Log, TEXT("Generation started, session ID: %s"), *SessionId);
	ActiveSessionId = SessionId;

	// Begin polling for status
	StartPolling(SessionId);
}

void UBrightForgeClientSubsystem::OnStatusPollResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	if (!bConnectedSuccessfully || !Response.IsValid())
	{
		UE_LOG(LogBrightForgeClient, Warning, TEXT("Status poll failed — no response"));
		return;
	}

	const FString ResponseBody = Response->GetContentAsString();

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogBrightForgeClient, Warning, TEXT("Failed to parse status response JSON"));
		return;
	}

	FBrightForgeGenerationStatus Status;
	if (!ParseGenerationStatus(JsonObject, Status))
	{
		return;
	}

	UE_LOG(LogBrightForgeClient, Verbose, TEXT("Generation status: state='%s' progress=%.2f"), *Status.State, Status.Progress);

	// Broadcast progress
	OnGenerationProgress.Broadcast(Status);

	// Handle terminal states
	if (Status.State == TEXT("complete") || Status.State == TEXT("success"))
	{
		UE_LOG(LogBrightForgeClient, Log, TEXT("Generation complete for session: %s"), *Status.SessionId);
		StopPolling();
		bIsGenerating = false;
		OnGenerationComplete.Broadcast(Status);
	}
	else if (Status.State == TEXT("failed") || Status.State == TEXT("error"))
	{
		UE_LOG(LogBrightForgeClient, Error, TEXT("Generation failed for session: %s — %s"), *Status.SessionId, *Status.Error);
		StopPolling();
		bIsGenerating = false;
		OnGenerationFailed.Broadcast(Status.SessionId, Status.Error);
	}
}

void UBrightForgeClientSubsystem::OnDownloadFbxResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	if (!bConnectedSuccessfully || !Response.IsValid())
	{
		UE_LOG(LogBrightForgeClient, Error, TEXT("FBX download failed — no response"));
		return;
	}

	const int32 ResponseCode = Response->GetResponseCode();
	if (ResponseCode != 200)
	{
		UE_LOG(LogBrightForgeClient, Error, TEXT("FBX download returned HTTP %d"), ResponseCode);
		return;
	}

	const TArray<uint8>& Content = Response->GetContent();
	if (Content.Num() == 0)
	{
		UE_LOG(LogBrightForgeClient, Error, TEXT("FBX download returned empty content"));
		return;
	}

	const FString FilePath = SaveFbxToStaging(PendingDownloadSessionId, Content);
	if (FilePath.IsEmpty())
	{
		UE_LOG(LogBrightForgeClient, Error, TEXT("Failed to save FBX to staging directory"));
		return;
	}

	UE_LOG(LogBrightForgeClient, Log, TEXT("FBX saved to staging: %s"), *FilePath);
}

void UBrightForgeClientSubsystem::OnProjectsResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	CachedProjects.Empty();

	if (!bConnectedSuccessfully || !Response.IsValid() || Response->GetResponseCode() != 200)
	{
		UE_LOG(LogBrightForgeClient, Warning, TEXT("Projects request failed"));
		return;
	}

	TSharedPtr<FJsonValue> RootValue;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	if (!FJsonSerializer::Deserialize(Reader, RootValue) || !RootValue.IsValid())
	{
		return;
	}

	const TArray<TSharedPtr<FJsonValue>>* ProjectsArray = nullptr;
	if (RootValue->Type == EJson::Array)
	{
		ProjectsArray = &RootValue->AsArray();
	}
	else if (RootValue->Type == EJson::Object)
	{
		const TArray<TSharedPtr<FJsonValue>>* Temp = nullptr;
		if (RootValue->AsObject()->TryGetArrayField(TEXT("projects"), Temp))
		{
			ProjectsArray = Temp;
		}
	}

	if (!ProjectsArray)
	{
		UE_LOG(LogBrightForgeClient, Warning, TEXT("Could not find projects array in response"));
		return;
	}

	for (const TSharedPtr<FJsonValue>& ProjectValue : *ProjectsArray)
	{
		TSharedPtr<FJsonObject> ProjectObj = ProjectValue->AsObject();
		if (!ProjectObj.IsValid())
		{
			continue;
		}

		FBrightForgeProject Project;
		ProjectObj->TryGetStringField(TEXT("id"), Project.Id);
		ProjectObj->TryGetStringField(TEXT("name"), Project.Name);
		ProjectObj->TryGetNumberField(TEXT("assetCount"), Project.AssetCount);
		CachedProjects.Add(Project);
	}

	UE_LOG(LogBrightForgeClient, Log, TEXT("Loaded %d BrightForge projects"), CachedProjects.Num());
}

void UBrightForgeClientSubsystem::OnFbxStatusResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	bFbxConverterAvailable = false;

	if (!bConnectedSuccessfully || !Response.IsValid() || Response->GetResponseCode() != 200)
	{
		UE_LOG(LogBrightForgeClient, Warning, TEXT("FBX status check failed"));
		return;
	}

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return;
	}

	JsonObject->TryGetBoolField(TEXT("available"), bFbxConverterAvailable);
	UE_LOG(LogBrightForgeClient, Log, TEXT("FBX converter available: %s"), bFbxConverterAvailable ? TEXT("Yes") : TEXT("No"));
}

// ============================================================================
// POLLING
// ============================================================================

void UBrightForgeClientSubsystem::PollStatus()
{
	if (ActiveSessionId.IsEmpty())
	{
		StopPolling();
		return;
	}

	const FString Url = BuildUrl(FString::Printf(TEXT("/api/forge3d/status/%s"), *ActiveSessionId));

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->OnProcessRequestComplete().BindUObject(this, &UBrightForgeClientSubsystem::OnStatusPollResponse);
	Request->ProcessRequest();
}

// ============================================================================
// HELPERS
// ============================================================================

FString UBrightForgeClientSubsystem::BuildUrl(const FString& Path) const
{
	const UBrightForgeSettings* Settings = UBrightForgeSettings::Get();
	const FString Base = Settings ? Settings->ServerUrl : TEXT("http://localhost:3847");
	return Base + Path;
}

void UBrightForgeClientSubsystem::SetConnectionState(EBrightForgeConnectionState NewState)
{
	if (ConnectionState != NewState)
	{
		ConnectionState = NewState;
		OnConnectionStateChanged.Broadcast(NewState);
	}
}

bool UBrightForgeClientSubsystem::ParseGenerationStatus(TSharedPtr<FJsonObject> JsonObject, FBrightForgeGenerationStatus& OutStatus) const
{
	if (!JsonObject.IsValid())
	{
		return false;
	}

	JsonObject->TryGetStringField(TEXT("id"), OutStatus.SessionId);
	JsonObject->TryGetStringField(TEXT("sessionId"), OutStatus.SessionId); // Try alternate key
	JsonObject->TryGetStringField(TEXT("state"), OutStatus.State);
	JsonObject->TryGetStringField(TEXT("status"), OutStatus.State); // Alternate key
	JsonObject->TryGetNumberField(TEXT("progress"), OutStatus.Progress);
	JsonObject->TryGetStringField(TEXT("error"), OutStatus.Error);
	JsonObject->TryGetStringField(TEXT("prompt"), OutStatus.Prompt);

	int32 GenTime = 0;
	if (JsonObject->TryGetNumberField(TEXT("generationTime"), GenTime))
	{
		OutStatus.GenerationTimeMs = GenTime;
	}

	return true;
}

FString UBrightForgeClientSubsystem::SaveFbxToStaging(const FString& SessionId, const TArray<uint8>& Data) const
{
	const FString StagingDir = FPaths::ProjectIntermediateDir() / TEXT("BrightForge");

	// Ensure staging directory exists
	IFileManager::Get().MakeDirectory(*StagingDir, true);

	const FString FileName = FString::Printf(TEXT("BF_%s.fbx"), *SessionId);
	const FString FilePath = StagingDir / FileName;

	if (!FFileHelper::SaveArrayToFile(Data, *FilePath))
	{
		UE_LOG(LogBrightForgeClient, Error, TEXT("Failed to save FBX file to: %s"), *FilePath);
		return FString();
	}

	return FilePath;
}
