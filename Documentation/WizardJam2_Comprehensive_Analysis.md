# WizardJam 2.0 Comprehensive Codebase Analysis & Incremental Demo Plan

**Developer:** Marcus Daley
**Analyst:** WizardJam Development Agent
**Date:** February 11, 2026
**Engine:** Unreal Engine 5.4
**Module:** END2507
**Project Root:** `C:\Users\daley\UnrealProjects\BaseGame`

---

## 1. Flight System Architecture

The flight system in WizardJam 2.0 is built on a modular, component-based architecture with a clear separation between configuration, control logic, and steering. The system's most important design decision is its input-agnostic Core API, meaning the same flight functions are called by both player input handlers and AI behavior tree tasks.

### Core Flight Component

The central class is `UAC_BroomComponent` (declared in `Public/Code/Flight/AC_BroomComponent.h` at 264 lines, implemented in `Private/Code/Flight/AC_BroomComponent.cpp` at 791 lines). This component attaches to any character and provides three primary API functions: `SetVerticalInput(float)` accepts a value from negative one to positive one for ascending and descending, `SetBoostEnabled(bool)` toggles high-speed flight, and `SetFlightEnabled(bool)` handles mounting and dismounting. When a player flies, the Enhanced Input system calls `HandleAscendInput()` which internally calls `SetVerticalInput()`. When AI flies, `BTTask_ControlFlight::TickTask()` calculates an altitude delta and calls the same `SetVerticalInput()` function. This parity ensures identical flight physics for both player and AI.

Movement uses Unreal Engine's native `MOVE_Flying` movement mode, set via `UCharacterMovementComponent::SetMovementMode()`. Lateral movement is handled by the character's standard movement system, while vertical movement is applied directly by the broom component in its `TickComponent()` method, setting `MovementComponent->Velocity.Z` to `CurrentVerticalVelocity`. The component defines `FlySpeed` (default 600 units per second) and `BoostSpeed` (default 1200), both designer-configurable via `EditDefaultsOnly`.

### Configuration System

All flight parameters live in `FBroomConfiguration`, a struct defined in `Public/Code/Flight/BroomTypes.h` (278 lines). This struct contains duration, stamina behavior, speed, deceleration, mounting parameters, and channel requirements, all initialized via a constructor initialization list with no header defaults. The struct provides three preset factory functions: `GetFreeFlightPreset()`, `GetQuidditchPreset()`, and `GetRacingPreset()`. The Quidditch preset enables infinite duration, drain-only-when-moving stamina, and idle regeneration at twelve units per second, creating a "catch your breath" mechanic where agents recover stamina by hovering briefly. New broom types require only a new Blueprint child of `ABroomActor` with a custom `FBroomConfiguration`, requiring zero C++ changes.

### Broom Actor and Mounting Logic

`ABroomActor` (declared in `Public/Code/Flight/BroomActor.h` at 231 lines, implemented in `Private/Code/Flight/BroomActor.cpp` at 367 lines) is the world-placeable entity holding the `FBroomConfiguration`. It implements the `IInteractable` interface, so both players and AI agents interact with it through the same `OnInteract_Implementation()` code path. When interaction occurs, the actor validates spell channels, dynamically creates or finds an `AC_BroomComponent` on the rider via `GetOrCreateBroomComponent()`, copies configuration via `ConfigureBroomComponent()`, and attaches the broom visual to the character's mesh socket. The mounting system supports separate socket names for player characters and AI agents through `BroomConfiguration.PlayerMountSocket` and `BroomConfiguration.AIMountSocket`, ensuring correct visual attachment regardless of skeleton differences.

### Stamina Integration

A critical bug fix discovered during development (documented at line 712-716 of `AC_BroomComponent.cpp`) established that normal flight is free of stamina cost. Only boosting drains stamina, using `BoostStaminaDrainRate` (default 25 units per second). The component listens to `AC_StaminaComponent::OnStaminaChanged` via delegate binding at BeginPlay, and forces dismount when stamina drops below `MinStaminaToFly` (default 20). The component broadcasts four delegates: `OnFlightStateChanged` for mount/dismount, `OnBoostStateChanged` for boost toggling, `OnBroomStaminaVisualUpdate` with color codes (Cyan for flying, Orange for boosting, Red for depleted), and `OnForcedDismount` for depletion feedback. All of these are marked `BlueprintAssignable` for widget binding.

### Obstacle-Aware Steering

`AC_FlightSteeringComponent` (declared in `Public/Code/Flight/AC_FlightSteeringComponent.h` at 246 lines, implemented at 512 lines) provides obstacle-aware steering for automated flight paths. It returns normalized input vectors for pitch, yaw, and thrust in the negative one to positive one range. Obstacle detection uses sphere traces with a center ray plus whisker rays (five by default) at a configurable `ObstacleDetectionRange` of 800 units. The component enforces altitude bounds between `MinAltitude` (200 units) and `MaxAltitude` (2000 units) with pitch correction, and provides arrival detection with `ArrivalRadius` (200 units) and `SlowdownRadius` (500 units) for smooth deceleration. The key steering functions are `CalculateSteeringToward(FVector)` for seek behavior, `CalculateFleeingFrom(FVector)` for evasion, and `CalculateSteeringTowardWithPrediction()` for leading moving targets like the Snitch.

### Flocking Steering for Team Coordination

`UFlyingSteeringComponent` (in `Public/Code/Utilities/FlyingSteeringComponent.h` at 246 lines, implemented at 407 lines) is a direct port of Full Sail's Flock.cs to Unreal Engine 5. It implements three Reynolds steering behaviors: alignment (match average velocity of nearby teammates), cohesion (steer toward center of flock or target location), and separation (push away to avoid collision). Each behavior has a configurable strength defaulting to 5.0, with `FlockRadius` at 500 units and `SafeRadius` at 150 units. The component uses `FlockTag` (an FName like "Team0_Chasers") to identify team members and finds them via `TActorIterator`, following the project's prohibition on `GameplayStatics::GetAllActorsWithTag`. Different Quidditch roles use different behavior mixes: Seekers use low alignment with high cohesion toward the Snitch, Chasers use full flocking with cohesion toward the goal, Beaters disable separation for pursuit behavior, and Keepers use high cohesion for position hold at goal center.

### Recent Bug Fix: Circular Flight

The circular flight bug was caused by using `VInterpTo` for velocity interpolation in AI flight tasks, which produced smooth but orbital trajectories at high speeds. The fix, implemented in `BTTask_ControlFlight.cpp` and `BTTask_FlyToStagingZone.cpp`, replaced velocity interpolation with direct velocity assignment: `MoveComp->Velocity = DesiredVelocity`. This produces instant direction changes that prevent the agent from overshooting and orbiting its target. The vertical component is preserved separately (`DesiredVelocity.Z = MoveComp->Velocity.Z`) since altitude is managed by `AC_BroomComponent`.

---

## 2. AI Behavior Tree System

The AI behavior tree system spans two layers: the behavior tree asset `BT_QuidditchAI` (located at `Content/Code/AI/BT_QuidditchAI.uasset`) and numerous C++ task, service, and decorator nodes that drive agent decision-making.

