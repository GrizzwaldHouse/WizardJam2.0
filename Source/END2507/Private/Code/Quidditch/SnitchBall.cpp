// ============================================================================
// SnitchBall.cpp
// Developer: Marcus Daley
// Date: January 24, 2026
// Project: END2507
// ============================================================================
// Movement Algorithm:
// 1. Wander: Random direction with periodic changes
// 2. Evade: Flee from nearby Seekers using inverse distance weighting
// 3. Boundary: Soft push toward center when approaching edges
// 4. Blend: Combine all forces with configurable weights
// ============================================================================

#include "Code/Quidditch/SnitchBall.h"
#include "Code/AI/AIC_SnitchController.h"
#include "Code/GameModes/QuidditchGameMode.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "GenericTeamAgentInterface.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "StructuredLoggingMacros.h"

DEFINE_LOG_CATEGORY(LogSnitchBall);

ASnitchBall::ASnitchBall()
    : BaseSpeed(600.0f)
    , MaxEvadeSpeed(1200.0f)
    , TurnRate(180.0f)
    , DirectionChangeInterval(2.0f)
    , DirectionChangeVariance(1.0f)
    , DetectionRadius(2000.0f)
    , EvadeRadius(800.0f)
    , EvadeStrength(1.5f)
    , PlayAreaVolumeRef(nullptr)
    , PlayAreaCenter(FVector::ZeroVector)
    , PlayAreaExtent(FVector(5000.0f, 5000.0f, 2000.0f))
    , BoundaryForce(2.0f)
    , ObstacleCheckDistance(300.0f)
    , ObstacleAvoidanceStrength(2.0f)
    , ObstacleChannel(ECC_WorldStatic)
    , MinHeightAboveGround(100.0f)
    , MaxHeightAboveGround(2000.0f)
    , GroundTraceDistance(5000.0f)
    , bShowDebug(false)
    , CurrentDirection(FVector::ForwardVector)
    , CurrentSpeed(600.0f)
    , DirectionChangeTimer(0.0f)
    , NextDirectionChangeTime(2.0f)
    , SnitchController(nullptr)
    , CurrentGroundHeight(0.0f)
    , bWasEvadingLastFrame(false)
{
    PrimaryActorTick.bCanEverTick = true;

    // Create collision sphere as root - blocks world geometry for floor/wall collision
    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    SetRootComponent(CollisionSphere);
    CollisionSphere->SetSphereRadius(30.0f);

    // Block WorldStatic (floors, walls) while allowing overlap for catch detection
    CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    CollisionSphere->SetCollisionObjectType(ECC_Pawn);
    CollisionSphere->SetCollisionResponseToAllChannels(ECR_Overlap);
    CollisionSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    CollisionSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
    CollisionSphere->SetGenerateOverlapEvents(true);

    // Bind overlap callback for catch detection
    CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ASnitchBall::OnSnitchOverlap);

    // Create visual mesh
    SnitchMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SnitchMesh"));
    SnitchMesh->SetupAttachment(CollisionSphere);
    SnitchMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Create AI Perception source so Seekers can detect us
    PerceptionSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("PerceptionSource"));
    PerceptionSource->bAutoRegister = true;
    PerceptionSource->RegisterForSense(UAISense_Sight::StaticClass());

    // Set our custom AI Controller class for perception-based pursuer detection
    AIControllerClass = AAIC_SnitchController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    // Add Snitch tags for BTService_FindSnitch detection
    Tags.Add(TEXT("Snitch"));
    Tags.Add(TEXT("GoldenSnitch"));
}

