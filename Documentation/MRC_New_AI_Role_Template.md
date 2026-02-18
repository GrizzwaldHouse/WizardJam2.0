# MRC-ROLE-001: Add New AI Role - [ROLE NAME]

Developer: Marcus Daley | Architecture Lead: Nick Penney AAA Standards
Engine: Unreal Engine 5.4 | Date: February 2026

---

## PURPOSE

This template provides a step-by-step checklist for adding any new Quidditch AI role
(Chaser, Beater, Keeper, etc.) to the WizardJam system. Every new role reuses the
shared infrastructure (broom acquisition, flight, staging, match synchronization) and
only adds role-specific perception services, action tasks, and decorators.

### System Context
- `AQuidditchGameMode` with role registration, team queries, max-per-role limits, and 10 sync delegates
- `AAIC_QuidditchController` managing 18+ BB keys, 5 GameMode delegate bindings, 4 BroomComponent delegate bindings
- 28 BT nodes: 14 tasks, 8 services, 2 decorators, 4 Snitch AI nodes (all compliant with key init standards)
- `EQuidditchRole` enum: None, Seeker, Chaser, Beater, Keeper
- `EQuidditchTeam` enum: None, TeamA, TeamB
- `EQuidditchMatchState`: 8 states with explicit transitions
- Flight via direct velocity (NO NavMesh), Observer Pattern (NO polling), NO GameplayStatics

---

## ROLE IDENTITY

Fill in these values before starting implementation.

| Field | Value |
|-------|-------|
| Role Name | [e.g., Chaser] |
| Enum Value | EQuidditchRole::[Role] |
| Base Controller | AIC_QuidditchController |
| Max Per Team | [1/2/3] |
| Primary Objective | [1-sentence description] |
| Required Ball Class | [QuaffleBall / BludgerBall / SnitchBall / None] |
| Algorithm Source | [e.g., Flock.cs, PathSearch.cpp, MsPacManAgent.java] |
| New BT Nodes Required | [List of node names] |

---

## PHASE 1: BEHAVIOR TREE SETUP

### Step 1.1: Create Role Decorator

Every role needs a gating decorator. Follow the `BTDecorator_IsSeeker` pattern.

**File to create:** `Source/END2507/Public/Code/AI/BTDecorator_Is[Role].h`
**File to create:** `Source/END2507/Private/Code/AI/BTDecorator_Is[Role].cpp`

Pattern from BTDecorator_IsSeeker:
```cpp
// Header
UCLASS(meta = (DisplayName = "Is [Role] Role"))
class END2507_API UBTDecorator_Is[Role] : public UBTDecorator
{
    GENERATED_BODY()
public:
    UBTDecorator_Is[Role]();
    virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
    virtual FString GetStaticDescription() const override;
};

// Implementation - queries GameMode for agent's assigned role
bool UBTDecorator_Is[Role]::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
    // Get controller -> GameMode -> check if role == EQuidditchRole::[Role]
}
```

### Step 1.2: Create Role Branch in BT_QuidditchAI

Add a new Selector node under the root, gated by `BTDecorator_Is[Role]`.

**BT Structure:**
```
Root (Selector)
  |
  +-- [Role] Branch (Selector) [BTDecorator_Is[Role]]
  |     |
  |     +-- Acquire Broom (Sequence)          <-- SHARED, already exists
  |     |     +-- BTService_FindCollectible
  |     |     +-- BTTask_Interact (broom)
  |     |     +-- BTTask_CheckBroomChannel
  |     |
  |     +-- Mount and Fly to Staging (Sequence)  <-- SHARED, already exists
  |     |     +-- BTTask_MountBroom
  |     |     +-- BTService_FindStagingZone
  |     |     +-- BTTask_FlyToStagingZone
  |     |
  |     +-- Wait for Match (Sequence)          <-- SHARED, already exists
  |     |     +-- BTTask_WaitForMatchStart
  |     |
  |     +-- [Role]-Specific Match Play (Sequence)  <-- NEW, role-specific
  |     |     +-- BTService_Find[Target]       <-- NEW service
  |     |     +-- BTTask_[RoleAction]          <-- NEW task(s)
  |     |
  |     +-- Return Home (Fallback)             <-- SHARED, already exists
  |           +-- BTTask_ReturnToHome
  |
  +-- Seeker Branch (existing)
  +-- ... other roles
```