### Behavior Tree Structure

The expected tree structure follows four major sequences. The first is AcquireBroom, gated by a decorator checking that the agent does not already have the "Broom" channel. This sequence runs `BTService_FindCollectible` to search perception for broom actors, then executes movement toward the perceived collectible, followed by `BTTask_MountBroom` to enable flight. The second sequence is FlyToStart, which uses `BTService_FindStagingZone` to perceive the correct staging zone (filtered by team and role), then executes `BTTask_FlyToStagingZone` to navigate there. Upon arrival, the controller detects overlap with the zone and notifies the GameMode. The third sequence handles the actual Quidditch match, branching by role: Seekers execute `BTTask_ChaseSnitch`, Chasers handle the Quaffle, Beaters target Bludgers, and Keepers defend goals. All match behaviors run `BTService_SyncFlightState` to keep the blackboard's `IsFlying` key synchronized with the actual `AC_BroomComponent` state. The fourth and final sequence is a fallback `BTTask_ReturnToHome` that navigates the agent back to its spawn position when no active tasks are available.

### Core Flight Tasks

`BTTask_ControlFlight` (declared in `Public/Code/AI/BTTask_ControlFlight.h` at 158 lines, implemented in `Private/Code/AI/BTTask_ControlFlight.cpp` at 476 lines) is the primary continuous flight control task. It ticks every frame, reading a `TargetKey` from the blackboard that can be either an Object (actor reference) or a Vector (world location), supported by both `AddObjectFilter()` and `AddVectorFilter()` in the constructor. The task calculates altitude difference, applies vertical input via `BroomComp->SetVerticalInput()`, manages boost based on distance thresholds, and uses direct velocity assignment for horizontal movement. It implements stamina-aware flight with four thresholds: `BoostStaminaThreshold` at 40% (disable boost below this), `ThrottleStaminaThreshold` at 25% (reduce movement speed), `LowStaminaMovementScale` at 50% (speed reduction factor), and `CriticalStaminaThreshold` at 15% (force landing if `bLandWhenStaminaCritical` is true).

`BTTask_FlyToStagingZone` (declared at 236 lines, implemented at 658 lines) handles navigation to staging zones with multiple arrival detection methods: standard distance check with an `ArrivalRadius` of 400 units, velocity-based overshoot detection using dot product to determine if the agent is moving away from the target, stuck detection via position history sampling, and a timeout duration as final safety. This multi-strategy approach prevents agents from endlessly circling their target.

`BTTask_MountBroom` (declared at 70 lines, implemented at 173 lines) is a single-frame task that calls `BroomComp->SetFlightEnabled()` with a `bMountBroom` parameter. It optionally writes the result to a `FlightStateKey` blackboard key. Both mount and dismount operations use this single task with the boolean toggled.

### Blackboard Keys