void ASnitchBall::BeginPlay()
{
    Super::BeginPlay();

    // If designer assigned a volume actor, read its bounds
    if (PlayAreaVolumeRef)
    {
        FVector Origin;
        FVector BoxExtent;
        PlayAreaVolumeRef->GetActorBounds(false, Origin, BoxExtent);

        PlayAreaCenter = Origin;
        PlayAreaExtent = BoxExtent;

        UE_LOG(LogSnitchBall, Log,
            TEXT("[Snitch] Using volume bounds from '%s': Center=%s Extent=%s"),
            *PlayAreaVolumeRef->GetName(),
            *PlayAreaCenter.ToString(),
            *PlayAreaExtent.ToString());
    }
    else if (PlayAreaCenter.IsNearlyZero())
    {
        // No volume and no manual center - use spawn location as center
        PlayAreaCenter = GetActorLocation();
    }

    // Initialize with random direction
    CurrentDirection = FMath::VRand();
    CurrentDirection.Z = FMath::FRandRange(-0.3f, 0.3f);
    CurrentDirection.Normalize();

    // Set first direction change time
    NextDirectionChangeTime = DirectionChangeInterval +
        FMath::FRandRange(-DirectionChangeVariance, DirectionChangeVariance);

    // Structured logging - Snitch spawned
    SLOG_EVENT(this, "Snitch.Lifecycle", "SnitchSpawned",
        Metadata.Add(TEXT("location"), GetActorLocation().ToString());
        Metadata.Add(TEXT("play_area_center"), PlayAreaCenter.ToString());
        Metadata.Add(TEXT("play_area_extent"), PlayAreaExtent.ToString());
    );

    UE_LOG(LogSnitchBall, Log,
        TEXT("[Snitch] '%s' spawned | Location=%s | PlayArea=%s Extent=%s"),
        *GetName(),
        *GetActorLocation().ToString(),
        *PlayAreaCenter.ToString(),
        *PlayAreaExtent.ToString());
}

void ASnitchBall::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    // Cache controller and bind to perception delegates - Observer Pattern
    SnitchController = Cast<AAIC_SnitchController>(NewController);
    if (SnitchController)
    {
        SnitchController->OnPursuerDetected.AddDynamic(this, &ASnitchBall::HandlePursuerDetected);
        SnitchController->OnPursuerLost.AddDynamic(this, &ASnitchBall::HandlePursuerLost);

        // Structured logging - controller possessed successfully
        SLOG_EVENT(this, "Snitch.Perception", "ControllerPossessed",
            Metadata.Add(TEXT("controller_class"), NewController->GetClass()->GetName());
        );

        UE_LOG(LogSnitchBall, Display,
            TEXT("[Snitch] Bound to controller perception delegates"));
    }
    else
    {
        // Structured logging - invalid controller type
        SLOG_WARNING(this, "Snitch.Perception", "ControllerInvalid",
            Metadata.Add(TEXT("controller_class"), NewController ? NewController->GetClass()->GetName() : TEXT("null"));
        );

        UE_LOG(LogSnitchBall, Warning,
            TEXT("[Snitch] Controller is not AIC_SnitchController - perception disabled!"));
    }
}

void ASnitchBall::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Unbind from controller perception delegates
    if (SnitchController)
    {
        SnitchController->OnPursuerDetected.RemoveDynamic(this, &ASnitchBall::HandlePursuerDetected);
        SnitchController->OnPursuerLost.RemoveDynamic(this, &ASnitchBall::HandlePursuerLost);
    }

    Super::EndPlay(EndPlayReason);
}

void ASnitchBall::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    UpdateMovement(DeltaTime);

    if (bShowDebug)
    {
        DrawDebugInfo();
    }
}

void ASnitchBall::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    // Snitch is not player-controlled
}

