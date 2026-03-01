# Seeker Vertical Slice - Editor Walkthrough

**Developer:** Marcus Daley
**Date:** February 18, 2026
**Estimated Time:** 30-45 minutes
**Goal:** One Seeker agent acquires broom, flies to staging zone, match starts, chases Snitch, catches it, match ends.

---

## Prerequisites

Before starting, verify:

- [x] Project builds clean (`Build.bat END2507Editor Win64 Development`)
- [ ] Unreal Editor is open with WizardJam2.0.uproject loaded
- [ ] You have a test level (or create a new one: File > New Level > Empty Open World)

Save your test level as `Content/Maps/L_QuidditchTest.umap`.

---

## Part 1: GameMode Setup (5 minutes)

### Step 1.1 - Create BP_QuidditchGameMode (if it doesn't exist)

1. Content Browser > right-click > Blueprint Class > search `QuidditchGameMode`
2. Select `AQuidditchGameMode` as parent
3. Name it `BP_QuidditchGameMode`
4. Save to `Content/Code/GameModes/`

### Step 1.2 - Configure BP_QuidditchGameMode Class Defaults

Open `BP_QuidditchGameMode` and set these in **Class Defaults**:

| Category | Property | Value | Why |
|----------|----------|-------|-----|
| Quidditch\|Debug | `RequiredAgentOverride` | **1** | Default calculates 14 agents. Set to 1 for single-Seeker test. |
| Quidditch\|Scoring | `SnitchCatchPoints` | **150** | Points awarded when Seeker catches Snitch |
| Quidditch\|Teams | `MaxSeekersPerTeam` | 1 | (should already be default) |
| Quidditch\|Sync | `MatchStartCountdown` | **3.0** | Seconds between all-ready and match start |

Leave `DefaultPawnClass` unset (agents spawn their own pawns).

### Step 1.3 - Set World Settings GameMode Override

1. Open your test level
2. Window > World Settings
3. **GameMode Override** > select `BP_QuidditchGameMode`
4. Verify it appears in the World Settings panel

---

## Part 2: AI Controller Setup (5 minutes)

### Step 2.1 - Create BP_QuidditchAIController (if it doesn't exist)

1. Content Browser > right-click > Blueprint Class > search `AIC_QuidditchController`
2. Select `AAIC_QuidditchController` as parent
3. Name it `BP_QuidditchAIController`
4. Save to `Content/Code/AI/`

### Step 2.2 - Assign the Behavior Tree

Open `BP_QuidditchAIController` Class Defaults:

| Category | Property | Value |
|----------|----------|-------|
| Quidditch\|AI | `BehaviorTreeAsset` | **BT_QuidditchAI** |

If `BT_QuidditchAI` doesn't appear in the dropdown, verify the asset exists in `Content/Code/AI/`.

### Step 2.3 - Verify Perception (should be defaults)

These should already be set from the C++ constructor, but confirm:

| Category | Property | Value |
|----------|----------|-------|
| Quidditch\|Perception | `SightRadius` | 2000.0 |
| Quidditch\|Perception | `LoseSightRadius` | 2500.0 |
| Quidditch\|Perception | `PeripheralVisionAngle` | 90.0 |

---

## Part 3: Blackboard Verification (5 minutes)

### Step 3.1 - Open BB_QuidditchAI

Navigate to `Content/Code/AI/` and open `BB_QuidditchAI`.

### Step 3.2 - Verify All 20 Keys Exist

Check that every key below exists with the correct type. If any are missing, add them.

**Agent State Keys:**
| Key Name | Type | Base Class (if Object) |
|----------|------|----------------------|
| `SelfActor` | Object | AActor |
| `HomeLocation` | Vector | - |
| `IsFlying` | Bool | - |
| `HasBroom` | Bool | - |

**Perception Keys:**
| Key Name | Type | Base Class (if Object) |
|----------|------|----------------------|
| `PerceivedCollectible` | Object | AActor |
| `TargetActor` | Object | AActor |
| `TargetLocation` | Vector | - |

**Match Sync Keys:**
| Key Name | Type | Base Class (if Object) |
|----------|------|----------------------|
| `MatchStarted` | Bool | - |
| `QuidditchRole` | Name | - |
| `ShouldSwapTeam` | Bool | - |
| `ReachedStagingZone` | Bool | - |
| `IsReady` | Bool | - |
| `GoalCenter` | Vector | - |

**Snitch Chase Keys:**
| Key Name | Type | Base Class (if Object) |
|----------|------|----------------------|
| `SnitchLocation` | Vector | - |
| `SnitchVelocity` | Vector | - |
| `SnitchActor` | Object | AActor |