### Step 1.3: Create Role-Specific BT Nodes

For EACH new node, follow this mandatory checklist:

| # | Check | Details |
|---|-------|---------|
| 1 | Header: FBlackboardKeySelector | Declare with `UPROPERTY(EditAnywhere, Category = "Blackboard")` |
| 2 | Constructor: Key Filter | Call `AddObjectFilter` / `AddVectorFilter` / `AddBoolFilter` |
| 3 | Header: InitializeFromAsset | Declare `virtual void InitializeFromAsset(UBehaviorTree& Asset) override;` |
| 4 | Cpp: InitializeFromAsset | Implement with `ResolveSelectedKey(*BBAsset)` for EACH key selector |
| 5 | Zero GameplayStatics | Use TActorIterator or AI Perception for world queries |
| 6 | Direct Velocity for Flight | Use `CharacterMovementComponent->Velocity` (NO NavMesh MoveTo) |
| 7 | GetStaticDescription | Return human-readable description for BT editor |
| 8 | UPROPERTY Meta | Use `EditAnywhere` on BT node properties, `ClampMin`/`ClampMax` on numerics |

**InitializeFromAsset Pattern (MANDATORY for every FBlackboardKeySelector):**
```cpp
void UMyBTNode::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);
    if (UBlackboardData* BBAsset = GetBlackboardAsset())
    {
        MyKey1.ResolveSelectedKey(*BBAsset);
        MyKey2.ResolveSelectedKey(*BBAsset);
        // One call per FBlackboardKeySelector property
    }
}
```

### Step 1.4: Role-Specific Services

| Service Name | Purpose | BB Keys Written | Tick Interval |
|-------------|---------|-----------------|---------------|
| BTService_Find[Target] | Locate role's primary target via perception | [Target]ActorKey, [Target]LocationKey, [Target]VelocityKey | 0.25s |
| [Additional as needed] | | | |

### Step 1.5: Role-Specific Tasks

| Task Name | Purpose | BB Keys Read | BB Keys Written | Ticking? |
|-----------|---------|-------------|-----------------|----------|
| BTTask_[Action1] | Primary role action | [Input keys] | [Output keys] | Yes/No |
| BTTask_[Action2] | Secondary action | [Input keys] | [Output keys] | Yes/No |

---

## PHASE 2: BLACKBOARD KEY SETUP

For EACH new BB key required by the role, complete ALL 7 steps.
Missing ANY step causes silent failure.

### New BB Key Checklist

| # | Check | Details | File |
|---|-------|---------|------|
| 1 | BB Asset | Key exists in BB_QuidditchAI with correct type (Bool/Vector/Object/Name/Float) | BB_QuidditchAI (Editor) |
| 2 | Controller FName | `UPROPERTY(EditDefaultsOnly, Category = "Quidditch|Sync") FName [KeyName]KeyName;` | AIC_QuidditchController.h |
| 3 | Constructor Init | Added to constructor init list: `, [KeyName]KeyName(TEXT("[KeyName]"))` | AIC_QuidditchController.cpp |
| 4 | Initial Value | `BBComp->SetValueAs[Type](FName("[KeyName]"), [default])` in SetupBlackboard() | AIC_QuidditchController.cpp |
| 5 | Runtime Update | Delegate handler or BT Service updates key when state changes | Various |
| 6 | Delegate Bind | Handler bound in BindToGameModeEvents() or OnPossess() | AIC_QuidditchController.cpp |
| 7 | Delegate Unbind | Handler unbound in UnbindFromGameModeEvents() or OnUnPossess() | AIC_QuidditchController.cpp |

### Key Type Reference

