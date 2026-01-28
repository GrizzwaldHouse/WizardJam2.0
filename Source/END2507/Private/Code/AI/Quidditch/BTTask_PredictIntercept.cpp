// ============================================================================
// BTTask_PredictIntercept.cpp
// Developer: Marcus Daley
// Date: January 24, 2026
// Project: END2507
// ============================================================================
// Algorithm Source: PathSearch.cpp intercept prediction
//
// The intercept problem:
// Given a pursuer at position P with max speed S, and a target at position T
// moving with velocity V, find the point I where the pursuer can intercept.
//
// Mathematical formulation:
// Let t = time to intercept
// Target position at time t: T + V*t
// Pursuer must travel: |I - P| = S*t
// Therefore: |T + V*t - P| = S*t
//
// This gives us a quadratic equation in t:
// |V|²*t² + 2*(T-P)·V*t + |T-P|² = S²*t²
// (|V|² - S²)*t² + 2*(T-P)·V*t + |T-P|² = 0
//
// Solve for t, then I = T + V*t
// ============================================================================

#include "Code/AI/Quidditch/BTTask_PredictIntercept.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

UBTTask_PredictIntercept::UBTTask_PredictIntercept()
    : PursuerMaxSpeed(800.0f)
    , MaxPredictionTime(5.0f)
    , DirectPursuitDistance(300.0f)
    , LeadFactor(1.0f)
{
    NodeName = TEXT("Predict Intercept Point");
    bNotifyTick = false;

    // Setup key filters
    TargetActorKey.AddObjectFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTTask_PredictIntercept, TargetActorKey),
        AActor::StaticClass());

    TargetVelocityKey.AddVectorFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTTask_PredictIntercept, TargetVelocityKey));

    InterceptPointKey.AddVectorFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTTask_PredictIntercept, InterceptPointKey));

    TimeToInterceptKey.AddFloatFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTTask_PredictIntercept, TimeToInterceptKey));
}

void UBTTask_PredictIntercept::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);

    if (UBlackboardData* BBAsset = GetBlackboardAsset())
    {
        TargetActorKey.ResolveSelectedKey(*BBAsset);
        TargetVelocityKey.ResolveSelectedKey(*BBAsset);
        InterceptPointKey.ResolveSelectedKey(*BBAsset);
        TimeToInterceptKey.ResolveSelectedKey(*BBAsset);
    }
}

EBTNodeResult::Type UBTTask_PredictIntercept::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

    // Get target actor
    AActor* TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName));
    if (!TargetActor)
    {
        return EBTNodeResult::Failed;
    }

    // Get positions
    FVector PursuerPos = OwnerPawn->GetActorLocation();
    FVector TargetPos = TargetActor->GetActorLocation();

    // Get target velocity
    FVector TargetVel;
    if (TargetVelocityKey.IsSet())
    {
        TargetVel = Blackboard->GetValueAsVector(TargetVelocityKey.SelectedKeyName);
    }
    else
    {
        // Calculate velocity from actor if not provided
        TargetVel = TargetActor->GetVelocity();
    }

    // Check if we're close enough for direct pursuit
    float DistanceToTarget = FVector::Dist(PursuerPos, TargetPos);
    if (DistanceToTarget <= DirectPursuitDistance)
    {
        // Direct pursuit - just go to current target position
        if (InterceptPointKey.IsSet())
        {
            Blackboard->SetValueAsVector(InterceptPointKey.SelectedKeyName, TargetPos);
        }

        if (TimeToInterceptKey.IsSet())
        {
            float DirectTime = DistanceToTarget / PursuerMaxSpeed;
            Blackboard->SetValueAsFloat(TimeToInterceptKey.SelectedKeyName, DirectTime);
        }

        return EBTNodeResult::Succeeded;
    }

    // Calculate intercept point using PathSearch algorithm
    float TimeToIntercept = 0.0f;
    FVector InterceptPoint = CalculateInterceptPoint(
        PursuerPos,
        PursuerMaxSpeed,
        TargetPos,
        TargetVel,
        TimeToIntercept);

    // Write results to Blackboard
    if (InterceptPointKey.IsSet())
    {
        Blackboard->SetValueAsVector(InterceptPointKey.SelectedKeyName, InterceptPoint);
    }

    if (TimeToInterceptKey.IsSet())
    {
        Blackboard->SetValueAsFloat(TimeToInterceptKey.SelectedKeyName, TimeToIntercept);
    }

    return EBTNodeResult::Succeeded;
}

