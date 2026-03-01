// ============================================================================
// BrightForgeSettings.cpp
// Developer: Marcus Daley
// Date: February 2026
// Project: BrightForgeConnect Plugin
// ============================================================================
// Purpose:
// Implementation of BrightForge Connect project settings.
// ============================================================================

#include "Core/BrightForgeSettings.h"

DEFINE_LOG_CATEGORY(LogBrightForgeSettings);

UBrightForgeSettings::UBrightForgeSettings()
	: ServerUrl(TEXT("http://localhost:3847"))
	, bAutoApplyDefaultMaterial(true)
	, bAutoOpenImportedAsset(true)
	, DefaultGenerationType(EBrightForgeGenerationType::Full)
	, StatusPollingIntervalMs(2000)
{
	DefaultImportPath.Path = TEXT("/Game/BrightForge/Generated");

	UE_LOG(LogBrightForgeSettings, Log, TEXT("BrightForgeSettings constructed with defaults (Server: %s)"), *ServerUrl);
}

UBrightForgeSettings* UBrightForgeSettings::Get()
{
	return GetMutableDefault<UBrightForgeSettings>();
}

#if WITH_EDITOR

FText UBrightForgeSettings::GetSectionText() const
{
	return NSLOCTEXT("BrightForgeConnect", "SettingsSectionText", "BrightForge Connect");
}

FText UBrightForgeSettings::GetSectionDescription() const
{
	return NSLOCTEXT("BrightForgeConnect", "SettingsSectionDescription",
		"Configure the BrightForge Connect plugin — server URL, import paths, and generation defaults.");
}

void UBrightForgeSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.Property
		? PropertyChangedEvent.Property->GetFName()
		: NAME_None;

	UE_LOG(LogBrightForgeSettings, Log, TEXT("BrightForgeSettings property changed: %s"), *PropertyName.ToString());

	// Clamp polling interval
	StatusPollingIntervalMs = FMath::Clamp(StatusPollingIntervalMs, 500, 10000);

	// Ensure ServerUrl is not empty
	if (ServerUrl.IsEmpty())
	{
		ServerUrl = TEXT("http://localhost:3847");
		UE_LOG(LogBrightForgeSettings, Warning, TEXT("ServerUrl was empty — reset to default"));
	}

	SaveConfig();
}

#endif
