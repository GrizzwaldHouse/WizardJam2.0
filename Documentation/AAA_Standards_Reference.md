# WizardJam AI System - AAA Standards Reference

Developer: Marcus Daley | Architecture Lead: Nick Penney AAA Standards
Engine: Unreal Engine 5.4 | Date: February 2026

---

## Purpose

This document explains WHY each design decision in the WizardJam Quidditch AI system
meets or exceeds AAA game development standards. It serves as both a portfolio reference
and an engineering rationale document for interviews, code reviews, and future contributors.

---

## 1. MODULAR BEHAVIOR TREE NODE LIBRARY

### Industry Standard

AAA studios (Naughty Dog, Ubisoft Montreal, CD Projekt RED) build reusable BT node
libraries that serve 60-80% of AI needs across roles. Role-specific behavior comes from
composing shared nodes with role-specific services and decorators, not from monolithic
per-role behavior trees.

### WizardJam Implementation

**28 BT nodes total across the system:**

| Category | Count | Nodes |
|----------|-------|-------|
| Shared Tasks | 9 | ControlFlight, FlyToStagingZone, MountBroom, WaitForMatchStart, Interact, CheckBroomChannel, ReturnToHome, SwapTeam, ForceMount (debug) |
| Shared Services | 4 | FindCollectible, FindInteractable, FindStagingZone, SyncFlightState |
| Shared Decorators | 2 | HasChannel, IsSeeker |
| Seeker Tasks | 3 | ChaseSnitch, PredictIntercept, CatchSnitch |
| Seeker Services | 1 | FindSnitch |
| Chaser Tasks | 2 | FlockWithTeam, ThrowQuaffle |
| Chaser Services | 1 | FindQuaffle |
| Beater Tasks | 1 | HitBludger |
| Beater Services | 1 | FindBludger |
| Keeper Tasks | 2 | PositionInGoal, BlockShot |
| Snitch AI | 2 | TrackNearestSeeker (Service), EvadeSeeker (Task) |

**Reuse pattern:** BTTask_ControlFlight, BTTask_FlyToStagingZone, BTTask_MountBroom,
BTTask_WaitForMatchStart, BTTask_ReturnToHome, BTService_FindStagingZone, and
BTService_SyncFlightState are SHARED across all roles. Only 2-3 nodes per role are
role-specific.

**Compliance:** 100% of nodes pass AddObjectFilter + InitializeFromAsset audit.
Every FBlackboardKeySelector property has both constructor filter and runtime resolution.

### Why This Matters

Shared nodes reduce maintenance burden exponentially. When a flight control bug is
fixed in BTTask_ControlFlight, it is fixed for ALL roles simultaneously. Role-specific
behavior is achieved through composition (combining shared nodes with role-specific
services), not through inheritance (duplicating entire BT branches). Adding a new role
requires only 2-3 new nodes, not rebuilding the entire acquisition-flight-staging
pipeline.

**Portfolio Significance:** This demonstrates understanding of software engineering
fundamentals applied to game AI. The node library is organized the same way AAA studios
organize theirs -- a shared foundation layer with role-specific extensions.

---

## 2. FLIGHT AI: DIRECT VELOCITY CONTROL

### Industry Standard

3D flight AI in AAA titles (Ace Combat 7, Star Wars Squadrons, Crimson Skies, Anthem)
uses direct physics and velocity control, NOT NavMesh pathfinding. NavMesh is designed
for 2D walkable surfaces and cannot represent 3D aerial space. NavMesh-based flight
causes agents to either ignore vertical movement entirely or attempt to walk along
invisible navmesh surfaces.

### WizardJam Implementation

**Zero NavMesh usage** confirmed across entire AI directory (28 nodes, 3 controllers).

Key flight tasks use direct BroomComponent Core API:

