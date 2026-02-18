// AAIC_CodeBaseAgentController - AI controller with perception
// Author: Marcus Daley
// Date: 9/26/2025
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GenericTeamAgentInterface.h" 
#include "AIC_CodeBaseAgentController.generated.h"
class UBehaviorTree;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class ASpawner;
struct FAIStimulus;
// AI Controller with team support and Observer pattern
// - Implements IGenericTeamAgentInterface for AI perception team filtering
// - Subscribes to Spawner's faction assignment broadcasts (Observer pattern)
// - Updates Blackboard with team data for BehaviorTree decisions
//
// Usage: Attached to BaseAgent, listens to Spawner broadcasts for team assignment

DECLARE_LOG_CATEGORY_EXTERN(LogAgentController, Log, All);
UCLASS()
class END2507_API AAIC_CodeBaseAgentController : public AAIController
{
	GENERATED_BODY()
public:

    AAIC_CodeBaseAgentController();
    //Sets this controller's team ID and updates perception system
    virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
    //Returns this controller's current team ID
    virtual FGenericTeamId GetGenericTeamId() const override;



    UFUNCTION()
    void HandleHealthChanged(float HealthRatio);

    UFUNCTION(BlueprintCallable, Category = "Faction")
    void UpdateFactionFromPawn(int32 FactionID, const FLinearColor& FactionColor);

protected:
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;
    virtual void Tick(float DeltaTime) override;

    UFUNCTION()
    void HandlePawnDeath(AActor* DestroyedActor);

    void UpdateBlackboardHealth(float HealthPercent);



    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
    UAIPerceptionComponent* AIPerception;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception")
    float SightRadius;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception")
    float LoseSightRadius;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception")
    float PeripheralVisionAngle;    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception")
    float AutoSuccessRange;

    // Hearing perception settings
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception")
    float HearingRange;
private:

    UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
    UBehaviorTree* BehaviorTreeAsset;

    UPROPERTY()
    UAISenseConfig_Sight* SightConfig;
    UPROPERTY()
    UAISenseConfig_Hearing* HearingConfig;
    FName PlayerKeyName;
    FName HasTargetKeyName;

    // Health and combat state
    FName HealthRatioKeyName;
    FName CanAttackKeyName;
    FName ShouldFleeKeyName;

    // Flight and movement
    FName ShouldFlyKeyName;
    // Key name for faction ID in Blackboard 
    const FName BB_FactionID = TEXT("FactionID");
    FGenericTeamId TeamId;
    // Key name for faction color in Blackboard
    const FName BB_FactionColor = TEXT("FactionColor");

    // Signal/distraction response
    FName DistractionLocationKeyName;
    FName HeardDistractionKeyName;
    FName MatchStartedKeyName;
    FName NewWaveStartedKeyName;
    void SetupPerception();
    void BindPawnDelegates(APawn* InPawn);
    void UnbindPawnDelegates();
    // Called when perception is updated 
    UFUNCTION()
    void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

    UFUNCTION()
    void HandleSightPerception(AActor* Actor, const FAIStimulus& Stimulus);

    UFUNCTION()
    void HandleHearingPerception(AActor* Actor, const FAIStimulus& Stimulus);
    void ForgetPlayer();

    FVector ValidateColorForBlackboard(const FLinearColor& InColor) const;
};