| BB Key Type | SetValueAs | AddFilter | Common Use |
|------------|-----------|-----------|------------|
| Bool | `SetValueAsBool` | `AddBoolFilter` | State flags (HasBroom, IsFlying, MatchStarted) |
| Vector | `SetValueAsVector` | `AddVectorFilter` | Locations (TargetLocation, SnitchLocation) |
| Object | `SetValueAsObject` | `AddObjectFilter` | Actor refs (SnitchActor, PerceivedCollectible) |
| Name | `SetValueAsName` | `AddNameFilter` | Enum strings (QuidditchRole) |
| Float | `SetValueAsFloat` | `AddFloatFilter` | Numeric values (Distance, Timer) |

---

## PHASE 3: EVENT BINDINGS AND DELEGATES

### GameMode Delegates (bind in BindToGameModeEvents)

All roles bind to these core delegates:

| Delegate | Handler | BB Key Updated | Purpose |
|----------|---------|---------------|---------|
| OnMatchStarted | HandleMatchStarted | MatchStarted = true | Gate match play branch |
| OnMatchEnded | HandleMatchEnded | MatchStarted = false | Stop match play |
| OnAgentSelectedForSwap | HandleAgentSelectedForSwap | ShouldSwapTeam = true | Player join flow |
| OnTeamSwapComplete | HandleTeamSwapComplete | ShouldSwapTeam = false | Post-swap cleanup |
| OnQuidditchRoleAssigned | HandleQuidditchRoleAssigned | QuidditchRole = [role] | Role initialization |

### BroomComponent Delegates (bind in OnPossess)

All roles need:

| Delegate | Handler | BB Keys Updated | Purpose |
|----------|---------|----------------|---------|
| OnFlightStateChanged | HandleFlightStateChanged | IsFlying, HasBroom | Sync flight state to BB |

### Pawn Overlap Delegates (bind via BindToPawnOverlapEvents)

All roles need:

| Delegate | Handler | BB Keys Updated | Purpose |
|----------|---------|----------------|---------|
| OnActorBeginOverlap | HandlePawnBeginOverlap | ReachedStagingZone, IsReady | Staging zone detection |
| OnActorEndOverlap | HandlePawnEndOverlap | ReachedStagingZone, IsReady | Staging zone exit |

### New Delegates (if role requires new GameMode events)

If the role needs a new event broadcast from GameMode:

1. Declare delegate type in QuidditchGameMode.h:
```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnNew[Event],
    APawn*, Agent,
    [ParamType], [ParamName]
);
```

2. Add UPROPERTY in AQuidditchGameMode:
```cpp
UPROPERTY(BlueprintAssignable, Category = "Quidditch|Sync")
FOnNew[Event] OnNew[Event];
```

3. Add handler in AIC_QuidditchController.h:
```cpp
UFUNCTION()
void HandleNew[Event](APawn* Agent, [ParamType] [ParamName]);
```

4. Bind in BindToGameModeEvents() and unbind in UnbindFromGameModeEvents().

---

## PHASE 4: GAMEMODE REGISTRATION

### Step 4.1: Verify Role Limits

Check QuidditchGameMode.h for existing Max[Role]PerTeam property:
- `MaxSeekersPerTeam` (default: 1)
- `MaxChasersPerTeam` (default: 3)
- `MaxBeatersPerTeam` (default: 2)
- `MaxKeepersPerTeam` (default: 1)

If new role, add:
```cpp
UPROPERTY(EditDefaultsOnly, Category = "Quidditch|Teams")
int32 Max[Role]sPerTeam;
```

Initialize in constructor and add to `GetMaxForRole()`.

### Step 4.2: Update FindAvailableRole

If new role needs fallback logic when preferred slot is full, update
`FindAvailableRole()` to include the new role in the priority chain.

### Step 4.3: Add Team Query (Optional)

If needed, add role-specific query like existing patterns:
```cpp
UFUNCTION(BlueprintPure, Category = "Quidditch|Teams")
TArray<APawn*> GetTeam[Role]s(EQuidditchTeam Team) const;
```

---

## PHASE 5: TESTING

### Minimal Test Setup