| Task | Flight Method | Purpose |
|------|--------------|---------|
| BTTask_ControlFlight | `BroomComponent->SetVerticalInput()`, `BroomComponent->SetBoostEnabled()` | Generic flight to any BB target |
| BTTask_FlyToStagingZone | Direct velocity + 5 navigation solutions | Fly to staging zone with obstacle avoidance |
| BTTask_ChaseSnitch | Direct velocity pursuit + altitude management + boost control | Chase moving target |
| BTTask_ReturnToHome | Direct velocity with boost hysteresis | Return to staging zone fallback |

**5 Navigation Solutions in BTTask_FlyToStagingZone:**
1. FlightSteeringComponent integration for obstacle avoidance
2. Configurable ArrivalRadius with extended default (200-500 units)
3. Velocity-based arrival detection (catches high-speed overshoots)
4. Stuck detection with position history sampling
5. Timeout fallback system

**Core API Design Pattern:**
Both players and AI use the same BroomComponent functions:
```
Player: Input Handler -> SetVerticalInput(1.0)
AI:     BT Task      -> SetVerticalInput(calculated_value)
                          ^
                     SAME FUNCTION
```

This is identical to how AAA studios implement vehicle AI -- the AI calls the same
movement API as the player, ensuring behavior parity.

### Why This Matters

NavMesh baking is computationally expensive, doesn't support dynamic 3D volumes,
requires navmesh modifiers for every obstacle, and produces artifacts at volume
boundaries. Direct velocity gives full 3D freedom, zero baking overhead, and works
with any level geometry without additional setup. The 5-solution approach in
BTTask_FlyToStagingZone demonstrates defensive programming against edge cases
(overshoots, stuck states, timeouts) that AAA QA teams routinely flag.

---

## 3. OBSERVER PATTERN (GAS STATION PATTERN)

### Industry Standard

AAA event-driven architecture uses delegates and events for cross-system communication.
The "Gas Station Pattern" (from concurrent programming coursework) embodies the Observer
Pattern: the broadcaster doesn't know who is listening, listeners bind once at
initialization and respond to events. This prevents tight coupling between systems and
eliminates per-frame polling.

### WizardJam Implementation

**10 GameMode Delegates:**

| Delegate | Signature | Purpose |
|----------|-----------|---------|
| OnQuidditchRoleAssigned | (APawn*, EQuidditchTeam, EQuidditchRole) | Role assignment notification |
| OnQuidditchTeamScored | (EQuidditchTeam, int32, int32) | Score change |
| OnSnitchCaught | (APawn*, EQuidditchTeam) | Match-ending event |
| OnMatchStateChanged | (EQuidditchMatchState, EQuidditchMatchState) | State machine transition |
| OnAgentReadyAtStart | (APawn*, int32) | Staging zone arrival |
| OnAllAgentsReady | () | All agents ready |
| OnMatchStarted | (float) | Match start (after countdown) |
| OnMatchEnded | () | Match termination |
| OnAgentSelectedForSwap | (APawn*) | Player join flow |
| OnTeamSwapComplete | (APawn*, EQuidditchTeam, EQuidditchTeam) | Post-swap notification |

**4 BroomComponent Delegates:**

| Delegate | Signature | Purpose |
|----------|-----------|---------|
| OnFlightStateChanged | (bool) | Mount/dismount notification |
| OnStaminaVisualUpdate | (FLinearColor) | Stamina color for HUD |
| OnForcedDismount | () | Emergency dismount |
| OnBoostStateChanged | (bool) | Boost indicator for HUD |

**Zero Tick() polling** in controllers for game state. All state changes flow through
delegate handlers that update Blackboard keys. BT decorators read BB keys to gate
branches -- this is event-driven all the way down.

**Blackboard as BT State Bridge:** Delegates fire -> handler updates BB key -> BT
decorator re-evaluates automatically. No polling anywhere in the chain.

### Gas Station Pattern Explained

The Gas Station analogy maps directly to the Quidditch match flow:

