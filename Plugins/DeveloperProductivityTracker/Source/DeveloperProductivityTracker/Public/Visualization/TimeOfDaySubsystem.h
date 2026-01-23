// ============================================================================
// TimeOfDaySubsystem.h
// Developer: Marcus Daley
// Date: January 2026
// Project: DeveloperProductivityTracker Plugin
// ============================================================================
// Purpose:
// Manages time-of-day progression based on work session elapsed time.
// Drives atmospheric visualization changes throughout the work day.
//
// Architecture:
// Editor Subsystem that maps session time to visual time of day.
// Integrates with wellness system for atmosphere tinting.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "Tickable.h"
#include "Wellness/BreakWellnessSubsystem.h"
#include "TimeOfDaySubsystem.generated.h"

class UProductivitySkyConfig;

// Delegate for time of day changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimeOfDayChanged, float, NewTimeOfDay);

UCLASS()
class DEVELOPERPRODUCTIVITYTRACKER_API UTimeOfDaySubsystem : public UEditorSubsystem, public FTickableEditorObject
{
	GENERATED_BODY()

public:
	UTimeOfDaySubsystem();

	// USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// FTickableEditorObject interface
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return bIsEnabled; }

	// ========================================================================
	// TIME QUERIES
	// ========================================================================

	// Get current time of day (0.0 = midnight, 0.5 = noon, 1.0 = midnight)
	UFUNCTION(BlueprintPure, Category = "Productivity|Sky")
	float GetCurrentTimeOfDay() const { return CurrentTimeOfDay; }

	// Get time as human readable string (e.g., "2:30 PM")
	UFUNCTION(BlueprintPure, Category = "Productivity|Sky")
	FString GetTimeDisplayString() const;

	// Check if currently daytime
	UFUNCTION(BlueprintPure, Category = "Productivity|Sky")
	bool IsDaytime() const;

	// Get current wellness tint multiplier
	UFUNCTION(BlueprintPure, Category = "Productivity|Sky")
	FLinearColor GetCurrentWellnessTint() const { return CurrentWellnessTint; }

	// ========================================================================
	// CONFIGURATION
	// ========================================================================

	// Set the sky configuration asset
	UFUNCTION(BlueprintCallable, Category = "Productivity|Sky")
	void SetSkyConfig(UProductivitySkyConfig* Config);

	// Get current sky configuration
	UFUNCTION(BlueprintPure, Category = "Productivity|Sky")
	UProductivitySkyConfig* GetSkyConfig() const { return SkyConfig; }

	// Enable/disable time of day system
	UFUNCTION(BlueprintCallable, Category = "Productivity|Sky")
	void SetEnabled(bool bEnabled);

	// Manually set time of day (for preview/testing)
	UFUNCTION(BlueprintCallable, Category = "Productivity|Sky")
	void SetTimeOfDay(float Time);

	// Reset to session start time
	UFUNCTION(BlueprintCallable, Category = "Productivity|Sky")
	void ResetToSessionStart();

	// ========================================================================
	// DELEGATES
	// ========================================================================

	UPROPERTY(BlueprintAssignable, Category = "Productivity|Events")
	FOnTimeOfDayChanged OnTimeOfDayChanged;

private:
	// State
	bool bIsEnabled;
	float CurrentTimeOfDay;
	float PreviousTimeOfDay;
	FLinearColor CurrentWellnessTint;
	FLinearColor TargetWellnessTint;
	float WellnessTintTransitionProgress;

	// Configuration
	UPROPERTY()
	UProductivitySkyConfig* SkyConfig;

	// Methods
	void UpdateTimeOfDay(float DeltaTime);
	void UpdateWellnessTint(float DeltaTime);
	FLinearColor GetTintForWellnessStatus(EWellnessStatus Status) const;

	// Delegate handler - must be UFUNCTION for dynamic delegate binding
	UFUNCTION()
	void HandleWellnessStatusChanged(EWellnessStatus NewStatus);
};
