# Quidditch AI - Visual Setup Guide
## From Empty Editor to Flying Agents

**Developer:** Marcus Daley
**Date:** February 15, 2026
**Target Completion:** February 21, 2026
**Engine:** Unreal Engine 5.4

---

## How to Use This Guide

This guide walks you through setting up the Quidditch AI system **entirely inside Unreal Editor**. No C++ coding required - all the C++ classes already exist. You are wiring them together.

**Read order:**
1. Start with the Architecture Overview (understand the pieces)
2. Follow the Setup Steps in exact order (they have dependencies)
3. Use the Diagrams as visual reference while working in editor
4. Use the Troubleshooting section when something doesn't work

**Color coding throughout this guide:**
- REUSABLE = nodes that work in any flying AI project
- QUIDDITCH = nodes specific to this game mode
- LEGACY = old nodes, do not use

---

## Visual Diagrams (FigJam Links)

Open these in separate tabs while working:

1. **Master Behavior Tree** - [Open in FigJam](https://www.figma.com/online-whiteboard/create-diagram/e7bb7a64-2d79-4f12-a3c8-03dfccde4f57?utm_source=claude&utm_content=edit_in_figjam)
2. **Role Sub-Trees (Seeker/Chaser/Beater/Keeper)** - [Open in FigJam](https://www.figma.com/online-whiteboard/create-diagram/b51f05a3-119f-42e0-8e5a-5284e9aa69e5?utm_source=claude&utm_content=edit_in_figjam)
3. **Data Flow Architecture** - [Open in FigJam](https://www.figma.com/online-whiteboard/create-diagram/87218338-38f9-4cae-baff-ed907cb8ef54?utm_source=claude&utm_content=edit_in_figjam)
4. **Setup Timeline (Gantt)** - [Open in FigJam](https://www.figma.com/online-whiteboard/create-diagram/04230fcd-3051-4791-bc82-e270d1a34b05?utm_source=claude&utm_content=edit_in_figjam)

---

## Part 1: Architecture Overview

### The Big Picture (5 Minutes)

The Quidditch AI system has 5 layers. Each layer talks to the next through **delegates** (event broadcasts), never through direct function calls.

```
LAYER 1: Data Assets          What team/role is this agent?
    |
LAYER 2: Agent Pawn           The physical character in the world
    |
LAYER 3: AI Controller        The brain - reads data, manages blackboard
    |
LAYER 4: Blackboard           Shared memory for the behavior tree
    |
LAYER 5: Behavior Tree        The decision-making logic
```

### How It Works at Runtime

1. You place a `BP_QuidditchAgent` in the level
2. You assign it a Data Asset (e.g., `DA_Seeker_TeamA`)
3. When the game starts, the AI Controller reads Team/Role from the agent
4. The Controller registers with the GameMode
5. The GameMode broadcasts events (match started, role assigned, etc.)
6. The Controller writes those events to the Blackboard
7. The Behavior Tree reads the Blackboard and decides what to do
8. BT Tasks call the same flight API that player input uses

**Key insight:** The AI uses the exact same flight functions as the player. `SetVerticalInput()`, `SetBoostEnabled()`, `SetFlightEnabled()` - same API, different caller.

---

## Part 2: What Already Exists (Inventory)

Before building anything, here's what's already in the project:

### Blueprint Assets (Already Created)

| Asset | Location | Status |
|-------|----------|--------|
| BB_QuidditchAI | Content/Both/DoNotConvert/ | Has ~16 keys, needs 23 more |
| BT_QuidditchAI | Content/Code/AI/ | Exists, needs rebuild |
| BP_QuidditchGameMode | Content/Code/GameModes/ | Exists |
| BP_CodeQuidditchAIController | Content/Code/AI/ | Exists |
| BP_QuidditchAgent | Content/Code/Actors/ | Exists |
| BP_BroomCollectible | Content/Code/Actors/ | Exists |
| BP_GoldenSnitch | Content/Code/Actors/ | Exists |
| BP_QuidditchStagingZone | Content/Blueprints/Actors/ | Exists |
| QuidditchLevel | Content/Code/Maps/ | Exists |

### C++ Classes (All Compiled, Ready to Use)

**31 BT Nodes** - 18 modern tasks, 8 services, 2 decorators, 3 legacy (don't use)
**3 AI Controllers** - QuidditchController, SnitchController, Legacy CodeBase
**1 GameMode** - QuidditchGameMode with full delegate system
**3 Flight Components** - BroomComponent, FlightSteeringComponent, StaminaComponent

### What Needs to Be Built in Editor

| Item | Count | Where |
|------|-------|-------|
| Blackboard keys to add | 23 | BB_QuidditchAI |
| Role sub-trees to create | 4 | BT_Seeker, BT_Chaser, BT_Beater, BT_Keeper |
| Master tree to rebuild | 1 | BT_QuidditchAI |
| Data assets to create | 2-14 | Content/Code/DataAssets/QuidditchAgents/ |
| Staging zones to place | 2-14 | QuidditchLevel |

---

## Part 3: Setup Steps (Follow In Order)

### STEP 0: Pre-Flight Check (15 minutes)

Before touching anything, verify the project is in a good state.

**0.1 - Verify the project compiles:**
1. Open Visual Studio / VS Code
2. Build > Build Solution (Ctrl+Shift+B)
3. Wait for "Build succeeded" - if it fails, fix build errors first

**0.2 - Verify Blueprint class defaults:**
1. Open Unreal Editor
2. Open `BP_QuidditchGameMode` > Class Defaults
   - Check: Does it have scoring properties? Team color properties?
3. Open `BP_CodeQuidditchAIController` > Class Defaults
   - Check: Is `BehaviorTreeAsset` set to `BT_QuidditchAI`?
   - If not, set it now
4. Open `BP_QuidditchAgent` > Class Defaults
   - Check: Is `AI Controller Class` set to `BP_CodeQuidditchAIController`?
   - If not, set it now

**0.3 - Open BB_QuidditchAI and count keys:**
1. Navigate to Content/Both/DoNotConvert/
2. Double-click `BB_QuidditchAI`
3. Count the existing keys - you should see approximately 16
4. Leave this window open - you'll need it for Step 1

---

### STEP 1: Add All Blackboard Keys (30 minutes)

This MUST be done first. Every BT node references these keys. If the keys don't exist, the editor will show errors when you build the trees.

**Open BB_QuidditchAI** (if not already open from Step 0).

For each key below, click **New Key**, select the type, and enter the exact name.

#### 1.1 - Add Object Keys (9 new keys)

Click **New Key > Object** for each. Set **Base Class** to **Actor** for all.

| # | Key Name | Base Class | Purpose |
|---|----------|------------|---------|
| 1 | SnitchActor | Actor | The Golden Snitch reference |
| 2 | StagingZoneActor | Actor | Staging zone the agent flies to |
| 3 | PerceivedInteractable | Actor | Nearest interactable object |
| 4 | QuaffleActor | Actor | The Quaffle ball reference |
| 5 | NearestBludger | Actor | Closest Bludger ball |
| 6 | ThreateningBludger | Actor | Bludger heading toward teammate |
| 7 | ThreatenedTeammate | Actor | Teammate being threatened |
| 8 | BestEnemyTarget | Actor | Best enemy to aim Bludger at |
| 9 | HeldQuaffle | Actor | Quaffle currently held by this agent |

#### 1.2 - Add Vector Keys (8 new keys)

Click **New Key > Vector** for each.

| # | Key Name | Purpose |
|---|----------|---------|
| 1 | QuaffleLocation | Where the Quaffle is right now |
| 2 | QuaffleVelocity | Which direction the Quaffle is moving |
| 3 | BludgerLocation | Where the nearest Bludger is |
| 4 | BludgerVelocity | Which direction the Bludger is moving |
| 5 | InterceptPoint | Calculated intercept position for moving targets |
| 6 | DefensePosition | Where to position for defense |
| 7 | InterceptPosition | Where to fly to block an incoming shot |
| 8 | FlockDirection | Calculated flocking direction for Chasers |

#### 1.3 - Add Bool Keys (4 new keys)

Click **New Key > Bool** for each.

| # | Key Name | Purpose |
|---|----------|---------|
| 1 | IsQuaffleFree | Is the Quaffle unclaimed right now? |
| 2 | TeammateHasQuaffle | Does a teammate have the Quaffle? |
| 3 | CanBlock | Can the Keeper reach the shot in time? |
| 4 | IsEvading | Is the Seeker currently evading a threat? |

#### 1.4 - Add Float Keys (1 new key)

Click **New Key > Float**.

| # | Key Name | Purpose |
|---|----------|---------|
| 1 | TimeToIntercept | Seconds until intercept point is reached |

#### 1.5 - Verify Your Work

Count all keys in BB_QuidditchAI. You should now have **39 total keys**.

Organized by type:
- Object:AActor = 14 keys (36%)
- Vector = 14 keys (36%)
- Bool = 8 keys (21%)
- Name = 1 key (3%)
- Float = 1 key (3%)

**Save BB_QuidditchAI.**

> PITFALL: If you mistype a key name, the BT nodes won't find it.
> Double-check spelling matches this guide exactly. PascalCase, no prefixes.

---

### STEP 2: Build Role Sub-Trees (60 minutes)

Build these BEFORE the master tree. The master tree references them as sub-trees.

#### Understanding the Sub-Tree Pattern

Each role has its own Behavior Tree that handles role-specific decisions. The master tree selects which sub-tree to run based on the `QuidditchRole` Blackboard key.

```
Master Tree decides WHAT phase the agent is in (broom, fly, wait, play)
    |
    v
Sub-Tree decides HOW the agent plays their role (seeker chases, keeper blocks, etc.)
```

---

#### 2a. Create BT_Seeker

1. Content Browser > Content/Code/AI/
2. Right-click > Artificial Intelligence > Behavior Tree
3. Name it `BT_Seeker`
4. Double-click to open
5. Set Blackboard Asset to `BB_QuidditchAI`

**Add Services to the Root node:**
- Right-click Root > Add Service > `BTService_FindSnitch`
  - SnitchActorKey = `SnitchActor`
  - SnitchLocationKey = `SnitchLocation`
  - SnitchVelocityKey = `SnitchVelocity`
- Right-click Root > Add Service > `BTService_TrackNearestSeeker`
  - NearestSeekerKey = `NearestSeeker`

**Build the Selector with 4 priorities:**

From Root, drag to create a **Selector** node.

**Priority 1 - Evade Threat:**
- From Selector, drag to create a **Sequence**
- Add Decorator > Blackboard Based
  - Key: `NearestSeeker`, Query: `Is Set`
- Add Task > `BTTask_EvadeSeeker`
  - ThreatActorKey = `NearestSeeker`
  - IsEvadingKey = `IsEvading`
  - SafeDistance = 1500
  - EvasionSpeedMultiplier = 1.5

**Priority 2 - Catch Snitch (within range):**
- From Selector, drag to create a **Sequence**
- Add Decorator > Blackboard Based
  - Key: `SnitchActor`, Query: `Is Set`
- Add Task > `BTTask_CatchSnitch`
  - SnitchActorKey = `SnitchActor`

**Priority 3 - Intercept Snitch (predicted path):**
- From Selector, drag to create a **Sequence**
- Add Decorator > Blackboard Based
  - Key: `SnitchActor`, Query: `Is Set`
- Add Task > `BTTask_PredictIntercept`
  - TargetActorKey = `SnitchActor`
  - TargetVelocityKey = `SnitchVelocity`
  - InterceptPointKey = `InterceptPoint`
  - TimeToInterceptKey = `TimeToIntercept`
- Add Task > `BTTask_ControlFlight`
  - TargetKey = `InterceptPoint`

**Priority 4 - Chase Snitch (direct pursuit fallback):**
- From Selector, drag to create a **Sequence**
- Add Decorator > Blackboard Based
  - Key: `SnitchLocation`, Query: `Is Set`
- Add Task > `BTTask_ChaseSnitch`
  - SnitchLocationKey = `SnitchLocation`

**Save BT_Seeker.**

---

#### 2b. Create BT_Chaser

1. Right-click in Content/Code/AI/ > Artificial Intelligence > Behavior Tree
2. Name it `BT_Chaser`
3. Set Blackboard Asset to `BB_QuidditchAI`

**Add Service to Root:**
- BTService_FindQuaffle
  - QuaffleActorKey = `QuaffleActor`
  - QuaffleLocationKey = `QuaffleLocation`
  - QuaffleVelocityKey = `QuaffleVelocity`
  - IsQuaffleFreeKey = `IsQuaffleFree`
  - TeammateHasQuaffleKey = `TeammateHasQuaffle`

**Build Selector with 4 priorities:**

**Priority 1 - Score Goal (holding Quaffle):**
- Sequence with Decorator: `HeldQuaffle` Is Set
- Task: `BTTask_ControlFlight` > TargetKey = `GoalCenter`
- Task: `BTTask_ThrowQuaffle`
  - HeldQuaffleKey = `HeldQuaffle`
  - ThrowTargetKey = `TargetActor`

**Priority 2 - Intercept Free Quaffle:**
- Sequence with Decorator: `IsQuaffleFree` == true
- Task: `BTTask_PredictIntercept`
  - TargetActorKey = `QuaffleActor`
  - TargetVelocityKey = `QuaffleVelocity`
  - InterceptPointKey = `InterceptPoint`
- Task: `BTTask_ControlFlight` > TargetKey = `InterceptPoint`

**Priority 3 - Support Teammate (teammate has Quaffle):**
- Sequence with Decorator: `TeammateHasQuaffle` == true
- Task: `BTTask_FlockWithTeam`
  - FlockDirectionKey = `FlockDirection`
  - FlockTargetKey = `TargetLocation`
- Task: `BTTask_ControlFlight` > TargetKey = `TargetLocation`

**Priority 4 - Defend (fallback):**
- Task: `BTTask_ControlFlight` > TargetKey = `DefensePosition`

**Save BT_Chaser.**

---

#### 2c. Create BT_Beater

1. Create new Behavior Tree: `BT_Beater`
2. Set Blackboard Asset to `BB_QuidditchAI`

**Add Service to Root:**
- BTService_FindBludger
  - NearestBludgerKey = `NearestBludger`
  - BludgerLocationKey = `BludgerLocation`
  - BludgerVelocityKey = `BludgerVelocity`
  - ThreateningBludgerKey = `ThreateningBludger`
  - ThreatenedTeammateKey = `ThreatenedTeammate`
  - BestEnemyTargetKey = `BestEnemyTarget`

**Build Selector with 3 priorities:**

**Priority 1 - Defend Teammate:**
- Sequence with Decorator: `ThreateningBludger` Is Set
- Task: `BTTask_ControlFlight` > TargetKey = `BludgerLocation`
- Task: `BTTask_HitBludger`
  - BludgerKey = `ThreateningBludger`
  - TargetEnemyKey = `BestEnemyTarget`

**Priority 2 - Attack Enemy:**
- Sequence with Decorator: `NearestBludger` Is Set AND `BestEnemyTarget` Is Set
- Task: `BTTask_ControlFlight` > TargetKey = `BludgerLocation`
- Task: `BTTask_HitBludger`
  - BludgerKey = `NearestBludger`
  - TargetEnemyKey = `BestEnemyTarget`

**Priority 3 - Patrol (fallback):**
- Task: `BTTask_ControlFlight` > TargetKey = `HomeLocation`

**Save BT_Beater.**

---

#### 2d. Create BT_Keeper

1. Create new Behavior Tree: `BT_Keeper`
2. Set Blackboard Asset to `BB_QuidditchAI`

**Add Service to Root:**
- BTService_FindQuaffle (same service as Chaser, different context)
  - QuaffleActorKey = `QuaffleActor`
  - QuaffleLocationKey = `QuaffleLocation`
  - QuaffleVelocityKey = `QuaffleVelocity`
  - IsQuaffleFreeKey = `IsQuaffleFree`

**Build Selector with 3 priorities:**

**Priority 1 - Block Incoming Shot:**
- Sequence with Decorator: `QuaffleActor` Is Set
- Task: `BTTask_BlockShot`
  - QuaffleKey = `QuaffleActor`
  - QuaffleVelocityKey = `QuaffleVelocity`
  - GoalCenterKey = `GoalCenter`
  - InterceptPositionKey = `InterceptPosition`
  - CanBlockKey = `CanBlock`
- Sequence with Decorator: `CanBlock` == true
  - Task: `BTTask_ControlFlight` > TargetKey = `InterceptPosition`

**Priority 2 - Reposition (Quaffle nearby but not shootable):**
- Sequence with Decorator: `QuaffleActor` Is Set
- Task: `BTTask_PositionInGoal`
  - GoalCenterKey = `GoalCenter`
  - ThreatActorKey = `QuaffleActor`
  - DefensePositionKey = `DefensePosition`
- Task: `BTTask_ControlFlight` > TargetKey = `DefensePosition`

**Priority 3 - Hold Position (nothing happening):**
- Task: `BTTask_ControlFlight` > TargetKey = `GoalCenter`

**Save BT_Keeper.**

---

### STEP 3: Build the Master Behavior Tree (45 minutes)

Now connect everything in `BT_QuidditchAI`.

1. Open `BT_QuidditchAI` (Content/Code/AI/)
2. **BACKUP FIRST:** Right-click > Duplicate. Name the copy `BT_QuidditchAI_BACKUP`
3. Clear the existing nodes in BT_QuidditchAI
4. Set Blackboard Asset to `BB_QuidditchAI`

**Add Service to Root:**
- BTService_SyncFlightState
  - IsFlyingKey = `IsFlying`

**Build the 6-priority Selector:**

From Root, create a **Selector** node. Then add 6 child branches left to right:

**Priority 1 - Team Swap Interrupt (leftmost = highest priority):**
- Sequence with Decorator: `ShouldSwapTeam` == true
- Task: `BTTask_SwapTeam` > ShouldSwapTeamKey = `ShouldSwapTeam`
- Task: `BTTask_ReturnToHome` > HomeLocationKey = `HomeLocation`

**Priority 2 - Acquire Broom:**
- Sequence with Decorator: `BTDecorator_HasChannel`
  - ChannelName = `Broom`
  - bInvertResult = **true** (runs when agent does NOT have broom)
- Add Service: `BTService_FindCollectible`
  - OutputKey = `PerceivedCollectible`
- Task: `BTTask_MoveTo` (built-in UE5) > BlackboardKey = `PerceivedCollectible`
- Task: `BTTask_Interact`
  - TargetKey = `PerceivedCollectible`
  - SuccessStateKey = `HasBroom`
- Task: `BTTask_CheckBroomChannel` > HasBroomKey = `HasBroom`

**Priority 3 - Mount and Fly to Start:**
- Sequence with TWO Decorators:
  - Decorator 1: Blackboard Based > `HasBroom` == true
  - Decorator 2: Blackboard Based > `IsFlying` == false
- Task: `BTTask_MountBroom` > bMountBroom = true
- Add Service: `BTService_FindStagingZone`
  - StagingZoneActorKey = `StagingZoneActor`
  - StagingZoneLocationKey = `StagingZoneLocation`
- Task: `BTTask_FlyToStagingZone`
  - StagingZoneLocationKey = `StagingZoneLocation`
  - TargetLocationKey = `TargetLocation`

**Priority 4 - Wait for Match:**
- Sequence with TWO Decorators:
  - Decorator 1: Blackboard Based > `ReachedStagingZone` == true
  - Decorator 2: Blackboard Based > `MatchStarted` == false
- Task: `BTTask_WaitForMatchStart` > MatchStartedKey = `MatchStarted`

**Priority 5 - Match Play (Role Branching):**
- Sequence with TWO Decorators:
  - Decorator 1: Blackboard Based > `MatchStarted` == true
  - Decorator 2: Blackboard Based > `IsFlying` == true
- Create a **Selector** child node for role branching:
  - Branch 1: Decorator > Blackboard Based > Key: `QuidditchRole`, Query: `Is Equal To`, Value: `Seeker`
    - Task: **Run Sub-Tree** > `BT_Seeker`
  - Branch 2: Decorator > Blackboard Based > Key: `QuidditchRole`, Value: `Chaser`
    - Task: **Run Sub-Tree** > `BT_Chaser`
  - Branch 3: Decorator > Blackboard Based > Key: `QuidditchRole`, Value: `Beater`
    - Task: **Run Sub-Tree** > `BT_Beater`
  - Branch 4: Decorator > Blackboard Based > Key: `QuidditchRole`, Value: `Keeper`
    - Task: **Run Sub-Tree** > `BT_Keeper`

**Priority 6 - Fallback (rightmost = lowest priority):**
- Task: `BTTask_ReturnToHome` > HomeLocationKey = `HomeLocation`

**Save BT_QuidditchAI.**

> PITFALL: Priority is determined by left-to-right order in the editor.
> The leftmost child of a Selector runs first. Make sure Team Swap is
> leftmost and Fallback is rightmost.

> PITFALL: When adding "Run Sub-Tree" tasks, make sure the sub-tree's
> Blackboard Asset matches the master tree's (both use BB_QuidditchAI).

---

### STEP 4: Create Data Assets (15 minutes)

Data Assets configure each agent's team and role without touching code.

#### 4.1 - Create the folder

1. Content Browser > Content/Code/
2. Right-click > New Folder > `DataAssets`
3. Inside DataAssets > New Folder > `QuidditchAgents`

#### 4.2 - Quick Demo: Create 2 Seeker Data Assets

For the minimum demo (2 Seekers chasing a Snitch):

**Create DA_Seeker_TeamA:**
1. Right-click in QuidditchAgents/ > Miscellaneous > Data Asset
2. Search for `QuidditchAgentData` > Select
3. Name: `DA_Seeker_TeamA`
4. Open and set:
   - Display Name: `Team A Seeker`
   - Agent Role: `Seeker`
   - Team Affiliation: `TeamA`

**Create DA_Seeker_TeamB:**
1. Same process, name: `DA_Seeker_TeamB`
2. Set: Display Name: `Team B Seeker`, Role: `Seeker`, Team: `TeamB`

#### 4.3 - Full Team: All 14 Data Assets

When you're ready for the full match:

| Asset Name | Team | Role |
|------------|------|------|
| DA_Seeker_TeamA | TeamA | Seeker |
| DA_Seeker_TeamB | TeamB | Seeker |
| DA_Chaser1_TeamA | TeamA | Chaser |
| DA_Chaser2_TeamA | TeamA | Chaser |
| DA_Chaser3_TeamA | TeamA | Chaser |
| DA_Chaser1_TeamB | TeamB | Chaser |
| DA_Chaser2_TeamB | TeamB | Chaser |
| DA_Chaser3_TeamB | TeamB | Chaser |
| DA_Beater1_TeamA | TeamA | Beater |
| DA_Beater2_TeamA | TeamA | Beater |
| DA_Beater1_TeamB | TeamB | Beater |
| DA_Beater2_TeamB | TeamB | Beater |
| DA_Keeper_TeamA | TeamA | Keeper |
| DA_Keeper_TeamB | TeamB | Keeper |

---

### STEP 5: Set Up the Level (30 minutes)

#### 5.1 - Set the GameMode

1. Open `QuidditchLevel` (Content/Code/Maps/)
2. Window > World Settings
3. GameMode Override = `BP_QuidditchGameMode`

#### 5.2 - Place Agents

1. In Place Actors panel, search `BP_QuidditchAgent`
2. Drag into level at desired spawn point
3. In Details Panel > Quidditch section:
   - Set **Agent Data Asset** = `DA_Seeker_TeamA`
4. Repeat for second agent with `DA_Seeker_TeamB`

> PITFALL: If the "Agent Data Asset" dropdown is empty, you haven't
> created the Data Assets yet (Step 4), or they aren't the right type.

#### 5.3 - Place Staging Zones

1. Search for `BP_QuidditchStagingZone` in Content Browser
2. Drag two into the level (one per team for demo)
3. First zone: Staging Team = `TeamA`, Staging Role = `Seeker`, Slot = 0
4. Second zone: Staging Team = `TeamB`, Staging Role = `Seeker`, Slot = 0

**Placement guide (top-down view):**
```
                [TEAM B SIDE]

    [KB] Keeper          [SB] Seeker

         [CB0] [CB1] [CB2] Chasers

         [BB0]      [BB1] Beaters

    =========== CENTER LINE ===========

         [BA0]      [BA1] Beaters

         [CA0] [CA1] [CA2] Chasers

    [KA] Keeper          [SA] Seeker

                [TEAM A SIDE]
```

#### 5.4 - Place Broom Collectibles

1. Search for `BP_BroomCollectible`
2. Place at least 2 (one per agent) on the ground near spawn points
3. Agents will perceive, walk to, and pick up brooms before flying

#### 5.5 - Place the Golden Snitch

1. Search for `BP_GoldenSnitch`
2. Place one at center of the play area
3. Set Play Area Center and Play Area Extent to match your arena
   - Example: Center = (0, 0, 1000), Extent = (5000, 5000, 2000)

#### 5.6 - Verify AI Controller Assignment

Select each `BP_QuidditchAgent` in the level and check:
- **AI Controller Class** = `BP_CodeQuidditchAIController` (or your BP child)
- **Auto Possess AI** = `Placed in World` or `Placed in World or Spawned`

---

### STEP 6: Test (30 minutes)

#### 6.1 - Single Agent Test

1. Keep only 1 agent + 1 broom in level
2. PIE (Alt+P or Play button)
3. Watch the agent - it should:
   - Perceive the broom (within 500 units sight range)
   - Walk to the broom
   - Pick it up (interact)
   - Mount the broom (start flying)
   - Fly toward the staging zone

4. Open BT Debugger: Window > AI > Behavior Tree
5. Select the agent in World Outliner
6. Check Blackboard values panel:
   - `HasBroom` = true (after pickup)
   - `IsFlying` = true (after mount)
   - `QuidditchRole` = Seeker (after GameMode assigns)

#### 6.2 - Two Seeker Demo

1. Add second agent + second broom + Snitch
2. PIE
3. Both agents should:
   - Independently find and collect brooms
   - Fly to their respective staging zones
   - Wait for match start
   - Chase the Snitch when match begins
   - Snitch evades when pursuers get close

#### 6.3 - What to Check in Output Log

**Good signs:**
```
[AIC_QuidditchController] Read from Agent: Team=TeamA, Role=Seeker, HasDataAsset=Yes
[AIC_QuidditchController] RegisterAgentWithGameMode SUCCESS
[AIC_QuidditchController] Direct BB write: QuidditchRole = Seeker
```

**Bad signs (and what to fix):**
```
"Pawn is not BaseAgent"
  -> BP_QuidditchAgent parent class wrong, must inherit ABaseAgent

"Team is None!"
  -> No Data Asset assigned AND no manual PlacedQuidditchTeam set

"OutputKey is not set!"
  -> BB key name mismatch between node config and BB_QuidditchAI asset

"No staging zone found"
  -> Missing staging zone for that Team+Role combo in the level
```

---

## Part 4: Node Reference (Quick Lookup)

### Reusable Nodes (Work in Any Project)

| Node | Type | What It Does |
|------|------|-------------|
| BTTask_MountBroom | Task | Mounts/dismounts broom via BroomComponent |
| BTTask_ControlFlight | Task | Flies toward a target location or actor |
| BTTask_ReturnToHome | Task | Flies back to spawn point |
| BTTask_Interact | Task | Interacts with any IInteractable actor |
| BTTask_CheckBroomChannel | Task | Checks if agent has "Broom" channel |
| BTTask_EvadeSeeker | Task | Flees from a threat actor |
| BTTask_PredictIntercept | Task | Calculates intercept point for moving target |
| BTTask_FlockWithTeam | Task | Boids flocking with teammates |
| BTService_FindCollectible | Service | Finds collectibles via perception |
| BTService_FindInteractable | Service | Finds interactable actors via perception |
| BTService_SyncFlightState | Service | Syncs IsFlying state to Blackboard |
| BTService_TrackNearestSeeker | Service | Tracks nearest threat by tag |
| BTDecorator_HasChannel | Decorator | Gates branch on spell channel |

### Quidditch-Only Nodes

| Node | Type | Role | What It Does |
|------|------|------|-------------|
| BTTask_FlyToStagingZone | Task | All | Flies to team staging zone |
| BTTask_WaitForMatchStart | Task | All | Waits for MatchStarted BB key |
| BTTask_SwapTeam | Task | All | Handles team swap mid-match |
| BTTask_ChaseSnitch | Task | Seeker | Continuous Snitch pursuit |
| BTTask_CatchSnitch | Task | Seeker | Catches Snitch (150 points) |
| BTTask_ThrowQuaffle | Task | Chaser | Throws Quaffle at goal |
| BTTask_HitBludger | Task | Beater | Hits Bludger at enemy |
| BTTask_PositionInGoal | Task | Keeper | Positions near goal |
| BTTask_BlockShot | Task | Keeper | Intercepts incoming Quaffle |
| BTService_FindStagingZone | Service | All | Finds staging zone via perception |
| BTService_FindSnitch | Service | Seeker | Tracks Snitch position/velocity |
| BTService_FindQuaffle | Service | Chaser/Keeper | Tracks Quaffle + possession state |
| BTService_FindBludger | Service | Beater | Tracks Bludgers + threats |

### Legacy Nodes (DO NOT USE)

| Node | Why Not | Use Instead |
|------|---------|-------------|
| BTTask_CodeEnemyAttack | No Blackboard integration | Role-specific tasks |
| BTTask_CodeEnemyReload | Uses FName not FBlackboardKeySelector | Modern tasks |
| BTTask_EnemyFlee | Incomplete, EQS callback removed | BTTask_EvadeSeeker |

---

## Part 5: Blackboard Key Quick Reference

### All 39 Keys by Category

**Identity (3) - Set once, never change:**
SelfActor, HomeLocation, QuidditchRole

**State (6) - Updated by controller delegates:**
IsFlying, HasBroom, MatchStarted, ShouldSwapTeam, ReachedStagingZone, IsReady

**Generic Perception (3):**
PerceivedCollectible, PerceivedInteractable, NearestSeeker

**Snitch Perception (3):**
SnitchActor, SnitchLocation, SnitchVelocity

**Staging Perception (2):**
StagingZoneActor, StagingZoneLocation

**Quaffle Perception (5):**
QuaffleActor, QuaffleLocation, QuaffleVelocity, IsQuaffleFree, TeammateHasQuaffle

**Bludger Perception (6):**
NearestBludger, BludgerLocation, BludgerVelocity, ThreateningBludger, ThreatenedTeammate, BestEnemyTarget

**Navigation (6):**
TargetLocation, TargetActor, InterceptPoint, DefensePosition, InterceptPosition, FlockDirection

**Role-Specific (5):**
GoalCenter (Keeper), HeldQuaffle (Chaser), CanBlock (Keeper), IsEvading (Seeker), TimeToIntercept (Seeker)

### Naming Convention

- PascalCase always: `SnitchLocation`, not `snitchLocation`
- Booleans use Is/Has/Can/Should: `IsFlying`, `HasBroom`, `CanBlock`
- No Hungarian prefix: `ShouldSwapTeam`, NOT `bShouldSwapTeam`
- Vectors end in Location/Velocity/Direction/Position
- Objects can optionally end in Actor: `SnitchActor`, `NearestBludger`

---

## Part 6: Common Pitfalls and Fixes

### Pitfall 1: "Key (invalid)" in Blackboard Debugger
**Cause:** Key exists in BT node config but doesn't exist in BB_QuidditchAI
**Fix:** Open BB_QuidditchAI, add the missing key with exact name match

### Pitfall 2: Sub-Tree Won't Accept BB Keys
**Cause:** Sub-tree has a different Blackboard Asset assigned
**Fix:** All sub-trees (BT_Seeker, etc.) must use `BB_QuidditchAI`

### Pitfall 3: Agent Doesn't Move After Picking Up Broom
**Cause:** `IsFlying` not updating in Blackboard
**Fix:** Verify BTService_SyncFlightState is on the master tree root node

### Pitfall 4: Role Branching Never Fires
**Cause:** `QuidditchRole` key is empty (Name type)
**Fix:** Check that GameMode registered the agent and assigned a role. Look for
"RegisterAgentWithGameMode SUCCESS" in Output Log

### Pitfall 5: Behavior Tree Doesn't Run At All
**Cause:** BT asset not assigned to controller
**Fix:** Open `BP_CodeQuidditchAIController` > Class Defaults > BehaviorTreeAsset = `BT_QuidditchAI`

### Pitfall 6: Agent Orbits Target Instead of Landing
**Cause:** ArrivalTolerance too small for the agent's turn radius
**Fix:** Increase ArrivalTolerance on BTTask_ControlFlight (try 300-500)

### Pitfall 7: UPROPERTY Changes Don't Appear in Editor
**Cause:** Used Live Coding (Ctrl+Alt+F11) for UPROPERTY changes
**Fix:** Close editor, full rebuild from IDE, reopen editor

### Pitfall 8: Team ID Shows 255
**Cause:** Team not assigned before AIController possesses the pawn
**Fix:** Ensure Data Asset has TeamAffiliation set, or set PlacedQuidditchTeam on pawn

---

## Part 7: Folder Structure Reference

### Where Everything Lives in Content Browser

```
Content/
|
+-- Code/
|   +-- AI/
|   |   +-- BT_QuidditchAI          Master behavior tree
|   |   +-- BT_Seeker               Seeker sub-tree (NEW)
|   |   +-- BT_Chaser               Chaser sub-tree (NEW)
|   |   +-- BT_Beater               Beater sub-tree (NEW)
|   |   +-- BT_Keeper               Keeper sub-tree (NEW)
|   |   +-- BP_CodeQuidditchAIController
|   |   +-- BP_AIC_SnitchController
|   |
|   +-- Actors/
|   |   +-- BP_QuidditchAgent
|   |   +-- BP_BroomCollectible
|   |   +-- BP_GoldenSnitch
|   |
|   +-- DataAssets/
|   |   +-- QuidditchAgents/
|   |       +-- DA_Seeker_TeamA      (NEW)
|   |       +-- DA_Seeker_TeamB      (NEW)
|   |       +-- DA_Chaser1_TeamA     (NEW, full team)
|   |       +-- ... (14 total for full match)
|   |
|   +-- GameModes/
|   |   +-- BP_QuidditchGameMode
|   |
|   +-- Maps/
|       +-- QuidditchLevel
|
+-- Blueprints/
|   +-- Actors/
|       +-- BP_QuidditchStagingZone
|       +-- BP_QuidditchPlayArea
|
+-- Both/
    +-- DoNotConvert/
        +-- BB_QuidditchAI           Blackboard (39 keys)
```

### Where Everything Lives in Source Code

```
Source/END2507/
|
+-- Public/Code/
|   +-- AI/
|   |   +-- AIC_QuidditchController.h     Main controller
|   |   +-- AIC_SnitchController.h        Snitch controller
|   |   +-- BTTask_*.h                    All task nodes
|   |   +-- BTService_*.h                 All service nodes
|   |   +-- BTDecorator_*.h               All decorator nodes
|   |   +-- Quidditch/
|   |       +-- BTService_FindSnitch.h    Role-specific nodes
|   |       +-- BTTask_ChaseSnitch.h
|   |       +-- ... (role-specific)
|   |
|   +-- Actors/
|   |   +-- BaseAgent.h                   Agent base class
|   |
|   +-- Flight/
|   |   +-- AC_BroomComponent.h           Flight system
|   |   +-- AC_FlightSteeringComponent.h  Obstacle avoidance
|   |   +-- AC_StaminaComponent.h         Stamina management
|   |
|   +-- Quidditch/
|   |   +-- QuidditchTypes.h              Enums, delegates
|   |   +-- QuidditchAgentData.h          Data asset class
|   |   +-- SnitchBall.h                  Snitch behavior
|   |   +-- QuidditchStagingZone.h        Staging zone actor
|   |
|   +-- GameModes/
|       +-- QuidditchGameMode.h           Match management
|
+-- Private/Code/
    +-- (matching .cpp files for all headers)
```

---

## Appendix: Schedule to February 21

| Day | Date | Focus | Deliverable |
|-----|------|-------|-------------|
| Sat | Feb 15 | Step 0-1: Pre-flight + Blackboard keys | BB_QuidditchAI has 39 keys |
| Sun | Feb 16 | Step 2a-2b: Seeker + Chaser sub-trees | BT_Seeker, BT_Chaser saved |
| Mon | Feb 17 | Step 2c-2d: Beater + Keeper sub-trees | BT_Beater, BT_Keeper saved |
| Tue | Feb 18 | Step 3: Master tree + Tuesday demo | BT_QuidditchAI rebuilt, 2 Seekers demo |
| Wed | Feb 19 | Step 4-5: Data assets + level setup | Full 14-agent configuration |
| Thu | Feb 20 | Step 6: Testing + Thursday demo | All roles functional |
| Fri | Feb 21 | Polish + bug fixes | Target completion |

**Critical path:** Steps 0 > 1 > 2 > 3 must be done in order.
**Can parallelize:** Steps 4-5 can start after Step 1 is done.

---

*End of Visual Setup Guide*
*Diagrams available in FigJam for editing and presentation*
