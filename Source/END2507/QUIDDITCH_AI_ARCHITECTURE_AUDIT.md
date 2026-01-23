# Quidditch AI Behavioral Architecture Audit

**Audit Date:** January 23, 2026
**Auditor:** WizardJam Development Agent
**Scope:** All AI Behavior Trees, Tasks, Services, Decorators, Controllers, and Components
**Standard:** Nick Penney AAA Observer-Driven AI Pattern

---

## Executive Summary

The Quidditch AI system has **5 critical architectural violations** and **2 secondary issues** that break the observer-driven AI pattern. The core problem: BT nodes bypass the Blackboard and directly query actor components, creating tight coupling and preventing proper reactive behavior.

**Required Pattern:** `Component → AI Controller → Blackboard → Behavior Tree`

**Current Anti-Pattern:** `BT Node → Direct Component Query` (bypasses Controller and Blackboard)

---

## Critical Violations

### 1. BTDecorator_HasChannel - Direct Component Query

**File:** `BTDecorator_HasChannel.cpp:56-68`

**Violation:** Decorator directly queries AC_SpellCollectionComponent from the pawn, completely bypassing the Blackboard.

```cpp
// VIOLATION: BT decorator directly queries component
UAC_SpellCollectionComponent* SpellComp =
    AIPawn->FindComponentByClass<UAC_SpellCollectionComponent>();
// ...
bool bHasChannel = SpellComp->HasChannel(ChannelName);
```

**Why This Is Wrong:**
- Decorators should ONLY read Blackboard keys
- Controller should interpret component state and write to Blackboard
- Without Blackboard observer, decorator only evaluates on BT tick (not reactive)
- Tight coupling prevents reuse across different pawn types

**Impact:** AI cannot immediately react to channel acquisition. BT must tick to re-evaluate.

**Required Fix:**
1. Controller binds to `OnChannelAdded`/`OnChannelRemoved` in OnPossess
2. Controller updates `HasBroomChannel` (Bool) Blackboard key
3. Decorator reads Blackboard key with `Notify Observer: On Value Change`
4. Observer Aborts enabled for reactive branch switching

---

### 2. BTTask_Interact::HasRequiredChannel() - Direct Component Query

**File:** `BTTask_Interact.cpp:207-223`

**Violation:** Task directly queries AC_SpellCollectionComponent to validate RequiredChannel.

```cpp
// VIOLATION: BT task directly queries component
UAC_SpellCollectionComponent* SpellComp = Pawn->FindComponentByClass<UAC_SpellCollectionComponent>();
if (SpellComp)
{
    return SpellComp->HasChannel(RequiredChannel);
}
```

**Why This Is Wrong:**
- Same violation as BTDecorator_HasChannel
- Tasks should read prerequisites from Blackboard
- Channel validation should occur at Controller level

**Impact:** Task validation bypasses centralized AI decision state.

**Required Fix:**
1. Task reads channel state from Blackboard key
2. Controller maintains channel state in Blackboard
3. Remove direct component access from task

---

### 3. BTTask_MountBroom::FlightStateKey - Missing Initialization

**File:** `BTTask_MountBroom.h:64-65`, `BTTask_MountBroom.cpp:29-36`

**Violation:** FBlackboardKeySelector `FlightStateKey` exists but lacks required initialization:
- ❌ Missing `AddBoolFilter()` in constructor
- ❌ Missing `InitializeFromAsset()` override

```cpp
// MISSING in constructor:
FlightStateKey.AddBoolFilter(this,
    GET_MEMBER_NAME_CHECKED(UBTTask_MountBroom, FlightStateKey));

// MISSING override:
void UBTTask_MountBroom::InitializeFromAsset(UBehaviorTree& Asset);
```

**Why This Is Wrong:**
- Per CLAUDE.md checklist, FBlackboardKeySelector requires BOTH pieces of initialization
- Without them, `IsSet()` returns false at runtime even when configured in editor
- This is a **silent failure** - code compiles and runs but Blackboard writes fail

**Impact:** FlightStateKey never updates Blackboard despite appearing configured.

**Required Fix:** Add AddBoolFilter() in constructor and InitializeFromAsset() override.

---

### 4. BTTask_ControlFlight::TickTask - Direct Flight State Query

