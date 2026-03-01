// ============================================================================
// WellnessHttpServer.cpp
// Developer: Marcus Daley
// Date: February 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Implementation of the HTTP API server for external wellness data access.
// ============================================================================

#include "Wellness/WellnessHttpServer.h"
#include "Wellness/BreakWellnessSubsystem.h"
#include "Wellness/PomodoroManager.h"
#include "Wellness/HabitStreakTracker.h"
#include "HttpServerModule.h"
#include "IHttpRouter.h"
#include "HttpPath.h"
#include "HttpRequestHandler.h"
#include "HttpResultCallback.h"
#include "HttpServerRequest.h"
#include "HttpServerResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY(LogWellnessHttp);

UWellnessHttpServer::UWellnessHttpServer()
	: ServerPort(8090)
	, bRequireLocalhost(true)
	, bEnableCORS(true)
	, bIsRunning(false)
{
}

// ============================================================================
// SERVER CONTROLS
// ============================================================================

void UWellnessHttpServer::StartServer(UBreakWellnessSubsystem* WellnessSubsystem)
{
	if (bIsRunning)
	{
		UE_LOG(LogWellnessHttp, Warning, TEXT("HTTP server already running on port %d"), ServerPort);
		return;
	}

	if (!WellnessSubsystem)
	{
		UE_LOG(LogWellnessHttp, Error, TEXT("Cannot start HTTP server without a valid WellnessSubsystem reference"));
		return;
	}

	WellnessSubsystemRef = WellnessSubsystem;

	// Get the HTTP router for our port
	FHttpServerModule& HttpServerModule = FHttpServerModule::Get();
	TSharedPtr<IHttpRouter> HttpRouter = HttpServerModule.GetHttpRouter(ServerPort);

	if (!HttpRouter.IsValid())
	{
		UE_LOG(LogWellnessHttp, Error, TEXT("Failed to get HTTP router for port %d"), ServerPort);
		return;
	}

	// Bind routes using UObject delegate binding
	RouteHandles.Add(HttpRouter->BindRoute(
		FHttpPath(TEXT("/productivity/wellness/status")),
		EHttpServerRequestVerbs::VERB_GET,
		FHttpRequestHandler::CreateUObject(this, &UWellnessHttpServer::HandleStatusRequest)
	));

	RouteHandles.Add(HttpRouter->BindRoute(
		FHttpPath(TEXT("/productivity/wellness/statistics")),
		EHttpServerRequestVerbs::VERB_GET,
		FHttpRequestHandler::CreateUObject(this, &UWellnessHttpServer::HandleStatisticsRequest)
	));

	RouteHandles.Add(HttpRouter->BindRoute(
		FHttpPath(TEXT("/productivity/wellness/pomodoro")),
		EHttpServerRequestVerbs::VERB_GET,
		FHttpRequestHandler::CreateUObject(this, &UWellnessHttpServer::HandlePomodoroRequest)
	));

	RouteHandles.Add(HttpRouter->BindRoute(
		FHttpPath(TEXT("/productivity/wellness/streaks")),
		EHttpServerRequestVerbs::VERB_GET,
		FHttpRequestHandler::CreateUObject(this, &UWellnessHttpServer::HandleStreaksRequest)
	));

	// Start listening
	HttpServerModule.StartAllListeners();
	bIsRunning = true;

	UE_LOG(LogWellnessHttp, Log, TEXT("Wellness HTTP server started on port %d (localhost only: %s, CORS: %s)"),
		ServerPort,
		bRequireLocalhost ? TEXT("Yes") : TEXT("No"),
		bEnableCORS ? TEXT("Yes") : TEXT("No"));
}

void UWellnessHttpServer::StopServer()
{
	if (!bIsRunning)
	{
		return;
	}

	// Unbind all routes
	FHttpServerModule& HttpServerModule = FHttpServerModule::Get();
	TSharedPtr<IHttpRouter> HttpRouter = HttpServerModule.GetHttpRouter(ServerPort);

	if (HttpRouter.IsValid())
	{
		for (const FHttpRouteHandle& Handle : RouteHandles)
		{
			HttpRouter->UnbindRoute(Handle);
		}
	}

	RouteHandles.Empty();
	HttpServerModule.StopAllListeners();
	bIsRunning = false;
	WellnessSubsystemRef.Reset();

	UE_LOG(LogWellnessHttp, Log, TEXT("Wellness HTTP server stopped"));
}

// ============================================================================
// ROUTE HANDLERS
// ============================================================================

