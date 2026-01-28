// ============================================================================
// BTTask_ThrowQuaffle.cpp
// Developer: Marcus Daley
// Date: January 24, 2026
// Project: END2507
// ============================================================================

#include "Code/AI/Quidditch/BTTask_ThrowQuaffle.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/PrimitiveComponent.h"

UBTTask_ThrowQuaffle::UBTTask_ThrowQuaffle()
    : ThrowType(EQuaffleThrowType::ToGoal)
    , ThrowSpeed(2000.0f)
    , LeadFactor(1.0f)
    , ArcHeight(0.0f)
    , MinThrowRange(200.0f)
    , MaxThrowRange(3000.0f)
{
    NodeName = TEXT("Throw Quaffle");
    bNotifyTick = false;

    // Setup key filters
    HeldQuaffleKey.AddObjectFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTTask_ThrowQuaffle, HeldQuaffleKey),
        AActor::StaticClass());

    ThrowTargetKey.AddObjectFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTTask_ThrowQuaffle, ThrowTargetKey),
        AActor::StaticClass());

    ThrowLocationKey.AddVectorFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTTask_ThrowQuaffle, ThrowLocationKey));
}

void UBTTask_ThrowQuaffle::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);

    if (UBlackboardData* BBAsset = GetBlackboardAsset())
    {
        HeldQuaffleKey.ResolveSelectedKey(*BBAsset);
        ThrowTargetKey.ResolveSelectedKey(*BBAsset);
        ThrowLocationKey.ResolveSelectedKey(*BBAsset);
    }
}

EBTNodeResult::Type UBTTask_ThrowQuaffle::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

    // Get held Quaffle
    AActor* Quaffle = Cast<AActor>(Blackboard->GetValueAsObject(HeldQuaffleKey.SelectedKeyName));
    if (!Quaffle)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ThrowQuaffle] No Quaffle in hand"));
        return EBTNodeResult::Failed;
    }

    FVector ThrowerPos = OwnerPawn->GetActorLocation();
    FVector TargetPos = FVector::ZeroVector;
    FVector TargetVelocity = FVector::ZeroVector;

    // Determine target based on throw type
    switch (ThrowType)
    {
    case EQuaffleThrowType::ToGoal:
    case EQuaffleThrowType::ToTeammate:
        {
            AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject(ThrowTargetKey.SelectedKeyName));
            if (!Target)
            {
                UE_LOG(LogTemp, Warning, TEXT("[ThrowQuaffle] No throw target"));
                return EBTNodeResult::Failed;
            }
            TargetPos = Target->GetActorLocation();
            TargetVelocity = Target->GetVelocity();
        }
        break;

    case EQuaffleThrowType::ToLocation:
        {
            if (!ThrowLocationKey.IsSet())
            {
                UE_LOG(LogTemp, Warning, TEXT("[ThrowQuaffle] No throw location key set"));
                return EBTNodeResult::Failed;
            }
            TargetPos = Blackboard->GetValueAsVector(ThrowLocationKey.SelectedKeyName);
        }
        break;
    }

    // Check range
    float Distance = FVector::Dist(ThrowerPos, TargetPos);
    if (Distance < MinThrowRange)
    {
        UE_LOG(LogTemp, Verbose, TEXT("[ThrowQuaffle] Target too close: %.1f < %.1f"),
            Distance, MinThrowRange);
        return EBTNodeResult::Failed;
    }

    if (Distance > MaxThrowRange)
    {
        UE_LOG(LogTemp, Verbose, TEXT("[ThrowQuaffle] Target too far: %.1f > %.1f"),
            Distance, MaxThrowRange);
        return EBTNodeResult::Failed;
    }

    // Calculate throw direction with prediction
    FVector ThrowDir = CalculateThrowDirection(ThrowerPos, TargetPos, TargetVelocity);
    FVector ThrowVelocity = ThrowDir * ThrowSpeed;

    // Execute the throw
    if (ExecuteThrow(Quaffle, OwnerPawn, ThrowVelocity))
    {
        UE_LOG(LogTemp, Display,
            TEXT("[ThrowQuaffle] %s threw Quaffle toward target at distance %.1f"),
            *OwnerPawn->GetName(), Distance);

        // Clear held Quaffle from Blackboard
        if (HeldQuaffleKey.IsSet())
        {
            Blackboard->ClearValue(HeldQuaffleKey.SelectedKeyName);
        }

        return EBTNodeResult::Succeeded;
    }

    return EBTNodeResult::Failed;
}

