// ============================================================================
// StructuredLoggingModule.cpp
// Developer: Marcus Daley
// Date: January 25, 2026
// Plugin: StructuredLogging
// ============================================================================
// Purpose:
// Plugin module implementation. Handles plugin lifecycle (startup/shutdown).
// The actual logging functionality is in UStructuredLoggingSubsystem which
// auto-initializes per GameInstance.
//
// Usage:
// This file is only for plugin registration - developers use the subsystem API.
// ============================================================================

#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogStructuredLogging, Log, All);

class FStructuredLoggingModule : public IModuleInterface
{
public:
	// Module startup
	virtual void StartupModule() override
	{
		UE_LOG(LogStructuredLogging, Log, TEXT("Structured Logging Plugin initialized"));
	}

	// Module shutdown
	virtual void ShutdownModule() override
	{
		UE_LOG(LogStructuredLogging, Log, TEXT("Structured Logging Plugin shutdown"));
	}
};

IMPLEMENT_MODULE(FStructuredLoggingModule, StructuredLogging)
