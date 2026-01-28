// ============================================================================
// QuidditchAgentData.h
// Primary Data Asset for Quidditch agent configuration
//
// Developer: Marcus Daley
// Date: January 25, 2026
// Project: WizardJam / GAR (END2507)
//
// PURPOSE:
// UPrimaryDataAsset that defines a complete agent configuration for Quidditch.
// Allows designers to create new agent types without C++ recompilation.
// Used by BP_QuidditchAgent at BeginPlay to configure stats, appearance, and AI.
//
// USAGE:
// 1. Create Data Asset in editor: Right-click → Miscellaneous → Data Asset
// 2. Select QuidditchAgentData as parent class
// 3. Configure properties (DisplayName, AgentRole, FlightSpeed, BehaviorTreeAsset, etc.)
// 4. Assign to BP_QuidditchAgent's AgentDataAsset property
// 5. Agent auto-configures on spawn
//
// EXAMPLES:
// - DA_SeekerAggressive: Fast (1200), risky (0.9), uses BT_Seeker
// - DA_SeekerDefensive: Slow (900), cautious (0.3), uses BT_Seeker
// - DA_ChaserStriker: Medium (1000), goal-oriented, uses BT_Chaser
//
// ARCHITECTURE:
// Data-driven design pattern - separates agent configuration (content)
// from agent logic (code). Industry standard for scalable AI systems.
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "QuidditchTypes.h"
#include "Code/GameModes/QuidditchGameMode.h"
#include "QuidditchAgentData.generated.h"

// Forward declarations
class UBehaviorTree;
class USkeletalMesh;
class UMaterialInterface;

// ============================================================================
// AGENT PERFORMANCE STATS STRUCT
// Defines movement, stamina, and aggression parameters
// ============================================================================

USTRUCT(BlueprintType)
struct END2507_API FAgentPerformanceStats
{
	GENERATED_BODY()

	// Flight speed in units/sec
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Performance")
	float FlightSpeed;

	// How quickly the agent accelerates to max speed
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Performance")
	float Acceleration;

	// Maximum stamina pool
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Performance")
	float MaxStamina;

	// Stamina regeneration rate per second
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Performance")
	float StaminaRegenRate;

	// How aggressive is this agent (0.0 = passive, 1.0 = highly aggressive)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Performance", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AggressionLevel;

	FAgentPerformanceStats()
		: FlightSpeed(1000.0f)
		, Acceleration(600.0f)
		, MaxStamina(100.0f)
		, StaminaRegenRate(10.0f)
		, AggressionLevel(0.5f)
	{}
};

// ============================================================================
// AGENT VISUAL CONFIGURATION STRUCT
// Defines mesh, materials, and visual appearance
// ============================================================================

USTRUCT(BlueprintType)
struct END2507_API FAgentVisualConfig
{
	GENERATED_BODY()

	// Skeletal mesh for this agent
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<USkeletalMesh> AgentMesh;

	// Materials to apply to mesh (indexed by material slot)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
	TArray<TObjectPtr<UMaterialInterface>> TeamMaterials;

	// Scale multiplier for this agent
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
	FVector MeshScale;

	FAgentVisualConfig()
		: AgentMesh(nullptr)
		, MeshScale(FVector::OneVector)
	{}
};

// ============================================================================
// AGENT AI CONFIGURATION STRUCT
// Defines behavior tree, perception, and decision-making parameters
// ============================================================================

USTRUCT(BlueprintType)
struct END2507_API FAgentAIConfig
{
	GENERATED_BODY()

	// Behavior tree asset for this agent type
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI Behavior")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	// AI sight perception radius
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI Perception")
	float SightRadius;

	// AI hearing perception radius
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI Perception")
	float HearingRadius;

	// Peripheral vision angle in degrees
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI Perception")
	float PeripheralVisionAngle;

	// How often the AI thinks (lower = faster reactions, higher cost)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI Behavior")
	float DecisionSpeed;

	// How willing is this agent to take risks (0.0 = safe, 1.0 = risky)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI Behavior", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RiskTolerance;

	FAgentAIConfig()
		: BehaviorTreeAsset(nullptr)
		, SightRadius(2000.0f)
		, HearingRadius(1000.0f)
		, PeripheralVisionAngle(90.0f)
		, DecisionSpeed(0.5f)
		, RiskTolerance(0.5f)
	{}
};

// ============================================================================
// AGENT QUIDDITCH-SPECIFIC CONFIGURATION STRUCT
// Defines Quidditch gameplay parameters (positioning, chase, intercept)
// ============================================================================

USTRUCT(BlueprintType)
struct END2507_API FAgentQuidditchConfig
{
	GENERATED_BODY()

	// Offset from staging zone position (allows slight variation)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quidditch")
	FVector PreferredPositionOffset;

	// How aggressively does this agent chase targets (0.0 = cautious, 1.0 = relentless)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quidditch", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ChaseAggression;

	// How accurately can this agent predict intercept points (0.0 = poor, 1.0 = perfect)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quidditch", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float InterceptPredictionSkill;

	// Formation preference (for future flocking behavior)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quidditch")
	FString FormationPreference;

	FAgentQuidditchConfig()
		: PreferredPositionOffset(FVector::ZeroVector)
		, ChaseAggression(0.5f)
		, InterceptPredictionSkill(0.5f)
		, FormationPreference(TEXT("Solo"))
	{}
};

// ============================================================================
// QUIDDITCH AGENT DATA ASSET
// Primary Data Asset that combines all agent configuration
// ============================================================================

UCLASS(BlueprintType)
class END2507_API UQuidditchAgentData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// ========================================================================
	// AGENT IDENTITY
	// ========================================================================

	// Display name shown in UI and logs
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Agent Identity")
	FText DisplayName;

	// Quidditch role for this agent type
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Agent Identity")
	EQuidditchRole AgentRole;

	// Team affiliation for this agent configuration
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Agent Identity")
	EQuidditchTeam TeamAffiliation;

	// ========================================================================
	// CONFIGURATION STRUCTS
	// ========================================================================

	// Performance stats (speed, stamina, aggression)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Performance")
	FAgentPerformanceStats PerformanceStats;

	// Visual appearance (mesh, materials, scale)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
	FAgentVisualConfig VisualConfig;

	// AI configuration (behavior tree, perception, decision-making)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	FAgentAIConfig AIConfig;

	// Quidditch-specific settings (positioning, chase, intercept)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quidditch")
	FAgentQuidditchConfig QuidditchConfig;

	// ========================================================================
	// PRIMARY DATA ASSET INTERFACE
	// ========================================================================

	// Returns the primary asset ID for this data asset
	// Format: QuidditchAgent:AssetName
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