void ASnitchBall::UpdateMovement(float DeltaTime)
{
    // Structured logging - scope timer for performance profiling
    SLOG_SCOPE_TIMER(this, "Snitch.Performance", "UpdateMovementFrame");

    // Update direction change timer
    DirectionChangeTimer += DeltaTime;
    if (DirectionChangeTimer >= NextDirectionChangeTime)
    {
        CurrentDirection = CalculateWanderDirection();
        DirectionChangeTimer = 0.0f;
        NextDirectionChangeTime = DirectionChangeInterval +
            FMath::FRandRange(-DirectionChangeVariance, DirectionChangeVariance);
    }

    // Calculate all movement influences
    FVector WanderDir = CurrentDirection;
    FVector EvadeDir = CalculateEvadeVector();
    FVector BoundaryDir = CalculateBoundaryForce();
    FVector ObstacleDir = CalculateObstacleAvoidance();

    // Blend directions - obstacle avoidance takes highest priority
    FVector FinalDirection = WanderDir;
    bool bIsEvading = false;

    // Apply obstacle avoidance (highest priority - floors, walls)
    if (!ObstacleDir.IsNearlyZero())
    {
        FinalDirection = (FinalDirection + ObstacleDir).GetSafeNormal();
    }

    // Apply pursuer evasion
    if (!EvadeDir.IsNearlyZero())
    {
        FinalDirection = (FinalDirection + EvadeDir * EvadeStrength).GetSafeNormal();
        CurrentSpeed = FMath::FInterpTo(CurrentSpeed, MaxEvadeSpeed, DeltaTime, 5.0f);
        bIsEvading = true;

        // Structured logging - evasion started (state change only)
        if (!bWasEvadingLastFrame)
        {
            SLOG_EVENT(this, "Snitch.Evasion", "EvadingStarted",
                Metadata.Add(TEXT("current_speed"), FString::SanitizeFloat(CurrentSpeed));
            );
            bWasEvadingLastFrame = true;
        }
    }
    else
    {
        CurrentSpeed = FMath::FInterpTo(CurrentSpeed, BaseSpeed, DeltaTime, 2.0f);

        // Structured logging - evasion stopped (state change only)
        if (bWasEvadingLastFrame)
        {
            SLOG_EVENT(this, "Snitch.Evasion", "EvadingStopped");
            bWasEvadingLastFrame = false;
        }
    }

    // Apply boundary avoidance
    if (!BoundaryDir.IsNearlyZero())
    {
        FinalDirection = (FinalDirection + BoundaryDir * BoundaryForce).GetSafeNormal();
    }

    // Smoothly rotate toward new direction
    CurrentDirection = FMath::VInterpNormalRotationTo(
        CurrentDirection, FinalDirection, DeltaTime, TurnRate);

    // Calculate new location
    FVector NewLocation = GetActorLocation() + CurrentDirection * CurrentSpeed * DeltaTime;

    // Enforce height constraints (min/max height above ground)
    NewLocation = EnforceHeightConstraints(NewLocation);

    // Apply movement WITH SWEEP (respects collision with floors/walls)
    FHitResult HitResult;
    SetActorLocation(NewLocation, true, &HitResult, ETeleportType::None);

    // If we hit something, adjust direction to slide along surface
    if (HitResult.bBlockingHit)
    {
        CurrentDirection = FVector::VectorPlaneProject(CurrentDirection, HitResult.Normal).GetSafeNormal();

        UE_LOG(LogSnitchBall, Verbose, TEXT("[Snitch] Collision with %s - sliding along surface"),
            HitResult.GetActor() ? *HitResult.GetActor()->GetName() : TEXT("Unknown"));
    }

    // Update ground height for debug display
    CurrentGroundHeight = GetGroundHeight();

    // Rotate mesh to face movement direction
    if (!CurrentDirection.IsNearlyZero())
    {
        FRotator NewRotation = CurrentDirection.Rotation();
        SetActorRotation(NewRotation);
    }
}

FVector ASnitchBall::CalculateWanderDirection() const
{
    // Random direction with vertical constraint
    FVector RandomDir = FMath::VRand();
    RandomDir.Z = FMath::FRandRange(-0.5f, 0.5f);
    RandomDir.Normalize();

    // Slight bias toward center to prevent drifting too far
    FVector ToCenter = (PlayAreaCenter - GetActorLocation()).GetSafeNormal();
    FVector BiasedDir = (RandomDir + ToCenter * 0.2f).GetSafeNormal();

    return BiasedDir;
}

