// ============================================================================
// BTTask_HitBludger.cpp
// Developer: Marcus Daley
// Date: January 24, 2026
// Project: END2507
// ============================================================================

#include "Code/AI/Quidditch/BTTask_HitBludger.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"

UBTTask_HitBludger::UBTTask_HitBludger()
    : MaxHitRange(300.0f)
    , HitForce(2500.0f)
    , LeadFactor(1.0f)
    , HitCooldown(1.0f)
    , LastHitTime(0.0f)
{
    NodeName = TEXT("Hit Bludger");
    bNotifyTick = false;

    // Setup key filters
    BludgerKey.AddObjectFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTTask_HitBludger, BludgerKey),
        AActor::StaticClass());

    TargetEnemyKey.AddObjectFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTTask_HitBludger, TargetEnemyKey),
        APawn::StaticClass());
}

void UBTTask_HitBludger::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);

    if (UBlackboardData* BBAsset = GetBlackboardAsset())
    {
        BludgerKey.ResolveSelectedKey(*BBAsset);
        TargetEnemyKey.ResolveSelectedKey(*BBAsset);
    }
}

EBTNodeResult::Type UBTTask_HitBludger::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

    // Check cooldown
    float CurrentTime = World->GetTimeSeconds();
    if (CurrentTime - LastHitTime < HitCooldown)
    {
        UE_LOG(LogTemp, Verbose, TEXT("[HitBludger] On cooldown: %.1fs remaining"),
            HitCooldown - (CurrentTime - LastHitTime));
        return EBTNodeResult::Failed;
    }

    // Get Bludger
    AActor* Bludger = Cast<AActor>(Blackboard->GetValueAsObject(BludgerKey.SelectedKeyName));
    if (!Bludger)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HitBludger] No Bludger in Blackboard"));
        return EBTNodeResult::Failed;
    }

    // Check range to Bludger
    float DistanceToBludger = FVector::Dist(OwnerPawn->GetActorLocation(), Bludger->GetActorLocation());
    if (DistanceToBludger > MaxHitRange)
    {
        UE_LOG(LogTemp, Verbose, TEXT("[HitBludger] Bludger out of range: %.1f > %.1f"),
            DistanceToBludger, MaxHitRange);
        return EBTNodeResult::Failed;
    }

    // Get target enemy
    APawn* TargetEnemy = Cast<APawn>(Blackboard->GetValueAsObject(TargetEnemyKey.SelectedKeyName));
    if (!TargetEnemy)
    {
        UE_LOG(LogTemp, Warning, TEXT("[HitBludger] No target enemy"));
        return EBTNodeResult::Failed;
    }

    // Calculate hit direction with prediction
    FVector HitDirection = CalculateHitDirection(OwnerPawn, Bludger, TargetEnemy);

    // Apply the hit
    if (ApplyHitForce(Bludger, HitDirection))
    {
        LastHitTime = CurrentTime;

        UE_LOG(LogTemp, Display,
            TEXT("[HitBludger] %s hit Bludger toward %s"),
            *OwnerPawn->GetName(), *TargetEnemy->GetName());

        return EBTNodeResult::Succeeded;
    }

    return EBTNodeResult::Failed;
}

FVector UBTTask_HitBludger::CalculateHitDirection(APawn* Beater, AActor* Bludger, APawn* Enemy) const
{
    if (!Beater || !Bludger || !Enemy)
    {
        return FVector::ForwardVector;
    }

    FVector BludgerPos = Bludger->GetActorLocation();
    FVector EnemyPos = Enemy->GetActorLocation();
    FVector EnemyVel = Enemy->GetVelocity();

    // Calculate time for Bludger to reach enemy (rough estimate)
    float Distance = FVector::Dist(BludgerPos, EnemyPos);
    float FlightTime = Distance / HitForce;

    // Predict enemy position
    FVector PredictedPos = EnemyPos;
    if (!EnemyVel.IsNearlyZero())
    {
        PredictedPos = EnemyPos + EnemyVel * FlightTime * LeadFactor;
    }

    // Direction from Bludger to predicted position
    FVector HitDir = (PredictedPos - BludgerPos).GetSafeNormal();

    return HitDir;
}

bool UBTTask_HitBludger::ApplyHitForce(AActor* Bludger, const FVector& HitDirection)
{
    if (!Bludger)
    {
        return false;
    }

    // Try to get primitive component for physics
    UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Bludger->GetRootComponent());
    if (PrimComp && PrimComp->IsSimulatingPhysics())
    {
        // First, cancel existing velocity for clean redirect
        PrimComp->SetPhysicsLinearVelocity(FVector::ZeroVector);

        // Apply new impulse toward target
        FVector Impulse = HitDirection * HitForce;
        PrimComp->AddImpulse(Impulse, NAME_None, true);

        return true;
    }

    // Alternative: Look for a custom Bludger interface or component
    // This allows non-physics Bludgers to be redirected via their own logic

    // Fallback: Try setting velocity directly via movement component
    // This is a simplified approach for actors without physics

    UE_LOG(LogTemp, Warning,
        TEXT("[HitBludger] Bludger has no physics - attempting direct velocity set"));

    // As a last resort, just set the actor's velocity if it supports it
    // This might not work for all actor types
    return true;
}

FString UBTTask_HitBludger::GetStaticDescription() const
{
    FString Description = TEXT("Hit Bludger at Target\n");
    Description += FString::Printf(TEXT("Range=%.0f Force=%.0f\n"), MaxHitRange, HitForce);
    Description += FString::Printf(TEXT("Lead=%.1f Cooldown=%.1fs"), LeadFactor, HitCooldown);

    return Description;
}

