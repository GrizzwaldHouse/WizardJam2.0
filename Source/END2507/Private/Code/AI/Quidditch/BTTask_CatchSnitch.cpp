// ============================================================================
// BTTask_CatchSnitch.cpp
// Developer: Marcus Daley
// Date: January 24, 2026
// Project: END2507
// ============================================================================

#include "Code/AI/Quidditch/BTTask_CatchSnitch.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Code/GameModes/QuidditchGameMode.h"

UBTTask_CatchSnitch::UBTTask_CatchSnitch()
    : CatchRadius(150.0f)
    , SnitchPointValue(150)
    , bDestroySnitchOnCatch(true)
{
    NodeName = TEXT("Catch Snitch");
    bNotifyTick = false;

    // Setup key filter for Object (Snitch actor)
    SnitchActorKey.AddObjectFilter(this,
        GET_MEMBER_NAME_CHECKED(UBTTask_CatchSnitch, SnitchActorKey),
        AActor::StaticClass());
}

void UBTTask_CatchSnitch::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);

    if (UBlackboardData* BBAsset = GetBlackboardAsset())
    {
        SnitchActorKey.ResolveSelectedKey(*BBAsset);
    }
}

EBTNodeResult::Type UBTTask_CatchSnitch::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

    // Get Snitch actor from Blackboard
    AActor* Snitch = Cast<AActor>(Blackboard->GetValueAsObject(SnitchActorKey.SelectedKeyName));
    if (!Snitch)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CatchSnitch] No Snitch actor in Blackboard"));
        return EBTNodeResult::Failed;
    }

    // Check distance to Snitch
    float DistanceToSnitch = FVector::Dist(OwnerPawn->GetActorLocation(), Snitch->GetActorLocation());
    if (DistanceToSnitch > CatchRadius)
    {
        UE_LOG(LogTemp, Verbose,
            TEXT("[CatchSnitch] Snitch out of range: %.1f > %.1f"),
            DistanceToSnitch, CatchRadius);
        return EBTNodeResult::Failed;
    }

    // Attempt catch
    if (TryCatchSnitch(Snitch, OwnerPawn))
    {
        UE_LOG(LogTemp, Display,
            TEXT("[CatchSnitch] %s caught the Golden Snitch! +%d points"),
            *OwnerPawn->GetName(), SnitchPointValue);

        // Clear Snitch from Blackboard
        if (SnitchActorKey.IsSet())
        {
            Blackboard->ClearValue(SnitchActorKey.SelectedKeyName);
        }

        return EBTNodeResult::Succeeded;
    }

    return EBTNodeResult::Failed;
}

bool UBTTask_CatchSnitch::TryCatchSnitch(AActor* Snitch, APawn* Seeker)
{
    if (!Snitch || !Seeker)
    {
        return false;
    }

    UWorld* World = Seeker->GetWorld();
    if (!World)
    {
        return false;
    }

    // Get GameMode to notify of catch
    AQuidditchGameMode* GameMode = Cast<AQuidditchGameMode>(World->GetAuthGameMode());
    if (!GameMode)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CatchSnitch] No QuidditchGameMode found"));
        return false;
    }

    // Get Seeker's team
    EQuidditchTeam SeekerTeam = GameMode->GetAgentTeam(Seeker);
    if (SeekerTeam == EQuidditchTeam::None)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CatchSnitch] Seeker has no team assigned"));
        return false;
    }

    // Notify GameMode - triggers OnSnitchCaught broadcast and match end
    // Point value is configured in GameMode::SnitchCatchPoints (EditDefaultsOnly)
    GameMode->NotifySnitchCaught(Seeker, SeekerTeam);

    // Optionally destroy Snitch
    if (bDestroySnitchOnCatch)
    {
        Snitch->Destroy();
    }

    return true;
}

FString UBTTask_CatchSnitch::GetStaticDescription() const
{
    FString Description = FString::Printf(TEXT("Catch within %.0f units\n"), CatchRadius);
    Description += FString::Printf(TEXT("+%d points on catch"), SnitchPointValue);

    if (SnitchActorKey.IsSet())
    {
        Description += FString::Printf(TEXT("\nTarget: %s"), *SnitchActorKey.SelectedKeyName.ToString());
    }

    return Description;
}