FVector ASnitchBall::CalculateEvadeVector() const
{
    // Use perception-based pursuer tracking instead of polling
    if (!SnitchController)
    {
        return FVector::ZeroVector;
    }

    TArray<AActor*> Pursuers = SnitchController->GetCurrentPursuers();
    if (Pursuers.Num() == 0)
    {
        return FVector::ZeroVector;
    }

    FVector MyLocation = GetActorLocation();
    FVector EvadeSum = FVector::ZeroVector;
    int32 EvadeCount = 0;

    for (AActor* Pursuer : Pursuers)
    {
        if (!Pursuer)
        {
            continue;
        }

        FVector ToPursuer = Pursuer->GetActorLocation() - MyLocation;
        float Distance = ToPursuer.Size();

        // Only evade if within evade radius
        if (Distance < EvadeRadius && Distance > KINDA_SMALL_NUMBER)
        {
            // Evade away from pursuer, stronger when closer
            FVector AwayDir = -ToPursuer.GetSafeNormal();
            float Strength = 1.0f - (Distance / EvadeRadius);
            EvadeSum += AwayDir * Strength;
            EvadeCount++;
        }
    }

    if (EvadeCount > 0)
    {
        return EvadeSum.GetSafeNormal();
    }

    return FVector::ZeroVector;
}

FVector ASnitchBall::CalculateBoundaryForce() const
{
    FVector MyLocation = GetActorLocation();
    FVector FromCenter = MyLocation - PlayAreaCenter;
    FVector Force = FVector::ZeroVector;

    // Check each axis against boundary
    for (int32 i = 0; i < 3; i++)
    {
        float Pos = FromCenter[i];
        float Extent = PlayAreaExtent[i];

        // Start pushing back at 80% of extent
        if (FMath::Abs(Pos) > Extent * 0.8f)
        {
            float Overshoot = (FMath::Abs(Pos) - Extent * 0.8f) / (Extent * 0.2f);
            Force[i] = -FMath::Sign(Pos) * FMath::Clamp(Overshoot, 0.0f, 1.0f);
        }
    }

    return Force.GetSafeNormal();
}

FVector ASnitchBall::CalculateObstacleAvoidance()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return FVector::ZeroVector;
    }

    FVector MyLocation = GetActorLocation();
    FVector AvoidanceSum = FVector::ZeroVector;

    // 5 ray directions: Forward, Down, Left, Right, and Down-Forward
    TArray<FVector> RayDirections;
    RayDirections.Add(CurrentDirection);
    RayDirections.Add(FVector::DownVector);
    RayDirections.Add(FVector::CrossProduct(CurrentDirection, FVector::UpVector).GetSafeNormal());
    RayDirections.Add(-FVector::CrossProduct(CurrentDirection, FVector::UpVector).GetSafeNormal());
    RayDirections.Add((CurrentDirection + FVector::DownVector).GetSafeNormal());

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    // Clear previous hits for debug visualization
    LastObstacleHits.Empty();

    for (const FVector& Direction : RayDirections)
    {
        FHitResult HitResult;
        FVector TraceEnd = MyLocation + Direction * ObstacleCheckDistance;

        bool bHit = World->LineTraceSingleByChannel(
            HitResult,
            MyLocation,
            TraceEnd,
            ObstacleChannel,
            QueryParams
        );

        if (bHit)
        {
            // Store for debug visualization
            LastObstacleHits.Add(HitResult);

            // Calculate avoidance: stronger when closer
            float DistanceRatio = 1.0f - (HitResult.Distance / ObstacleCheckDistance);
            FVector AwayFromObstacle = HitResult.Normal * DistanceRatio;
            AvoidanceSum += AwayFromObstacle;
        }
    }

    return AvoidanceSum.GetSafeNormal() * ObstacleAvoidanceStrength;
}