| Gas Station | Quidditch AI |
|-------------|-------------|
| Gas station posts prices on sign | GameMode broadcasts OnMatchStarted |
| Cars drive by and read sign | Controllers bind at BeginPlay |
| Station doesn't track which cars read sign | GameMode doesn't know which controllers are bound |
| Cars decide whether to stop | Controller checks if event applies to its pawn |
| Cars fill tank at their own pump | Controller writes to its own Blackboard |
| Cars leave when done | Task succeeds, BT moves to next branch |
| "Gun fired" (race start signal) | OnMatchStarted.Broadcast(CountdownSeconds) |
| Cars waiting at starting line | BTTask_WaitForMatchStart checks BB.MatchStarted |

The key principle: the broadcaster (GameMode) is completely decoupled from the
listeners (controllers). You can add 50 agents or 0 agents, and the GameMode code
doesn't change. Each controller decides independently how to respond.

### Why This Matters

Polling creates O(N) per-frame cost for N agents checking state. With 14 agents
(standard Quidditch team size of 7 per side), that means 14 GameMode lookups and casts
every tick. Delegates fire only when state actually changes -- typically 4-6 times per
match total. This is the difference between 14 casts x 60 FPS x 300 seconds (252,000
operations) vs. 14 x 6 (84 operations). The Observer Pattern is 3,000x more efficient
for this use case.

---

## 4. DELEGATE LIFECYCLE MANAGEMENT

### Industry Standard

Every `AddDynamic` MUST have a corresponding `RemoveDynamic`. Missing unbinds cause:
- Dangling delegate references pointing to destroyed objects
- Crashes on garbage collection when delegate fires to dead controller
- Memory leaks from prevented GC
- Duplicate handler calls from re-binding without unbinding

AAA studios enforce this through static analysis tools and code review checklists. UE5
does not warn about missing unbinds at compile time.

### WizardJam Implementation

**AIC_QuidditchController Delegate Lifecycle:**

| Phase | Action | Delegates |
|-------|--------|-----------|
| BeginPlay | Bind perception | AIPerceptionComp->OnTargetPerceptionUpdated |
| OnPossess | Bind GameMode (5 delegates) | OnMatchStarted, OnMatchEnded, OnAgentSelectedForSwap, OnTeamSwapComplete, OnQuidditchRoleAssigned |
| OnPossess | Bind BroomComponent | OnFlightStateChanged |
| OnPossess | Bind pawn overlaps (2 delegates) | OnActorBeginOverlap, OnActorEndOverlap |
| OnUnPossess | Unbind BroomComponent | OnFlightStateChanged.RemoveDynamic |
| OnUnPossess | Unbind pawn overlaps | OnActorBeginOverlap.RemoveDynamic, OnActorEndOverlap.RemoveDynamic |
| EndPlay | Unbind GameMode (5 delegates) | All 5 GameMode delegates |

**BroomComponent Delegate Lifecycle:**

| Phase | Action | Delegates |
|-------|--------|-----------|
| BeginPlay | Bind StaminaComponent | OnStaminaChanged |
| EndPlay | Unbind StaminaComponent | OnStaminaChanged.RemoveDynamic |

**AIC_SnitchController Delegate Lifecycle:**

| Phase | Action | Delegates |
|-------|--------|-----------|
| BeginPlay | Bind perception | OnTargetPerceptionUpdated |
| OnUnPossess | Unbind perception | OnTargetPerceptionUpdated.RemoveDynamic |

### Bugs Found and Fixed During Code Review

Three delegate unbind bugs were discovered and fixed during the February 2026 forensic
audit:

| Bug | Location | Symptom | Fix |
|-----|----------|---------|-----|
| Missing BroomComponent unbind | AIC_QuidditchController::OnUnPossess | Potential crash on pawn destruction if flight state changes after unpossess | Added `BroomComp->OnFlightStateChanged.RemoveDynamic(this, ...)` in OnUnPossess |
| Missing SnitchController unbind | AIC_SnitchController::OnUnPossess | Perception delegate could fire on destroyed controller | Added perception unbind in OnUnPossess |
| Missing staging zone exit handler | AIC_QuidditchController | Agent could leave staging zone without decrementing ready count, blocking match start | Added HandlePawnEndOverlap + HandleAgentLeftStagingZone |