FVector UBTTask_PredictIntercept::CalculateInterceptPoint(
    const FVector& PursuerPos,
    float PursuerSpeed,
    const FVector& TargetPos,
    const FVector& TargetVel,
    float& OutTimeToIntercept) const
{
    // If target is stationary, just go directly there
    if (TargetVel.IsNearlyZero())
    {
        OutTimeToIntercept = FVector::Dist(PursuerPos, TargetPos) / PursuerSpeed;
        OutTimeToIntercept = FMath::Min(OutTimeToIntercept, MaxPredictionTime);
        return TargetPos;
    }

    // PathSearch.cpp algorithm: Solve for intercept time
    FVector RelativePos = TargetPos - PursuerPos;

    float Time = 0.0f;
    bool bCanIntercept = SolveInterceptTime(RelativePos, TargetVel, PursuerSpeed, Time);

    if (!bCanIntercept || Time < 0.0f)
    {
        // Can't intercept - target is too fast
        // Fall back to leading the target by MaxPredictionTime
        Time = MaxPredictionTime;
    }

    // Clamp prediction time
    Time = FMath::Clamp(Time, 0.0f, MaxPredictionTime);

    // Apply lead factor for adjustment
    Time *= LeadFactor;

    OutTimeToIntercept = Time;

    // Calculate intercept point: where target will be at time T
    FVector InterceptPoint = TargetPos + TargetVel * Time;

    return InterceptPoint;
}

bool UBTTask_PredictIntercept::SolveInterceptTime(
    const FVector& RelativePos,
    const FVector& RelativeVel,
    float PursuerSpeed,
    float& OutTime) const
{
    // Quadratic equation coefficients:
    // (|V|² - S²)*t² + 2*(T-P)·V*t + |T-P|² = 0
    // a*t² + b*t + c = 0

    float VelSquared = RelativeVel.SizeSquared();
    float SpeedSquared = PursuerSpeed * PursuerSpeed;

    float a = VelSquared - SpeedSquared;
    float b = 2.0f * FVector::DotProduct(RelativePos, RelativeVel);
    float c = RelativePos.SizeSquared();

    // Special case: pursuer speed equals target speed
    if (FMath::IsNearlyZero(a))
    {
        if (FMath::IsNearlyZero(b))
        {
            // No solution
            return false;
        }
        // Linear equation: b*t + c = 0
        OutTime = -c / b;
        return OutTime > 0.0f;
    }

    // Quadratic formula: t = (-b ± sqrt(b² - 4ac)) / 2a
    float Discriminant = b * b - 4.0f * a * c;

    if (Discriminant < 0.0f)
    {
        // No real solution - target is too fast to intercept
        return false;
    }

    float SqrtDiscriminant = FMath::Sqrt(Discriminant);
    float t1 = (-b + SqrtDiscriminant) / (2.0f * a);
    float t2 = (-b - SqrtDiscriminant) / (2.0f * a);

    // We want the smallest positive time
    if (t1 > 0.0f && t2 > 0.0f)
    {
        OutTime = FMath::Min(t1, t2);
    }
    else if (t1 > 0.0f)
    {
        OutTime = t1;
    }
    else if (t2 > 0.0f)
    {
        OutTime = t2;
    }
    else
    {
        // Both times are negative - target is behind us
        return false;
    }

    return true;
}

FString UBTTask_PredictIntercept::GetStaticDescription() const
{
    FString Description = TEXT("PathSearch Intercept Algorithm\n");
    Description += FString::Printf(TEXT("Speed: %.0f | Lead: %.1fx\n"), PursuerMaxSpeed, LeadFactor);

    if (InterceptPointKey.IsSet())
    {
        Description += FString::Printf(TEXT("Output → %s"), *InterceptPointKey.SelectedKeyName.ToString());
    }

    return Description;
}