float ASnitchBall::GetGroundHeight() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return 0.0f;
    }

    FVector MyLocation = GetActorLocation();
    FHitResult HitResult;

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    // Trace straight down to find ground
    bool bHit = World->LineTraceSingleByChannel(
        HitResult,
        MyLocation,
        MyLocation + FVector::DownVector * GroundTraceDistance,
        ECC_WorldStatic,
        QueryParams
    );

    if (bHit)
    {
        return HitResult.Location.Z;
    }

    // No ground found - return very low value
    return MyLocation.Z - GroundTraceDistance;
}

FVector ASnitchBall::EnforceHeightConstraints(const FVector& DesiredLocation) const
{
    FVector ConstrainedLocation = DesiredLocation;
    float GroundZ = GetGroundHeight();

    float MinZ = GroundZ + MinHeightAboveGround;
    float MaxZ = GroundZ + MaxHeightAboveGround;

    // Clamp Z within height bounds
    ConstrainedLocation.Z = FMath::Clamp(ConstrainedLocation.Z, MinZ, MaxZ);

    return ConstrainedLocation;
}

void ASnitchBall::HandlePursuerDetected(AActor* Pursuer)
{
    // Structured logging - pursuer detected
    SLOG_EVENT(this, "Snitch.Perception", "PursuerDetected",
        Metadata.Add(TEXT("pursuer_name"), Pursuer ? Pursuer->GetName() : TEXT("null"));
    );

    UE_LOG(LogSnitchBall, Display, TEXT("[Snitch] Pursuer detected: %s"), *Pursuer->GetName());
    // Could trigger visual/audio feedback here (wing flutter, glow, etc.)
}

void ASnitchBall::HandlePursuerLost(AActor* Pursuer)
{
    // Structured logging - pursuer lost
    SLOG_EVENT(this, "Snitch.Perception", "PursuerLost",
        Metadata.Add(TEXT("pursuer_name"), Pursuer ? Pursuer->GetName() : TEXT("null"));
    );

    UE_LOG(LogSnitchBall, Display, TEXT("[Snitch] Pursuer lost: %s"), *Pursuer->GetName());
}

void ASnitchBall::OnSnitchOverlap(UPrimitiveComponent* OverlappedComponent,
                                   AActor* OtherActor,
                                   UPrimitiveComponent* OtherComp,
                                   int32 OtherBodyIndex,
                                   bool bFromSweep,
                                   const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == this)
    {
        return;
    }

    APawn* CatchingPawn = Cast<APawn>(OtherActor);
    if (!CatchingPawn)
    {
        return;
    }

    // Only Seekers can catch the Snitch
    if (!CatchingPawn->ActorHasTag(TEXT("Seeker")))
    {
        return;
    }

    UE_LOG(LogSnitchBall, Display, TEXT("[Snitch] CAUGHT by %s!"), *CatchingPawn->GetName());

    // Notify GameMode - Observer Pattern (SILENT FAILURE FIX)
    AQuidditchGameMode* GM = Cast<AQuidditchGameMode>(GetWorld()->GetAuthGameMode());
    if (!GM)
    {
        // Structured logging - GameMode not found (silent failure fix)
        SLOG_WARNING(this, "Snitch.Gameplay", "GameModeNotFound",
            Metadata.Add(TEXT("expected_class"), TEXT("AQuidditchGameMode"));
        );

        UE_LOG(LogSnitchBall, Warning, TEXT("[Snitch] No QuidditchGameMode - cannot notify catch!"));
        return;
    }

    // Determine team from controller's team ID
    EQuidditchTeam CatchingTeam = EQuidditchTeam::None;
    FString TeamResolvedStatus = TEXT("no_controller");

    if (AController* CatcherController = CatchingPawn->GetController())
    {
        if (IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(CatcherController))
        {
            FGenericTeamId TeamId = TeamAgent->GetGenericTeamId();
            // TeamA = 0, TeamB = 1 (based on QuidditchGameMode convention)
            CatchingTeam = (TeamId.GetId() == 0) ? EQuidditchTeam::TeamA : EQuidditchTeam::TeamB;
            TeamResolvedStatus = TEXT("resolved");

            UE_LOG(LogSnitchBall, Display, TEXT("[Snitch] Catcher team ID: %d -> %s"),
                TeamId.GetId(),
                (CatchingTeam == EQuidditchTeam::TeamA) ? TEXT("TeamA") : TEXT("TeamB"));
        }
        else
        {
            TeamResolvedStatus = TEXT("no_team_interface");
        }
    }

    // Structured logging - Snitch caught
    SLOG_EVENT(this, "Snitch.Gameplay", "SnitchCaught",
        Metadata.Add(TEXT("catcher_name"), CatchingPawn->GetName());
        Metadata.Add(TEXT("catcher_team"), FString::FromInt(static_cast<int32>(CatchingTeam)));
        Metadata.Add(TEXT("catcher_class"), CatchingPawn->GetClass()->GetName());
        Metadata.Add(TEXT("team_id_resolved"), TeamResolvedStatus);
    );

    // Observer Pattern: GameMode handles scoring and match end
    GM->NotifySnitchCaught(CatchingPawn, CatchingTeam);

    // Disable further overlaps (prevent double-catch)
    CollisionSphere->SetGenerateOverlapEvents(false);
}

