// ACodeBaseAgent - AI agent with attack behavior
// Author: [Marcus Daley]
// Date: [9/26/2025]
#pragma once

#include "CoreMinimal.h"
#include "Code/Actors/BaseCharacter.h"
#include "Code/IEnemyInterface.h"
#include "Code/CodeFactionInterface.h"
#include "GenericTeamAgentInterface.h"
#include "Code/Quidditch/QuidditchTypes.h"
#include "BaseAgent.generated.h"

class UMaterialInstanceDynamic;
class ABaseRifle;
class UQuidditchAgentData;
class UAC_StaminaComponent;
//DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAIShootDelegate);

UCLASS()
class END2507_API ABaseAgent : public ABaseCharacter, public IEnemyInterface, public IGenericTeamAgentInterface, public ICodeFactionInterface

{
	GENERATED_BODY()
public:
	ABaseAgent();
	
	void EnemyAttack(AActor* Target) override;
	void EnemyReload(AActor* Target) override;
	UFUNCTION(BlueprintCallable, Category = "AI Appearance")
	void SetAgentColor(const FLinearColor& NewColor);

	UFUNCTION(BlueprintPure, Category = "Weapon")
	ABaseRifle* GetEquippedRifle() const;

	// Getters for faction configuration (used by controller in OnPossess)
	UFUNCTION(BlueprintPure, Category = "Faction")
	int32 GetPlacedFactionID() const { return PlacedAgentFactionID; }

	UFUNCTION(BlueprintPure, Category = "Faction")
	FLinearColor GetPlacedFactionColor() const { return PlacedAgentFactionColor; }

	// ========================================================================
	// QUIDDITCH CONFIGURATION
	// Data asset is PRIMARY source; falls back to manual properties if not set
	// Controller reads these at OnPossess and registers with GameMode
	// ========================================================================

	// Returns team from data asset (if set) or PlacedQuidditchTeam (fallback)
	UFUNCTION(BlueprintPure, Category = "Quidditch")
	EQuidditchTeam GetQuidditchTeam() const;

	// Returns role from data asset (if set) or PlacedPreferredRole (fallback)
	UFUNCTION(BlueprintPure, Category = "Quidditch")
	EQuidditchRole GetPreferredQuidditchRole() const;

	// Returns the assigned data asset (may be null)
	UFUNCTION(BlueprintPure, Category = "Quidditch")
	UQuidditchAgentData* GetAgentDataAsset() const { return AgentDataAsset; }

	virtual void OnFactionAssigned_Implementation(int32 FactionID, const FLinearColor& FactionColor) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual bool CanPickAmmo() const override;
protected:
	virtual void BeginPlay() override;
	
	virtual void HandleHurt(float HealthRatio) ;
	virtual void HandleDeathStart(float Ratio) override;
	//Appearance
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI Appearance")
	FLinearColor AgentColor;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI Appearance")
	FName MaterialParameterName;
	// Faction configuration for agents placed in level (not spawned)
	UPROPERTY(EditInstanceOnly, Category = "Faction", meta = (ClampMin = "0", ClampMax = "255"))
	int32 PlacedAgentFactionID;

	UPROPERTY(EditInstanceOnly, Category = "Faction")
	FLinearColor PlacedAgentFactionColor;

	// ========================================================================
	// QUIDDITCH CONFIGURATION - Set per-instance in level
	// Level designer selects team and preferred role for each placed agent
	// Controller reads these at possess time
	// ========================================================================

	// Which Quidditch team this agent belongs to (set per-instance in level)
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Quidditch")
	EQuidditchTeam PlacedQuidditchTeam;

	// Preferred Quidditch role (GameMode may assign different if slot is full)
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Quidditch")
	EQuidditchRole PlacedPreferredRole;

	// ========================================================================
	// DATA ASSET - PRIMARY configuration source
	// If assigned, Team/Role are read from this asset (overrides manual props)
	// If not assigned, uses PlacedQuidditchTeam/PlacedPreferredRole as fallback
	// ========================================================================

	// Primary data asset for this agent - designer assigns in level
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Quidditch")
	TObjectPtr<UQuidditchAgentData> AgentDataAsset;

	// Cached team ID - stored locally as backup when controller isn't ready
	// or when controller type doesn't match expected cast
	FGenericTeamId CachedTeamId;

	// ========================================================================
	// FLIGHT SUPPORT - Required for AI broom flight
	// ========================================================================

	// Stamina component for flight (required by AC_BroomComponent)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAC_StaminaComponent> StaminaComponent;

public:
	// Getter for stamina component (used by BroomComponent)
	UFUNCTION(BlueprintPure, Category = "Agent|Stamina")
	UAC_StaminaComponent* GetStaminaComponent() const { return StaminaComponent; }

private:


	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> DynamicMaterials;

	// Notify behavior tree that an action has completed 
	UFUNCTION(BlueprintCallable, Category = "AI Behavior")
	void HandleActionFinished();
	// Updates blackboard with current ammo ratio
// Called when rifle ammo changes or at initialization
	void UpdateBlackboardAmmo();
	// Delegate handler for rifle ammo changes
// Updates blackboard ammo value for AI decision making
	UFUNCTION()
	void HandleAmmoChanged(float CurrentAmmo, float MaxAmmo);


	void UpdateBlackboardHealth(float HealthRatio);
	void SetupAgentAppearance();
};