The complete set of blackboard keys initialized in `AIC_QuidditchController::SetupBlackboard()` (lines 275-310 of the controller's implementation) includes: `SelfActor` (Object, set to controlled pawn), `HomeLocation` (Vector, spawn position), `TargetLocation` (Vector, navigation destination), `TargetActor` (Object, actor to follow), `IsFlying` (Bool, current flight state), `HasBroom` (Bool, broom possession), `MatchStarted` (Bool, game state sync), `bShouldSwapTeam` (Bool, team swap flag), `QuidditchRole` (Name, assigned role), `SnitchLocation` and `SnitchVelocity` (Vectors for Seeker behavior), `StagingZoneLocation` and `StagingZoneActor` (for staging navigation), `ReachedStagingZone` and `IsReady` (Bools for match readiness), `StageLocation` (Vector, written on staging zone arrival), and `PerceivedCollectible` (Object, nearest sensed item). All Bool keys are initialized to false, all Vectors to zero, and Object keys to null, preventing the "(invalid)" marker issue discovered during the Day 26 blackboard integrity analysis.

### Services and Decorators

`BTService_SyncFlightState` (49 lines header) runs at 0.25-second intervals, reading `AC_BroomComponent::IsFlying()` and writing to the `IsFlyingKey` blackboard key. This ensures blackboard state reflects real flight state even if delegate updates are missed. `BTService_FindCollectible` (74 lines header, 160 lines implementation) queries `AIPerceptionComponent::GetPerceivedActors()` on a 0.5-second interval, filters by class type and required channel grant, and writes the nearest result to `OutputKey`. `BTService_FindStagingZone` (72 lines header, 290 lines implementation) implements the "Bee and Flower" pattern, where the agent perceives staging zones and decides if a zone matches its team and role by reading `TeamHint` and `RoleHint` from the zone actor, comparing against the agent's own assignment from the GameMode. `BTDecorator_HasChannel` (56 lines) gates behavior tree branches based on whether the agent's `AC_SpellCollectionComponent` contains a specific channel name.

---

## 3. AI Perception Configuration

### Quidditch Controller Perception

`AIC_QuidditchController` configures sight perception in its constructor (lines 54-68 of the implementation). The sight sensor uses a `SightRadius` of 2000 units with a `LoseSightRadius` of 2500 units, creating a hysteresis band that prevents perception flickering when targets are near the detection boundary. The `PeripheralVisionAngle` is set to 90 degrees, providing a 180-degree forward cone. Critically, detection by affiliation is set to detect enemies, friendlies, and neutrals, because collectibles like brooms register as neutral actors and would be invisible to perception without this setting. The `MaxAge` of stimuli is 5.0 seconds, meaning perceived actors remain in the system for five seconds after leaving the visual cone.

The perception callback `HandlePerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)` follows the Nick Penney pattern: it checks `Stimulus.WasSuccessfullySensed()` to differentiate between perception entry and exit events. On successful sensing, it stores the actor reference in the appropriate blackboard key. On loss of sensing, it clears the key if the lost actor was the currently tracked target. This event-driven approach means the controller never polls for perceived actors; it reacts to perception changes through the delegate.

### Snitch Controller Perception

`AIC_SnitchController` (declared in `Public/Code/AI/AIC_SnitchController.h` at 117 lines, implemented at 181 lines) provides the Snitch's awareness of pursuing Seekers. Its `DetectionRadius` is 2000 units with a `LoseDetectionRadius` of 2500 units, but its `PeripheralVisionAngle` is set to 180 degrees, giving it effectively 360-degree omnidirectional vision. The Snitch is assigned `FGenericTeamId::NoTeam` (neutral), and its `ValidPursuerTags` array (designer-configurable) defaults to "Seeker", "Flying", and "Player". The controller broadcasts two delegates: `OnPursuerDetected` and `OnPursuerLost`, to which the SnitchBall actor binds in its `PossessedBy()` method. This allows the Snitch to reactively adjust its evasion behavior without polling.

### Stimulus Sources

Actors that register as AI perception stimuli include `ABroomCollectible` (has `UAIPerceptionStimuliSourceComponent` for AI detection of broom pickups), `AQuidditchStagingZone` (registered for zone discovery during flight), and `ASnitchBall` (registered for Seeker targeting and Snitch self-awareness of pursuers). The perception data flows from these stimulus sources into the AI controllers' perception callbacks, which update blackboard keys, which BT services and tasks then read to make decisions.

---

## 4. Quidditch Match Infrastructure

### Game Mode Architecture

`AQuidditchGameMode` (declared in `Public/Code/GameModes/QuidditchGameMode.h`, inheriting from `AWizardJamGameMode`) manages the complete match lifecycle through an eight-state state machine defined in `QuidditchTypes.h` (lines 66-82). The states progress from `Initializing` (agents spawning and acquiring brooms) through `FlyingToStart` (agents navigating to staging zones), `WaitingForReady` (agents at starting positions), `Countdown` (visual countdown), `InProgress` (match running), `PlayerJoining` (player entering, AI swapping), `SnitchCaught` (ending sequence), and `Ended` (match complete). State transitions occur through `TransitionToState()` which broadcasts `OnMatchStateChanged`, and all controllers and widgets listen to this delegate rather than polling the GameMode.

### Agent Registration

The GameMode exposes `RegisterQuidditchAgent(APawn*, EQuidditchRole PreferredRole, EQuidditchTeam Team)` as a `BlueprintCallable` function. Registration stores agent info in an `FQuidditchAgentInfo` struct containing a weak pointer to the agent, its team, assigned role, and preferred role. The `FindAvailableRole()` function checks role limits (one Seeker per team, three Chasers, two Beaters, one Keeper) and assigns the preferred role if available, falling back through a priority chain of Chaser, Beater, Keeper, and Seeker. After registration, the GameMode broadcasts `OnQuidditchRoleAssigned` with the agent, team, and assigned role. The controller's delegate handler writes the role to the blackboard, and a direct-write fallback ensures the role is set even if delegate binding timing is off.

### Staging Zone Coordination

`AQuidditchStagingZone` actors (declared in `Public/Code/Quidditch/QuidditchStagingZone.h`) have `EditInstanceOnly` properties for `ZoneIdentifier` (FName like "TeamA_Seeker"), `TeamHint` (int32, 0 for TeamA, 1 for TeamB), and `RoleHint` (int32 mapping to role enum). These zones register as AI perception stimuli, and agents discover them through `BTService_FindStagingZone`. The "Bee and Flower" pattern means zones simply broadcast their existence, and agents decide if a zone matches their assignment. Landing detection happens in `AIC_QuidditchController` (lines 933-1012) where the controller binds to the pawn's `OnActorBeginOverlap`, reads the zone's hints, compares against the agent's team and role, and if matched, calls `HandleAgentReachedStagingZone()` on the GameMode. The GameMode increments a ready count, broadcasts `OnAgentReadyAtStart`, and when all agents are ready, transitions to the Countdown state.

### Ball Classes

The Golden Snitch (`ASnitchBall`, declared in `Public/Code/Quidditch/SnitchBall.h`) implements blend-based steering with four movement influences prioritized from highest to lowest: obstacle avoidance (floors and walls via five-direction raycasting), evasion (flee from nearby Seekers using inverse distance weighting), boundary constraint (soft push toward center at 80% of play area extent), and wander (random direction changes every two seconds with a 20% bias toward center). Its base speed is 600 units per second, ramping to 1200 during active evasion. Catch detection uses an overlap sphere, and when triggered, the GameMode's `NotifySnitchCaught()` awards 150 points and ends the match. The Quaffle and Bludger have BT services declared (`BTService_FindQuaffle`, `BTService_FindBludger`) and task classes (`BTTask_ThrowQuaffle`, `BTTask_HitBludger`) but their full implementations are stretch goals for after the demo.

### Synchronization Delegates

The GameMode declares nine key delegates following the Gas Station pattern. `OnMatchStateChanged` fires on every state transition. `OnAgentReadyAtStart` fires when each agent reaches its staging zone, carrying the ready count for HUD display. `OnAllAgentsReady` triggers the countdown sequence. `OnMatchStarted` fires with a countdown duration parameter, causing controllers to set `BB.MatchStarted = true`. `OnMatchEnded` triggers cleanup. `OnQuidditchRoleAssigned` fires during registration. `OnAgentSelectedForSwap` and `OnTeamSwapComplete` handle dynamic team rebalancing when a player joins. `OnSnitchCaught` fires with the catching seeker and winning team for results display.

---

## 5. UI Widget Implementation and Editor Setup

### Widget Architecture

The UI system uses a modular, delegate-driven Observer pattern with clear separation between legacy and modern implementations. The widget hierarchy descends from `UUserWidget` into `UWizardJamHUDWidget` (modern gameplay HUD), `UWizardJamQuidditchWidget` (scoreboard), `UWizardJamResultsWidget` (match results), `UMainMenuWidget` (navigation), `UOverheadBarWidget` (floating AI bars), `UTooltipWidget` (interaction prompts), `UButtonWidgetComponent` (reusable button), and the legacy `UPlayerHUD` (being phased out).

### Modern HUD Widget

`UWizardJamHUDWidget` (declared in `Public/Code/UI/WizardJamHUDWidget.h` at 249 lines, implemented at 598 lines) is the primary gameplay interface. It binds to four component delegate chains: `AC_HealthComponent::OnHealthChanged` drives a progress bar with color gradient (green above 60%, yellow above 30%, red below), `AC_StaminaComponent::OnStaminaChanged` drives a stamina bar, `AC_SpellCollectionComponent::OnChannelAdded` updates spell slot visuals, and `AC_BroomComponent` fires `OnFlightStateChanged`, `OnBoostStateChanged`, `OnBroomStaminaVisualUpdate`, and `OnForcedDismount` for flight-related UI elements. The spell slot system uses a `TArray<FSpellSlotConfig>` defined in `Public/Code/Spells/SpellSlotConfig.h`, where each config entry has an FName `SpellTypeName` (like "Flame" or "Ice"), locked and unlocked textures, colors, and a `SlotIndex`. At `NativeConstruct()`, the widget builds `TMap` lookups from this array, matching config entries to Image widgets named "SpellSlot_0", "SpellSlot_1", and so on. Designers add new spells by appending to the array without touching C++ code.

The widget also dynamically creates a `UWizardJamQuidditchWidget` if `QuidditchWidgetClass` is set, adding it to the viewport at Z-order 1 (above the HUD at Z-order 0). Visibility is controlled via `ShowQuidditchUI()` and `HideQuidditchUI()` with `ESlateVisibility::Visible` and `ESlateVisibility::Collapsed`.

### Quidditch Scoreboard Widget

`UWizardJamQuidditchWidget` (declared at 155 lines, implemented at 265 lines) binds to three GameMode delegates: `OnQuidditchTeamScored` for score updates, `OnMatchStarted` for timer initialization, and `OnSnitchCaught` for end-of-match display. It displays team scores via `PlayerScoreText` and `AIScoreText`, a match timer in MM:SS format via `MatchTimerText`, and customizable team name labels.

### Results Widget

`UWizardJamResultsWidget` (declared at 220 lines, implemented at 386 lines) supports designer-configurable result types through a `TArray<FResultConfiguration>` (defined in `Public/Code/UI/ResultTypes.h`). Each configuration includes title text, subtitle, background image, and sound. The widget broadcasts `OnRestartRequested`, `OnMenuRequested`, and `OnAutoReturnTriggered` delegates, with the PlayerController handling actual navigation. An auto-return timer can be configured to return to the menu after a delay.

### Widget Instantiation from WizardPlayer

`AWizardPlayer::SetupHUD()` (lines 541-590 of `WizardPlayer.cpp`) iterates through `HUDWidgetClasses`, a `TArray<TSubclassOf<UUserWidget>>` property. For each class, it calls `CreateWidget<UUserWidget>()` and `AddToViewport(i)` where `i` is the array index, creating a Z-order stack. This array-based approach, using the widened `UUserWidget` base type rather than a specific HUD class, is the result of the legacy-to-modern migration documented in `CLAUDE.md`. Designers configure this array in `BP_WizardPlayer` by adding `WBP_WizardJamHUD` at index 0 and optionally `WBP_PlayerHUD` (legacy) at index 1. Both coexist without type conflicts.

### What Remains for Editor Implementation

The C++ side is complete for all modern widgets. What remains is Blueprint widget creation in the Unreal Editor. For the main HUD, a Widget Blueprint `WBP_WizardJamHUD` must be created with a Canvas Panel hierarchy containing `HealthProgressBar` and `StaminaProgressBar` (as `UProgressBar` widgets matching `BindWidget` expectations), a `SpellSlotContainer` with Image children named "SpellSlot_0" through "SpellSlot_3", and optional elements like `BroomIcon`, `BoostIndicatorImage`, and `OutOfStaminaWarningText` (as `BindWidgetOptional` elements that gracefully handle absence). For the Quidditch scoreboard, `WBP_QuidditchHUD` needs `PlayerScoreText`, `AIScoreText`, and `MatchTimerText` TextBlocks. For results, `WBP_GameResults` needs title and subtitle TextBlocks, score display, and `UButtonWidgetComponent` instances for restart and menu buttons. For the main menu, `WBP_MainMenu` needs play and quit `UButtonWidgetComponent` instances. In all cases, the C++ code finds widgets by name at `NativeConstruct()` and binds delegates automatically.

---

## 6. Snitch Flight Behavior

### Movement Implementation

The Golden Snitch (`ASnitchBall`, implemented in `Private/Code/Quidditch/SnitchBall.cpp`) uses a blend-based steering algorithm in its `UpdateMovement()` method (lines 192-289). Four movement influences are calculated independently and then combined with priority weighting. The base behavior is wandering: every two seconds (plus or minus one second of randomization), the Snitch picks a new random direction with a slight vertical constraint (Z component limited to plus or minus 0.5) and a 20% bias toward the play area center. This prevents the Snitch from drifting monotonically toward boundaries while maintaining unpredictable movement.

When obstacles are detected, avoidance takes highest priority. The Snitch casts five rays (forward, down, left, right, and down-forward) at 300 units range. Hit results produce away-vectors weighted by proximity: an obstacle at half the detection range produces twice the avoidance force of one at full range. The avoidance vector is added to the current direction and renormalized.

Evasion activates when Seekers (identified via perception from `AIC_SnitchController`) enter the 800-unit `EvadeRadius`. The `CalculateEvadeVector()` method uses inverse distance weighting across all current pursuers, summing away-vectors where each is scaled by `1.0 - (Distance / EvadeRadius)`. This means a Seeker at 200 units produces four times the evasion force of one at 600 units. When evading, the Snitch's speed interpolates from `BaseSpeed` (600) toward `MaxEvadeSpeed` (1200) using `FMath::FInterpTo` with a rate of 5.0, creating a visible acceleration burst that communicates urgency to players.

Boundary constraint applies a soft push force starting at 80% of the play area extent, with increasing strength as the Snitch approaches 100%. This is applied per-axis, so the Snitch can be near the X boundary without affecting Y movement. Height is hard-clamped between `MinHeightAboveGround` (100 units) and `MaxHeightAboveGround` (2000 units) via a downward trace to find ground level.

### Perception Integration

The Snitch's perception works bidirectionally. The `AIC_SnitchController` detects Seekers via 360-degree sight with 2000-unit radius and 0.5-second max age. When a valid pursuer is detected (checked against `ValidPursuerTags`), the controller broadcasts `OnPursuerDetected`, and the SnitchBall's bound handler adjusts evasion parameters. When a pursuer leaves detection, `OnPursuerLost` fires and the Snitch can relax back to base speed. The controller's `GetCurrentPursuers()` method provides the current pursuer list to the Snitch's movement calculation without requiring the Snitch to directly query the perception system.

### Seeker Chase Task

`BTTask_ChaseSnitch` (declared in `Public/Code/AI/Quidditch/BTTask_ChaseSnitch.h`, implemented in `Private/Code/AI/Quidditch/BTTask_ChaseSnitch.cpp`) is the Seeker's primary engagement task. It reads `SnitchLocation` from the blackboard (updated by `BTService_FindSnitch` at 0.1-second intervals), calculates altitude difference with a tolerance of 100 units, applies vertical input proportional to the delta, enables boost when distance exceeds `BoostDistanceThreshold` (1000 units), and sets horizontal velocity directly toward the Snitch. Rotation is smoothly interpolated on the horizontal plane only using `FMath::RInterpTo` with a rate of 5.0. The task succeeds when the Seeker enters the `CatchRadius` of 200 units, at which point the Snitch's overlap sphere triggers the actual catch event.

### Opportunities for Enhancement

The current system already combines raycasting (obstacle avoidance) with perception (pursuer detection), creating a hybrid approach. Further enhancement could include predictive evasion using `AC_FlightSteeringComponent::CalculateSteeringTowardWithPrediction()` in reverse (predicting where Seekers will be), adding altitude-based evasion strategies (diving or climbing when pursued from the side), and integrating the flocking component so multiple Seekers create coordinated pincer movements rather than clustering.

---

## 7. Code Quality and Architecture Patterns

### Observer Pattern Compliance

The codebase demonstrates consistent, project-wide adherence to the Observer pattern. Every cross-system communication pathway uses delegate broadcasts rather than direct method calls or Tick-based polling. Delegate declarations found across the codebase include `FOnHealthChanged`, `FOnHealed`, `FOnDeath`, and `FOnDeathEnded` in `AC_HealthComponent`; `FOnFlightStateChanged`, `FOnBroomStaminaVisualUpdate`, `FOnForcedDismount`, and `FOnBoostStateChanged` in `AC_BroomComponent`; `FOnStaminaChanged` and `FOnStaminaDepleted` in `AC_StaminaComponent`; `FOnQuidditchTeamScored`, `FOnGameModeRoleAssigned`, and `FOnGameModeSnitchCaught` in `QuidditchGameMode`; and `FOnPursuerPerceptionChanged` in `AIC_SnitchController`. All bindings occur at `BeginPlay()` or `OnPossess()`, and all unbindings occur at `EndPlay()` or `OnUnPossess()`, preventing dangling delegate references.

### UPROPERTY Specifier Compliance

The project follows a strict "no `EditAnywhere`" rule across nearly all files. Properties are consistently marked `EditDefaultsOnly` for designer-configurable class defaults (movement speeds, stamina rates, perception radii) and `EditInstanceOnly` for per-level placement settings (play area volume references, zone identifiers). Runtime state is marked `VisibleAnywhere, BlueprintReadOnly` for component references and `BlueprintAssignable` for all delegates. Two violations were identified in `WizardPlayer.h` at lines 189 and 193, where team and Quidditch role properties use `EditAnywhere, BlueprintReadWrite` instead of `EditDefaultsOnly`. These should be corrected to maintain standards consistency.

### Constructor Initialization Lists

Every class examined uses constructor initialization lists correctly. `UAC_BroomComponent`'s constructor (lines 31-54 of its implementation) initializes over twenty properties in the initialization list before the body, including all pointer initializations to nullptr, numeric defaults, and FName values. `AWizardPlayer`'s constructor initializes `PlayerTeamID`, `QuidditchRole`, `BallThrowForce`, `HeldBall`, and `HeldBallActor` before the body where `CreateDefaultSubobject` calls create components. No header-default initialization patterns were found, and no improper BeginPlay-based initialization was detected.

### Forward Declarations

Headers consistently use forward declarations for types only referenced by pointer or reference. `WizardPlayer.h` forward-declares `UAC_BroomComponent`, `UAC_StaminaComponent`, `UAC_HealthComponent`, `UInteractionComponent`, `UInputAction`, and `UUserWidget`, with full includes only appearing in the corresponding .cpp file. The `.generated.h` include appears last in every header, conforming to the UHT requirement.

### Forbidden Pattern Absence

No instances of `ConstructorHelpers` were found anywhere in the codebase. `UGameplayStatics` usage is limited to a single fallback case in `BTService_FindSnitch.cpp` (line 171), where `GetAllActorsOfClass` is used only when perception-based discovery fails. The primary discovery path uses the perception system. No Tick-based polling for game state was found; all state synchronization flows through delegates. World queries use `TActorIterator` instead of `GameplayStatics::GetAllActorsOfClass` or `GetAllActorsWithTag`.

### Logging Practices

Every source file defines a log category via `DEFINE_LOG_CATEGORY` with a corresponding `DECLARE_LOG_CATEGORY_EXTERN` in the header. Log messages include the actor name in brackets for context, use `TEXT()` macro for all string literals, and employ appropriate severity levels: `Display` for normal state changes, `Warning` for recoverable issues, `Error` for failures, and `Verbose` for per-frame debugging. The project also uses structured logging via `SLOG_EVENT` macros from the StructuredLogging plugin for Datadog integration, adding metadata like detection radii and speed values to events.

### Stubbed Functions and TODOs

The Quaffle and Bludger ball classes have BT service and task headers declared but implementations are incomplete, as these are intentionally deferred stretch goals. The `OverheadBarWidget` has update functions but no visible consumer code for binding and initialization, suggesting it needs an `AC_OverheadBarComponent` to manage it on AI agents. The `TooltipWidget` has text update methods but no interactable system consumer code shown, indicating the interaction tooltip display pipeline needs wiring.

---

## 8. Integration Points and Dependencies

### Data Flow Chain

The complete data flow from perception to physical movement follows a three-step chain. First, the `UAIPerceptionComponent` on `AIC_QuidditchController` detects a stimulus source (like a `BroomCollectible` with `UAIPerceptionStimuliSourceComponent`) and fires `HandlePerceptionUpdated()`, which writes the perceived actor to a blackboard key such as `PerceivedCollectible`. Second, BT services like `BTService_FindCollectible` query `GetCurrentlyPerceivedActors()` on their tick interval and write refined results (nearest matching collectible, correct staging zone) to specific blackboard keys. Third, BT tasks like `BTTask_ControlFlight` read the target from the blackboard and translate it into movement commands by calling `AC_BroomComponent::SetVerticalInput()` for altitude and setting `CharacterMovementComponent::Velocity` directly for horizontal flight.

### Interface Contracts

Three primary interfaces enable loose coupling. `IInteractable` (in `Public/Code/Interfaces/Interactable.h`) provides `OnInteract()`, `CanInteract()`, `GetTooltipText()`, `GetInteractionPrompt()`, and `GetInteractionRange()`, implemented by `ABroomActor` for mounting. `IPickupInterface` (in `Public/Code/PickupInterface.h`) provides `CanPickHealth()`, `CanPickAmmo()`, and `AddMaxAmmo()`, implemented by player and AI characters for collectible acceptance. `IQuidditchAgent` (in `Public/Code/Quidditch/IQuidditchAgent.h`) provides `GetQuidditchRole()`, `IsOnBroom()`, `TryMountBroom()`, and `GetFlockMembers()`, implemented by both `AWizardPlayer` and AI agent classes for Quidditch coordination. The `IGenericTeamAgentInterface` (engine-provided) enables AI perception filtering by team affiliation.

### Component Dependency Map

`AC_BroomComponent` requires `AC_StaminaComponent` for stamina checks (bound via delegate at BeginPlay), `UCharacterMovementComponent` for movement mode switching and velocity control, and optionally `UEnhancedInputComponent` for player input (skipped for AI). `ABroomActor` requires the `IInteractable` interface for mounting, dynamically creates `AC_BroomComponent`, checks `AC_SpellCollectionComponent` for channel requirements, and uses `UAIPerceptionStimuliSourceComponent` for AI detection. `AIC_QuidditchController` requires `UAIPerceptionComponent` for perception, `UBlackboardComponent` and `UBehaviorTreeComponent` for AI decision-making, and binds to `AQuidditchGameMode` delegates for match state synchronization.

### Build Configuration

The `END2507.Build.cs` file declares public dependencies on `NavigationSystem`, `GameplayTasks`, `Core`, `CoreUObject`, `Engine`, `InputCore`, `EnhancedInput`, `AnimGraphRuntime`, `UMG`, `AIModule`, `Niagara`, and `ProceduralMeshComponent`. Private dependencies include `Slate`, `SlateCore`, and `StructuredLogging`. The `.uproject` file specifies UE 5.4 with `ModelingToolsEditorMode` (editor-only) and `StructuredLogging` plugins enabled.

### Blueprint Class Inheritance

The critical Blueprint classes that must exist in the editor are: `BP_QuidditchGameMode` extending `AQuidditchGameMode` (sets `DefaultPawnClass` and class defaults), `BP_CodeWizardPlayer` extending `AWizardPlayer` (configures `HUDWidgetClasses` array and input actions), `BP_QuidditchAgent` extending the AI agent base class (configures AI properties), `BP_QuidditchAIController` extending `AIC_QuidditchController` (sets behavior tree and blackboard assets), `BP_BroomCollectible_C` extending `ABroomCollectible` (configures broom visual and channel grant), and `BP_QuidditchStagingZone` extending `AQuidditchStagingZone` (sets team and role hints per instance).

---

## 9. Short-Term Priority: Quidditch Demo (Seeker vs. Seeker)

This section provides a micro-increment step-by-step plan to achieve a working demo of two AI agents (Seeker vs. Seeker) flying and interacting in the editor. Each increment builds on the previous one and includes verification steps.

### Increment 1: Verify Build and Existing Assets

Open the project in Unreal Editor and perform a full build to confirm compilation succeeds. Navigate to `Content/Code/AI/` and verify that `BT_QuidditchAI.uasset` and `BB_QuidditchAI.uasset` exist. Open the Blackboard asset and confirm all expected keys are present with correct types: `IsFlying` (Bool), `HasBroom` (Bool), `MatchStarted` (Bool), `TargetLocation` (Vector), `SnitchLocation` (Vector), `SnitchVelocity` (Vector), `StagingZoneLocation` (Vector), `PerceivedCollectible` (Object), `SelfActor` (Object), `QuidditchRole` (Name), `HomeLocation` (Vector), `ReachedStagingZone` (Bool), and `IsReady` (Bool). If any keys are missing, add them now. Open the Behavior Tree asset and confirm the major sequences (AcquireBroom, FlyToStart, QuidditchMatch) are connected with the correct tasks, services, and decorators.

**Verification:** Project compiles cleanly. Blackboard has all required keys. Behavior Tree node graph is visible and connected.

### Increment 2: Verify or Create Blueprint Classes

Check that `BP_QuidditchGameMode` exists as a child of `AQuidditchGameMode`. In its Class Defaults, set `DefaultPawnClass` to `BP_CodeWizardPlayer` (or the player Blueprint), and configure `MaxSeekersPerTeam` to 1 per team. Check that `BP_QuidditchAIController` exists as a child of `AIC_QuidditchController`. In its defaults, set the `BehaviorTreeAsset` to `BT_QuidditchAI` and `BlackboardAsset` to `BB_QuidditchAI`. Verify the `SightRadius` is 2000 and `PeripheralVisionAngle` is 90. Check that `BP_BroomCollectible_C` exists as a child of `ABroomCollectible` with a visual mesh assigned, the channel grant set to "Broom", and `UAIPerceptionStimuliSourceComponent` present and configured. Check that `BP_QuidditchStagingZone` exists as a child of `AQuidditchStagingZone` with a collision volume for overlap detection and `UAIPerceptionStimuliSourceComponent` present.

**Verification:** All Blueprint classes exist. Class defaults are correctly assigned. Perception stimuli components are visible in each Blueprint's component list.

### Increment 3: Configure the Demo Level

Create or open the Quidditch demo level. Set the level's World Settings to use `BP_QuidditchGameMode` as the GameMode Override. Place two `BP_BroomCollectible_C` actors in the level at accessible ground-level positions, separated by at least 500 units so agents do not compete for the same broom. Place two `BP_QuidditchStagingZone` actors at elevated positions (Z offset of approximately 500-800 units) representing starting positions. Configure the first zone with `TeamHint = 0` and `RoleHint` matching the Seeker enum value. Configure the second zone with `TeamHint = 1` and the same `RoleHint`. Place one `BP_GoldenSnitch` (the Snitch Blueprint) at a central elevated position. Set its `PlayAreaVolumeRef` to a Box Trigger actor that defines the play area bounds, or manually configure `PlayAreaCenter` and `PlayAreaExtent`. Optionally place a Nav Mesh Bounds Volume covering the ground area where agents will walk before mounting brooms.

**Verification:** Level contains two broom collectibles, two staging zones (one per team), one Snitch, and a play area volume. World Settings shows the Quidditch GameMode.

### Increment 4: Spawn AI Agents

In the level, place two AI agent pawns (using whatever Blueprint child of the base agent class has been configured for Quidditch, likely `BP_QuidditchAgent`). Configure the first agent's `AIControllerClass` to `BP_QuidditchAIController` and set its default properties for Team 0 with preferred role Seeker. Configure the second agent similarly for Team 1 with preferred role Seeker. Alternatively, if agents are spawned by the GameMode at runtime, configure the GameMode's spawn settings with two agent entries (one per team) and ensure spawn points are placed in the level.

**Verification:** Press Play. In the Output Log, look for `[AIC_QuidditchController] SetupBlackboard` messages indicating both controllers initialized. Look for `RegisterQuidditchAgent` logs showing both agents received Seeker role assignments. Both agents should be visible in the level at their spawn points.

### Increment 5: Verify Broom Acquisition

With the level playing, observe both agents. They should begin executing the AcquireBroom sequence: `BTService_FindCollectible` should detect the `BP_BroomCollectible_C` actors via perception. In the Output Log, look for perception update messages indicating collectibles were detected. The agents should navigate toward the brooms (ground movement using Nav Mesh). Upon reaching the broom, the agent interacts with it, and `BTTask_MountBroom` fires `SetFlightEnabled(true)`. Look for `[AC_BroomComponent] Flight enabled` log messages.

**Verification:** Both agents walk to brooms and mount them. The `IsFlying` and `HasBroom` blackboard keys transition to true (visible in the Behavior Tree debugger by selecting an agent and opening the BT editor's Blackboard panel).

### Increment 6: Verify Staging Zone Navigation

After mounting brooms, agents should enter the FlyToStart sequence. `BTService_FindStagingZone` should detect staging zones via perception and filter to the correct one based on team and role hints. `BTTask_FlyToStagingZone` should fly the agent to the zone's location. Observe flight behavior: agents should ascend, fly horizontally, and arrive at their respective staging zones. Look for arrival detection logs in the Output Log.

**Verification:** Both agents fly to their correct staging zones (Team 0 agent to Team 0 zone, Team 1 to Team 1 zone). The GameMode should log ready count updates. When both agents are ready, the match state should transition to WaitingForReady and then Countdown.

### Increment 7: Verify Snitch Chase

When `OnMatchStarted` broadcasts and the blackboard's `MatchStarted` key transitions to true, agents should enter the Quidditch match sequence. As Seekers, they should activate `BTService_FindSnitch` to detect the Golden Snitch via perception. `BTTask_ChaseSnitch` should drive them toward the Snitch's location with altitude-matching, boost activation at distance, and smooth rotation toward the target. The Snitch should react to pursuers by increasing speed and evading.

**Verification:** Both agents actively chase the Snitch. The Snitch visibly accelerates and changes direction when agents approach. Flight paths show direct pursuit rather than circular orbits. Stamina does not deplete during normal flight (only during boost).

### Increment 8: Verify Catch and Match End

When a Seeker enters the Snitch's catch radius (200 units), the overlap event fires. The Snitch broadcasts catch notification to the GameMode. The GameMode should transition to `SnitchCaught` state, award 150 points to the catching team, and broadcast `OnSnitchCaught`. If the Results widget is configured, it should appear showing the winning team.

**Verification:** One agent catches the Snitch. Output Log shows "SnitchCaught" and score assignment. Match state transitions to Ended. If UI is configured, results display correctly.

### Debugging Guidance

If agents do not detect brooms, check that `BP_BroomCollectible_C` has `UAIPerceptionStimuliSourceComponent` enabled and that the controller's sight radius (2000 units) is sufficient for the placement distance. Enable `AI Debug` view in the editor (apostrophe key) to visualize perception cones. If agents detect brooms but do not move toward them, open the Behavior Tree debugger (select agent, Window, Behavior Tree) and check which node is active. Verify that `BTService_FindCollectible`'s `OutputKey` is correctly assigned to the `PerceivedCollectible` blackboard key. If agents mount brooms but do not fly, check that `AC_BroomComponent` is present on the agent pawn and that `CharacterMovementComponent` supports MOVE_Flying (it does by default). If agents fly but orbit their target, verify that `BTTask_ControlFlight` or `BTTask_FlyToStagingZone` uses direct velocity assignment rather than `VInterpTo`. If the Snitch does not move, verify it has an `AIC_SnitchController` assigned as its AIControllerClass and that `AutoPossessAI` is set to "Placed in World" or "Spawned".

---

## 10. Stretch Goals: Elemental Scoring and Spell Collection

### Conceptual Framework

The elemental system would assign each AI agent an elemental affinity (Flame, Ice, Lightning, or Arcane) using the existing FName-based spell type system from `FSpellSlotConfig`. Agents would be visually distinguished by color-coded materials applied through the `SetupAgentAppearance()` pattern from the GAR project, creating dynamic materials per mesh slot at BeginPlay. The elemental affinity would determine which spell collectibles the agent prioritizes and what scoring bonuses apply.

### Elemental Agent Design

Each agent would have an `ElementalAffinity` FName property (EditDefaultsOnly) set in its Blueprint defaults. A new `BTService_FindElementalCollectible` would extend `BTService_FindCollectible` to prioritize collectibles matching the agent's affinity. When an agent collects a matching spell, it receives a scoring multiplier (configurable, perhaps 1.5x) for subsequent actions like Quaffle goals or Bludger hits. Collecting a non-matching spell provides the base score with no multiplier. This creates strategic depth without requiring new C++ spell combination systems.

### Scoring Mechanics

The Quidditch scoring system would extend beyond the Snitch catch. Quaffle goals through hoops would award base points (10), multiplied by the scoring agent's elemental multiplier if active. Bludger hits that dismount an opponent would award base points (5) with the same multiplier system. The Snitch catch remains at 150 points (unmultiplied, as it's the match-ending event). A new `FQuidditchScoreEvent` struct could carry the scoring agent, event type, base points, multiplier, and final score, broadcast via a `OnQuidditchScoreEvent` delegate for the scoreboard widget to display.

### Spell Collection Integration

Spell collectibles (`ASpellCollectible`, already implemented with auto-color and material system) would be placed throughout the arena as mid-air pickups. Collection during flight would use the existing `IPickupInterface` and `AC_SpellCollectionComponent`. The HUD's spell slot system already supports dynamic unlock visualization through the delegate binding in `UWizardJamHUDWidget::HandleChannelAdded()`. No new UI code would be needed for displaying collected spells.

### Prerequisites Before Implementation

Before elemental scoring can be implemented, the demo must be working with basic Seeker vs. Seeker gameplay. The Quaffle and Bludger ball classes need their movement and interaction implementations completed. The scoring system in `QuidditchGameMode` needs a `FQuidditchScoreEvent` struct and associated delegate. The agent appearance system needs the dynamic material pattern ported from the GAR project. The spell collectible placement system needs to support mid-air spawning (currently ground-based).

### Incremental Roadmap

The first phase would add elemental affinity to agents without scoring changes, purely as a visual distinction. The second phase would implement `BTService_FindElementalCollectible` and spell collection during flight. The third phase would add the scoring multiplier system with `FQuidditchScoreEvent` broadcasts. The fourth phase would implement Quaffle goals and Bludger hits as scoring events. The fifth phase would polish the scoreboard widget to show multiplier indicators and elemental icons.

---

## 11. Recommendations and Next Steps

### Critical Path to Demo

The shortest path to a working Seeker vs. Seeker demo follows three primary tracks. The first track is asset verification: confirming all Blueprint classes exist with correct class defaults, perception components are registered, and the Behavior Tree asset is properly configured. This is pure editor work requiring no code changes. The second track is level setup: placing brooms, staging zones, the Snitch, and agent spawners with correct team and role configurations. Again, this is editor work. The third track is play-testing and debugging: running the level, observing agent behavior at each stage (walk to broom, mount, fly to staging zone, chase Snitch), and using the Behavior Tree debugger and Output Log to diagnose any breakdowns. The C++ systems are production-ready. The bottleneck is entirely in editor configuration and asset wiring.

### Recommended Step-by-Step Editor Tasks

Begin by opening the project and doing a full build to verify compilation. Then open each Blueprint class listed in Increment 2 and confirm class defaults. Next, build the demo level following Increment 3. Then run the level and observe each behavior phase sequentially, fixing any configuration issues discovered. If agents fail at a particular phase, use the debugging guidance in the Increment 8 section to identify the root cause. Prioritize getting agents through the full AcquireBroom and FlyToStart sequences before worrying about the Snitch chase, as the early phases exercise the most integration points.

### Post-Demo Enhancement Order

Once the demo works, enhancements should follow this logical order. First, implement broom orientation and banking during flight by adding pitch and roll adjustments based on vertical input and turn rate in `AC_BroomComponent::TickComponent()`. Second, integrate `AC_FlightSteeringComponent`'s obstacle avoidance into `BTTask_ControlFlight` for arena-aware flight paths. Third, add acrobatic maneuvers (barrel rolls, dives) as triggered animations during Snitch pursuit. Fourth, implement hybrid raycast-plus-perception flight where agents use raycasts for immediate obstacle avoidance and perception for strategic target tracking. Fifth, enable the flocking component for multi-agent coordination when expanding beyond Seeker-only gameplay.

### Quick Wins

Several improvements can be achieved with minimal effort. Adding `DrawDebugSphere` calls at staging zone locations and Snitch position during development helps visualize the demo setup. Enabling `ShowDebug AI` in the console during play provides real-time perception and blackboard visualization. Setting the Snitch's `BaseSpeed` lower (400 instead of 600) for initial testing makes catches happen faster, allowing more complete match flow testing. Temporarily reducing `SightRadius` on agents to force closer engagement creates more visible, dramatic gameplay for demos.

### Performance Considerations

The current architecture is well-optimized for a small number of agents. BT services tick at configurable intervals (0.1-0.5 seconds) rather than every frame. Perception updates are event-driven rather than polled. Direct velocity assignment in flight tasks avoids expensive physics calculations. For scaling beyond the demo's two agents to a full Quidditch match (fourteen agents), the primary concern would be the flocking component's `TActorIterator` search, which could be optimized with spatial partitioning if needed. The Snitch's five-ray obstacle avoidance runs every tick on a single actor and is negligible in cost.

### Elemental Scoring Integration Notes

When adding elemental features after the demo, avoid modifying the existing scoring delegates. Instead, create a new `FQuidditchScoreEvent` wrapper that carries the base score alongside multiplier information, and add a new delegate that widgets bind to. This preserves the existing `OnQuidditchTeamScored` for backward compatibility while enabling the richer scoring display. The spell collection system already works at runtime through the `AC_SpellCollectionComponent` and `OnChannelAdded` delegate chain, so the primary work is placing spell collectibles at flight-accessible positions and implementing the `BTService_FindElementalCollectible` perception filter.

---

## Appendix: Complete File Reference

### Flight System
| File | Lines | Purpose |
|------|-------|---------|
| `Public/Code/Flight/BroomTypes.h` | 278 | Configuration struct with presets |
| `Public/Code/Flight/BroomActor.h` | 231 | Mountable world entity |
| `Private/Code/Flight/BroomActor.cpp` | 367 | Mounting/dismounting logic |
| `Public/Code/Flight/AC_BroomComponent.h` | 264 | Core flight component (input-agnostic API) |
| `Private/Code/Flight/AC_BroomComponent.cpp` | 791 | Flight mechanics, stamina, input |
| `Public/Code/Flight/AC_FlightSteeringComponent.h` | 246 | Obstacle-aware steering |
| `Private/Code/Flight/AC_FlightSteeringComponent.cpp` | 512 | Steering calculations with traces |
| `Public/Code/Utilities/FlyingSteeringComponent.h` | 246 | Flocking behaviors (Flock.cs port) |
| `Private/Code/Utilities/FlyingSteeringComponent.cpp` | 407 | Flocking implementations |

### AI Behavior Tree
| File | Lines | Purpose |
|------|-------|---------|
| `Public/Code/AI/AIC_QuidditchController.h` | 259 | Primary AI controller |
| `Private/Code/AI/AIC_QuidditchController.cpp` | 1014 | Perception, blackboard, GameMode sync |
| `Public/Code/AI/AIC_SnitchController.h` | 117 | Snitch perception controller |
| `Private/Code/AI/AIC_SnitchController.cpp` | 181 | Pursuer detection |
| `Public/Code/AI/BTTask_ControlFlight.h` | 158 | Continuous flight control |
| `Private/Code/AI/BTTask_ControlFlight.cpp` | 476 | Flight control implementation |
| `Public/Code/AI/BTTask_FlyToStagingZone.h` | 236 | Staging zone navigation |
| `Private/Code/AI/BTTask_FlyToStagingZone.cpp` | 658 | Multi-strategy arrival detection |
| `Public/Code/AI/BTTask_MountBroom.h` | 70 | Mount/dismount task |
| `Private/Code/AI/BTTask_MountBroom.cpp` | 173 | Broom component toggling |
| `Public/Code/AI/BTTask_ReturnToHome.h` | 117 | Fallback navigation |
| `Private/Code/AI/BTTask_ReturnToHome.cpp` | 423 | Return to spawn |
| `Public/Code/AI/Quidditch/BTTask_ChaseSnitch.h` | - | Seeker chase behavior |
| `Private/Code/AI/Quidditch/BTTask_ChaseSnitch.cpp` | - | Pursuit implementation |
| `Public/Code/AI/BTService_SyncFlightState.h` | 49 | Flight state synchronization |
| `Private/Code/AI/BTService_SyncFlightState.cpp` | 117 | Blackboard sync |
| `Public/Code/AI/BTService_FindCollectible.h` | 74 | Collectible perception |
| `Private/Code/AI/BTService_FindCollectible.cpp` | 160 | Perception query |
| `Public/Code/AI/Quidditch/BTService_FindStagingZone.h` | 72 | Staging zone discovery |
| `Private/Code/AI/Quidditch/BTService_FindStagingZone.cpp` | 290 | Team/role filtering |
| `Public/Code/AI/Quidditch/BTService_FindSnitch.h` | - | Snitch tracking |
| `Private/Code/AI/Quidditch/BTService_FindSnitch.cpp` | - | Perception + world fallback |
| `Public/Code/AI/BTDecorator_HasChannel.h` | 56 | Channel gate decorator |

### Quidditch Infrastructure
| File | Lines | Purpose |
|------|-------|---------|
| `Public/Code/GameModes/QuidditchGameMode.h` | 200+ | Match management |
| `Private/Code/GameModes/QuidditchGameMode.cpp` | - | State machine, registration |
| `Public/Code/Quidditch/QuidditchTypes.h` | 249 | Enums, delegates, structs |
| `Public/Code/Quidditch/SnitchBall.h` | - | Golden Snitch actor |
| `Private/Code/Quidditch/SnitchBall.cpp` | - | Blend-based steering |
| `Public/Code/Quidditch/QuidditchStagingZone.h` | - | Staging zone actor |
| `Public/Code/Quidditch/IQuidditchAgent.h` | - | Agent interface |

### UI Widgets
| File | Lines | Purpose |
|------|-------|---------|
| `Public/Code/UI/WizardJamHUDWidget.h` | 249 | Modern gameplay HUD |
| `Private/Code/UI/WizardJamHUDWidget.cpp` | 598 | Delegate binding, spell slots |
| `Public/Code/UI/WizardJamQuidditchWidget.h` | 155 | Quidditch scoreboard |
| `Private/Code/UI/WizardJamQuidditchWidget.cpp` | 265 | Score/timer display |
| `Public/Code/UI/WizardJamResultsWidget.h` | 220 | Match results |
| `Private/Code/UI/WizardJamResultsWidget.cpp` | 386 | Result configuration |
| `Public/Code/UI/ResultTypes.h` | 171 | Result data types |
| `Public/Code/Spells/SpellSlotConfig.h` | 151 | Spell slot configuration |
| `Public/Code/UI/OverheadBarWidget.h` | 84 | AI floating bars |
| `Public/Code/UI/TooltipWidget.h` | 44 | Interaction tooltips |

### Player and Actors
| File | Lines | Purpose |
|------|-------|---------|
| `Public/Code/Actors/WizardPlayer.h` | 246 | Player character |
| `Private/Code/Actors/WizardPlayer.cpp` | 637 | HUD creation, delegates |
| `Public/Code/Actors/BroomCollectible.h` | 48 | Broom pickup |
| `Public/Code/Actors/WorldSignalEmitter.h` | - | Global signal bus |

### Interfaces
| File | Purpose |
|------|---------|
| `Public/Code/Interfaces/Interactable.h` | Interaction contract |
| `Public/Code/PickupInterface.h` | Pickup acceptance |
| `Public/Code/Quidditch/IQuidditchAgent.h` | Quidditch agent contract |

### Build Configuration
| File | Purpose |
|------|---------|
| `Source/END2507/END2507.Build.cs` | Module dependencies |
| `WizardJam2.0.uproject` | Project configuration |
| `Config/DefaultEngine.ini` | GameMode and map defaults |
| `Config/DefaultInput.ini` | Enhanced Input settings |
