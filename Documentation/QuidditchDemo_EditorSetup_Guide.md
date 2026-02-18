# WizardJam 2.0 - Quidditch Demo Editor Setup Guide

**Developer:** Marcus Daley
**Date:** February 11, 2026
**Goal:** Working Seeker vs. Seeker demo with two AI agents flying and chasing the Golden Snitch

---

## Current State Assessment

The project is in excellent shape. Nearly all required Blueprint assets already exist in the Content directory. The C++ systems are production-ready and have been verified working (broom acquisition confirmed January 21, 2026; flight system confirmed working). The gap between "code complete" and "demo running" is primarily configuration verification and one set of missing data assets.

### Confirmed Existing Assets

The following critical assets have been verified to exist on disk:

**Level:** `Content/Code/Maps/QuidditchLevel.umap` is the demo level and is already set as `GameDefaultMap` in `DefaultEngine.ini`.

**GameMode:** `Content/Code/GameModes/BP_QuidditchGameMode.uasset` exists as a Blueprint child of `AQuidditchGameMode`. The C++ constructor intentionally leaves `DefaultPawnClass` unset, with a comment noting the Blueprint must set it to `BP_CodeWizardPlayer`. The GameMode includes spawn debug logging at `BeginPlay()` that prints the current `DefaultPawnClass` to the Output Log, making misconfiguration immediately visible.

**AI Controllers:** Two versions exist. `Content/Code/AI/BP_CodeQuidditchAIController.uasset` is the C++ version and `Content/Blueprints/AI/BP_QuidditchAIController.uasset` is a Blueprint version. The Snitch has its own controller at `Content/Code/Actors/BP_AIC_SnitchController.uasset`.

**AI Agent Pawn:** `Content/Code/Actors/BP_QuidditchAgent.uasset` exists as the AI agent character.

**Broom System:** Both `Content/Code/Actors/BP_BroomCollectible.uasset` (the pickup that grants the "Broom" channel) and `Content/Code/Actors/BP_Broom.uasset` (the mountable broom actor) exist.

**Golden Snitch:** `Content/Code/Actors/BP_GoldenSnitch.uasset` exists with its controller `BP_AIC_SnitchController.uasset`. A spawner `Content/Blueprints/Actors/BP_QuidditchBallSpawner_Snitch.uasset` is also available.

**Staging Zones:** `Content/Blueprints/Actors/BP_QuidditchStagingZone.uasset` exists for agent starting positions. A play area volume `Content/Blueprints/Actors/BP_QuidditchPlayArea.uasset` also exists.

**Behavior Tree & Blackboard:** `Content/Code/AI/BT_QuidditchAI.uasset` and `Content/Both/DoNotConvert/BB_QuidditchAI.uasset` both exist.

**Player Character:** `Content/Code/Actors/BP_CodeWizardPlayer.uasset` exists as a child of `AWizardPlayer`.

**UI Widgets:** `Content/UI/WBP_WizardJamHUD.uasset` (main HUD), `Content/UI/WBP_WizardJamQuidditchWidget.uasset` (Quidditch scoreboard), `Content/UI/WBP_OverheadBar.uasset` (overhead bars), and `Content/UI/WBP_InteractionTooltip.uasset` (tooltips) all exist.

**Input System:** All 16 Input Actions and both Mapping Contexts (`IMC_Default` and `IMC_FlightContext`) exist in `Content/Input/InputAction/`.

**Visual Assets:** `SM_QuidditchStadium.uasset`, `M_QuidditchStadium_Master.uasset`, `SM_GoldenSnitch.uasset`, `M_GoldenSnitch.uasset`, `M_QuidditchAgent_Colorable.uasset`, and `T_Icon_Broom.uasset` all exist.

### Identified Gaps

**Gap 1 - Missing Data Assets:** The C++ class `UQuidditchAgentData` (declared in `QuidditchAgentData.h`) defines a `UPrimaryDataAsset` for configuring agent variants, but no instances of this data asset have been created in the Content directory. For the Seeker demo, at minimum one Seeker variant data asset is needed (such as `DA_SeekerAggressive`).