bool UWellnessHttpServer::HandleStatusRequest(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	OnHttpRequestReceived.Broadcast(TEXT("/productivity/wellness/status"), Request.PeerAddress.IsValid() ? Request.PeerAddress->ToString(false) : TEXT("unknown"));
	return SendJsonResponse(BuildStatusJson(), OnComplete);
}

bool UWellnessHttpServer::HandleStatisticsRequest(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	OnHttpRequestReceived.Broadcast(TEXT("/productivity/wellness/statistics"), Request.PeerAddress.IsValid() ? Request.PeerAddress->ToString(false) : TEXT("unknown"));
	return SendJsonResponse(BuildStatisticsJson(), OnComplete);
}

bool UWellnessHttpServer::HandlePomodoroRequest(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	OnHttpRequestReceived.Broadcast(TEXT("/productivity/wellness/pomodoro"), Request.PeerAddress.IsValid() ? Request.PeerAddress->ToString(false) : TEXT("unknown"));
	return SendJsonResponse(BuildPomodoroJson(), OnComplete);
}

bool UWellnessHttpServer::HandleStreaksRequest(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	OnHttpRequestReceived.Broadcast(TEXT("/productivity/wellness/streaks"), Request.PeerAddress.IsValid() ? Request.PeerAddress->ToString(false) : TEXT("unknown"));
	return SendJsonResponse(BuildStreaksJson(), OnComplete);
}

// ============================================================================
// JSON BUILDERS
// ============================================================================

TSharedRef<FJsonObject> UWellnessHttpServer::BuildStatusJson() const
{
	TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();

	UBreakWellnessSubsystem* Subsystem = WellnessSubsystemRef.Get();
	if (!Subsystem)
	{
		Json->SetStringField(TEXT("error"), TEXT("Wellness subsystem unavailable"));
		return Json;
	}

	Json->SetStringField(TEXT("status"), Subsystem->GetWellnessStatusDisplayString());
	Json->SetNumberField(TEXT("minutesSinceLastBreak"), Subsystem->GetMinutesSinceLastBreak());
	Json->SetBoolField(TEXT("wellnessEnabled"), Subsystem->IsWellnessEnabled());

	// Status color
	FLinearColor Color = Subsystem->GetWellnessStatusColor();
	TSharedRef<FJsonObject> ColorObject = MakeShared<FJsonObject>();
	ColorObject->SetNumberField(TEXT("r"), Color.R);
	ColorObject->SetNumberField(TEXT("g"), Color.G);
	ColorObject->SetNumberField(TEXT("b"), Color.B);
	ColorObject->SetNumberField(TEXT("a"), Color.A);
	Json->SetObjectField(TEXT("statusColor"), ColorObject);

	// On break status from smart break detector
	USmartBreakDetector* BreakDetector = Subsystem->GetSmartBreakDetector();
	Json->SetBoolField(TEXT("isOnBreak"), BreakDetector ? BreakDetector->IsOnDetectedBreak() : false);

	return Json;
}

TSharedRef<FJsonObject> UWellnessHttpServer::BuildStatisticsJson() const
{
	TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();

	UBreakWellnessSubsystem* Subsystem = WellnessSubsystemRef.Get();
	if (!Subsystem)
	{
		Json->SetStringField(TEXT("error"), TEXT("Wellness subsystem unavailable"));
		return Json;
	}

	FWellnessStatistics Stats = Subsystem->GetWellnessStatistics();

	Json->SetNumberField(TEXT("todayWorkMinutes"), Stats.TodayWorkMinutes);
	Json->SetNumberField(TEXT("todayBreakMinutes"), Stats.TodayBreakMinutes);
	Json->SetNumberField(TEXT("todayPomodorosCompleted"), Stats.TodayPomodorosCompleted);
	Json->SetNumberField(TEXT("todayStretchesCompleted"), Stats.TodayStretchesCompleted);
	Json->SetNumberField(TEXT("averageBreakQuality"), Stats.AverageBreakQuality);
	Json->SetNumberField(TEXT("minutesSinceLastBreak"), Stats.MinutesSinceLastBreak);
	Json->SetStringField(TEXT("currentStatus"), UEnum::GetValueAsString(Stats.CurrentStatus));

	return Json;
}

