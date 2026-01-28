// ============================================================================
// BTDecorator_IsSeeker.h
// Behavior Tree decorator that checks if AI has Seeker role
//
// Developer: Marcus Daley
// Date: January 25, 2026
// Project: WizardJam (END2507)
//
// PURPOSE:
// Gates Seeker-specific behavior tree branches. Only allows execution if the
// AI agent's assigned role (from QuidditchGameMode) is Seeker.
//
// USAGE:
// 1. Add as decorator to Seeker behavior branch in BT_QuidditchAI
// 2. No configuration needed - queries GameMode automatically
// 3. Returns true if agent role == Seeker, false otherwise
// ============================================================================

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_IsSeeker.generated.h"

UCLASS(meta = (DisplayName = "Is Seeker Role"))
class END2507_API UBTDecorator_IsSeeker : public UBTDecorator
{
    GENERATED_BODY()

public:
    UBTDecorator_IsSeeker();

    // Check if AI agent has Seeker role
    virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

    // Description shown in Behavior Tree editor
    virtual FString GetStaticDescription() const override;
};