**Gap 2 - Configuration Verification Required:** While the Blueprint assets exist, their internal property assignments need verification. Specifically: does `BP_QuidditchGameMode` have its `DefaultPawnClass` set to `BP_CodeWizardPlayer`? Does `BP_QuidditchAgent` have its `AIControllerClass` set to `BP_CodeQuidditchAIController`? Does `BP_CodeQuidditchAIController` have `BehaviorTreeAsset` and `BlackboardAsset` assigned? Does `BP_GoldenSnitch` have `AIControllerClass` set to `BP_AIC_SnitchController`?

**Gap 3 - Blackboard Key Verification:** `BB_QuidditchAI.uasset` must contain all fourteen-plus keys that the C++ code writes to. Any missing key will produce "(invalid)" warnings at runtime.

**Gap 4 - Behavior Tree Wiring:** `BT_QuidditchAI.uasset` must have its node graph correctly connected with the Quidditch-specific C++ tasks (BTTask_MountBroom, BTTask_ControlFlight, BTTask_FlyToStagingZone, BTTask_ChaseSnitch) and services (BTService_FindCollectible, BTService_FindStagingZone, BTService_FindSnitch, BTService_SyncFlightState).

---

## Session 1: Asset Verification and Configuration (90 minutes)

This session is entirely editor work. No code changes are needed.

### Step 1.1: Build the Project

Open the project in Unreal Editor. If prompted to build, allow it. Once loaded, verify there are no compilation errors in the Output Log. If the project was last built successfully, this step is quick. If there are build errors, resolve them before proceeding.

### Step 1.2: Verify Blackboard Keys

Open `Content/Both/DoNotConvert/BB_QuidditchAI.uasset` in the Blackboard editor. Verify the following keys exist with the correct types. If any are missing, add them using the "New Key" button in the Blackboard editor.

The required keys are: `SelfActor` as Object (base class AActor), `HomeLocation` as Vector, `TargetLocation` as Vector, `TargetActor` as Object (base class AActor), `IsFlying` as Bool, `HasBroom` as Bool, `MatchStarted` as Bool, `bShouldSwapTeam` as Bool, `QuidditchRole` as Name, `PerceivedCollectible` as Object (base class AActor), `SnitchLocation` as Vector, `SnitchVelocity` as Vector, `StagingZoneLocation` as Vector, `StagingZoneActor` as Object (base class AActor), `ReachedStagingZone` as Bool, `IsReady` as Bool, and `StageLocation` as Vector. That is seventeen keys total. Any key that exists but has the wrong type must be deleted and recreated with the correct type. Save the Blackboard asset after any changes.

### Step 1.3: Verify Behavior Tree Structure

Open `Content/Code/AI/BT_QuidditchAI.uasset` in the Behavior Tree editor. The tree should have a Root node connected to a Selector (or Sequence) that branches into the major behavioral phases. Verify the following nodes are present and correctly wired.

The AcquireBroom sequence should contain `BTService_FindCollectible` as an attached service, a Move To node targeting the `PerceivedCollectible` blackboard key, and `BTTask_MountBroom` with `bMountBroom` set to true. This sequence should be gated by a decorator checking that `HasBroom` is false (so it only runs when the agent does not have a broom).

The FlyToStart sequence should contain `BTService_FindStagingZone` as an attached service, and `BTTask_FlyToStagingZone` reading from `StagingZoneLocation`. This should be gated by a decorator checking that `IsFlying` is true and `ReachedStagingZone` is false.

The QuidditchMatch sequence (specifically the Seeker branch for this demo) should contain `BTService_FindSnitch` as an attached service and `BTTask_ChaseSnitch` reading from `SnitchLocation`. This should be gated by decorators checking that `MatchStarted` is true and `QuidditchRole` equals "Seeker".

The ReturnToHome fallback should contain `BTTask_ReturnToHome` reading from `HomeLocation`.

If any C++ task nodes do not appear in the node browser when right-clicking to add nodes, this indicates a compilation issue with that specific class. Check the Output Log for UHT errors related to that class.

For each BTService and BTTask that uses `FBlackboardKeySelector`, click on the node in the BT editor and verify the blackboard key dropdown is assigned to the correct key (not "None"). This is the most common configuration oversight.

