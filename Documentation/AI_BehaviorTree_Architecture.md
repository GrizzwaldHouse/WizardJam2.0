# AI Behavior Tree Architecture Specification
## WizardJam 2.0 - Quidditch AI System

**Developer:** Marcus Daley
**Date:** February 11, 2026
**Status:** Pre-Implementation Reference (Session 1 Deliverable)
**Scope:** Verification, Diagrams, Key Convention, Reusability, Session 2 Plan

---

## Table of Contents

1. [Existing System Verification](#1-existing-system-verification)
2. [Blackboard Key Convention](#2-blackboard-key-convention)
3. [Canonical Behavior Tree Diagrams](#3-canonical-behavior-tree-diagrams)
4. [Node Reusability Catalog](#4-node-reusability-catalog)
5. [Recommended Minor Refactoring](#5-recommended-minor-refactoring)
6. [Session 2 Implementation Order](#6-session-2-implementation-order)

---

## 1. Existing System Verification

### 1.1 Node Inventory Summary

| Category | Count | InitializeFromAsset | AddFilter | Status |
|----------|-------|---------------------|-----------|--------|
| BT Tasks (Modern) | 18 | 18/18 (100%) | 18/18 (100%) | Production-ready |
| BT Tasks (Legacy) | 3 | 0/3 (0%) | 0/3 (0%) | Deprecated |
| BT Services | 8 | 8/8 (100%) | 8/8 (100%) | Production-ready |
| BT Decorators | 2 | N/A | N/A | Production-ready |
| **Total** | **31** | **26/29** | **26/29** | |

### 1.2 Complete BT Task Catalog (21 Tasks)

#### Core Flight System (10 Tasks)

| # | Class | Parent | Execution | BB Keys (Selectors) | Reusable? |
|---|-------|--------|-----------|---------------------|-----------|
| 1 | BTTask_MountBroom | UBTTaskNode | Instant | FlightStateKey (Object) | Yes |
| 2 | BTTask_ControlFlight | UBTTaskNode | Latent | TargetKey (Vector/Object) | Yes |
| 3 | BTTask_FlyToStagingZone | UBTTaskNode | Latent | StagingZoneLocationKey (Vector), TargetLocationKey (Vector) | Quidditch |
| 4 | BTTask_ReturnToHome | UBTTaskNode | Latent | HomeLocationKey (Vector) | Yes |
| 5 | BTTask_ForceMount | UBTTaskNode | Instant | None | Debug only |
| 6 | BTTask_Interact | UBTTaskNode | Latent | TargetKey (Object), SuccessStateKey (Bool) | Yes |
| 7 | BTTask_CheckBroomChannel | UBTTaskNode | Instant | HasBroomKey (Bool) | Yes |
| 8 | BTTask_WaitForMatchStart | UBTTaskNode | Latent | MatchStartedKey (Bool) | Quidditch |
| 9 | BTTask_SwapTeam | UBTTaskNode | Instant | ShouldSwapTeamKey (Bool) | Quidditch |
| 10 | BTTask_EvadeSeeker | UBTTaskNode | Latent | ThreatActorKey (Object), IsEvadingKey (Bool) | Yes |

#### Quidditch Match Gameplay (8 Tasks)

| # | Class | Parent | Execution | BB Keys (Selectors) | Role |
|---|-------|--------|-----------|---------------------|------|
| 11 | BTTask_ChaseSnitch | UBTTaskNode | Latent | SnitchLocationKey (Vector) | Seeker |
| 12 | BTTask_CatchSnitch | UBTTaskNode | Instant | SnitchActorKey (Object) | Seeker |
| 13 | BTTask_PredictIntercept | UBTTaskNode | Instant | TargetActorKey (Object), TargetVelocityKey (Vector), InterceptPointKey (Vector), TimeToInterceptKey (Float) | Any |
| 14 | BTTask_ThrowQuaffle | UBTTaskNode | Instant | HeldQuaffleKey (Object), ThrowTargetKey (Object), ThrowLocationKey (Vector) | Chaser |
| 15 | BTTask_FlockWithTeam | UBTTaskNode | Instant | FlockDirectionKey (Vector), FlockTargetKey (Vector) | Chaser |
| 16 | BTTask_HitBludger | UBTTaskNode | Instant | BludgerKey (Object), TargetEnemyKey (Object) | Beater |
| 17 | BTTask_PositionInGoal | UBTTaskNode | Instant | GoalCenterKey (Vector), ThreatActorKey (Object), DefensePositionKey (Vector) | Keeper |
| 18 | BTTask_BlockShot | UBTTaskNode | Instant | QuaffleKey (Object), QuaffleVelocityKey (Vector), GoalCenterKey (Vector), InterceptPositionKey (Vector), CanBlockKey (Bool) | Keeper |

#### Legacy Tasks (3 Tasks - DEPRECATED)

| # | Class | Parent | Execution | BB Keys | Issues |
|---|-------|--------|-----------|---------|--------|
| 19 | BTTask_CodeEnemyAttack | UBTTaskNode | Latent | None | No BB integration |
| 20 | BTTask_CodeEnemyReload | UBTTaskNode | Latent | FName only (no selector) | Missing InitializeFromAsset |
| 21 | BTTask_EnemyFlee | UBTTaskNode | Instant | FName only, EQS callback commented out | Incomplete, use BTTask_EvadeSeeker |

### 1.3 Complete BT Service Catalog (8 Services)

| # | Class | Interval | BB Keys Written | Filter Types |
|---|-------|----------|-----------------|-------------|
| 1 | BTService_FindCollectible | 0.5s | OutputKey (Object) | Object:AActor |
| 2 | BTService_FindInteractable | 0.5s | OutputKey (Object) | Object:AActor |
| 3 | BTService_SyncFlightState | 0.25s | IsFlyingKey (Bool) | Bool |
| 4 | BTService_TrackNearestSeeker | 0.2s | NearestSeekerKey (Object) | Object:AActor |
| 5 | BTService_FindStagingZone | 0.5s | StagingZoneActorKey (Object), StagingZoneLocationKey (Vector) | Object:AActor, Vector |
| 6 | BTService_FindSnitch | 0.1s | SnitchActorKey (Object), SnitchLocationKey (Vector), SnitchVelocityKey (Vector) | Object:AActor, Vector, Vector |
| 7 | BTService_FindQuaffle | 0.15s | QuaffleActorKey (Object), QuaffleLocationKey (Vector), QuaffleVelocityKey (Vector), IsQuaffleFreeKey (Bool), TeammateHasQuaffleKey (Bool) | Object:AActor, Vector x2, Bool x2 |
| 8 | BTService_FindBludger | 0.12s | NearestBludgerKey (Object), BludgerLocationKey (Vector), BludgerVelocityKey (Vector), ThreateningBludgerKey (Object), ThreatenedTeammateKey (Object), BestEnemyTargetKey (Object) | Object:AActor x4, Vector x2 |

### 1.4 Complete BT Decorator Catalog (2 Decorators)

| # | Class | Condition Type | Configuration |
|---|-------|---------------|---------------|
| 1 | BTDecorator_HasChannel | SpellCollectionComponent channel check | ChannelName (FName), bInvertResult (Bool) |
| 2 | BTDecorator_IsSeeker | GameMode role query | None (auto-queries role) |

### 1.5 Controller Blackboard Key Properties

Current FName properties in `AIC_QuidditchController.h`:

| Property | Constructor Default | Category |
|----------|-------------------|----------|
| TargetLocationKeyName | `TEXT("TargetLocation")` | Protected |
| TargetActorKeyName | `TEXT("TargetActor")` | Protected |
| IsFlyingKeyName | `TEXT("IsFlying")` | Protected |
| SelfActorKeyName | `TEXT("SelfActor")` | Protected |
| PerceivedCollectibleKeyName | `TEXT("PerceivedCollectible")` | Private |
| MatchStartedKeyName | `TEXT("MatchStarted")` | Private |
| ShouldSwapTeamKeyName | `TEXT("bShouldSwapTeam")` | Private (NAMING ISSUE) |
| QuidditchRoleKeyName | `TEXT("QuidditchRole")` | Private |
| HasBroomKeyName | `TEXT("HasBroom")` | Private |

**Issue Found:** `ShouldSwapTeamKeyName` initializes to `TEXT("bShouldSwapTeam")` which uses Hungarian notation prefix `b` for a BB key name. All other keys use PascalCase without prefix. See Section 5 for fix.

### 1.6 SetupBlackboard() Initial Values

Keys initialized at possess time (`AIC_QuidditchController.cpp:275-310`):

| Key Name | Type | Initial Value | Source |
|----------|------|---------------|--------|
| SelfActor | Object | InPawn | Direct |
| HomeLocation | Vector | InPawn->GetActorLocation() | FName literal |
| IsFlying | Bool | false | IsFlyingKeyName |
| MatchStarted | Bool | false | MatchStartedKeyName |
| bShouldSwapTeam | Bool | false | ShouldSwapTeamKeyName |
| HasBroom | Bool | false | HasBroomKeyName |
| TargetLocation | Vector | FVector::ZeroVector | TargetLocationKeyName |
| SnitchLocation | Vector | FVector::ZeroVector | FName literal |
| SnitchVelocity | Vector | FVector::ZeroVector | FName literal |
| StageLocation | Vector | FVector::ZeroVector | FName literal |
| StagingZoneLocation | Vector | FVector::ZeroVector | FName literal |
| ReachedStagingZone | Bool | false | FName literal |
| IsReady | Bool | false | FName literal |

**Keys left unset until runtime:** TargetActor, PerceivedCollectible, QuidditchRole

**Total currently initialized:** 13 keys + 3 deferred = 16 keys

### 1.7 Delegate-to-Blackboard Sync (Gas Station Pattern)

| GameMode Delegate | Controller Handler | BB Key Updated |
|-------------------|-------------------|----------------|
| OnMatchStarted | HandleMatchStarted() | MatchStarted = true |
| OnMatchEnded | HandleMatchEnded() | MatchStarted = false |
| OnAgentSelectedForSwap | HandleAgentSelectedForSwap() | ShouldSwapTeam = true |
| OnTeamSwapComplete | HandleTeamSwapComplete() | ShouldSwapTeam = false |
| OnQuidditchRoleAssigned | HandleQuidditchRoleAssigned() | QuidditchRole = Name |

| Component Delegate | Controller Handler | BB Key Updated |
|-------------------|-------------------|----------------|
| BroomComponent.OnFlightStateChanged | HandleFlightStateChanged() | IsFlying, HasBroom |

---

## 2. Blackboard Key Convention

### 2.1 Naming Rules

| Rule | Convention | Example |
|------|-----------|---------|
| General | PascalCase, no prefix | `SnitchLocation`, `DefensePosition` |
| Boolean | PascalCase with Is/Has/Can/Should | `IsFlying`, `HasBroom`, `CanBlock` |
| Object | Actor suffix optional | `SnitchActor`, `NearestBludger` |
| Vector | Location/Velocity/Direction/Position suffix | `SnitchLocation`, `SnitchVelocity` |
| Name | Role/Type descriptor | `QuidditchRole` |
| Float | Descriptive name | `TimeToIntercept` |
| NO Hungarian | Never prefix with b, f, n, etc. | `ShouldSwapTeam` NOT `bShouldSwapTeam` |

### 2.2 Complete Blackboard Key Inventory (39 Keys)

#### Category 1: Identity (3 keys) - Set once at possess, never change

| # | Key Name | Type | Writer | Reader(s) |
|---|----------|------|--------|-----------|
| 1 | SelfActor | Object:AActor | Controller.SetupBlackboard | Any task needing self-reference |
| 2 | HomeLocation | Vector | Controller.SetupBlackboard | BTTask_ReturnToHome |
| 3 | QuidditchRole | Name | Controller.HandleQuidditchRoleAssigned | Decorators for role branching |

#### Category 2: State (6 keys) - Updated by controller delegates

| # | Key Name | Type | Writer | Reader(s) |
|---|----------|------|--------|-----------|
| 4 | IsFlying | Bool | Controller.HandleFlightStateChanged + BTService_SyncFlightState | Flight decorators, task guards |
| 5 | HasBroom | Bool | Controller.HandleFlightStateChanged + BTTask_CheckBroomChannel | Broom acquisition decorators |
| 6 | MatchStarted | Bool | Controller.HandleMatchStarted/HandleMatchEnded | BTTask_WaitForMatchStart |
| 7 | ShouldSwapTeam | Bool | Controller.HandleAgentSelectedForSwap | BTTask_SwapTeam |
| 8 | ReachedStagingZone | Bool | Controller.HandlePawnBeginOverlap | Match start flow |
| 9 | IsReady | Bool | Controller.HandlePawnBeginOverlap | Match start flow |

#### Category 3: Perception (19 keys) - Written by BT Services

**Generic Perception (3 keys):**

| # | Key Name | Type | Service Writer |
|---|----------|------|---------------|
| 10 | PerceivedCollectible | Object:AActor | BTService_FindCollectible |
| 11 | PerceivedInteractable | Object:AActor | BTService_FindInteractable |
| 12 | NearestSeeker | Object:AActor | BTService_TrackNearestSeeker |

**Snitch Perception (3 keys):**

| # | Key Name | Type | Service Writer |
|---|----------|------|---------------|
| 13 | SnitchActor | Object:AActor | BTService_FindSnitch |
| 14 | SnitchLocation | Vector | BTService_FindSnitch |
| 15 | SnitchVelocity | Vector | BTService_FindSnitch |

**Staging Perception (2 keys):**

| # | Key Name | Type | Service Writer |
|---|----------|------|---------------|
| 16 | StagingZoneActor | Object:AActor | BTService_FindStagingZone |
| 17 | StagingZoneLocation | Vector | BTService_FindStagingZone |

**Quaffle Perception (5 keys):**

| # | Key Name | Type | Service Writer |
|---|----------|------|---------------|
| 18 | QuaffleActor | Object:AActor | BTService_FindQuaffle |
| 19 | QuaffleLocation | Vector | BTService_FindQuaffle |
| 20 | QuaffleVelocity | Vector | BTService_FindQuaffle |
| 21 | IsQuaffleFree | Bool | BTService_FindQuaffle |
| 22 | TeammateHasQuaffle | Bool | BTService_FindQuaffle |

**Bludger Perception (6 keys):**

| # | Key Name | Type | Service Writer |
|---|----------|------|---------------|
| 23 | NearestBludger | Object:AActor | BTService_FindBludger |
| 24 | BludgerLocation | Vector | BTService_FindBludger |
| 25 | BludgerVelocity | Vector | BTService_FindBludger |
| 26 | ThreateningBludger | Object:AActor | BTService_FindBludger |
| 27 | ThreatenedTeammate | Object:AActor | BTService_FindBludger |
| 28 | BestEnemyTarget | Object:AActor | BTService_FindBludger |

#### Category 4: Navigation (6 keys) - Written by tasks during execution

| # | Key Name | Type | Task Writer |
|---|----------|------|------------|
| 29 | TargetLocation | Vector | Controller.SetFlightTarget, BTTask_PredictIntercept |
| 30 | TargetActor | Object:AActor | Controller.SetFlightTargetActor |
| 31 | InterceptPoint | Vector | BTTask_PredictIntercept |
| 32 | DefensePosition | Vector | BTTask_PositionInGoal |
| 33 | InterceptPosition | Vector | BTTask_BlockShot |
| 34 | FlockDirection | Vector | BTTask_FlockWithTeam |

#### Category 5: Role-Specific (5 keys) - Only relevant to one role

| # | Key Name | Type | Role | Task Writer |
|---|----------|------|------|------------|
| 35 | GoalCenter | Vector | Keeper | Initialized in SetupBlackboard (NEW) |
| 36 | HeldQuaffle | Object:AActor | Chaser | BTTask_ThrowQuaffle |
| 37 | CanBlock | Bool | Keeper | BTTask_BlockShot |
| 38 | IsEvading | Bool | Seeker | BTTask_EvadeSeeker |
| 39 | TimeToIntercept | Float | Seeker | BTTask_PredictIntercept |

### 2.3 Keys Requiring Addition to BB_QuidditchAI

The existing BB asset has ~16 keys. The following **23 keys need to be added** in Session 2:

```
NEW Object Keys (9):
  SnitchActor, StagingZoneActor, PerceivedInteractable,
  QuaffleActor, NearestBludger, ThreateningBludger,
  ThreatenedTeammate, BestEnemyTarget, HeldQuaffle

NEW Vector Keys (8):
  QuaffleLocation, QuaffleVelocity, BludgerLocation, BludgerVelocity,
  InterceptPoint, DefensePosition, InterceptPosition, FlockDirection

NEW Bool Keys (4):
  IsQuaffleFree, TeammateHasQuaffle, CanBlock, IsEvading

NEW Float Keys (1):
  TimeToIntercept

NEW Name Keys (0):
  (QuidditchRole already exists)

RENAME (1):
  bShouldSwapTeam -> ShouldSwapTeam
```

### 2.4 Key Type Distribution

| Type | Count | Percentage |
|------|-------|------------|
| Object:AActor | 14 | 36% |
| Vector | 14 | 36% |
| Bool | 8 | 21% |
| Name | 1 | 3% |
| Float | 1 | 3% |
| Int | 0 | 0% |
| **Total** | **39** | **100%** |

---

## 3. Canonical Behavior Tree Diagrams

### 3.1 BT_QuidditchAI_Master - Root Tree

```
ROOT: BT_QuidditchAI_Master
│
├── [Priority 1] SELECTOR: Team Swap Interrupt
│   └── DECORATOR: BB.ShouldSwapTeam == true
│       └── SEQUENCE: Execute Swap
│           ├── BTTask_SwapTeam {ShouldSwapTeamKey -> "ShouldSwapTeam"}
│           └── BTTask_ReturnToHome {HomeLocationKey -> "HomeLocation"}
│
├── [Priority 2] SELECTOR: Acquire Broom
│   └── DECORATOR: BB.HasBroom == false (BTDecorator_HasChannel "Broom" INVERTED)
│       └── SEQUENCE: Find and Collect Broom
│           ├── SERVICE: BTService_FindCollectible {OutputKey -> "PerceivedCollectible"}
│           ├── BTTask_MoveTo {TargetKey -> "PerceivedCollectible"}
│           ├── BTTask_Interact {TargetKey -> "PerceivedCollectible", SuccessStateKey -> "HasBroom"}
│           └── BTTask_CheckBroomChannel {HasBroomKey -> "HasBroom"}
│
├── [Priority 3] SELECTOR: Mount and Fly to Start
│   └── DECORATOR: BB.HasBroom == true AND BB.IsFlying == false
│       └── SEQUENCE: Mount and Navigate
│           ├── BTTask_MountBroom {bMountBroom = true}
│           ├── SERVICE: BTService_SyncFlightState {IsFlyingKey -> "IsFlying"}
│           ├── SERVICE: BTService_FindStagingZone {
│           │     StagingZoneActorKey -> "StagingZoneActor",
│           │     StagingZoneLocationKey -> "StagingZoneLocation"}
│           └── BTTask_FlyToStagingZone {
│                 StagingZoneLocationKey -> "StagingZoneLocation",
│                 TargetLocationKey -> "TargetLocation"}
│
├── [Priority 4] SELECTOR: Wait for Match
│   └── DECORATOR: BB.ReachedStagingZone == true AND BB.MatchStarted == false
│       └── BTTask_WaitForMatchStart {MatchStartedKey -> "MatchStarted"}
│
├── [Priority 5] SELECTOR: Match Play (Role Branching)
│   └── DECORATOR: BB.MatchStarted == true AND BB.IsFlying == true
│       └── SELECTOR: Role Sub-Trees
│           │
│           ├── DECORATOR: BB.QuidditchRole == "Seeker" (Blackboard compare)
│           │   └── SUB-TREE: BT_Seeker (see 3.2)
│           │
│           ├── DECORATOR: BB.QuidditchRole == "Chaser" (Blackboard compare)
│           │   └── SUB-TREE: BT_Chaser (see 3.3)
│           │
│           ├── DECORATOR: BB.QuidditchRole == "Beater" (Blackboard compare)
│           │   └── SUB-TREE: BT_Beater (see 3.4)
│           │
│           └── DECORATOR: BB.QuidditchRole == "Keeper" (Blackboard compare)
│               └── SUB-TREE: BT_Keeper (see 3.5)
│
└── [Priority 6] FALLBACK: Return Home
    └── BTTask_ReturnToHome {HomeLocationKey -> "HomeLocation"}
```

**Role Branching Strategy:** Use UE5's built-in `BTDecorator_BlackboardBase` with `KeyQuery::IsEqualTo` comparing `QuidditchRole` Name key against role string values. This requires **zero new C++ decorators** and is fully configurable in the editor. `BTDecorator_IsSeeker` still works but is redundant with this approach.

### 3.2 BT_Seeker - Seeker Sub-Tree

```
ROOT: BT_Seeker
│
├── SERVICE: BTService_FindSnitch {
│     SnitchActorKey -> "SnitchActor",
│     SnitchLocationKey -> "SnitchLocation",
│     SnitchVelocityKey -> "SnitchVelocity"}
│
├── SERVICE: BTService_TrackNearestSeeker {NearestSeekerKey -> "NearestSeeker"}
│
└── SELECTOR: Seeker Priorities
    │
    ├── [Priority 1] SEQUENCE: Evade Threat
    │   └── DECORATOR: BB.NearestSeeker IS SET (enemy close)
    │       ├── BTTask_EvadeSeeker {
    │       │     ThreatActorKey -> "NearestSeeker",
    │       │     IsEvadingKey -> "IsEvading"}
    │       └── NOTE: Returns to chase after safe distance reached
    │
    ├── [Priority 2] SEQUENCE: Catch Snitch (within range)
    │   └── DECORATOR: BB.SnitchActor IS SET
    │       └── DECORATOR: Distance(SelfActor, SnitchActor) < CatchRadius
    │           └── BTTask_CatchSnitch {SnitchActorKey -> "SnitchActor"}
    │
    ├── [Priority 3] SEQUENCE: Intercept Snitch (predicted)
    │   └── DECORATOR: BB.SnitchActor IS SET
    │       └── SEQUENCE:
    │           ├── BTTask_PredictIntercept {
    │           │     TargetActorKey -> "SnitchActor",
    │           │     TargetVelocityKey -> "SnitchVelocity",
    │           │     InterceptPointKey -> "InterceptPoint",
    │           │     TimeToInterceptKey -> "TimeToIntercept"}
    │           └── BTTask_ControlFlight {TargetKey -> "InterceptPoint"}
    │
    └── [Priority 4] SEQUENCE: Chase Snitch (direct pursuit)
        └── DECORATOR: BB.SnitchLocation IS SET
            └── BTTask_ChaseSnitch {SnitchLocationKey -> "SnitchLocation"}
```

**Algorithm Source:** PathSearch.cpp intercept prediction for Priority 3

### 3.3 BT_Chaser - Chaser Sub-Tree

```
ROOT: BT_Chaser
│
├── SERVICE: BTService_FindQuaffle {
│     QuaffleActorKey -> "QuaffleActor",
│     QuaffleLocationKey -> "QuaffleLocation",
│     QuaffleVelocityKey -> "QuaffleVelocity",
│     IsQuaffleFreeKey -> "IsQuaffleFree",
│     TeammateHasQuaffleKey -> "TeammateHasQuaffle"}
│
└── SELECTOR: Chaser Priorities
    │
    ├── [Priority 1] SEQUENCE: Score Goal (holding Quaffle)
    │   └── DECORATOR: BB.HeldQuaffle IS SET
    │       ├── BTTask_ControlFlight {TargetKey -> "GoalCenter"}
    │       └── BTTask_ThrowQuaffle {
    │             HeldQuaffleKey -> "HeldQuaffle",
    │             ThrowTargetKey -> "TargetActor",
    │             ThrowType = ToGoal}
    │
    ├── [Priority 2] SEQUENCE: Intercept Free Quaffle
    │   └── DECORATOR: BB.IsQuaffleFree == true
    │       ├── BTTask_PredictIntercept {
    │       │     TargetActorKey -> "QuaffleActor",
    │       │     TargetVelocityKey -> "QuaffleVelocity",
    │       │     InterceptPointKey -> "InterceptPoint"}
    │       └── BTTask_ControlFlight {TargetKey -> "InterceptPoint"}
    │
    ├── [Priority 3] SEQUENCE: Support Teammate (teammate has Quaffle)
    │   └── DECORATOR: BB.TeammateHasQuaffle == true
    │       ├── BTTask_FlockWithTeam {
    │       │     FlockDirectionKey -> "FlockDirection",
    │       │     FlockTargetKey -> "TargetLocation"}
    │       └── BTTask_ControlFlight {TargetKey -> "TargetLocation"}
    │
    └── [Priority 4] SEQUENCE: Defend (enemy has Quaffle)
        └── BTTask_ControlFlight {TargetKey -> "DefensePosition"}
```

**Algorithm Source:** Flock.cs full flocking for Priority 3

### 3.4 BT_Beater - Beater Sub-Tree

```
ROOT: BT_Beater
│
├── SERVICE: BTService_FindBludger {
│     NearestBludgerKey -> "NearestBludger",
│     BludgerLocationKey -> "BludgerLocation",
│     BludgerVelocityKey -> "BludgerVelocity",
│     ThreateningBludgerKey -> "ThreateningBludger",
│     ThreatenedTeammateKey -> "ThreatenedTeammate",
│     BestEnemyTargetKey -> "BestEnemyTarget"}
│
└── SELECTOR: Beater Priorities
    │
    ├── [Priority 1] SEQUENCE: Defend Teammate (Bludger threatening ally)
    │   └── DECORATOR: BB.ThreateningBludger IS SET
    │       ├── BTTask_ControlFlight {TargetKey -> "BludgerLocation"}
    │       └── BTTask_HitBludger {
    │             BludgerKey -> "ThreateningBludger",
    │             TargetEnemyKey -> "BestEnemyTarget"}
    │
    ├── [Priority 2] SEQUENCE: Attack Enemy (hit Bludger at target)
    │   └── DECORATOR: BB.NearestBludger IS SET AND BB.BestEnemyTarget IS SET
    │       ├── BTTask_ControlFlight {TargetKey -> "BludgerLocation"}
    │       └── BTTask_HitBludger {
    │             BludgerKey -> "NearestBludger",
    │             TargetEnemyKey -> "BestEnemyTarget"}
    │
    └── [Priority 3] SEQUENCE: Patrol
        └── BTTask_ControlFlight {TargetKey -> "HomeLocation"}
```

**Algorithm Source:** MsPacManAgent.java INVERTED safety scoring for threat prioritization

### 3.5 BT_Keeper - Keeper Sub-Tree

```
ROOT: BT_Keeper
│
├── SERVICE: BTService_FindQuaffle {
│     QuaffleActorKey -> "QuaffleActor",
│     QuaffleLocationKey -> "QuaffleLocation",
│     QuaffleVelocityKey -> "QuaffleVelocity",
│     IsQuaffleFreeKey -> "IsQuaffleFree"}
│
└── SELECTOR: Keeper Priorities
    │
    ├── [Priority 1] SEQUENCE: Block Incoming Shot
    │   └── DECORATOR: BB.QuaffleActor IS SET
    │       ├── BTTask_BlockShot {
    │       │     QuaffleKey -> "QuaffleActor",
    │       │     QuaffleVelocityKey -> "QuaffleVelocity",
    │       │     GoalCenterKey -> "GoalCenter",
    │       │     InterceptPositionKey -> "InterceptPosition",
    │       │     CanBlockKey -> "CanBlock"}
    │       └── DECORATOR: BB.CanBlock == true
    │           └── BTTask_ControlFlight {TargetKey -> "InterceptPosition"}
    │
    ├── [Priority 2] SEQUENCE: Reposition (threat nearby)
    │   └── DECORATOR: BB.QuaffleActor IS SET
    │       ├── BTTask_PositionInGoal {
    │       │     GoalCenterKey -> "GoalCenter",
    │       │     ThreatActorKey -> "QuaffleActor",
    │       │     DefensePositionKey -> "DefensePosition"}
    │       └── BTTask_ControlFlight {TargetKey -> "DefensePosition"}
    │
    └── [Priority 3] SEQUENCE: Hold Position
        └── BTTask_ControlFlight {TargetKey -> "GoalCenter"}
```

**Algorithm Source:** Flock.cs Cohesion adapted for goal-relative positioning

### 3.6 BT_FlightAgent - Generic Reusable Tree

This tree extracts the common flight acquisition flow for non-Quidditch reuse:

```
ROOT: BT_FlightAgent
│
├── [Priority 1] SELECTOR: Acquire Broom
│   └── DECORATOR: BTDecorator_HasChannel "Broom" INVERTED
│       └── SEQUENCE: Find and Collect
│           ├── SERVICE: BTService_FindCollectible {OutputKey -> "PerceivedCollectible"}
│           ├── BTTask_MoveTo {TargetKey -> "PerceivedCollectible"}
│           └── BTTask_Interact {TargetKey -> "PerceivedCollectible"}
│
├── [Priority 2] SELECTOR: Mount and Fly
│   └── DECORATOR: BTDecorator_HasChannel "Broom" AND BB.IsFlying == false
│       └── SEQUENCE:
│           ├── BTTask_MountBroom {bMountBroom = true}
│           └── SERVICE: BTService_SyncFlightState {IsFlyingKey -> "IsFlying"}
│
├── [Priority 3] SELECTOR: Fly to Target
│   └── DECORATOR: BB.IsFlying == true AND BB.TargetLocation IS SET
│       └── BTTask_ControlFlight {TargetKey -> "TargetLocation"}
│
└── [Priority 4] FALLBACK: Return Home
    └── BTTask_ReturnToHome {HomeLocationKey -> "HomeLocation"}
```

**Reuse Scenarios:**
- Dog companion flight mode
- Wizard duel arena flight
- Free roam exploration
- Any future flying AI agent

---

## 4. Node Reusability Catalog

### 4.1 Generic Nodes (14) - Reusable across any project

| Node | Type | Why Generic |
|------|------|-------------|
| BTTask_MountBroom | Task | Uses BroomComponent API, no game-specific logic |
| BTTask_ControlFlight | Task | Input-agnostic flight control with Core API |
| BTTask_ReturnToHome | Task | Generic fallback navigation |
| BTTask_Interact | Task | Works with any IInteractable actor |
| BTTask_CheckBroomChannel | Task | SpellCollection channel check, configurable |
| BTTask_EvadeSeeker | Task | Generic flee behavior, rename to BTTask_Evade |
| BTTask_PredictIntercept | Task | Pure math, works with any moving target |
| BTTask_FlockWithTeam | Task | Classic Boids, tag-based team identification |
| BTService_FindCollectible | Service | Perception-based, class-configurable |
| BTService_FindInteractable | Service | Interface-based, any IInteractable |
| BTService_SyncFlightState | Service | Component state sync pattern |
| BTService_TrackNearestSeeker | Service | Tag-based threat tracking, rename to TrackNearestThreat |
| BTDecorator_HasChannel | Decorator | Channel-based gating, any channel name |
| BTTask_ForceMount | Task | Debug utility, project-agnostic |

### 4.2 Quidditch-Specific Nodes (14) - Correctly scoped to Quidditch

| Node | Type | Role | Why Specific |
|------|------|------|-------------|
| BTTask_FlyToStagingZone | Task | All | References staging zone system |
| BTTask_WaitForMatchStart | Task | All | References match state |
| BTTask_SwapTeam | Task | All | References team swap system |
| BTTask_ChaseSnitch | Task | Seeker | Snitch-specific chase behavior |
| BTTask_CatchSnitch | Task | Seeker | Snitch catch + 150 points |
| BTTask_ThrowQuaffle | Task | Chaser | Quaffle-specific throwing |
| BTTask_HitBludger | Task | Beater | Bludger-specific hit mechanics |
| BTTask_PositionInGoal | Task | Keeper | Goal-relative positioning |
| BTTask_BlockShot | Task | Keeper | Shot intercept calculation |
| BTService_FindStagingZone | Service | All | Staging zone perception |
| BTService_FindSnitch | Service | Seeker | Snitch-specific perception |
| BTService_FindQuaffle | Service | Chaser | Quaffle perception + possession |
| BTService_FindBludger | Service | Beater | Bludger + threat scoring |
| BTDecorator_IsSeeker | Decorator | Seeker | Role-specific gate (redundant with BB compare) |

### 4.3 Legacy Nodes (3) - Deprecated, do not use

| Node | Type | Replacement |
|------|------|-------------|
| BTTask_CodeEnemyAttack | Task | Role-specific attack tasks |
| BTTask_CodeEnemyReload | Task | Modern BTTask with FBlackboardKeySelector |
| BTTask_EnemyFlee | Task | BTTask_EvadeSeeker (fully implemented) |

### 4.4 FBlackboardKeySelector Compliance Totals

| Metric | Modern (26) | Legacy (3) |
|--------|-------------|------------|
| Constructor AddFilter | 100% | 0% |
| InitializeFromAsset override | 100% | 0% |
| FBlackboardKeySelector usage | 100% | 0% (uses FName) |
| GetStaticDescription | 100% | 33% |
| Total FBlackboardKeySelector props | 57 | 0 |

---

## 5. Recommended Minor Refactoring

All items deferred to Session 2. No code changes in this session.

### 5.1 Rename bShouldSwapTeam Key (Naming Convention)

**File:** `Source/END2507/Private/Code/AI/AIC_QuidditchController.cpp` line 47
**Current:** `ShouldSwapTeamKeyName(TEXT("bShouldSwapTeam"))`
**Proposed:** `ShouldSwapTeamKeyName(TEXT("ShouldSwapTeam"))`
**Reason:** All other BB keys use PascalCase without Hungarian `b` prefix. BB keys are not C++ booleans - the `b` prefix is misleading.
**Impact:** Must also rename in BB_QuidditchAI asset. Low risk - only used by SwapTeam task and controller handlers.

### 5.2 Fix EditAnywhere on Controller Properties (Standards)

**File:** `Source/END2507/Public/Code/AI/AIC_QuidditchController.h` lines 171, 175
**Current:**
```cpp
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quidditch|Configuration")
EQuidditchTeam AgentQuidditchTeam;

UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quidditch|Configuration")
EQuidditchRole AgentPreferredRole;
```
**Proposed:** Change `EditAnywhere` to `EditDefaultsOnly`
**Reason:** Per CLAUDE.md standards, controllers should use EditDefaultsOnly. Per-instance config should be on the agent pawn (via data asset), not the controller.
**Impact:** Requires full rebuild (UPROPERTY change). Per-instance overrides move to BP_QuidditchAgent data asset.

### 5.3 Add GoalCenter Initialization in SetupBlackboard

**File:** `Source/END2507/Private/Code/AI/AIC_QuidditchController.cpp` in SetupBlackboard()
**Current:** GoalCenter key not initialized
**Proposed:** Add initialization after role assignment:
```cpp
// Initialize GoalCenter for Keeper/Chaser roles
// Actual value set when GameMode assigns staging zone
BBComp->SetValueAsVector(FName("GoalCenter"), FVector::ZeroVector);
```
**Reason:** BTTask_PositionInGoal and BTTask_BlockShot both read GoalCenter. Without initialization, key shows (invalid) in debugger.
**Impact:** Minimal - just adds initial zero-vector value.

---

## 6. Session 2 Implementation Order

### Phase 0: Pre-Flight Checklist (15 min)

```
[ ] Verify project compiles: Build > Build Solution (Ctrl+Shift+B)
[ ] Verify BP class defaults:
    [ ] BP_QuidditchGameMode: DefaultPawnClass assigned?
    [ ] BP_QuidditchAIController: BehaviorTreeAsset = BT_QuidditchAI?
    [ ] BP_QuidditchAgent: AIControllerClass = BP_QuidditchAIController?
[ ] Open BB_QuidditchAI: Note current key count (expect ~16)
[ ] Open BT_QuidditchAI: Note current node layout
```

### Phase 1: Blackboard Keys (30 min) - MUST BE FIRST

**Why first:** Every BT node references BB keys. Adding keys first prevents editor warnings.

```
BB_QuidditchAI Editor Actions:
[ ] 1. Rename "bShouldSwapTeam" -> "ShouldSwapTeam" (if C++ fix applied)
[ ] 2. Add Object keys (9):
       SnitchActor, StagingZoneActor, PerceivedInteractable,
       QuaffleActor, NearestBludger, ThreateningBludger,
       ThreatenedTeammate, BestEnemyTarget, HeldQuaffle
       (Base Class: AActor for all)
[ ] 3. Add Vector keys (8):
       QuaffleLocation, QuaffleVelocity, BludgerLocation,
       BludgerVelocity, InterceptPoint, DefensePosition,
       InterceptPosition, FlockDirection
[ ] 4. Add Bool keys (4):
       IsQuaffleFree, TeammateHasQuaffle, CanBlock, IsEvading
[ ] 5. Add Float keys (1):
       TimeToIntercept
[ ] 6. Verify total key count = 39
[ ] 7. Save BB_QuidditchAI
```

### Phase 2: Role Sub-Trees Bottom-Up (60 min)

Build sub-trees before the master tree that references them.

```
[ ] 2a. Create BT_Seeker (see Section 3.2)
    [ ] Add BTService_FindSnitch to root
    [ ] Add BTService_TrackNearestSeeker to root
    [ ] Build 4-priority Selector: Evade > Catch > Intercept > Chase
    [ ] Configure all BB key bindings per diagram
    [ ] Verify each task shows green checkmarks (keys resolved)

[ ] 2b. Create BT_Chaser (see Section 3.3)
    [ ] Add BTService_FindQuaffle to root
    [ ] Build 4-priority Selector: Score > Intercept > Support > Defend
    [ ] Configure all BB key bindings per diagram

[ ] 2c. Create BT_Beater (see Section 3.4)
    [ ] Add BTService_FindBludger to root
    [ ] Build 3-priority Selector: Defend > Attack > Patrol
    [ ] Configure all BB key bindings per diagram

[ ] 2d. Create BT_Keeper (see Section 3.5)
    [ ] Add BTService_FindQuaffle to root
    [ ] Build 3-priority Selector: Block > Reposition > Hold
    [ ] Configure all BB key bindings per diagram
```

### Phase 3: Master Tree (45 min)

```
[ ] 3a. Open BT_QuidditchAI (existing asset)
[ ] 3b. Clear existing nodes (backup first!)
[ ] 3c. Build 6-priority root Selector per Section 3.1:
    [ ] Priority 1: Team Swap Interrupt
    [ ] Priority 2: Acquire Broom
    [ ] Priority 3: Mount and Fly to Start
    [ ] Priority 4: Wait for Match
    [ ] Priority 5: Match Play with role branching
    [ ] Priority 6: Fallback Return Home
[ ] 3d. For Priority 5, add Sub-Tree references:
    [ ] Decorator: BB.QuidditchRole == "Seeker" -> Run BT_Seeker
    [ ] Decorator: BB.QuidditchRole == "Chaser" -> Run BT_Chaser
    [ ] Decorator: BB.QuidditchRole == "Beater" -> Run BT_Beater
    [ ] Decorator: BB.QuidditchRole == "Keeper" -> Run BT_Keeper
[ ] 3e. Add Services to root:
    [ ] BTService_SyncFlightState on root node
[ ] 3f. Save BT_QuidditchAI
```

### Phase 4: Generic Flight Tree (15 min, optional)

```
[ ] 4a. Create BT_FlightAgent (see Section 3.6)
[ ] 4b. Build 4-priority structure: Acquire > Mount > Fly > Return
[ ] 4c. Save - reusable for non-Quidditch flying AI
```

### Phase 5: Controller and Agent Config (30 min)

```
[ ] 5a. Apply C++ fixes (requires rebuild):
    [ ] Fix bShouldSwapTeam -> ShouldSwapTeam in constructor
    [ ] Fix EditAnywhere -> EditDefaultsOnly on lines 171, 175
    [ ] Add GoalCenter initialization in SetupBlackboard()
    [ ] Build Solution (full rebuild required for UPROPERTY changes)

[ ] 5b. Configure BP_QuidditchAIController:
    [ ] BehaviorTreeAsset = BT_QuidditchAI
    [ ] BlackboardAsset = BB_QuidditchAI (or leave null, BT provides it)
    [ ] Verify perception settings: Sight=2000, Lose=2500, Angle=90

[ ] 5c. Create Data Asset instances:
    [ ] DA_SeekerAggressive: Role=Seeker, Speed=1200, Aggression=0.9
    [ ] DA_SeekerDefensive: Role=Seeker, Speed=900, Aggression=0.3
    [ ] Assign to BP_QuidditchAgent instances
```

### Phase 6: Level Setup (30 min)

```
[ ] 6a. QuidditchLevel verification:
    [ ] BP_QuidditchGameMode set in World Settings
    [ ] At least 2 staging zones placed (TeamA Seeker, TeamB Seeker)
    [ ] Staging zones have correct TeamHint/RoleHint values
    [ ] BP_BroomCollectible placed (at least 2)
    [ ] BP_GoldenSnitch placed with BP_AIC_SnitchController

[ ] 6b. Agent placement:
    [ ] Place 2x BP_QuidditchAgent (one per team)
    [ ] Set AgentDataAsset on each
    [ ] Set PlacedQuidditchTeam on each
    [ ] Verify AIControllerClass = BP_QuidditchAIController
```

### Phase 7: Integration Testing (45 min)

```
[ ] 7a. Single Agent Test:
    [ ] Place 1 agent + 1 broom + 1 staging zone
    [ ] PIE: Agent perceives broom? Moves to it? Picks up? Mounts?
    [ ] PIE: Agent flies to staging zone? Lands?
    [ ] Check BB debugger: All keys showing values (not invalid)?

[ ] 7b. Two Seeker Demo:
    [ ] Place 2 agents (TeamA + TeamB) + 2 brooms + Snitch + 2 staging zones
    [ ] PIE: Both agents acquire brooms independently?
    [ ] PIE: Both fly to correct staging zones (Bee and Flower)?
    [ ] PIE: Match starts after both ready?
    [ ] PIE: Both chase Snitch?
    [ ] PIE: Snitch evades pursuers?

[ ] 7c. Known Issues to Watch:
    [ ] Team ID 255 (faction not assigned before possess)
    [ ] Circular flight orbits (VInterpTo instead of direct velocity)
    [ ] BB key (invalid) markers (missing initialization)
    [ ] "OutputKey is not set!" (missing InitializeFromAsset)
```

### Phase Dependency Graph

```
Phase 0 ──> Phase 1 ──> Phase 2 ──> Phase 3 ──> Phase 7
                │                        │
                └── Phase 5 ────────────┘
                         │
                         └── Phase 6 ──> Phase 7

Phase 4 is independent (optional, can run anytime after Phase 1)
```

**Critical Path:** 0 -> 1 -> 2 -> 3 -> 7
**Parallel Path:** 5 can start after 1, 6 after 5

---

## Appendix A: Algorithm Source Mapping

| BT Node | Algorithm Source | Original Context |
|---------|-----------------|-----------------|
| BTTask_PredictIntercept | PathSearch.cpp | Quadratic solver for moving target intercept |
| BTTask_FlockWithTeam | Flock.cs | Reynolds Boids: Separation + Alignment + Cohesion |
| BTTask_PositionInGoal | Flock.cs (Cohesion) | Adapted: Cohesion to fixed goal point |
| BTTask_BlockShot | PathSearch.cpp | Adapted: Intercept for incoming projectiles |
| BTService_FindBludger | MsPacManAgent.java | INVERTED safety scoring: find threats TO intercept |
| SnitchBall | Blend-based steering | Obstacle > Evasion > Boundary > Wander |

## Appendix B: File Reference

### Critical Source Files

```
Source/END2507/Public/Code/AI/AIC_QuidditchController.h           (Controller, BB keys, perception)
Source/END2507/Private/Code/AI/AIC_QuidditchController.cpp         (SetupBlackboard, delegates)
Source/END2507/Public/Code/Quidditch/QuidditchTypes.h              (Enums, delegates)
Source/END2507/Public/Code/Quidditch/QuidditchAgentData.h          (Data asset structure)
Source/END2507/Public/Code/GameModes/QuidditchGameMode.h           (Match state, registration)
Source/END2507/Public/Code/Flight/AC_BroomComponent.h              (Flight API)
Source/END2507/Public/Code/Flight/BroomTypes.h                     (Flight presets)
Source/END2507/Public/Code/Flight/AC_FlightSteeringComponent.h     (Obstacle avoidance)
Source/END2507/Public/Code/Utilities/FlyingSteeringComponent.h     (Flocking)
Source/END2507/Public/Code/AI/Quidditch/SnitchBall.h               (Snitch behavior)
```

### Existing Blueprint Assets (Verified on Disk)

```
Content/Code/AI/BT_QuidditchAI.uasset
Content/Both/DoNotConvert/BB_QuidditchAI.uasset
Content/Code/GameModes/BP_QuidditchGameMode.uasset
Content/Code/AI/BP_CodeQuidditchAIController.uasset
Content/Blueprints/AI/BP_QuidditchAIController.uasset
Content/Code/Actors/BP_QuidditchAgent.uasset
Content/Code/Actors/BP_BroomCollectible.uasset
Content/Code/Actors/BP_GoldenSnitch.uasset
Content/Code/Actors/BP_AIC_SnitchController.uasset
Content/Blueprints/Actors/BP_QuidditchStagingZone.uasset
Content/Blueprints/Actors/BP_QuidditchPlayArea.uasset
Content/Code/Maps/QuidditchLevel.umap
```

## Appendix C: Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| BB key name mismatch between C++ and asset | Medium | High (silent failure) | Phase 1 verification step |
| Sub-tree reference breaks in master tree | Low | Medium | Build sub-trees first (Phase 2 before 3) |
| Data asset instances not created | High | Medium | Phase 5 checklist item |
| DefaultPawnClass not set in BP GameMode | Medium | High (no agents spawn) | Phase 0 pre-flight check |
| Circular flight orbits on fast targets | Low | Low (visual glitch) | Direct velocity already implemented |

---

*End of Architecture Specification*
*Session 2 begins with Phase 0 Pre-Flight Checklist*