**File:** `BTTask_ControlFlight.cpp:128-136`

**Violation:** Task directly queries BroomComponent::IsFlying() every tick instead of reading Blackboard.

```cpp
// VIOLATION: Polling component state in tick
UAC_BroomComponent* BroomComp = AIPawn->FindComponentByClass<UAC_BroomComponent>();
if (!BroomComp || !BroomComp->IsFlying())
{
    // Lost flight capability - task fails
    FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
}
```

**Why This Is Wrong:**
- Tick-based polling instead of event-driven
- Should read `IsFlying` from Blackboard (set by Controller binding to OnFlightStateChanged)
- If component broadcasts dismount, Controller updates Blackboard, decorator aborts branch

**Impact:** Flight state not centralized; duplicate queries across multiple tasks.

**Required Fix:**
1. Task reads `IsFlying` from Blackboard
2. Controller binds to OnFlightStateChanged and updates Blackboard
3. Parent decorator observes IsFlying and aborts branch on change

---

### 5. AIC_QuidditchController - Missing Delegate Bindings

**File:** `AIC_QuidditchController.cpp`

**Violation:** Controller does not bind to component delegates for state synchronization:
- ❌ No binding to `AC_BroomComponent::OnFlightStateChanged`
- ❌ No binding to `AC_BroomComponent::OnBoostStateChanged`
- ❌ No binding to `AC_SpellCollectionComponent::OnChannelAdded`
- ❌ No binding to `AC_SpellCollectionComponent::OnChannelRemoved`

**Current State:**
- `SetIsFlying()` API exists but is **orphaned** - nothing calls it
- Perception bindings exist (good) but component state bindings missing

**Why This Is Wrong:**
- Controller is supposed to be the "Brain" - the ONLY layer that writes to Blackboard
- Components ("Body") broadcast state changes
- Controller interprets meaning and updates Blackboard
- BT decorators observe Blackboard with Observer Aborts

**Impact:** Blackboard state is stale. IsFlying never updates. Channel state never tracked.

**Required Fix:** Add delegate bindings in OnPossess, unbind in OnUnPossess.

---

## Secondary Issues

### 6. BTTask_MountBroom Direct Blackboard Write

**File:** `BTTask_MountBroom.cpp:93-99`

**Observation:** Task writes directly to Blackboard using `FlightStateKey`.

```cpp
if (FlightStateKey.IsSet())
{
    Blackboard->SetValueAsBool(FlightStateKey.SelectedKeyName, bIsNowFlying);
}
```

**Assessment:** This is acceptable for task-immediate results (the task that caused the state change can update it). However, the Controller should ALSO bind to `OnFlightStateChanged` for consistency, so if flight ends externally (stamina depletion), Blackboard stays current.

**Recommendation:** Keep task write, but add redundant Controller binding for external state changes.

---

### 7. BTDecorator_HasChannel No Observer Abort Support

**File:** `BTDecorator_HasChannel.h`

**Observation:** Decorator doesn't configure `CheckConditionOnBlackboardKeyChange` or set `FlowAbortMode`.

**Assessment:** Even after refactoring to read Blackboard, without Observer Abort configuration, the decorator only re-evaluates on BT tick intervals.

**Recommendation:** After refactoring to use Blackboard:
1. Set `FlowAbortMode = EBTFlowAbortMode::Self` (or `Both`)
2. Configure key observation in constructor

---

## Component Analysis (CORRECT Architecture)

### AC_SpellCollectionComponent ✅

**Delegates Defined:**
- `OnChannelAdded(FName Channel)`
- `OnChannelRemoved(FName Channel)`
- `OnSpellAdded(FName SpellType, int32 TotalCount)`
- `OnSpellRemoved(FName SpellType, int32 RemainingCount)`
- `OnAllSpellsCleared(int32 PreviousCount)`

**Assessment:** Component correctly follows observer pattern. Broadcasts state changes via delegates. Does NOT write to Blackboard (correct - that's Controller's job).

---

### AC_BroomComponent ✅

**Delegates Defined:**
- `OnFlightStateChanged(bool bIsFlying)`
- `OnBoostStateChanged(bool bIsBoosting)`
- `OnStaminaVisualUpdate(FLinearColor NewColor)`
- `OnForcedDismount()`