### Step 1.4: Verify BP_QuidditchGameMode

Open `Content/Code/GameModes/BP_QuidditchGameMode.uasset`. In the Class Defaults panel, verify or set: `DefaultPawnClass` to `BP_CodeWizardPlayer`, `MaxSeekersPerTeam` to 1, `MaxChasersPerTeam` to 3, `MaxBeatersPerTeam` to 2, `MaxKeepersPerTeam` to 1, `SnitchCatchPoints` to 150, `QuaffleGoalPoints` to 10, and `MatchStartCountdown` to 3.0. If `DefaultPawnClass` is null, this is a critical bug. The spawn debug logging at BeginPlay will print "DefaultPawnClass: NULL! <- THIS IS THE BUG" to the Output Log. Set it now.

### Step 1.5: Verify BP_CodeQuidditchAIController

Open `Content/Code/AI/BP_CodeQuidditchAIController.uasset`. In the Class Defaults, verify: `BehaviorTreeAsset` is set to `BT_QuidditchAI`, `BlackboardAsset` is set to `BB_QuidditchAI`, `SightRadius` is approximately 2000, `LoseSightRadius` is approximately 2500, and `PeripheralVisionAngle` is 90. If the BT or BB fields are empty, the AI controller will not run any behavior tree and agents will stand motionless.

### Step 1.6: Verify BP_QuidditchAgent

Open `Content/Code/Actors/BP_QuidditchAgent.uasset`. In Class Defaults, verify: `AIControllerClass` is set to `BP_CodeQuidditchAIController` (or whichever controller Blueprint is the active one). Verify the agent has the following components visible in the Components panel: a Skeletal Mesh component (with a mesh assigned for visual representation), and that `AutoPossessAI` is set to "Placed in World" or "Spawned" (not "Disabled").

### Step 1.7: Verify BP_GoldenSnitch

Open `Content/Code/Actors/BP_GoldenSnitch.uasset`. In Class Defaults, verify: `AIControllerClass` is set to `BP_AIC_SnitchController`, `AutoPossessAI` is set to "Placed in World" or "Spawned", `BaseSpeed` is approximately 600, `MaxEvadeSpeed` is approximately 1200, and `EvadeRadius` is approximately 800. In the Components panel, verify a `UAIPerceptionStimuliSourceComponent` is present (this makes the Snitch visible to Seeker perception). If the mesh component is present, verify `SM_GoldenSnitch` is assigned. Verify `M_GoldenSnitch` material is applied.

### Step 1.8: Verify BP_BroomCollectible

Open `Content/Code/Actors/BP_BroomCollectible.uasset`. Verify it has a `UAIPerceptionStimuliSourceComponent` in its Components panel (this makes brooms detectable by AI perception). Verify the collectible grants the "Broom" channel (check the channel grant property in Class Defaults). If the perception component is missing, add one and configure it to register for the Sight sense.

### Step 1.9: Verify BP_QuidditchStagingZone

Open `Content/Blueprints/Actors/BP_QuidditchStagingZone.uasset`. Verify it has a collision volume (Box or Sphere Collision component) that generates overlap events. In the collision component's details, verify "Generate Overlap Events" is checked. Verify a `UAIPerceptionStimuliSourceComponent` is present. The `TeamHint` and `RoleHint` properties will be set per-instance in the level, not in the Blueprint defaults.

### Step 1.10: Create QuidditchAgentData Assets (If Missing)

If the C++ code requires `UQuidditchAgentData` instances and none exist, create them. In the Content Browser, right-click, select "Miscellaneous" then "Data Asset", and choose `QuidditchAgentData` as the parent class. Create at least one: `DA_SeekerDefault` with the Seeker role, flight speed of 1200, and the `BT_QuidditchAI` behavior tree reference. Save it to `Content/Code/Data/`. If agents are configured to reference these data assets, assign `DA_SeekerDefault` to both demo agents. If the agent system does not require data assets (agents can function with controller defaults alone), this step can be deferred.

### Session 1 Success Criteria