void ASnitchBall::DrawDebugInfo() const
{
    DrawEnhancedDebugInfo();
    DrawOnScreenDebugText();
}

void ASnitchBall::DrawEnhancedDebugInfo() const
{
#if ENABLE_DRAW_DEBUG
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FVector MyLocation = GetActorLocation();

    // GREEN ARROW: Current velocity/movement direction
    DrawDebugDirectionalArrow(World, MyLocation,
        MyLocation + CurrentDirection * CurrentSpeed * 0.3f,
        30.0f, FColor::Green, false, -1.0f, 0, 4.0f);

    // YELLOW ARROW: Desired wander direction (offset up slightly to not overlap)
    FVector WanderDir = CalculateWanderDirection();
    DrawDebugDirectionalArrow(World, MyLocation + FVector(0, 0, 20),
        MyLocation + FVector(0, 0, 20) + WanderDir * 150.0f,
        20.0f, FColor::Yellow, false, -1.0f, 0, 2.0f);

    // CYAN LINES: Obstacle check rays (5 directions)
    TArray<FVector> RayDirections;
    RayDirections.Add(CurrentDirection);
    RayDirections.Add(FVector::DownVector);
    RayDirections.Add(FVector::CrossProduct(CurrentDirection, FVector::UpVector).GetSafeNormal());
    RayDirections.Add(-FVector::CrossProduct(CurrentDirection, FVector::UpVector).GetSafeNormal());
    RayDirections.Add((CurrentDirection + FVector::DownVector).GetSafeNormal());

    for (const FVector& Dir : RayDirections)
    {
        DrawDebugLine(World, MyLocation,
            MyLocation + Dir * ObstacleCheckDistance,
            FColor::Cyan, false, -1.0f, 0, 1.0f);
    }

    // RED LINES + SPHERES: Obstacle hit points and surface normals
    for (const FHitResult& Hit : LastObstacleHits)
    {
        DrawDebugLine(World, Hit.Location,
            Hit.Location + Hit.Normal * 100.0f,
            FColor::Red, false, -1.0f, 0, 3.0f);
        DrawDebugSphere(World, Hit.Location, 10.0f, 8, FColor::Red, false, -1.0f, 0, 2.0f);
    }

    // ORANGE LINE: Minimum height boundary
    float GroundZ = CurrentGroundHeight;
    FVector MinHeightPoint = FVector(MyLocation.X, MyLocation.Y, GroundZ + MinHeightAboveGround);
    DrawDebugLine(World,
        MinHeightPoint + FVector(-200, 0, 0),
        MinHeightPoint + FVector(200, 0, 0),
        FColor::Orange, false, -1.0f, 0, 2.0f);

    // PURPLE LINE: Maximum height boundary
    FVector MaxHeightPoint = FVector(MyLocation.X, MyLocation.Y, GroundZ + MaxHeightAboveGround);
    DrawDebugLine(World,
        MaxHeightPoint + FVector(-200, 0, 0),
        MaxHeightPoint + FVector(200, 0, 0),
        FColor::Purple, false, -1.0f, 0, 2.0f);

    // Play area boundary box (cyan wireframe)
    DrawDebugBox(World, PlayAreaCenter, PlayAreaExtent, FColor::Cyan, false, -1.0f, 0, 1.0f);

    // Evade radius sphere (only shown when pursuers are detected)
    if (SnitchController && SnitchController->GetCurrentPursuers().Num() > 0)
    {
        DrawDebugSphere(World, MyLocation, EvadeRadius, 16, FColor::Red, false, -1.0f, 0, 1.0f);
    }
#endif
}