1. Set `RequiredAgentOverride = 1` in BP_QuidditchGameMode
2. Place 1 agent with PreferredRole = [Role], Team = TeamA
3. Place 1 BP_BroomCollectible within 3000 units of agent
4. Place 1 QuidditchStagingZone (TeamHint = TeamA int value, RoleHint = [Role] int value)
5. Place role-specific actors (ball, goal, etc.)
6. PIE and verify Output Log

### Verification Checklist

| # | Check | Expected Output Log |
|---|-------|-------------------|
| 1 | Agent registers with correct role | `RegisterAgentWithGameMode SUCCESS \| Assigned=[Role]` |
| 2 | BB keys initialized | Check BB debugger in BT editor - no (invalid) markers |
| 3 | Broom acquisition works | `PERCEIVED: BP_BroomCollectible_C` then `HasBroom=true` |
| 4 | Flight to staging zone works | `LANDED on staging zone` then `ReachedStagingZone=true` |
| 5 | Match starts after staging | `HandleMatchStarted - BB.MatchStarted = true` |
| 6 | Role-specific behavior executes | [Role-specific log output] |
| 7 | Match end terminates cleanly | `HandleMatchEnded - BB.MatchStarted = false` |
| 8 | No warnings or errors | Zero `Warning` or `Error` entries from LogQuidditchAI |

### AAA Best Practices Checklist

| # | Check | Violation Example |
|---|-------|------------------|
| 1 | Zero GameplayStatics usage | `UGameplayStatics::GetGameMode(this)` |
| 2 | Observer Pattern for all cross-system comms | Polling GameMode in Tick() |
| 3 | Direct velocity for flight | `UNavigationSystemV1::SimpleMoveToLocation()` |
| 4 | FBlackboardKeySelector dual-initialization | Missing InitializeFromAsset or AddFilter |
| 5 | EditDefaultsOnly on UPROPERTY | `EditAnywhere` on class defaults |
| 6 | Constructor initialization lists | Assigning defaults in header |
| 7 | RemoveDynamic for every AddDynamic | Missing unbind in OnUnPossess/EndPlay |
| 8 | No hardcoded values | Magic numbers in .h files |

---

## APPENDIX A: FILLED EXAMPLE - SEEKER ROLE

### Role Identity

| Field | Value |
|-------|-------|
| Role Name | Seeker |
| Enum Value | EQuidditchRole::Seeker |
| Base Controller | AIC_QuidditchController |
| Max Per Team | 1 |
| Primary Objective | Chase and catch the Golden Snitch to end the match (+150 points) |
| Required Ball Class | SnitchBall (spawned by GameMode, NOT placed manually) |
| Algorithm Source | PathSearch.cpp intercept prediction |
| New BT Nodes Required | BTDecorator_IsSeeker, BTService_FindSnitch, BTTask_ChaseSnitch, BTTask_PredictIntercept, BTTask_CatchSnitch |

### Seeker BT Structure (5 Phases)

```
Root (Selector)
  |
  +-- Seeker Branch (Selector) [BTDecorator_IsSeeker]
        |
        +-- Phase 1: Acquire Broom (Sequence) [BTDecorator_HasChannel("Broom", Inverse=true)]
        |     +-- BTService_FindCollectible (-> PerceivedCollectible)
        |     +-- BTService_FindInteractable (-> TargetActor, class=ABroomActor)
        |     +-- BTTask_Interact (TargetActor, RequiredChannel="")
        |     +-- BTTask_CheckBroomChannel (-> HasBroom=true)
        |
        +-- Phase 2: Mount & Fly to Staging (Sequence) [BB.HasBroom == true, BB.ReachedStagingZone == false]
        |     +-- BTTask_MountBroom (bMountBroom=true)
        |     +-- BTService_SyncFlightState (-> IsFlying)
        |     +-- BTService_FindStagingZone (-> StagingZoneLocation, StagingZoneActor)
        |     +-- BTTask_FlyToStagingZone (StagingZoneLocationKey)
        |
        +-- Phase 3: Wait for Match (Sequence) [BB.ReachedStagingZone == true, BB.MatchStarted == false]
        |     +-- BTTask_WaitForMatchStart (MatchStartedKey)
        |
        +-- Phase 4: Chase Snitch (Sequence) [BB.MatchStarted == true]
        |     +-- BTService_FindSnitch (-> SnitchActor, SnitchLocation, SnitchVelocity)
        |     +-- BTTask_PredictIntercept (SnitchActor, SnitchVelocity -> InterceptPoint)
        |     +-- BTTask_ChaseSnitch (SnitchLocationKey, CatchRadius=200)
        |     +-- BTTask_CatchSnitch (SnitchActorKey, CatchRadius=200, Points=150)
        |
        +-- Phase 5: Fallback (Sequence)
              +-- BTTask_ReturnToHome (HomeLocationKey)
```