### Why This Matters

These bugs are particularly dangerous because they only manifest under specific
conditions (object destruction order, GC timing) and are nearly impossible to reproduce
deterministically. In a shipping title, they would appear as rare crashes that QA
struggles to reproduce. The cost of prevention (writing unbind code) is trivial compared
to the cost of diagnosis (hours of debugging rare crashes in release builds).

---

## 5. BLACKBOARD KEY SILENT FAILURE PREVENTION

### The Problem

UE5's FBlackboardKeySelector requires TWO separate initialization steps. Missing
either causes SILENT FAILURE -- code compiles without errors, runs without crashes,
logs look correct, but Blackboard writes never actually happen. The AI appears to
perceive targets and run logic correctly, but never acts on them because the BB key
it thinks it is writing to is actually unresolved.

### Dual-Initialization Requirement

**Step 1 - Constructor: Key Type Filter**
```cpp
// Tells the BT editor which key types are valid for this selector
MyKeySelector.AddObjectFilter(this,
    GET_MEMBER_NAME_CHECKED(UMyBTNode, MyKeySelector),
    AActor::StaticClass());
```

**Step 2 - InitializeFromAsset: Runtime Key Resolution**
```cpp
void UMyBTNode::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);
    if (UBlackboardData* BBAsset = GetBlackboardAsset())
    {
        MyKeySelector.ResolveSelectedKey(*BBAsset);
    }
}
```

Without Step 1: The BT editor shows no valid keys in the dropdown. Empty selection.
Without Step 2: The editor shows the key selected, IsSet() returns true in editor,
but at RUNTIME the key ID is unresolved. Writes go to an invalid slot.

### WizardJam Implementation

- All 28 BT nodes audited and confirmed compliant
- 15+ FBlackboardKeySelector properties across all nodes -- ALL have both init steps
- Discovery date: January 21, 2026 (BTService_FindCollectible appeared to work but
  never moved AI because OutputKey.IsSet() returned false at runtime despite being
  configured correctly in the editor)

### Why This Is Critical

This is arguably UE5's most dangerous AI development pitfall for three reasons:

1. **No compile error.** The code is syntactically valid without either init step.
2. **No runtime error.** No crash, no exception, no assert. The BB just silently
   ignores the write.
3. **No log warning.** UE5 does not log "key not resolved" or "write to invalid slot."
   The only symptom is "AI doesn't do what it should."

A developer unfamiliar with this pitfall can spend days debugging logic, perception
configuration, and BT structure before discovering the root cause is a missing
two-line function override. The WizardJam codebase prevents this by treating dual-init
as a mandatory code review gate -- no BT node with FBlackboardKeySelector passes review
without both steps verified.

---

## 6. MATCH STATE MACHINE

### Industry Standard

Complex game flow should use explicit state machines, not boolean flag combinations.
State machines provide: clear valid transitions, debugging visibility, event broadcasting
on each transition, and prevention of invalid state combinations. AAA titles use state
machines for match flow, character state (idle/moving/attacking/dying), and UI screens.

### WizardJam Implementation

8 explicit states with transitions managed by `TransitionToState()`:

```
Initializing
    |
    v
FlyingToStart
    |
    v
WaitingForReady
    |
    v
Countdown
    |
    v
InProgress <-------> PlayerJoining
    |
    v
SnitchCaught
    |
    v
Ended
```

Each call to `TransitionToState()` broadcasts `OnMatchStateChanged(OldState, NewState)`,
enabling any system to react to state transitions without polling.

### Comparison: Boolean Flags vs State Machine

| Approach | Possible States | Valid States | Invalid States |
|----------|----------------|-------------|----------------|
| 4 booleans (bMatchStarted, bMatchEnded, bCountdownActive, bAllReady) | 16 (2^4) | ~6 | ~10 |
| Enum state machine | 8 | 8 | 0 |