**Assessment:** Component correctly follows observer pattern. Controller should bind to these.

---

## Required Blackboard Keys

| Key Name | Type | Source | Purpose |
|----------|------|--------|---------|
| `IsFlying` | Bool | Controller → OnFlightStateChanged | Flight state for decorators/tasks |
| `IsBoosting` | Bool | Controller → OnBoostStateChanged | Boost state for stamina logic |
| `HasBroomChannel` | Bool | Controller → OnChannelAdded | Gate broom-related branches |
| `HasFireChannel` | Bool | Controller → OnChannelAdded | Gate fire spell branches |
| `HasTeleportChannel` | Bool | Controller → OnChannelAdded | Gate teleport branches |

**Pattern:** For each channel type, create a Bool key. Controller updates when channel is added/removed.

---

## Refactor Summary

### Files Requiring Changes

| File | Change Type | Priority |
|------|-------------|----------|
| `AIC_QuidditchController.cpp` | Add delegate bindings | CRITICAL |
| `BTDecorator_HasChannel.cpp` | Read Blackboard, not component | CRITICAL |
| `BTTask_MountBroom.cpp` | Add FBlackboardKeySelector init | CRITICAL |
| `BTTask_ControlFlight.cpp` | Read IsFlying from Blackboard | HIGH |
| `BTTask_Interact.cpp` | Read channel from Blackboard | HIGH |

### New Blackboard Keys Required

Add to `BB_QuidditchAI` (or equivalent):
- `IsFlying` (Bool) - already exists, needs binding
- `IsBoosting` (Bool) - new
- `HasBroomChannel` (Bool) - new
- Additional per-channel keys as needed

---

## Architectural Diagram

### BEFORE (Current - Anti-Pattern)

```
┌─────────────────────────────────────────────────────────────┐
│                      BEHAVIOR TREE                          │
│  ┌─────────────────────────────────────────────────────────┐│
│  │ BTDecorator_HasChannel                                  ││
│  │   ├── GetPawn()                                         ││
│  │   ├── FindComponentByClass<SpellCollection>() ──────────┼┼──┐
│  │   └── SpellComp->HasChannel() ◄─────────────────────────┼┼──┤ DIRECT
│  └─────────────────────────────────────────────────────────┘│  │ QUERY
│  ┌─────────────────────────────────────────────────────────┐│  │
│  │ BTTask_ControlFlight                                    ││  │
│  │   ├── FindComponentByClass<BroomComponent>() ───────────┼┼──┤
│  │   └── BroomComp->IsFlying() ◄───────────────────────────┼┼──┤
│  └─────────────────────────────────────────────────────────┘│  │
└─────────────────────────────────────────────────────────────┘  │
                              │                                   │
                              │ BYPASSED                          │
                              ▼                                   │
┌─────────────────────────────────────────────────────────────┐  │
│                      BLACKBOARD                             │  │
│  ┌─────────────────────────────────────────────────────────┐│  │
│  │ IsFlying: false (STALE - never updated)                 ││  │
│  │ PerceivedCollectible: (managed by service)              ││  │
│  └─────────────────────────────────────────────────────────┘│  │
└─────────────────────────────────────────────────────────────┘  │
                              │                                   │
                              │ BYPASSED                          │
                              ▼                                   │
┌─────────────────────────────────────────────────────────────┐  │
│                    AI CONTROLLER                            │  │
│  ┌─────────────────────────────────────────────────────────┐│  │
│  │ SetIsFlying() exists but NEVER CALLED                   ││  │
│  │ No bindings to component delegates                      ││  │
│  └─────────────────────────────────────────────────────────┘│  │
└─────────────────────────────────────────────────────────────┘  │
                                                                  │
┌─────────────────────────────────────────────────────────────┐  │
│                     COMPONENTS                              │◄─┘
│  ┌───────────────────────┐  ┌───────────────────────────┐  │
│  │ AC_SpellCollection    │  │ AC_BroomComponent         │  │
│  │ Delegates: ✅ defined │  │ Delegates: ✅ defined     │  │
│  │ But: ❌ not bound     │  │ But: ❌ not bound         │  │
│  └───────────────────────┘  └───────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### AFTER (Required - Observer Pattern)

```
┌─────────────────────────────────────────────────────────────┐
│                      BEHAVIOR TREE                          │
│  ┌─────────────────────────────────────────────────────────┐│
│  │ BTDecorator_HasChannel                                  ││
│  │   └── Blackboard->GetValueAsBool("HasBroomChannel") ────┼┼──┐
│  │       [Observer Abort: On Value Change]                 ││  │
│  └─────────────────────────────────────────────────────────┘│  │
│  ┌─────────────────────────────────────────────────────────┐│  │ READS
│  │ BTTask_ControlFlight                                    ││  │ ONLY
│  │   └── Blackboard->GetValueAsBool("IsFlying") ───────────┼┼──┤
│  └─────────────────────────────────────────────────────────┘│  │
└─────────────────────────────────────────────────────────────┘  │
                              ▲                                   │
                              │ OBSERVER PATTERN                  │
                              │                                   │