**Staging Zone Keys:**
| Key Name | Type | Base Class (if Object) |
|----------|------|----------------------|
| `StagingZoneLocation` | Vector | - |
| `StagingZoneActor` | Object | AActor |
| `StageLocation` | Vector | - |

**Snitch Evasion Key (used by Snitch AI):**
| Key Name | Type | Base Class (if Object) |
|----------|------|----------------------|
| `NearestSeeker` | Object | AActor |

---

## Part 4: Behavior Tree Verification (5 minutes)

### Step 4.1 - Open BT_QuidditchAI

Navigate to `Content/Code/AI/` and open `BT_QuidditchAI`.

### Step 4.2 - Verify Structure

The BT should have this top-level sequence (left to right):

```
Root
└── Selector
    ├── AcquireBroom (Sequence)
    │   ├── Decorator: HasBroom == false
    │   ├── BTService_FindCollectible (runs continuously)
    │   ├── BTTask_MoveToCollectible (or BTTask_Interact)
    │   └── BTTask_MountBroom
    │
    ├── FlyToStaging (Sequence)
    │   ├── Decorator: HasBroom == true, ReachedStagingZone == false
    │   ├── BTService_FindStagingZone (runs continuously)
    │   └── BTTask_FlyToStagingZone
    │
    ├── WaitForMatch (Sequence)
    │   ├── Decorator: ReachedStagingZone == true, MatchStarted == false
    │   └── BTTask_WaitForMatchStart
    │
    └── PlayMatch (Sequence)
        ├── Decorator: MatchStarted == true
        ├── BTDecorator_IsSeeker
        ├── BTService_FindSnitch (runs continuously)
        ├── BTTask_ChaseSnitch
        └── BTTask_CatchSnitch
```

### Step 4.3 - Verify Key Bindings on Each Node

Click each BT node and check its **Blackboard Key** dropdown shows the correct key (not "None" or red/invalid). Critical ones:

| Node | Key Property | Should Be Set To |
|------|-------------|------------------|
| BTService_FindCollectible | OutputKey | `PerceivedCollectible` |
| BTService_FindStagingZone | OutputKey | `StagingZoneActor` |
| BTService_FindSnitch | SnitchKey | `SnitchActor` |
| BTService_FindSnitch | SnitchLocationKey | `SnitchLocation` |
| BTTask_ChaseSnitch | SnitchLocationKey | `SnitchLocation` |
| BTTask_CatchSnitch | SnitchKey | `SnitchActor` |
| BTTask_FlyToStagingZone | StagingZoneKey | `StagingZoneActor` |

If any key shows **(invalid)** or won't populate the dropdown, the BT node is missing its `AddObjectFilter`/`InitializeFromAsset` implementation. This is a **silent failure** - the node will run but never write to the blackboard.

---

## Part 5: Agent Setup (5 minutes)

### Step 5.1 - Create BP_QuidditchAgent (if it doesn't exist)

1. Content Browser > right-click > Blueprint Class > search `BaseAgent`
2. Select `ABaseAgent` as parent
3. Name it `BP_QuidditchAgent`
4. Save to `Content/Code/Actors/`

### Step 5.2 - Configure BP_QuidditchAgent Class Defaults

| Category | Property | Value |
|----------|----------|-------|
| AI | `AIControllerClass` | **BP_QuidditchAIController** |
| AI | `AutoPossessAI` | **Placed in World or Spawned** |

### Step 5.3 - Verify Components on Agent

The agent needs these components (some inherited from BaseAgent/BaseCharacter):

- **AC_BroomComponent** - If not present, add it manually in the Components panel
- **AC_StaminaComponent** - Required for broom flight (stamina drain)
- **AC_HealthComponent** - Required for BaseCharacter

### Step 5.4 - Place Agent in Level

1. Drag `BP_QuidditchAgent` into the level
2. Position it on the ground near where you'll place the broom
3. In the **Details panel**, set these **per-instance** properties:

| Category | Property | Value |
|----------|----------|-------|
| Quidditch | `PlacedQuidditchTeam` | **Team A** |
| Quidditch | `PlacedPreferredRole` | **Seeker - Catches Snitch** |

These are `EditInstanceOnly` - set them on the placed instance, not class defaults.

---

## Part 6: Broom Collectible (3 minutes)

### Step 6.1 - Verify BP_BroomCollectible Exists

Check `Content/Code/Actors/` or `Content/Code/Flight/` for `BP_BroomCollectible`.

### Step 6.2 - Verify Perception Source

Open `BP_BroomCollectible` and confirm it has a `UAIPerceptionStimuliSourceComponent`:
- In the Components panel, look for **AIPerceptionStimuliSource**
- If missing, add one: Add Component > AI Perception Stimuli Source
- In its details, enable **Auto Register as Source** and add **AISense_Sight** to the registered senses