With booleans, you must constantly guard against invalid combinations:
- bMatchStarted=true AND bMatchEnded=true (contradiction)
- bCountdownActive=true AND bMatchEnded=true (impossible)
- bAllReady=true AND bMatchStarted=false AND bCountdownActive=false (stuck state)

With the state machine, these combinations are structurally impossible. The transition
function only allows valid moves. If a bug tries to go from Ended to Countdown,
the transition function rejects it.

### Why This Matters

AAA QA teams spend significant time on "impossible state" bugs where boolean flags
get out of sync. State machines prevent entire categories of bugs by making invalid
states unrepresentable. The WizardJam implementation with 8 named states and broadcast
transitions is the standard approach used by Naughty Dog's match systems and Epic's
own GameMode architecture.

---

## 7. FORBIDDEN PATTERNS AND WHY

### GameplayStatics

**Forbidden:** `UGameplayStatics::GetGameMode()`, `GetAllActorsOfClass()`,
`GetAllActorsWithTag()`, `GetPlayerCharacter()`

**Why Forbidden:**
- Creates hidden dependency on Kismet module (adds ~200KB to binary)
- Bypasses proper component references and dependency injection
- `GetAllActorsOfClass` is O(N) over ALL actors in the world, not just relevant ones
- Prevents modular packaging -- code using GameplayStatics cannot be extracted into
  standalone plugins without pulling Kismet along
- Encourages lazy architecture -- instead of proper references set in editor or
  bound via delegates, developers poll the entire world every frame

**Correct Alternative:** For world queries, use `TActorIterator<T>` which is O(M)
where M is actors of type T only. For GameMode access, cache once in BeginPlay via
`GetWorld()->GetAuthGameMode()` and use Observer Pattern for state updates.

### EditAnywhere

**Forbidden on class defaults.** Use `EditDefaultsOnly` or `EditInstanceOnly`.

**Why Forbidden:**
- `EditAnywhere` exposes properties to BOTH class defaults AND per-instance override
- Per-instance overrides create configuration divergence -- Agent_7 has different
  perception radius than Agent_3, and nobody knows why
- In AAA production with 50+ AI agents in a level, per-instance overrides become
  untrackable. A designer changes one instance, forgets, and the bug only appears
  when that specific instance runs
- `EditDefaultsOnly` ensures ALL instances share the same configuration (class-level)
- `EditInstanceOnly` is for level placement data (spawn position, assigned zone)

### Tick() Polling for State

**Forbidden for game state queries.** Tick-based services may poll perception
(which is already event-driven internally).

**Why Forbidden:**
- O(N) every frame for N agents -- scales linearly with agent count
- Creates frame budget pressure: 14 agents x GetGameMode cast x state check = measurable
  cost at 60 FPS
- Masks the actual control flow -- state changes happen at discrete moments (match
  starts, snitch caught), not continuously
- Observer Pattern captures the EXACT moment of state change with zero delay, no
  missed frames, and O(1) notification cost

### ConstructorHelpers

**Forbidden:** `ConstructorHelpers::FObjectFinder<T>`, `ConstructorHelpers::FClassFinder<T>`

**Why Forbidden:**
- Loads assets at construction time (CDO creation), before the world exists
- Prevents async loading -- asset MUST be loaded synchronously during module init
- Hard path reference -- if asset is moved or renamed, code crashes at startup
- No fallback -- if asset doesn't exist, entire module fails to load (crash to desktop)
- AAA projects use soft references (TSoftObjectPtr, TSoftClassPtr) or Blueprint
  configuration (UPROPERTY with EditDefaultsOnly) instead

---

## 8. MRC CARD INDEX

MRC (Marcus Review Card) cards provide step-by-step setup and verification procedures
for each phase of the AI system.

