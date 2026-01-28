// ============================================================================
// BTTask_PositionInGoal.cpp
// Developer: Marcus Daley
// Date: January 24, 2026
// Project: END2507
// ============================================================================
// Algorithm: Flock.cs Cohesion (ADAPTED for Keeper positioning)
//
// Standard Cohesion: Position = CenterOfMass(Neighbors)
// Keeper Cohesion:   Position = GoalCenter + ThreatOffset
//
// ThreatOffset = Direction(Goal → Threat) * ResponseStrength
// ResponseStrength = ThreatResponseFactor * (1 - Distance/MaxThreatDistance)
//
// Result: Keeper stays near goal but shifts toward incoming threats.
// ============================================================================

#include "Code/AI/Quidditch/BTTask_PositionInGoal.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

UBTTask_PositionInGoal::UBTTask_PositionInGoal()
    : MaxDefenseRadius(800.0f)
    , MinDefenseRadius(200.0f)
    , DefenseHeight(300.0f)
    , ThreatResponseFactor(0.7f)
    , MaxThreatDistance(3000.0f)
{
    NodeName = TEXT("Position In Goal");
    bNotifyTick = false;

    // Setup key filters
    GoalCenterKey.AddVectorFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTTask_PositionInGoal, GoalCenterKey));

    ThreatActorKey.AddObjectFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTTask_PositionInGoal, ThreatActorKey),
        AActor::StaticClass());

    DefensePositionKey.AddVectorFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTTask_PositionInGoal, DefensePositionKey));
}

void UBTTask_PositionInGoal::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);

    if (UBlackboardData* BBAsset = GetBlackboardAsset())
    {
        GoalCenterKey.ResolveSelectedKey(*BBAsset);
        ThreatActorKey.ResolveSelectedKey(*BBAsset);
        DefensePositionKey.ResolveSelectedKey(*BBAsset);
    }
}

EBTNodeResult::Type UBTTask_PositionInGoal::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();

    if (!AIController || !Blackboard)
    {
        return EBTNodeResult::Failed;
    }

    APawn* OwnerPawn = AIController->GetPawn();
    if (!OwnerPawn)
    {
        return EBTNodeResult::Failed;
    }

    // Get goal center
    FVector GoalCenter = FVector::ZeroVector;
    if (GoalCenterKey.IsSet())
    {
        GoalCenter = Blackboard->GetValueAsVector(GoalCenterKey.SelectedKeyName);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[PositionInGoal] No goal center defined"));
        return EBTNodeResult::Failed;
    }

    // Get threat (optional - defend center if no threat)
    AActor* Threat = nullptr;
    if (ThreatActorKey.IsSet())
    {
        Threat = Cast<AActor>(Blackboard->GetValueAsObject(ThreatActorKey.SelectedKeyName));
    }

    // Calculate defense position using Cohesion-like algorithm
    FVector DefensePosition = CalculateDefensePosition(GoalCenter, Threat);

    // Write to Blackboard
    if (DefensePositionKey.IsSet())
    {
        Blackboard->SetValueAsVector(DefensePositionKey.SelectedKeyName, DefensePosition);

        UE_LOG(LogTemp, Verbose,
            TEXT("[PositionInGoal] %s defense position set to (%.0f, %.0f, %.0f)"),
            *OwnerPawn->GetName(), DefensePosition.X, DefensePosition.Y, DefensePosition.Z);
    }

    return EBTNodeResult::Succeeded;
}

FVector UBTTask_PositionInGoal::CalculateDefensePosition(const FVector& GoalCenter, AActor* Threat) const
{
    // Start at goal center with height offset
    FVector BasePosition = GoalCenter;
    BasePosition.Z += DefenseHeight;

    // If no threat, stay at base position
    if (!Threat)
    {
        return BasePosition;
    }

    FVector ThreatPos = Threat->GetActorLocation();

    // Calculate direction from goal to threat
    FVector GoalToThreat = ThreatPos - GoalCenter;
    float ThreatDistance = GoalToThreat.Size();

    // Normalize for direction only (keep horizontal plane for positioning)
    FVector ThreatDirection = GoalToThreat;
    ThreatDirection.Z = 0.0f;  // Stay at defense height
    ThreatDirection.Normalize();

    // Calculate response strength (Cohesion-inspired)
    // Closer threats = stronger response
    float DistanceFactor = FMath::Clamp(1.0f - (ThreatDistance / MaxThreatDistance), 0.0f, 1.0f);
    float ResponseStrength = ThreatResponseFactor * DistanceFactor;

    // Calculate offset from goal center toward threat
    float OffsetDistance = FMath::Lerp(MinDefenseRadius, MaxDefenseRadius, ResponseStrength);

    // Final position = Goal center + offset toward threat
    FVector DefensePosition = BasePosition + ThreatDirection * OffsetDistance;

    // Clamp to defense radius
    FVector FromGoal = DefensePosition - GoalCenter;
    FromGoal.Z = 0.0f;
    float HorizontalDist = FromGoal.Size();

    if (HorizontalDist > MaxDefenseRadius)
    {
        FromGoal = FromGoal.GetSafeNormal() * MaxDefenseRadius;
        DefensePosition = GoalCenter + FromGoal;
        DefensePosition.Z = BasePosition.Z;
    }

    UE_LOG(LogTemp, Verbose,
        TEXT("[PositionInGoal] Threat at %.0f units, Response=%.2f, Offset=%.0f"),
        ThreatDistance, ResponseStrength, OffsetDistance);

    return DefensePosition;
}

FString UBTTask_PositionInGoal::GetStaticDescription() const
{
    FString Description = TEXT("Cohesion-Based Goal Defense\n");
    Description += FString::Printf(TEXT("Radius: %.0f - %.0f\n"), MinDefenseRadius, MaxDefenseRadius);
    Description += FString::Printf(TEXT("Response: %.0f%% | Height: %.0f"),
        ThreatResponseFactor * 100.0f, DefenseHeight);

    return Description;
}

