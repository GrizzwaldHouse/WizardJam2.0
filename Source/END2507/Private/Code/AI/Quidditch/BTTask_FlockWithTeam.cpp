// ============================================================================
// BTTask_FlockWithTeam.cpp
// Developer: Marcus Daley
// Date: January 24, 2026
// Project: END2507
// ============================================================================
// Algorithm Source: Flock.cs
//
// Boids flocking implementation:
// Separation = Sum of (normalized direction away from each neighbor / distance)
// Alignment = Average velocity of all neighbors
// Cohesion = Direction toward center of mass of neighbors
//
// Final = Normalize(w1*Separation + w2*Alignment + w3*Cohesion) * MaxSpeed
// ============================================================================

#include "Code/AI/Quidditch/BTTask_FlockWithTeam.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "GameFramework/Pawn.h"
#include "EngineUtils.h"
#include "Code/GameModes/QuidditchGameMode.h"

UBTTask_FlockWithTeam::UBTTask_FlockWithTeam()
    : SeparationWeight(1.5f)
    , AlignmentWeight(1.0f)
    , CohesionWeight(1.0f)
    , NeighborRadius(1500.0f)
    , SeparationRadius(400.0f)
    , MaxFlockSpeed(600.0f)
    , FlockTargetDistance(500.0f)
{
    NodeName = TEXT("Flock With Team");
    bNotifyTick = false;

    // Setup key filters
    FlockDirectionKey.AddVectorFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTTask_FlockWithTeam, FlockDirectionKey));

    FlockTargetKey.AddVectorFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTTask_FlockWithTeam, FlockTargetKey));
}

void UBTTask_FlockWithTeam::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);

    if (UBlackboardData* BBAsset = GetBlackboardAsset())
    {
        FlockDirectionKey.ResolveSelectedKey(*BBAsset);
        FlockTargetKey.ResolveSelectedKey(*BBAsset);
    }
}

EBTNodeResult::Type UBTTask_FlockWithTeam::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

    UWorld* World = OwnerPawn->GetWorld();
    if (!World)
    {
        return EBTNodeResult::Failed;
    }

    // Get all teammates within range
    TArray<APawn*> Neighbors = GetNeighbors(OwnerPawn, World);

    FVector FlockDirection = FVector::ZeroVector;

    if (Neighbors.Num() > 0)
    {
        // Calculate three flocking components
        FVector Separation = CalculateSeparation(OwnerPawn, Neighbors);
        FVector Alignment = CalculateAlignment(OwnerPawn, Neighbors);
        FVector Cohesion = CalculateCohesion(OwnerPawn, Neighbors);

        // Combine with weights (from Flock.cs)
        FlockDirection = Separation * SeparationWeight
                       + Alignment * AlignmentWeight
                       + Cohesion * CohesionWeight;

        // Normalize and scale
        if (!FlockDirection.IsNearlyZero())
        {
            FlockDirection = FlockDirection.GetSafeNormal() * MaxFlockSpeed;
        }

        UE_LOG(LogTemp, Verbose,
            TEXT("[Flock] %s | Neighbors=%d | Sep=%.1f Align=%.1f Coh=%.1f"),
            *OwnerPawn->GetName(), Neighbors.Num(),
            Separation.Size(), Alignment.Size(), Cohesion.Size());
    }
    else
    {
        // No neighbors - maintain current heading
        FlockDirection = OwnerPawn->GetVelocity();
        if (FlockDirection.IsNearlyZero())
        {
            FlockDirection = OwnerPawn->GetActorForwardVector() * MaxFlockSpeed;
        }
    }

    // Write flock direction to Blackboard
    if (FlockDirectionKey.IsSet())
    {
        Blackboard->SetValueAsVector(FlockDirectionKey.SelectedKeyName, FlockDirection);
    }

    // Calculate target point ahead in flock direction
    if (FlockTargetKey.IsSet())
    {
        FVector OwnerLocation = OwnerPawn->GetActorLocation();
        FVector TargetPoint = OwnerLocation + FlockDirection.GetSafeNormal() * FlockTargetDistance;
        Blackboard->SetValueAsVector(FlockTargetKey.SelectedKeyName, TargetPoint);
    }

    return EBTNodeResult::Succeeded;
}