| Card ID | Title | Classification | Primary Focus |
|---------|-------|----------------|---------------|
| MRC-SEK-001 | Initialize Staging Zone and Agent Positions | Critical | Level setup: placing agents, brooms, staging zones in editor |
| MRC-SEK-002 | Configure Agent Controller and Blackboard | Critical | BB key verification: all 18+ keys initialized without (invalid) |
| MRC-SEK-003 | Validate Broom Acquisition Pipeline | Routine | Perception + interaction: agent finds, walks to, and mounts broom |
| MRC-SEK-004 | Validate Flight-to-Staging Pipeline | Routine | Flight + overlap: agent flies to staging zone and triggers ready state |
| MRC-SEK-005 | Validate Match Play and Snitch Catch | Critical | Match flow: countdown -> chase -> catch -> end with scoring |
| MRC-ROLE-001 | Add New AI Role Template | Reference | Step-by-step role expansion guide with filled Seeker example |

### MRC Card Principles

1. **One card = one verifiable outcome.** Each card has clear pass/fail criteria.
2. **Cards reference specific Output Log strings.** No subjective "it looks right."
3. **Cards are ordered by dependency.** MRC-SEK-001 must pass before MRC-SEK-002.
4. **Cards include rollback instructions.** If a step fails, what to undo.
5. **Cards are reusable across roles.** MRC-ROLE-001 templates the entire process.

---

## GLOSSARY

### Gas Station Pattern
Observer Pattern applied to multi-agent synchronization. Named after the concurrent
programming exercise where gas pumps (agents) wait for a shared signal (match start)
without polling. The gas station (GameMode) broadcasts events; cars (controllers) bind
once and respond independently. Key property: the broadcaster is completely decoupled
from listeners. Adding or removing listeners requires zero changes to the broadcaster.

### Bee and Flower Pattern
Agent-side filtering pattern where the agent (bee) decides if an overlapped actor
(flower) is relevant, rather than the overlapped actor knowing about the agent.
Used in staging zone detection: the staging zone broadcasts its team/role hints as
public properties, and each controller reads those hints and decides whether it has
found its assigned zone. The zone never references or tracks controllers.

### FBlackboardKeySelector
UE5 struct that maps a designer-selected Blackboard key name to a runtime key ID.
Requires dual initialization: constructor filter (AddObjectFilter/AddVectorFilter/etc.)
and runtime resolution (InitializeFromAsset with ResolveSelectedKey). Silent failure
mode when either init step is missing.

### TActorIterator
UE5 templated iterator for finding actors of a specific class in the world. Preferred
over GameplayStatics::GetAllActorsOfClass because it returns a typed iterator (no cast
needed), doesn't require the Kismet module, and supports range-based for loops.

### Observer Pattern
Design pattern where an object (subject) maintains a list of dependents (observers)
and notifies them of state changes. In UE5, implemented via DECLARE_DYNAMIC_MULTICAST_DELEGATE
and AddDynamic/RemoveDynamic. The subject doesn't know who is listening -- it just
broadcasts. Listeners bind once and handle events independently.

### Dual-Initialization
The requirement for FBlackboardKeySelector to have both a constructor-time type filter
AND a runtime-time key resolution call. Term used internally in this project to
reference the Day 17 lesson learned where BTService_FindCollectible silently failed
due to missing InitializeFromAsset override.

### Constructor Initialization List
C++ language feature where member variables are initialized before the constructor
body executes. Required by Nick Penney AAA standards: all member variables must be
initialized in the constructor initialization list, never in the header declaration
and never via assignment in the constructor body. This ensures consistent initialization
order matching declaration order and prevents default-then-assign overhead.

### Direct Velocity Control
Flight AI approach where movement is achieved by directly setting velocity vectors
on the CharacterMovementComponent, rather than using NavMesh pathfinding. The BroomComponent
Core API (SetVerticalInput, SetBoostEnabled) translates high-level intent into velocity
changes. Both player input handlers and AI tasks call the same Core API functions.

### EditDefaultsOnly
UE5 UPROPERTY specifier that restricts property editing to class defaults (Blueprint
editor) only. Prevents per-instance override in the level editor, ensuring all instances
of a class share identical configuration. Contrast with EditAnywhere (allows per-instance
override) and EditInstanceOnly (only allows per-instance, not class defaults).