### Step 6.3 - Place in Level

1. Drag `BP_BroomCollectible` into the level
2. Place it within **2000 units** of the agent spawn (the Seeker's perception radius)
3. Place it on the ground where the agent can walk to it

---

## Part 7: Staging Zone (5 minutes)

### Step 7.1 - Create BP_QuidditchStagingZone (if it doesn't exist)

1. Content Browser > right-click > Blueprint Class > search `QuidditchStagingZone`
2. Select `AQuidditchStagingZone` as parent
3. Name it `BP_QuidditchStagingZone`
4. Save to `Content/Code/Quidditch/`

### Step 7.2 - Verify Perception Source on Class Defaults

Open `BP_QuidditchStagingZone` and confirm:
- Has `UAIPerceptionStimuliSourceComponent` (should be auto-created in C++ constructor)
- **Auto Register as Source** is enabled
- **AISense_Sight** is registered

### Step 7.3 - Place in Level and Configure

1. Drag `BP_QuidditchStagingZone` into the level
2. Place it **elevated** (Z = 500-1000 above ground) since agents fly to it
3. Set these **per-instance** properties in Details:

| Category | Property | Value | Why |
|----------|----------|-------|-----|
| Zone Identity | `ZoneIdentifier` | `TeamA_Seeker` | Human-readable label |
| Zone Identity | `TeamHint` | **1** | Maps to EQuidditchTeam::TeamA |
| Zone Identity | `RoleHint` | **1** | Maps to EQuidditchRole::Seeker |

**Enum-to-Int Mapping Reference:**

Teams: `None = 0, TeamA = 1, TeamB = 2`
Roles: `None = 0, Seeker = 1, Chaser = 2, Beater = 3, Keeper = 4`

The agent reads `TeamHint` and `RoleHint` as integers and compares against its own team/role cast to int. If they don't match, the agent silently ignores the zone.

---

## Part 8: Snitch Ball (5 minutes)

### Step 8.1 - Verify BP_GoldenSnitch Exists

Check `Content/Code/Quidditch/` or `Content/Code/Actors/` for `BP_GoldenSnitch`.

The Snitch auto-possesses its own AI controller (`AAIC_SnitchController`) - you do NOT need to create a separate controller Blueprint for it.

### Step 8.2 - Configure BP_GoldenSnitch Class Defaults (if needed)

Open `BP_GoldenSnitch` and verify/set:

| Category | Property | Suggested Value |
|----------|----------|----------------|
| Movement | `BaseSpeed` | 400.0 |
| Movement | `MaxEvadeSpeed` | 800.0 |
| Evasion | `EvadeRadius` | 800.0 |
| Debug | `bShowDebug` | **true** (for testing) |

### Step 8.3 - Place in Level

1. Drag `BP_GoldenSnitch` into the level
2. Place it **elevated** (Z = 800-1200 above ground) in the play area
3. Give it room to wander - it uses boundary avoidance, so place it near the center

### Step 8.4 - Play Area Bounds (Optional but Recommended)

If the Snitch has a `PlayAreaVolumeRef` property:
1. Place a large Box Trigger volume in the level to define the play area
2. Assign it to the Snitch's `PlayAreaVolumeRef` in Details

If not configured, the Snitch will wander using its default center/extent values.

---

## Part 9: Level Layout Summary

Your test level should now have exactly **4 actors** placed:

```
Level Layout (Top-Down View)
============================

  [BP_BroomCollectible]          [BP_QuidditchStagingZone]
       (ground level)                 (Z = 500-1000)
       near agent                     TeamHint=1, RoleHint=1

  [BP_QuidditchAgent]            [BP_GoldenSnitch]
       (ground level)                 (Z = 800-1200)
       TeamA, Seeker                  center of play area
       within 2000u of broom

World Settings:
  GameMode Override = BP_QuidditchGameMode
```

**Spatial Guidelines:**
- Agent and Broom: within **2000 units** of each other (perception radius)
- Staging Zone: within **2000 units** of where the agent will fly after mounting broom
- Snitch: anywhere in the play area, elevated
- All actors should be in reasonable proximity (within a 5000-10000 unit area)

---

## Part 10: Play-Test and Debug (10 minutes)

### Step 10.1 - Hit Play

Press Play (Alt+P or PIE button). Open the **Output Log** (Window > Developer Tools > Output Log).

### Step 10.2 - Filter Output Log

Type these filters in the Output Log search bar to track the pipeline:

```
LogQuidditchAI        → Agent controller logs (registration, BB updates, overlaps)
LogQuidditchGameMode  → GameMode logs (registration, ready count, match state)
LogQuidditchStagingZone → Staging zone overlap events
LogSnitchAI           → Snitch perception and evasion logs
```

### Step 10.3 - Expected Log Sequence

You should see these messages in order:

```
1. [QuidditchAI]  OnPossess - agent possessed, setting up blackboard
2. [QuidditchAI]  RegisterAgentWithGameMode - registering as TeamA Seeker
3. [GameMode]     RegisterQuidditchAgent - agent registered, role assigned: Seeker
4. [QuidditchAI]  HandleQuidditchRoleAssigned - Added 'Seeker' tag (role confirmed)
5. [QuidditchAI]  BT running...
6.                (agent walks to broom, picks it up)
7. [QuidditchAI]  HandleFlightStateChanged - HasBroom=true, IsFlying=true
8.                (agent flies toward staging zone)
9. [StagingZone]  Pawn entered zone
10. [QuidditchAI] HandlePawnBeginOverlap - staging zone matched, notifying GameMode
11. [GameMode]    HandleAgentReachedStagingZone - 1/1 agents ready
12. [GameMode]    CheckAllAgentsReady - all agents ready, starting countdown
13. [GameMode]    StartCountdown - 3... 2... 1...
14. [GameMode]    OnCountdownComplete - Match started!
15. [QuidditchAI] HandleMatchStarted - MatchStarted=true
16.               (Seeker chases Snitch)
17. [SnitchAI]    Pursuer detected - evading!
18.               (Seeker catches Snitch)
19. [GameMode]    NotifySnitchCaught - transitioning to SnitchCaught
20. [GameMode]    EndMatch - Final score, transitioning to Ended
```

### Step 10.4 - Gameplay Debugger

Press **apostrophe key (')** during play to open the Gameplay Debugger:
- **Numpad 3** = AI/BT view (shows current BT node, BB values)
- **Numpad 4** = Perception view (shows what the agent perceives)

Click on the agent to select it, then check:
- BB keys updating as expected
- Active BT node progressing through the tree
- Perception cones showing broom/staging zone/snitch

---

## Troubleshooting Quick Reference

| Symptom | Root Cause | Fix |
|---------|-----------|-----|
| Agent spawns but stands still | BehaviorTreeAsset not assigned | Set `BT_QuidditchAI` in BP_QuidditchAIController |
| Agent runs BT but ignores broom | Broom missing AIPerceptionStimuliSource | Add component, enable AISense_Sight |
| Agent finds broom but can't pick up | Missing AC_BroomComponent on agent | Add component in Blueprint |
| Agent has broom but doesn't fly | HasBroom BB key not updating | Check BroomComponent delegate binding in controller |
| Agent flies but ignores staging zone | Zone missing perception source OR TeamHint/RoleHint mismatch | Verify int values: TeamA=1, Seeker=1 |
| Agent reaches zone but match never starts | RequiredAgentOverride still 0 (needs 14 agents) | Set to 1 in BP_QuidditchGameMode |
| Match starts but agent doesn't chase Snitch | BTDecorator_IsSeeker failing OR FindSnitch key not bound | Check BB key bindings in BT, verify role=Seeker |
| Snitch doesn't flee from Seeker | Agent missing "Seeker" actor tag | Verify HandleQuidditchRoleAssigned ran (check logs) |
| Snitch caught but nothing happens | SnitchBall overlap not calling NotifySnitchCaught | Verify Snitch has overlap collision enabled |
| BB key shows (invalid) in BT editor | Key name mismatch between BB asset and BT node | Exact spelling matters - check for typos, case sensitivity |

---

## What Success Looks Like

When the vertical slice is working:

1. Agent spawns, perceives broom, walks to it, picks it up
2. Agent mounts broom, lifts off, flies toward staging zone
3. Agent enters staging zone trigger, GameMode counts 1/1 ready
4. 3-second countdown, match starts
5. Seeker agent chases Snitch (Snitch actively evades)
6. Seeker catches Snitch (overlap trigger)
7. Score updates, match transitions to SnitchCaught then Ended
8. All agents receive OnMatchEnded broadcast

**Total actors in level: 4**
**Total Blueprints configured: 4** (GameMode, AIController, Agent, StagingZone)
**Zero C++ changes required.**

---

## Related Documentation

- **Full Team Setup (14 agents):** `Documentation/QUIDDITCH_AGENT_SETUP_GUIDE.md`
- **Architecture Deep Dive:** `Documentation/WizardJam_AI_Training_Guide.md`
- **MRC Validation Cards:** `Documentation/MRC_Seeker_Vertical_Slice.md`
- **AAA Standards:** `Documentation/AAA_Standards_Reference.md`
