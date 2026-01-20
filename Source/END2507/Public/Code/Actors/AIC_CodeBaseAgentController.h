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
class ASpawner;
struct FAIStimulus;
/**
 * AI Controller with team support and Observer pattern
 * - Implements IGenericTeamAgentInterface for AI perception team filtering
 * - Subscribes to Spawner's faction assignment broadcasts (Observer pattern)
 * - Updates Blackboard with team data for BehaviorTree decisions
 *
 * Usage: Attached to BaseAgent, listens to Spawner broadcasts for team assignment
 */
UCLASS()
class END2507_API AAIC_CodeBaseAgentController : public AAIController
{
	GENERATED_BODY()
public:
    AAIC_CodeBaseAgentController();
    UFUNCTION(BlueprintCallable, Category = "Faction")
    void UpdateFactionFromPawn(int32 FactionID, const FLinearColor& FactionColor);

    //Sets this controller's team ID and updates perception system
    virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
    //Returns this controller's current team ID
    virtual FGenericTeamId GetGenericTeamId() const override;
protected:
 
    virtual void OnPossess(APawn* InPawn) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
    UAIPerceptionComponent* AIPerception;

    FVector ValidateColorForBlackboard(const FLinearColor& InColor) const;
private:

    UPROPERTY(EditDefaultsOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
    UBehaviorTree* BehaviorTreeAsset;

    UPROPERTY()
    UAISenseConfig_Sight* SightConfig;
  
    FName PlayerKeyName;
    FName HealthRatioKeyName;
    // Key name for faction ID in Blackboard 
    const FName BB_FactionID = TEXT("FactionID");

    // Key name for faction color in Blackboard
    const FName BB_FactionColor = TEXT("FactionColor");
    void SetupPerception();

    // Called when perception is updated 
    UFUNCTION()
    void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
    void ForgetPlayer();
};
