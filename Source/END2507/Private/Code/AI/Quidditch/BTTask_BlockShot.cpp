// ============================================================================
// BTTask_BlockShot.cpp
// Developer: Marcus Daley
// Date: January 24, 2026
// Project: END2507
// ============================================================================
// Algorithm: PathSearch intercept (same as BTTask_PredictIntercept)
//
// Solve for intercept time t:
// (|V|² - S²)*t² + 2*(T-P)·V*t + |T-P|² = 0
//
// Where:
// P = Keeper position
// T = Quaffle position
// V = Quaffle velocity
// S = Keeper max speed
//
// If solution exists and t > 0, shot is blockable.
// Intercept point I = T + V*t
// ============================================================================

#include "Code/AI/Quidditch/BTTask_BlockShot.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

UBTTask_BlockShot::UBTTask_BlockShot()
    : KeeperMaxSpeed(1000.0f)
    , MinShotSpeed(500.0f)
    , MaxPredictionTime(3.0f)
    , BlockRadius(150.0f)
{
    NodeName = TEXT("Block Shot");
    bNotifyTick = false;

    // Setup key filters
    QuaffleKey.AddObjectFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTTask_BlockShot, QuaffleKey),
        AActor::StaticClass());

    QuaffleVelocityKey.AddVectorFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTTask_BlockShot, QuaffleVelocityKey));

    GoalCenterKey.AddVectorFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTTask_BlockShot, GoalCenterKey));

    InterceptPositionKey.AddVectorFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTTask_BlockShot, InterceptPositionKey));

    CanBlockKey.AddBoolFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTTask_BlockShot, CanBlockKey));
}

void UBTTask_BlockShot::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);

    if (UBlackboardData* BBAsset = GetBlackboardAsset())
    {
        QuaffleKey.ResolveSelectedKey(*BBAsset);
        QuaffleVelocityKey.ResolveSelectedKey(*BBAsset);
        GoalCenterKey.ResolveSelectedKey(*BBAsset);
        InterceptPositionKey.ResolveSelectedKey(*BBAsset);
        CanBlockKey.ResolveSelectedKey(*BBAsset);
    }
}

EBTNodeResult::Type UBTTask_BlockShot::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

    // Get Quaffle
    AActor* Quaffle = Cast<AActor>(Blackboard->GetValueAsObject(QuaffleKey.SelectedKeyName));
    if (!Quaffle)
    {
        if (CanBlockKey.IsSet())
        {
            Blackboard->SetValueAsBool(CanBlockKey.SelectedKeyName, false);
        }
        return EBTNodeResult::Failed;
    }

    // Get Quaffle velocity
    FVector QuaffleVel = FVector::ZeroVector;
    if (QuaffleVelocityKey.IsSet())
    {
        QuaffleVel = Blackboard->GetValueAsVector(QuaffleVelocityKey.SelectedKeyName);
    }
    else
    {
        QuaffleVel = Quaffle->GetVelocity();
    }

    // Check if Quaffle is moving fast enough to be a shot
    float QuaffleSpeed = QuaffleVel.Size();
    if (QuaffleSpeed < MinShotSpeed)
    {
        if (CanBlockKey.IsSet())
        {
            Blackboard->SetValueAsBool(CanBlockKey.SelectedKeyName, false);
        }
        UE_LOG(LogTemp, Verbose, TEXT("[BlockShot] Quaffle too slow: %.0f < %.0f"),
            QuaffleSpeed, MinShotSpeed);
        return EBTNodeResult::Failed;
    }

    // Get goal center
    FVector GoalCenter = FVector::ZeroVector;
    if (GoalCenterKey.IsSet())
    {
        GoalCenter = Blackboard->GetValueAsVector(GoalCenterKey.SelectedKeyName);
    }

    FVector QuafflePos = Quaffle->GetActorLocation();

    // Check if shot is heading toward our goal
    if (!IsHeadingTowardGoal(QuafflePos, QuaffleVel, GoalCenter))
    {
        if (CanBlockKey.IsSet())
        {
            Blackboard->SetValueAsBool(CanBlockKey.SelectedKeyName, false);
        }
        UE_LOG(LogTemp, Verbose, TEXT("[BlockShot] Shot not heading toward goal"));
        return EBTNodeResult::Failed;
    }

    // Calculate intercept point
    FVector KeeperPos = OwnerPawn->GetActorLocation();
    FVector InterceptPoint;
    float TimeToIntercept;

    bool bCanIntercept = CalculateIntercept(KeeperPos, QuafflePos, QuaffleVel,
        InterceptPoint, TimeToIntercept);

    if (bCanIntercept && TimeToIntercept > 0.0f && TimeToIntercept <= MaxPredictionTime)
    {
        // Shot is blockable
        if (InterceptPositionKey.IsSet())
        {
            Blackboard->SetValueAsVector(InterceptPositionKey.SelectedKeyName, InterceptPoint);
        }

        if (CanBlockKey.IsSet())
        {
            Blackboard->SetValueAsBool(CanBlockKey.SelectedKeyName, true);
        }

        UE_LOG(LogTemp, Display,
            TEXT("[BlockShot] %s can block shot in %.2fs at (%.0f, %.0f, %.0f)"),
            *OwnerPawn->GetName(), TimeToIntercept,
            InterceptPoint.X, InterceptPoint.Y, InterceptPoint.Z);

        return EBTNodeResult::Succeeded;
    }

    // Cannot intercept
    if (CanBlockKey.IsSet())
    {
        Blackboard->SetValueAsBool(CanBlockKey.SelectedKeyName, false);
    }

    UE_LOG(LogTemp, Verbose, TEXT("[BlockShot] Cannot intercept - Quaffle too fast"));
    return EBTNodeResult::Failed;
}