FVector UBTTask_ThrowQuaffle::CalculateThrowDirection(
    const FVector& FromPos,
    const FVector& TargetPos,
    const FVector& TargetVelocity) const
{
    FVector ToTarget = TargetPos - FromPos;
    float Distance = ToTarget.Size();

    // Estimate time to reach target
    float FlightTime = Distance / ThrowSpeed;

    // Lead target if moving
    FVector PredictedPos = TargetPos;
    if (!TargetVelocity.IsNearlyZero())
    {
        PredictedPos = TargetPos + TargetVelocity * FlightTime * LeadFactor;
    }

    // Calculate base direction
    FVector ThrowDir = (PredictedPos - FromPos).GetSafeNormal();

    // Add arc if configured
    if (ArcHeight > KINDA_SMALL_NUMBER)
    {
        // Add upward component based on arc height
        // This is a simplified arc - full projectile motion would need parabolic calculation
        float ArcFactor = ArcHeight / Distance;
        ThrowDir.Z += ArcFactor;
        ThrowDir.Normalize();
    }

    return ThrowDir;
}

bool UBTTask_ThrowQuaffle::ExecuteThrow(AActor* Quaffle, APawn* Thrower, const FVector& ThrowVelocity)
{
    if (!Quaffle || !Thrower)
    {
        return false;
    }

    // Detach Quaffle from thrower
    Quaffle->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

    // Remove "Held" tag if present
    Quaffle->Tags.Remove(TEXT("Held"));

    // Clear owner
    Quaffle->SetOwner(nullptr);

    // Position slightly in front of thrower
    FVector SpawnOffset = Thrower->GetActorForwardVector() * 100.0f;
    Quaffle->SetActorLocation(Thrower->GetActorLocation() + SpawnOffset);

    // Try to use ProjectileMovementComponent if available
    UProjectileMovementComponent* ProjectileComp = Quaffle->FindComponentByClass<UProjectileMovementComponent>();
    if (ProjectileComp)
    {
        ProjectileComp->Velocity = ThrowVelocity;
        ProjectileComp->Activate();
        return true;
    }

    // Fallback: Apply physics impulse
    UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Quaffle->GetRootComponent());
    if (PrimComp && PrimComp->IsSimulatingPhysics())
    {
        PrimComp->AddImpulse(ThrowVelocity, NAME_None, true);
        return true;
    }

    // Last resort: Just set velocity if the actor supports it
    // This may not work for all actor types
    UE_LOG(LogTemp, Warning,
        TEXT("[ThrowQuaffle] Quaffle has no ProjectileMovement or physics - throw may not work"));

    return true;
}

FString UBTTask_ThrowQuaffle::GetStaticDescription() const
{
    FString TypeStr;
    switch (ThrowType)
    {
    case EQuaffleThrowType::ToGoal: TypeStr = TEXT("To Goal"); break;
    case EQuaffleThrowType::ToTeammate: TypeStr = TEXT("To Teammate"); break;
    case EQuaffleThrowType::ToLocation: TypeStr = TEXT("To Location"); break;
    }

    FString Description = FString::Printf(TEXT("Throw %s\n"), *TypeStr);
    Description += FString::Printf(TEXT("Speed=%.0f Lead=%.1f\n"), ThrowSpeed, LeadFactor);
    Description += FString::Printf(TEXT("Range: %.0f - %.0f"), MinThrowRange, MaxThrowRange);

    return Description;
}

