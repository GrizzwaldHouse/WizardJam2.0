// ============================================================================
// WellnessHttpServer.h
// Developer: Marcus Daley
// Date: February 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// HTTP API server that exposes wellness data for external tools like
// dashboards, Discord bots, or Bob-AICompanion automation.
//
// Architecture:
// Uses UE5's built-in FHttpServerModule to serve JSON endpoints.
// All data is read-only - external tools query but never modify state.
//
// NOTE: Requires "HTTPServer" module in Build.cs PrivateDependencyModuleNames.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "HttpRouteHandle.h"
#include "HttpResultCallback.h"
#include "HttpServerRequest.h"
#include "HttpServerResponse.h"
#include "WellnessHttpServer.generated.h"

// Forward declarations
class UBreakWellnessSubsystem;
class IHttpRouter;

// Log category
DECLARE_LOG_CATEGORY_EXTERN(LogWellnessHttp, Log, All);

// Delegate for request logging
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHttpRequestReceived, const FString&, Endpoint, const FString&, ClientIP);

UCLASS(BlueprintType)
class DEVELOPERPRODUCTIVITYTRACKER_API UWellnessHttpServer : public UObject
{
	GENERATED_BODY()

public:
	UWellnessHttpServer();

	// ========================================================================
	// SERVER CONTROLS
	// ========================================================================

	// Start the HTTP server with a reference to the wellness subsystem for data
	void StartServer(UBreakWellnessSubsystem* WellnessSubsystem);

	// Stop the HTTP server and unbind all routes
	UFUNCTION(BlueprintCallable, Category = "Productivity|API")
	void StopServer();

	// Check if server is currently running
	UFUNCTION(BlueprintPure, Category = "Productivity|API")
	bool IsServerRunning() const { return bIsRunning; }

	// Get the port the server is listening on
	UFUNCTION(BlueprintPure, Category = "Productivity|API")
	int32 GetActivePort() const { return ServerPort; }

	// ========================================================================
	// CONFIGURATION
	// ========================================================================

	// Port for the HTTP server
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "API|Config",
		meta = (ClampMin = "1024", ClampMax = "65535"))
	int32 ServerPort;

	// Only accept connections from localhost (security)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "API|Config")
	bool bRequireLocalhost;

	// Add CORS headers for browser dashboard access
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "API|Config")
	bool bEnableCORS;

	// ========================================================================
	// DELEGATES
	// ========================================================================

	// Fires on each HTTP request (for logging/monitoring)
	UPROPERTY(BlueprintAssignable, Category = "Productivity|Events")
	FOnHttpRequestReceived OnHttpRequestReceived;

private:
	// State
	bool bIsRunning;

	// Cached reference to wellness subsystem for data access
	TWeakObjectPtr<UBreakWellnessSubsystem> WellnessSubsystemRef;

	// Route handles for cleanup
	TArray<FHttpRouteHandle> RouteHandles;

	// Route handlers
	bool HandleStatusRequest(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	bool HandleStatisticsRequest(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	bool HandlePomodoroRequest(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	bool HandleStreaksRequest(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);

	// JSON builders
	TSharedRef<FJsonObject> BuildStatusJson() const;
	TSharedRef<FJsonObject> BuildStatisticsJson() const;
	TSharedRef<FJsonObject> BuildPomodoroJson() const;
	TSharedRef<FJsonObject> BuildStreaksJson() const;

	// Response helpers
	bool SendJsonResponse(const TSharedRef<FJsonObject>& Json, const FHttpResultCallback& OnComplete);
	TUniquePtr<FHttpServerResponse> CreateJsonResponse(const FString& JsonString);
};