bool UBTTask_BlockShot::CalculateIntercept(
    const FVector& KeeperPos,
    const FVector& QuafflePos,
    const FVector& QuaffleVel,
    FVector& OutInterceptPoint,
    float& OutTimeToIntercept) const
{
    // Same algorithm as BTTask_PredictIntercept

    FVector RelativePos = QuafflePos - KeeperPos;

    // Quadratic coefficients: a*t² + b*t + c = 0
    float VelSquared = QuaffleVel.SizeSquared();
    float SpeedSquared = KeeperMaxSpeed * KeeperMaxSpeed;

    float a = VelSquared - SpeedSquared;
    float b = 2.0f * FVector::DotProduct(RelativePos, QuaffleVel);
    float c = RelativePos.SizeSquared();

    // Special case: Keeper speed equals Quaffle speed
    if (FMath::IsNearlyZero(a))
    {
        if (FMath::IsNearlyZero(b))
        {
            return false;
        }
        OutTimeToIntercept = -c / b;
        if (OutTimeToIntercept > 0.0f)
        {
            OutInterceptPoint = QuafflePos + QuaffleVel * OutTimeToIntercept;
            return true;
        }
        return false;
    }

    // Quadratic formula
    float Discriminant = b * b - 4.0f * a * c;
    if (Discriminant < 0.0f)
    {
        // No real solution - Quaffle is too fast
        return false;
    }

    float SqrtDiscriminant = FMath::Sqrt(Discriminant);
    float t1 = (-b + SqrtDiscriminant) / (2.0f * a);
    float t2 = (-b - SqrtDiscriminant) / (2.0f * a);

    // Choose smallest positive time
    if (t1 > 0.0f && t2 > 0.0f)
    {
        OutTimeToIntercept = FMath::Min(t1, t2);
    }
    else if (t1 > 0.0f)
    {
        OutTimeToIntercept = t1;
    }
    else if (t2 > 0.0f)
    {
        OutTimeToIntercept = t2;
    }
    else
    {
        return false;
    }

    OutInterceptPoint = QuafflePos + QuaffleVel * OutTimeToIntercept;
    return true;
}

bool UBTTask_BlockShot::IsHeadingTowardGoal(
    const FVector& QuafflePos,
    const FVector& QuaffleVel,
    const FVector& GoalCenter) const
{
    if (QuaffleVel.IsNearlyZero())
    {
        return false;
    }

    // Direction from Quaffle to goal
    FVector ToGoal = (GoalCenter - QuafflePos).GetSafeNormal();

    // Direction Quaffle is moving
    FVector VelDir = QuaffleVel.GetSafeNormal();

    // Dot product > 0 means heading toward goal
    float Dot = FVector::DotProduct(VelDir, ToGoal);

    // Require at least 45 degree cone toward goal
    return Dot > 0.5f;
}

FString UBTTask_BlockShot::GetStaticDescription() const
{
    FString Description = TEXT("PathSearch Intercept for Keeper\n");
    Description += FString::Printf(TEXT("Speed=%.0f Block=%.0f\n"), KeeperMaxSpeed, BlockRadius);
    Description += FString::Printf(TEXT("Min Shot Speed: %.0f"), MinShotSpeed);

    return Description;
}