### All BT Nodes in Seeker Pipeline

**Shared Nodes (reused across all roles):**

| # | Node | Type | Purpose | BB Keys |
|---|------|------|---------|---------|
| 1 | BTDecorator_HasChannel | Decorator | Gate by channel ("Broom") | None (reads SpellCollectionComponent) |
| 2 | BTService_FindCollectible | Service | Scan perception for collectibles | OutputKey (Object) |
| 3 | BTService_FindInteractable | Service | Scan perception for IInteractable | OutputKey (Object) |
| 4 | BTTask_Interact | Task | Call IInteractable::Interact() | TargetKey (Object), SuccessStateKey (Bool) |
| 5 | BTTask_CheckBroomChannel | Task | Sync channel to BB.HasBroom | HasBroomKey (Bool) |
| 6 | BTTask_MountBroom | Task | Call BroomComponent::SetFlightEnabled(true) | FlightStateKey (Bool) |
| 7 | BTService_SyncFlightState | Service | Sync BroomComponent::IsFlying to BB | IsFlyingKey (Bool) |
| 8 | BTService_FindStagingZone | Service | Find staging zone via perception | StagingZoneActorKey (Object), StagingZoneLocationKey (Vector) |
| 9 | BTTask_FlyToStagingZone | Task | Fly to staging zone with 5 nav solutions | StagingZoneLocationKey (Vector), TargetLocationKey (Vector) |
| 10 | BTTask_WaitForMatchStart | Task | Wait for BB.MatchStarted == true | MatchStartedKey (Bool) |
| 11 | BTTask_ReturnToHome | Task | Return to staging zone (fallback) | HomeLocationKey (Vector) |
| 12 | BTTask_ControlFlight | Task | Generic flight to BB target | TargetKey (Vector/Object) |

**Seeker-Specific Nodes:**

| # | Node | Type | Purpose | BB Keys |
|---|------|------|---------|---------|
| 13 | BTDecorator_IsSeeker | Decorator | Gate Seeker branch | None (queries GameMode) |
| 14 | BTService_FindSnitch | Service | Track Snitch via perception | SnitchActorKey (Object), SnitchLocationKey (Vector), SnitchVelocityKey (Vector) |
| 15 | BTTask_PredictIntercept | Task | PathSearch.cpp intercept algorithm | TargetActorKey, TargetVelocityKey -> InterceptPointKey, TimeToInterceptKey |
| 16 | BTTask_ChaseSnitch | Task | Continuous pursuit with boost | SnitchLocationKey (Vector) |
| 17 | BTTask_CatchSnitch | Task | Trigger catch + score 150 points | SnitchActorKey (Object) |

**Snitch AI Nodes (opponent, not Seeker-specific):**

| # | Node | Type | Purpose |
|---|------|------|---------|
| 18 | BTService_TrackNearestSeeker | Service | Snitch tracks nearest threat |
| 19 | BTTask_EvadeSeeker | Task | Snitch evasive movement |

### All BB Keys Used by Seeker