TSharedRef<FJsonObject> UWellnessHttpServer::BuildPomodoroJson() const
{
	TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();

	UBreakWellnessSubsystem* Subsystem = WellnessSubsystemRef.Get();
	if (!Subsystem)
	{
		Json->SetStringField(TEXT("error"), TEXT("Wellness subsystem unavailable"));
		return Json;
	}

	UPomodoroManager* Pomodoro = Subsystem->GetPomodoroManager();
	if (!Pomodoro)
	{
		Json->SetStringField(TEXT("error"), TEXT("Pomodoro manager unavailable"));
		return Json;
	}

	Json->SetStringField(TEXT("state"), Pomodoro->GetStateDisplayName());
	Json->SetNumberField(TEXT("remainingSeconds"), Pomodoro->GetRemainingSeconds());
	Json->SetNumberField(TEXT("elapsedSeconds"), Pomodoro->GetElapsedSeconds());
	Json->SetNumberField(TEXT("progress"), Pomodoro->GetIntervalProgress());
	Json->SetStringField(TEXT("formattedRemaining"), Pomodoro->GetFormattedRemainingTime());
	Json->SetNumberField(TEXT("completedWorkIntervals"), Pomodoro->GetCompletedWorkIntervals());
	Json->SetNumberField(TEXT("intervalsUntilLongBreak"), Pomodoro->GetIntervalsUntilLongBreak());

	return Json;
}

TSharedRef<FJsonObject> UWellnessHttpServer::BuildStreaksJson() const
{
	TSharedRef<FJsonObject> Json = MakeShared<FJsonObject>();

	UBreakWellnessSubsystem* Subsystem = WellnessSubsystemRef.Get();
	if (!Subsystem)
	{
		Json->SetStringField(TEXT("error"), TEXT("Wellness subsystem unavailable"));
		return Json;
	}

	UHabitStreakTracker* Tracker = Subsystem->GetHabitStreakTracker();
	if (!Tracker)
	{
		Json->SetStringField(TEXT("error"), TEXT("Habit tracker unavailable"));
		return Json;
	}

	FHabitStreakData Data = Tracker->GetStreakData();
	FDailyHabitRecord Today = Tracker->GetTodayRecord();

	Json->SetNumberField(TEXT("currentStreak"), Data.CurrentStreak);
	Json->SetNumberField(TEXT("longestStreak"), Data.LongestStreak);
	Json->SetNumberField(TEXT("totalDaysTracked"), Data.TotalDaysTracked);
	Json->SetNumberField(TEXT("todayProgress"), Tracker->GetTodayProgress());
	Json->SetBoolField(TEXT("allGoalsMetToday"), Today.bMetAllGoals);

	// Today's progress breakdown
	TSharedRef<FJsonObject> TodayObject = MakeShared<FJsonObject>();
	TodayObject->SetNumberField(TEXT("stretches"), Today.StretchesCompleted);
	TodayObject->SetNumberField(TEXT("breaks"), Today.BreaksTaken);
	TodayObject->SetNumberField(TEXT("pomodoros"), Today.PomodorosCompleted);
	TodayObject->SetBoolField(TEXT("metStretchGoal"), Today.bMetStretchGoal);
	TodayObject->SetBoolField(TEXT("metBreakGoal"), Today.bMetBreakGoal);
	TodayObject->SetBoolField(TEXT("metPomodoroGoal"), Today.bMetPomodoroGoal);
	Json->SetObjectField(TEXT("today"), TodayObject);

	return Json;
}

// ============================================================================
// RESPONSE HELPERS
// ============================================================================

bool UWellnessHttpServer::SendJsonResponse(const TSharedRef<FJsonObject>& Json, const FHttpResultCallback& OnComplete)
{
	// Serialize JSON to string
	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(Json, Writer);

	// Build response
	TUniquePtr<FHttpServerResponse> Response = CreateJsonResponse(JsonString);
	OnComplete(MoveTemp(Response));

	return true;
}

TUniquePtr<FHttpServerResponse> UWellnessHttpServer::CreateJsonResponse(const FString& JsonString)
{
	TUniquePtr<FHttpServerResponse> Response = FHttpServerResponse::Create(JsonString, TEXT("application/json"));

	// Add CORS headers if enabled (allows browser dashboards to access the API)
	if (bEnableCORS)
	{
		Response->Headers.Add(TEXT("Access-Control-Allow-Origin"), { TEXT("*") });
		Response->Headers.Add(TEXT("Access-Control-Allow-Methods"), { TEXT("GET, OPTIONS") });
		Response->Headers.Add(TEXT("Access-Control-Allow-Headers"), { TEXT("Content-Type") });
	}

	return Response;
}