All Blueprint assets open without errors. Blackboard has all seventeen keys. Behavior Tree shows connected nodes with correct blackboard key assignments. GameMode has `DefaultPawnClass` set. Controller has BT and BB assigned. Agent has correct `AIControllerClass`. Snitch has correct controller and `AutoPossessAI` enabled.

---

## Session 2: Level Setup and First Flight Test (120 minutes)

### Step 2.1: Open QuidditchLevel

Open `Content/Code/Maps/QuidditchLevel.umap`. Check the World Settings (Window menu, World Settings) and verify `GameMode Override` is set to `BP_QuidditchGameMode`. If it is set to a different GameMode or "None" (inheriting the global default `WizardJamGameMode` from `DefaultEngine.ini`), change it to `BP_QuidditchGameMode`.

### Step 2.2: Audit Existing Level Placement

Before placing new actors, check what is already in the level. Open the World Outliner (Window menu) and search for existing actors. Look for any `BP_QuidditchAgent` instances, `BP_BroomCollectible` instances, `BP_GoldenSnitch` instances, `BP_QuidditchStagingZone` instances, `BP_QuidditchPlayArea` instances, and `PlayerStart` actors. If the level was previously configured, many of these may already be placed. Document what exists and what is missing.

### Step 2.3: Ensure Play Area