| Key Name | Type | Initial Value | Set By | Read By |
|----------|------|---------------|--------|---------|
| SelfActor | Object | InPawn | SetupBlackboard | Various tasks |
| HomeLocation | Vector | SpawnLocation | SetupBlackboard | BTTask_ReturnToHome |
| IsFlying | Bool | false | HandleFlightStateChanged | BTDecorator, BTService_SyncFlightState |
| HasBroom | Bool | false | HandleFlightStateChanged, BTTask_CheckBroomChannel | BTDecorator_HasChannel |
| MatchStarted | Bool | false | HandleMatchStarted/HandleMatchEnded | BTTask_WaitForMatchStart |
| ShouldSwapTeam | Bool | false | HandleAgentSelectedForSwap | BTTask_SwapTeam |
| QuidditchRole | Name | (unset) | HandleQuidditchRoleAssigned | BTDecorator_IsSeeker |
| TargetLocation | Vector | ZeroVector | Various tasks | BTTask_ControlFlight |
| TargetActor | Object | (unset) | Perception handler | BTTask_Interact |
| PerceivedCollectible | Object | (unset) | BTService_FindCollectible | BTTask_Interact |
| SnitchLocation | Vector | ZeroVector | BTService_FindSnitch | BTTask_ChaseSnitch |
| SnitchVelocity | Vector | ZeroVector | BTService_FindSnitch | BTTask_PredictIntercept |
| StagingZoneLocation | Vector | ZeroVector | BTService_FindStagingZone | BTTask_FlyToStagingZone |
| GoalCenter | Vector | ZeroVector | SetupBlackboard | BTTask_PositionInGoal, BTTask_BlockShot |
| ReachedStagingZone | Bool | false | HandlePawnBeginOverlap/EndOverlap | BT decorator |
| IsReady | Bool | false | HandlePawnBeginOverlap/EndOverlap | BT decorator |
| StageLocation | Vector | ZeroVector | SetupBlackboard | BTTask_FlyToStagingZone fallback |

### All Delegate Bindings for Seeker

**GameMode Delegates (bound in BindToGameModeEvents, unbound in UnbindFromGameModeEvents):**

| # | Delegate | Handler | Bind Call |
|---|----------|---------|-----------|
| 1 | OnMatchStarted | HandleMatchStarted | `AddDynamic(this, &AAIC_QuidditchController::HandleMatchStarted)` |
| 2 | OnMatchEnded | HandleMatchEnded | `AddDynamic(this, &AAIC_QuidditchController::HandleMatchEnded)` |
| 3 | OnAgentSelectedForSwap | HandleAgentSelectedForSwap | `AddDynamic(this, &AAIC_QuidditchController::HandleAgentSelectedForSwap)` |
| 4 | OnTeamSwapComplete | HandleTeamSwapComplete | `AddDynamic(this, &AAIC_QuidditchController::HandleTeamSwapComplete)` |
| 5 | OnQuidditchRoleAssigned | HandleQuidditchRoleAssigned | `AddDynamic(this, &AAIC_QuidditchController::HandleQuidditchRoleAssigned)` |

**BroomComponent Delegates (bound in OnPossess, unbound in OnUnPossess):**

| # | Delegate | Handler |
|---|----------|---------|
| 1 | OnFlightStateChanged | HandleFlightStateChanged |

**Pawn Overlap Delegates (bound via BindToPawnOverlapEvents, unbound via UnbindFromPawnOverlapEvents):**

| # | Delegate | Handler |
|---|----------|---------|
| 1 | OnActorBeginOverlap | HandlePawnBeginOverlap |
| 2 | OnActorEndOverlap | HandlePawnEndOverlap |

### Seeker Testing Results (Phase 1 Vertical Slice)

**Minimal Setup:**
- 1 agent (PreferredRole=Seeker, Team=TeamA)
- 1 BP_BroomCollectible within 3000 units
- 1 QuidditchStagingZone (TeamA, Seeker)
- 1 SnitchBall (spawns via GameMode or manual placement)
- RequiredAgentOverride = 1

**Expected Flow:**
1. Agent spawns, controller possesses, BB initialized
2. Agent perceives BroomCollectible, walks to it, interacts -> HasBroom=true
3. Agent mounts broom -> IsFlying=true
4. Agent perceives StagingZone via BTService_FindStagingZone
5. Agent flies to staging zone -> ReachedStagingZone=true
6. GameMode transitions WaitingForReady -> Countdown -> InProgress
7. HandleMatchStarted fires -> BB.MatchStarted=true
8. BTService_FindSnitch begins tracking Snitch
9. BTTask_ChaseSnitch pursues Snitch with boost management
10. BTTask_CatchSnitch triggers on proximity -> OnSnitchCaught -> MatchEnded