void ASnitchBall::DrawOnScreenDebugText() const
{
#if ENABLE_DRAW_DEBUG
    if (!GEngine)
    {
        return;
    }

    FVector MyLocation = GetActorLocation();
    int32 PursuerCount = SnitchController ? SnitchController->GetCurrentPursuers().Num() : 0;
    bool bIsEvading = PursuerCount > 0;

    // Each message has unique key (first param) - prevents overlap
    // Duration 0.0f = single frame, refreshes each tick

    GEngine->AddOnScreenDebugMessage(100, 0.0f, FColor::Yellow,
        TEXT("======== SNITCH DEBUG ========"));

    GEngine->AddOnScreenDebugMessage(101, 0.0f, FColor::White,
        FString::Printf(TEXT("Location: X=%.0f Y=%.0f Z=%.0f"),
            MyLocation.X, MyLocation.Y, MyLocation.Z));

    GEngine->AddOnScreenDebugMessage(102, 0.0f, FColor::Green,
        FString::Printf(TEXT("Speed: %.0f / %.0f (Base/Max)"),
            CurrentSpeed, bIsEvading ? MaxEvadeSpeed : BaseSpeed));

    GEngine->AddOnScreenDebugMessage(103, 0.0f, FColor::Cyan,
        FString::Printf(TEXT("Direction: X=%.2f Y=%.2f Z=%.2f"),
            CurrentDirection.X, CurrentDirection.Y, CurrentDirection.Z));

    GEngine->AddOnScreenDebugMessage(104, 0.0f, bIsEvading ? FColor::Red : FColor::White,
        FString::Printf(TEXT("Evasion: %s (Pursuers: %d)"),
            bIsEvading ? TEXT("ACTIVE") : TEXT("Inactive"), PursuerCount));

    GEngine->AddOnScreenDebugMessage(105, 0.0f, FColor::Orange,
        FString::Printf(TEXT("Ground Height: %.0f | Current Z: %.0f"),
            CurrentGroundHeight, MyLocation.Z));

    GEngine->AddOnScreenDebugMessage(106, 0.0f, FColor::Magenta,
        FString::Printf(TEXT("Height Bounds: %.0f - %.0f"),
            CurrentGroundHeight + MinHeightAboveGround,
            CurrentGroundHeight + MaxHeightAboveGround));

    GEngine->AddOnScreenDebugMessage(107, 0.0f, FColor::White,
        FString::Printf(TEXT("Direction Timer: %.1f / %.1f"),
            DirectionChangeTimer, NextDirectionChangeTime));

    GEngine->AddOnScreenDebugMessage(108, 0.0f, FColor::Cyan,
        FString::Printf(TEXT("Obstacles Detected: %d"), LastObstacleHits.Num()));

    GEngine->AddOnScreenDebugMessage(109, 0.0f, FColor::Yellow,
        TEXT("=============================="));
#endif
}