If no `BP_QuidditchPlayArea` is placed, drag one from the Content Browser into the level. Position it to encompass the entire playable flight space. This defines the Snitch's boundary constraints. If one exists, select it and verify its bounds cover the desired play volume. The Snitch's `PlayAreaVolumeRef` property should reference this actor (set it in the Snitch's Details panel under the appropriate category, or in `BP_GoldenSnitch` defaults if all instances should use the same area).

### Step 2.4: Place Broom Collectibles

The demo needs two broom collectibles, one for each agent. Drag `BP_BroomCollectible` from the Content Browser and place two instances on the ground level, separated by at least 500 units so agents do not compete. Position them in locations reachable by navigation (on the Nav Mesh). Ensure each collectible is not inside a wall or below the floor. Verify they are selectable in the Outliner.

If broom collectibles are already placed from previous testing, verify their positions are reasonable and they have perception components.

### Step 2.5: Place Staging Zones

The demo needs two staging zones, one per team. Drag `BP_QuidditchStagingZone` and place two instances at elevated positions (Z offset of 500 to 800 units above ground), representing where Seekers should line up before the match starts.

Select the first staging zone. In the Details panel, set `TeamHint` to 0 and `RoleHint` to the integer value corresponding to the Seeker role (check `QuidditchTypes.h` for the enum mapping; Seeker is likely 0 or 1). Set `ZoneIdentifier` to something descriptive like "TeamA_Seeker". Repeat for the second zone with `TeamHint` set to 1 and the same `RoleHint` for Seeker.

### Step 2.6: Place or Verify Golden Snitch

If no Snitch is in the level, drag `BP_GoldenSnitch` and place it at a central elevated position (approximately halfway between the two staging zones, at a similar altitude). In the Details panel, if there is a `PlayAreaVolumeRef` property, set it to the `BP_QuidditchPlayArea` actor placed in Step 2.3. If the Snitch uses `PlayAreaCenter` and `PlayAreaExtent` instead, configure those to match the play area bounds.

Alternatively, if `BP_QuidditchBallSpawner_Snitch` handles Snitch spawning through the GameMode, place the spawner instead and configure it.

### Step 2.7: Place AI Agents

Drag two `BP_QuidditchAgent` instances into the level at ground-level spawn positions. Select the first agent. In the Details panel, look for team and role assignment properties. If the agent has an `AgentQuidditchTeam` or `DefaultTeam` property, set it to Team 0 (TeamA). If it has an `AgentPreferredRole` property, set it to Seeker. Repeat for the second agent with Team 1 (TeamB) and Seeker role.

If agents are spawned by the GameMode at runtime (rather than placed in the level), look for spawn configuration in `BP_QuidditchGameMode`'s Class Defaults. There may be a `RequiredAgentCount` property or a spawn array that needs to be configured with two entries.

Verify both agents have `AutoPossessAI` set to "Placed in World" (if manually placed) or that the GameMode's spawn logic handles possession automatically.

### Step 2.8: Verify Navigation

Press P in the viewport to toggle Nav Mesh visualization. Verify that the green Nav Mesh covers the ground area where agents spawn and where broom collectibles are placed. If there is no Nav Mesh, add a `NavMeshBoundsVolume` that encompasses the ground area. The Nav Mesh only needs to cover ground-level areas; flight does not use Nav Mesh.

### Step 2.9: First Play Test - Broom Acquisition

Press Play (PIE). Immediately open the Output Log (Window menu, Output Log) and filter for "LogQuidditchGameMode" and "LogAI". Look for the spawn debug section: "=== SPAWN DEBUG START ===" should appear, and `DefaultPawnClass` should show the correct Blueprint name, not "NULL".

Observe the two AI agents. They should begin executing the AcquireBroom behavior tree sequence. In the Output Log, look for perception messages indicating the agents detected the broom collectibles. The agents should navigate (walk) toward the brooms using the Nav Mesh. Upon reaching a broom, the agent should interact with it and mount, triggering flight mode. Look for "[AC_BroomComponent] Flight enabled" or similar log messages.

If agents do not move, check the Behavior Tree debugger. Select an agent in the viewport, then open Window, Behavior Tree. The BT debugger shows which node is currently active and the state of all blackboard keys. If the tree is not running, verify the controller has BT and BB assigned. If the tree is running but stuck on a node, check that node's blackboard key assignment.

If agents move but do not reach the broom, verify Nav Mesh coverage. If agents reach the broom but do not interact, verify the `IInteractable` interface is implemented on the broom and that the BT task for interaction is correctly configured. If agents interact but do not fly, verify `AC_BroomComponent` is present on the agent pawn.

### Step 2.10: Debug Perception Issues

If agents ignore brooms entirely, the most likely cause is missing perception registration. While playing, press the apostrophe key (') to toggle AI debug visualization. Green cones show each agent's sight perception range. Verify the cones extend far enough to reach the broom locations. If cones are visible but brooms are not highlighted as perceived, the broom Blueprint is missing its `UAIPerceptionStimuliSourceComponent`.

If perception works but the blackboard key is not being written, open the BT debugger and check the `PerceivedCollectible` key. If it shows "(invalid)" or is empty despite the agent seeing the broom, the `BTService_FindCollectible` node may have an unassigned `OutputKey` in the BT editor.

### Session 2 Success Criteria

Both agents walk to brooms, interact, and mount. Flight mode activates on both agents. The Output Log shows successful spawn configuration and flight state changes. The Behavior Tree debugger shows `HasBroom = true` and `IsFlying = true` for both agents.

---

## Session 3: Full Demo Flow (90 minutes)

### Step 3.1: Observe Staging Zone Navigation

After broom acquisition, agents should transition to the FlyToStart behavior. `BTService_FindStagingZone` should detect the staging zones via perception and filter to the correct one based on the agent's team and role. Agents should fly upward and toward their respective staging zones. Watch for correct team assignment: Team 0 agent should fly to the Team 0 zone, Team 1 to Team 1.

If agents fly but go to the wrong zone, the `TeamHint` or `RoleHint` on the staging zone actors may be swapped. Verify in the level editor.

If agents fly but never arrive (circling the zone), this would indicate the old VInterpTo velocity bug. However, this was already fixed with direct velocity assignment. If it reoccurs, check that `BTTask_FlyToStagingZone` is using the updated code.

If agents arrive at zones but the match does not advance, check the Output Log for "HandleAgentReachedStagingZone" messages. If these do not appear, the overlap detection on the staging zone may not be triggering. Verify the zone's collision component has "Generate Overlap Events" enabled and that the agent's capsule component also generates overlap events.

### Step 3.2: Verify Match State Transitions

Watch the Output Log for match state transitions. The expected sequence is: Initializing (at level start), FlyingToStart (when agents begin flying), WaitingForReady (when first agent reaches zone), Countdown (when all agents are ready), and InProgress (when countdown completes). If `OnMatchStarted` fires, the blackboard's `MatchStarted` key should transition to true on both agents.

If the match stays in WaitingForReady, check `AgentsReadyCount` versus `RequiredAgentCount` in the GameMode. If `RequiredAgentCount` is set higher than the number of placed agents, the match will never start. Adjust `RequiredAgentCount` to 2 for this demo.

### Step 3.3: Observe Snitch Chase

When the match enters InProgress, Seeker agents should activate `BTService_FindSnitch` and `BTTask_ChaseSnitch`. Observe the agents pursuing the Golden Snitch. Both agents should fly directly toward the Snitch's current position. The Snitch should react by accelerating and changing direction when agents approach within its `EvadeRadius` (800 units).

Verify the chase looks natural: agents should not orbit the Snitch (direct velocity assignment prevents this), agents should adjust altitude to match the Snitch's height, and agents should occasionally activate boost when far from the Snitch (visible as increased speed).

If agents do not chase the Snitch, check that `BTService_FindSnitch` is writing to the `SnitchLocation` blackboard key. Open the BT debugger and verify `SnitchLocation` contains non-zero coordinates. If it is zero, the service may not be detecting the Snitch. Verify the Snitch has its `UAIPerceptionStimuliSourceComponent` configured and that it is within the agents' sight radius.

If the Snitch does not move, verify `BP_AIC_SnitchController` is assigned as its `AIControllerClass` and that `AutoPossessAI` is enabled. The Snitch's movement is driven by its `UpdateMovement()` method in `Tick()`, which requires the controller to be possessed.

### Step 3.4: Observe Catch Event

The Snitch catch occurs via overlap detection. When a Seeker's collision capsule overlaps the Snitch's collision sphere, `OnSnitchOverlap` fires. This notifies the GameMode, which awards 150 points and transitions to `SnitchCaught` state. Watch the Output Log for "SnitchCaught" messages and score assignment.

If catches never happen despite agents being close, the Snitch's collision sphere may be too small. Check `BP_GoldenSnitch`'s sphere collision radius. Increasing it to 150-200 units makes catches easier during testing. Also verify both actors have their collision presets configured to generate overlap events with each other.

If the catch triggers but the match does not end, check that the GameMode's `NotifySnitchCaught()` is connected to the state transition logic. Look for "TransitionToState: SnitchCaught" in the Output Log.

### Step 3.5: Verify Results Display

If `WBP_WizardJamQuidditchWidget` is configured and `BP_CodeWizardPlayer` has it in its `HUDWidgetClasses` array, the scoreboard should be visible during the match. After the Snitch is caught, if a results widget is configured, it should display the winning team. If no UI appears, this is a non-blocking issue for the demo. The AI behavior and match flow are the priority; UI polish can follow.

### Step 3.6: Quick Tuning for Demo Quality

If the demo works but the Snitch is caught too quickly, increase `BaseSpeed` on the Snitch (try 800). If agents catch the Snitch too slowly, decrease `BaseSpeed` (try 400) or increase the agents' flight speed. If the Snitch stays too close to boundaries, increase `PlayAreaExtent` or expand the `BP_QuidditchPlayArea` volume. If agents take too long to reach staging zones, move the staging zones closer to the broom collectible locations or adjust flight speed.

### Step 3.7: Record Demo

Once the full loop (spawn, mount, staging, chase, catch) works reliably, use Unreal's built-in Sequencer or a screen recorder to capture the demo. Focus on showing: agents walking to brooms and mounting, the flight transition, navigation to staging zones, the Snitch chase with evasion, and the catch event. This footage serves as portfolio material.

### Session 3 Success Criteria

Complete gameplay loop: spawn, mount broom, fly to staging zone, wait for match start, chase Snitch, catch Snitch, match ends. Both agents demonstrate correct team-based behavior. The Snitch demonstrates evasion. The match state transitions are visible in the Output Log.

---

## Common Issues Quick Reference

**Agents stand motionless:** Controller has no BT/BB assigned, or `AutoPossessAI` is disabled. Fix: assign BT and BB in controller Blueprint, set AutoPossessAI to "Placed in World".

**Agents walk but ignore brooms:** Broom Blueprint missing `UAIPerceptionStimuliSourceComponent`. Fix: add the component and configure it for Sight sense.

**Agents see brooms but do not walk to them:** `BTService_FindCollectible` has unassigned `OutputKey`, or Nav Mesh does not cover the broom location. Fix: assign the key in BT editor, verify Nav Mesh with P key.

**Agents mount brooms but do not fly:** `AC_BroomComponent` missing on agent pawn, or `CharacterMovementComponent` does not support Flying mode. Fix: verify component exists on the agent Blueprint.

**Agents fly in circles:** Old VInterpTo velocity interpolation bug. Fix: already resolved in current code with direct velocity assignment. If reoccurring, verify the correct BTTask code is compiled.

**Agents fly to wrong staging zone:** `TeamHint` or `RoleHint` swapped on zone instances. Fix: check Details panel for each zone in the level.

**Match never starts (stuck in WaitingForReady):** `RequiredAgentCount` higher than placed agents. Fix: set to 2 for demo, or set to 0 if auto-detection is used.

**Snitch does not move:** No AI controller assigned, or `AutoPossessAI` disabled. Fix: assign `BP_AIC_SnitchController` and enable auto-possession.

**Snitch never caught:** Collision sphere too small or overlap events disabled. Fix: increase sphere radius to 150-200 units, enable "Generate Overlap Events" on both actors.

**No UI visible:** `HUDWidgetClasses` array empty on `BP_CodeWizardPlayer`. Fix: add `WBP_WizardJamHUD` at index 0. Non-blocking for demo.

**Output Log shows "DefaultPawnClass: NULL":** `BP_QuidditchGameMode` does not have `DefaultPawnClass` set. Fix: open the Blueprint and set it to `BP_CodeWizardPlayer`.

---

## Debugging Toolkit

**Output Log Filters:** Use the following filter strings to isolate relevant log messages:
- `LogQuidditchGameMode` for match state transitions and spawn debugging
- `LogAI` for general AI behavior
- `LogBroomComponent` for flight state changes
- `LogSnitchBall` for Snitch movement and catch events
- `LogSnitchController` for Snitch perception of pursuers
- `SPAWN DEBUG` for the startup configuration check

**AI Debug Visualization:** Press the apostrophe key (') during Play to toggle AI debug overlays. These show perception cones (green), current BT node name, and blackboard key values floating above each agent's head.

**Behavior Tree Debugger:** Select an agent during Play, then open Window, Behavior Tree. The left panel shows the tree with active nodes highlighted in green. The right panel shows all blackboard key values in real-time. This is the single most useful tool for diagnosing AI behavior issues.

**Console Commands:** Type these in the console (~) during Play:
- `ShowDebug AI` shows detailed AI state on the HUD
- `ShowDebug Perception` shows perception information
- `stat FPS` shows frame rate to check for performance issues

---

## Post-Demo Enhancement Roadmap

Once the Seeker vs. Seeker demo is working, enhancements can be added incrementally in this order, with each building on the previous:

**Phase 1 - Visual Polish:** Add broom banking and pitching during flight by reading the current vertical input and turn rate, applying them as pitch and roll offsets to the broom visual mesh attachment. This is purely cosmetic and does not affect gameplay.

**Phase 2 - Obstacle Avoidance Integration:** Wire `AC_FlightSteeringComponent` into `BTTask_ControlFlight` so agents avoid arena walls and stadium geometry during pursuit. The steering component already exists with whisker traces and altitude enforcement.

**Phase 3 - Flocking for Team Chasers:** Expand the demo to include Chasers using `UFlyingSteeringComponent` for Reynolds flocking behaviors. This requires completing the Quaffle ball implementation and adding Chaser-specific BT sequences.

**Phase 4 - Elemental Agents:** Add `ElementalAffinity` (FName) to agents, color-code them using `M_QuidditchAgent_Colorable` material, implement `BTService_FindElementalCollectible` for spell collection, and add scoring multipliers for elemental matching.

**Phase 5 - Full Match:** Add Beaters (Bludger targeting), Keepers (goal defense), and complete all role-specific BT branches. Implement the Quaffle scoring system and integrate with the Quidditch scoreboard widget.