---

## APPENDIX B: ROLE-SPECIFIC NODE CATALOG

Reference for existing nodes organized by which roles use them.

### Chaser-Specific Nodes (Phase 3 - Not Yet Active)

| Node | Type | Algorithm | BB Keys |
|------|------|-----------|---------|
| BTService_FindQuaffle | Service | Perception + possession tracking | QuaffleActorKey, QuaffleLocationKey, QuaffleVelocityKey, IsQuaffleFreeKey, TeammateHasQuaffleKey |
| BTTask_FlockWithTeam | Task | Flock.cs (Separation + Alignment + Cohesion) | FlockDirectionKey, FlockTargetKey |
| BTTask_ThrowQuaffle | Task | Prediction throw with lead factor | HeldQuaffleKey, ThrowTargetKey, ThrowLocationKey |

### Beater-Specific Nodes (Phase 3 - Not Yet Active)

| Node | Type | Algorithm | BB Keys |
|------|------|-----------|---------|
| BTService_FindBludger | Service | MsPacManAgent.java INVERTED threat scoring | NearestBludgerKey, BludgerLocationKey, BludgerVelocityKey, ThreateningBludgerKey, ThreatenedTeammateKey, BestEnemyTargetKey |
| BTTask_HitBludger | Task | Directional hit with prediction | BludgerKey, TargetEnemyKey |

### Keeper-Specific Nodes (Phase 3 - Not Yet Active)

| Node | Type | Algorithm | BB Keys |
|------|------|-----------|---------|
| BTTask_PositionInGoal | Task | Flock.cs Cohesion adapted for fixed point defense | GoalCenterKey, ThreatActorKey -> DefensePositionKey |
| BTTask_BlockShot | Task | PathSearch.cpp intercept for Quaffle | QuaffleKey, QuaffleVelocityKey, GoalCenterKey -> InterceptPositionKey, CanBlockKey |

### Utility Nodes (Available to All Roles)

| Node | Type | Purpose |
|------|------|---------|
| BTTask_ForceMount | Task (DEBUG) | Skip broom acquisition for testing |
| BTTask_SwapTeam | Task | Execute team swap when player joins |
| BTTask_EvadeSeeker | Task | Generic evasion from threat actor |
| BTService_TrackNearestSeeker | Service | Track nearest threat by tag |

---

## APPENDIX C: FILE PATH REFERENCE

### Source Files for New Roles

Headers go in: `Source/END2507/Public/Code/AI/` or `Source/END2507/Public/Code/AI/Quidditch/`
Implementations go in: `Source/END2507/Private/Code/AI/` or `Source/END2507/Private/Code/AI/Quidditch/`

### Key System Files

| File | Purpose |
|------|---------|
| `Source/END2507/Public/Code/Quidditch/QuidditchTypes.h` | EQuidditchRole, EQuidditchMatchState, EQuidditchBall enums |
| `Source/END2507/Public/Code/GameModes/QuidditchGameMode.h` | All delegates, registration API, team queries |
| `Source/END2507/Public/Code/AI/AIC_QuidditchController.h` | BB key names, delegate handlers, sync infrastructure |
| `Source/END2507/Private/Code/AI/AIC_QuidditchController.cpp` | SetupBlackboard, BindToGameModeEvents, all handler implementations |
| `Source/END2507/Public/Code/Flight/AC_BroomComponent.h` | Flight delegates, Core API (SetFlightEnabled, SetVerticalInput, SetBoostEnabled) |
| `Source/END2507/Public/Code/AI/AIC_SnitchController.h` | Snitch perception and pursuer tracking |

### Build System

| File | Purpose |
|------|---------|
| `Source/END2507/END2507.Build.cs` | Module dependencies - add new modules here if needed |