┌─────────────────────────────────────────────────────────────┐  │
│                      BLACKBOARD                             │◄─┘
│  ┌─────────────────────────────────────────────────────────┐│
│  │ IsFlying: true/false (LIVE - updated by controller)     ││
│  │ HasBroomChannel: true/false (LIVE)                      ││
│  │ IsBoosting: true/false (LIVE)                           ││
│  └─────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
                              ▲
                              │ WRITES
                              │
┌─────────────────────────────────────────────────────────────┐
│                    AI CONTROLLER                            │
│  ┌─────────────────────────────────────────────────────────┐│
│  │ OnPossess():                                            ││
│  │   BroomComp->OnFlightStateChanged.AddDynamic(...)       ││
│  │   SpellComp->OnChannelAdded.AddDynamic(...)             ││
│  │                                                          ││
│  │ HandleFlightStateChanged(bool bIsFlying):               ││
│  │   Blackboard->SetValueAsBool("IsFlying", bIsFlying)     ││
│  │                                                          ││
│  │ HandleChannelAdded(FName Channel):                      ││
│  │   if (Channel == "Broom")                               ││
│  │     Blackboard->SetValueAsBool("HasBroomChannel", true) ││
│  └─────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
                              ▲
                              │ BROADCASTS
                              │
┌─────────────────────────────────────────────────────────────┐
│                     COMPONENTS                              │
│  ┌───────────────────────┐  ┌───────────────────────────┐  │
│  │ AC_SpellCollection    │  │ AC_BroomComponent         │  │
│  │ OnChannelAdded ───────┼──┼─► Controller binds here   │  │
│  │ OnChannelRemoved ─────┼──┼─► Controller binds here   │  │
│  └───────────────────────┘  │ OnFlightStateChanged ─────┼──┼─►
│                              │ OnBoostStateChanged ──────┼──┼─►
│                              └───────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

---

## Quality Assurance Checklist

After implementing fixes, verify:

- [ ] AIC_QuidditchController binds to OnFlightStateChanged in OnPossess
- [ ] AIC_QuidditchController binds to OnChannelAdded in OnPossess
- [ ] AIC_QuidditchController unbinds delegates in OnUnPossess
- [ ] BTDecorator_HasChannel reads Blackboard key, not component
- [ ] BTDecorator_HasChannel has Observer Abort configured
- [ ] BTTask_MountBroom has AddBoolFilter() in constructor
- [ ] BTTask_MountBroom has InitializeFromAsset() override
- [ ] BTTask_ControlFlight reads IsFlying from Blackboard
- [ ] BTTask_Interact reads channel state from Blackboard
- [ ] BB_QuidditchAI has required Bool keys defined
- [ ] AI immediately reacts to broom pickup (doesn't wait for BT tick)
- [ ] AI immediately reacts to forced dismount (stamina depletion)

---

## Conclusion

The component architecture (AC_SpellCollectionComponent, AC_BroomComponent) correctly implements the observer pattern with proper delegates. The violation occurs at the AI Controller and Behavior Tree levels, where nodes directly query components instead of using the Blackboard as the single source of truth.

The fixes preserve the excellent component design while properly connecting the observer chain through the AI Controller to the Blackboard, enabling reactive Behavior Tree evaluation via Observer Aborts.

**Estimated Refactor Time:** 2-3 hours for all critical fixes
**Risk:** Low - changes are additive (binding) not destructive
**Testing Required:** Verify AI reacts immediately to channel acquisition and flight state changes