TArray<APawn*> UBTTask_FlockWithTeam::GetNeighbors(APawn* OwnerPawn, UWorld* World) const
{
    TArray<APawn*> Neighbors;

    if (!OwnerPawn || !World)
    {
        return Neighbors;
    }

    // Get owner's team
    AQuidditchGameMode* GameMode = Cast<AQuidditchGameMode>(World->GetAuthGameMode());
    if (!GameMode)
    {
        return Neighbors;
    }

    EQuidditchTeam OwnerTeam = GameMode->GetAgentTeam(OwnerPawn);
    if (OwnerTeam == EQuidditchTeam::None)
    {
        return Neighbors;
    }

    FVector OwnerLocation = OwnerPawn->GetActorLocation();
    float NeighborRadiusSq = NeighborRadius * NeighborRadius;

    // Iterate all pawns in world
    for (TActorIterator<APawn> It(World); It; ++It)
    {
        APawn* OtherPawn = *It;

        // Skip self
        if (OtherPawn == OwnerPawn)
        {
            continue;
        }

        // Check team
        EQuidditchTeam OtherTeam = GameMode->GetAgentTeam(OtherPawn);
        if (OtherTeam != OwnerTeam)
        {
            continue;
        }

        // Check distance
        float DistSq = FVector::DistSquared(OwnerLocation, OtherPawn->GetActorLocation());
        if (DistSq <= NeighborRadiusSq)
        {
            Neighbors.Add(OtherPawn);
        }
    }

    return Neighbors;
}

FVector UBTTask_FlockWithTeam::CalculateSeparation(APawn* OwnerPawn, const TArray<APawn*>& Neighbors) const
{
    // Separation: Steer away from neighbors that are too close
    // Sum of (direction away / distance) for each neighbor within SeparationRadius

    FVector SeparationForce = FVector::ZeroVector;
    FVector OwnerLocation = OwnerPawn->GetActorLocation();
    int32 CloseCount = 0;

    for (APawn* Neighbor : Neighbors)
    {
        if (!Neighbor)
        {
            continue;
        }

        FVector NeighborLocation = Neighbor->GetActorLocation();
        float Distance = FVector::Dist(OwnerLocation, NeighborLocation);

        // Only apply separation within SeparationRadius
        if (Distance < SeparationRadius && Distance > KINDA_SMALL_NUMBER)
        {
            // Direction away from neighbor
            FVector AwayDir = (OwnerLocation - NeighborLocation).GetSafeNormal();

            // Inverse distance weighting - closer = stronger push
            SeparationForce += AwayDir / Distance;
            CloseCount++;
        }
    }

    // Average if multiple close neighbors
    if (CloseCount > 0)
    {
        SeparationForce /= CloseCount;
    }

    return SeparationForce;
}

FVector UBTTask_FlockWithTeam::CalculateAlignment(APawn* OwnerPawn, const TArray<APawn*>& Neighbors) const
{
    // Alignment: Steer toward average heading of neighbors
    // Average velocity of all neighbors

    FVector AverageVelocity = FVector::ZeroVector;

    for (APawn* Neighbor : Neighbors)
    {
        if (!Neighbor)
        {
            continue;
        }

        AverageVelocity += Neighbor->GetVelocity();
    }

    if (Neighbors.Num() > 0)
    {
        AverageVelocity /= Neighbors.Num();
    }

    return AverageVelocity.GetSafeNormal();
}

FVector UBTTask_FlockWithTeam::CalculateCohesion(APawn* OwnerPawn, const TArray<APawn*>& Neighbors) const
{
    // Cohesion: Steer toward center of mass of neighbors
    // Direction from owner to average position of neighbors

    FVector CenterOfMass = FVector::ZeroVector;

    for (APawn* Neighbor : Neighbors)
    {
        if (!Neighbor)
        {
            continue;
        }

        CenterOfMass += Neighbor->GetActorLocation();
    }

    if (Neighbors.Num() > 0)
    {
        CenterOfMass /= Neighbors.Num();

        // Direction toward center of mass
        FVector OwnerLocation = OwnerPawn->GetActorLocation();
        return (CenterOfMass - OwnerLocation).GetSafeNormal();
    }

    return FVector::ZeroVector;
}

FString UBTTask_FlockWithTeam::GetStaticDescription() const
{
    FString Description = TEXT("Boids Flocking Algorithm\n");
    Description += FString::Printf(TEXT("Sep=%.1f Align=%.1f Coh=%.1f\n"),
        SeparationWeight, AlignmentWeight, CohesionWeight);
    Description += FString::Printf(TEXT("Range=%.0f Sep=%.0f"),
        NeighborRadius, SeparationRadius);

    return Description;
}

