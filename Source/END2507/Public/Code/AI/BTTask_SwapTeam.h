// ============================================================================
// BTTask_SwapTeam.h
// Developer: Marcus Daley
// Date: January 23, 2026
// Project: END2507
// ============================================================================
// Purpose:
// BT Task that executes team swap when AI is selected to make room for player.
// Updates team ID and visual appearance, then notifies GameMode.
//
// Gas Station Pattern:
// This task runs when the "testOver" condition is detected (player joining).
// The selected AI swaps teams and changes house colors.
//
// Usage:
// 1. Add to BT under decorator [BB.bShouldSwapTeam == true]
// 2. Task reads current team from agent, swaps to opposite
// 3. Updates controller team ID and agent appearance
// 4. Notifies GameMode via ExecuteTeamSwap()
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "Code/GameModes/QuidditchGameMode.h"
#include "BTTask_SwapTeam.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBTTask_SwapTeam, Log, All);

UCLASS(meta = (DisplayName = "Swap Team"))
class END2507_API UBTTask_SwapTeam : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_SwapTeam();

    // ========================================================================
    // BTTaskNode Interface
    // ========================================================================

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

    virtual FString GetStaticDescription() const override;

    // ========================================================================
    // MANDATORY: FBlackboardKeySelector Initialization
    // ========================================================================

    virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

protected:
    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    // Blackboard key to clear after swap (Bool)
    UPROPERTY(EditDefaultsOnly, Category = "Blackboard")
    FBlackboardKeySelector ShouldSwapTeamKey;
};
