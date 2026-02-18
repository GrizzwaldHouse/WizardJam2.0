# MRC-MATH-S004: Discrete Logic Patterns (Ternary, Switch, While in UE5 C++)

**Developer:** Marcus Daley
**Project:** WizardJam (END2507)
**Status:** Ready
**Date Created:** February 16, 2026
**Skill Level:** Foundational

---

## System Information

| Field | Value |
|-------|-------|
| MRC Number | MRC-MATH-S004 |
| System | C++ Control Flow Patterns |
| Subsystem | Discrete Logic for Game AI |
| Prerequisites | C++ control flow, UE5 macros |
| Existing Code | `QuidditchTypes.h`, `AIC_QuidditchController.cpp` |

---

## Concept Overview

Game AI relies on **discrete decisions** -- choosing between a finite set of actions based on current state. Three C++ patterns handle this cleanly, but each has correct and incorrect uses in Unreal Engine.

---

## Pattern 1: Ternary Operator `? :`

### When to Use
- Short, single-value assignments
- Inline conditional expressions
- Blueprint-exposed function returns

### When NOT to Use
- Complex logic with side effects
- Nested ternaries (unreadable)
- Anywhere a `switch` or `if` would be clearer

### Correct Usage Examples

```cpp
// Simple conditional assignment
float Speed = bIsBoosting ? BoostSpeed : NormalSpeed;

// Clamp with readable intent
float VerticalInput = (AltitudeDiff > 0.0f) ? 1.0f : -1.0f;

// Null-safe with fallback
FName RoleName = (AgentData != nullptr) ? AgentData->RoleName : TEXT("Unassigned");

// Conditional function argument
BroomComp->SetBoostEnabled(Distance > BoostThreshold ? true : false);
// Simpler version (bool expression IS the value):
BroomComp->SetBoostEnabled(Distance > BoostThreshold);
```

### Anti-Patterns

```cpp
// ❌ Nested ternary -- unreadable
float Score = (bHasSnitch ? 150.0f : (bHasQuaffle ? 10.0f : (bIsKeeper ? 0.0f : 5.0f)));

// ✅ Use switch or if-chain instead
float Score;
if (bHasSnitch)        Score = 150.0f;
else if (bHasQuaffle)  Score = 10.0f;
else if (bIsKeeper)    Score = 0.0f;
else                   Score = 5.0f;

// ❌ Side effects in ternary -- confusing
(bReady ? StartMatch() : WaitForAgents());

// ✅ Use explicit if
if (bReady) { StartMatch(); }
else        { WaitForAgents(); }
```

---

## Pattern 2: Switch Statement for Role Dispatch

### When to Use
- Dispatching behavior based on enum values
- Selecting configurations per agent role
- State machine transitions

### WizardJam Examples

#### Role-Based Configuration
```cpp
// From QuidditchTypes.h - EQuidditchRole enum
// NOTE: We use FName for QuidditchRole in blackboard (designer-expandable)
// But enum is used internally for C++ logic

void ConfigureRoleBehavior(EQuidditchRole Role, UFlyingSteeringComponent* Steering)
{
    if (!Steering) return;

    switch (Role)
    {
    case EQuidditchRole::Seeker:
        // High cohesion toward Snitch, minimal alignment
        Steering->AlignmentStrength = 1.0f;
        Steering->CohesionStrength = 15.0f;
        Steering->SeparationStrength = 3.0f;
        Steering->bEnableAlignment = false;
        break;

    case EQuidditchRole::Chaser:
        // Full flocking for team coordination
        Steering->AlignmentStrength = 5.0f;
        Steering->CohesionStrength = 5.0f;
        Steering->SeparationStrength = 5.0f;
        Steering->bEnableAlignment = true;
        break;

    case EQuidditchRole::Beater:
        // Pursuit-focused, separation disabled
        Steering->AlignmentStrength = 2.0f;
        Steering->CohesionStrength = 8.0f;
        Steering->SeparationStrength = 0.0f;
        Steering->bEnableSeparation = false;
        break;

    case EQuidditchRole::Keeper:
        // Strong position hold at goal
        Steering->AlignmentStrength = 0.0f;
        Steering->CohesionStrength = 20.0f;
        Steering->SeparationStrength = 2.0f;
        Steering->bEnableAlignment = false;
        break;

    default:
        UE_LOG(LogTemp, Warning, TEXT("Unknown role, using defaults"));
        break;
    }
}
```

#### Match State Machine
```cpp
// State machine using switch (simplified from QuidditchGameMode)
void AQuidditchGameMode::TickMatchState(float DeltaTime)
{
    switch (CurrentMatchState)
    {
    case EMatchState::WaitingForAgents:
        // Check if all required agents are registered
        if (RegisteredAgents.Num() >= RequiredAgentCount)
        {
            TransitionToState(EMatchState::Countdown);
        }
        break;

    case EMatchState::Countdown:
        CountdownTimer -= DeltaTime;
        if (CountdownTimer <= 0.0f)
        {
            OnMatchStarted.Broadcast(0.0f);
            TransitionToState(EMatchState::InProgress);
        }
        break;

    case EMatchState::InProgress:
        // Match runs via delegates and BT, no polling needed here
        break;

    case EMatchState::SnitchCaught:
        // Transition after brief delay for celebration
        EndMatchTimer -= DeltaTime;
        if (EndMatchTimer <= 0.0f)
        {
            OnMatchEnded.Broadcast(WinningTeam);
            TransitionToState(EMatchState::Results);
        }
        break;

    case EMatchState::Results:
        // Terminal state - no transition
        break;
    }
}
```

### Switch Best Practices in UE5

| Practice | Reason |
|----------|--------|
| Always include `default:` case | Catches unhandled enum values after future additions |
| Use `break;` on every case | Fall-through is error-prone and rarely intentional |
| Log in `default:` | Makes unhandled cases visible during development |
| Don't use `switch` for `FName` | FName doesn't work in switch; use `if/else if` chain with `==` |

### FName Dispatch (When Switch Can't Be Used)

Since WizardJam uses `FName` for Quidditch roles in the Blackboard (designer-expandable), use named comparisons:

```cpp
void HandleRoleAssigned(FName RoleName)
{
    if (RoleName == TEXT("Seeker"))
    {
        ActivateSeekerBehavior();
    }
    else if (RoleName == TEXT("Chaser"))
    {
        ActivateChaserBehavior();
    }
    else if (RoleName == TEXT("Beater"))
    {
        ActivateBeaterBehavior();
    }
    else if (RoleName == TEXT("Keeper"))
    {
        ActivateKeeperBehavior();
    }
    else
    {
        UE_LOG(LogQuidditchAI, Warning,
            TEXT("[%s] Unknown role: %s"), *GetName(), *RoleName.ToString());
    }
}
```

---

## Pattern 3: While Loops (Use With Extreme Caution)

### The Rule

**Avoid heavy while loops on the game thread.** Unreal Engine runs at 30-120 FPS. A while loop that takes more than ~1ms will cause frame hitches.

### When While is Acceptable
- **Bounded iteration count** known at compile time
- **Small data sets** (< 100 elements)
- **Not every frame** (initialization, one-shot setup)
- **Math convergence** with guaranteed termination

### When While is FORBIDDEN
- **Unbounded conditions** (`while (!bFound)` with no max iterations)
- **Network/file I/O waits** (use async instead)
- **Per-frame polling** (use delegates/events per CLAUDE.md standards)
- **Searching all actors** (use `TActorIterator` which handles chunking)

### Correct While Usage: Iterative Math Solver

```cpp
// Newton-Raphson for finding a root -- bounded iteration
float SolveForParameter(float InitialGuess, float Tolerance, int32 MaxIterations)
{
    float x = InitialGuess;
    int32 Iteration = 0;

    while (Iteration < MaxIterations)
    {
        float fx = EvaluateFunction(x);
        float fpx = EvaluateDerivative(x);

        if (FMath::Abs(fx) < Tolerance)
        {
            break; // Converged
        }

        if (FMath::IsNearlyZero(fpx))
        {
            break; // Derivative is zero, can't continue
        }

        x = x - fx / fpx;
        Iteration++;
    }

    return x;
}
```

### Correct While Usage: Slot Assignment (Bounded)

```cpp
// Assign agents to formation slots -- bounded by agent count
void AssignFormationSlots(TArray<APawn*>& Agents, const TArray<FVector>& Slots)
{
    int32 SlotIndex = 0;
    int32 AgentIndex = 0;

    // Both arrays are bounded, loop terminates
    while (AgentIndex < Agents.Num() && SlotIndex < Slots.Num())
    {
        if (IsValid(Agents[AgentIndex]))
        {
            AssignSlot(Agents[AgentIndex], Slots[SlotIndex]);
            SlotIndex++;
        }
        AgentIndex++;
    }
}
```

### Anti-Pattern: Unbounded While

```cpp
// ❌ FORBIDDEN -- no guaranteed termination
while (!bSnitchCaught)
{
    UpdateSeekerPosition();  // This runs in Tick, not a while loop
}

// ✅ CORRECT -- use Tick or BT TickTask
void UBTTask_ChaseSnitch::TickTask(UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory, float DeltaSeconds)
{
    // BT ticks this each frame automatically
    // FinishLatentTask when snitch is caught
}
```

### For vs While Decision Table

| Scenario | Use For | Use While |
|----------|---------|-----------|
| Known iteration count | `for (int i = 0; i < N; i++)` | No |
| Array iteration | `for (AActor* Actor : Actors)` | No |
| TActorIterator | `for (TActorIterator<T> It(World); It; ++It)` | No |
| Convergence with bound | No | `while (Iter < Max && Error > Tol)` |
| Queue/stack processing | No | `while (!Queue.IsEmpty())` |
| Unknown count, bounded | No | `while (Condition && Count < Safety)` |

---

## UE5-Specific Patterns

### Safe Returns with checkf / ensure

```cpp
// check -- crashes in Development, logged in Shipping
void ProcessAgent(AQuidditchAgent* Agent)
{
    check(Agent != nullptr); // Hard crash if null -- development only
    // ... rest of function
}

// ensure -- logs callstack but continues
void TryProcessAgent(AQuidditchAgent* Agent)
{
    if (!ensure(Agent != nullptr))
    {
        return; // Logged the callstack, returned safely
    }
    // ... rest of function
}

// ensureMsgf -- same but with custom message
if (!ensureMsgf(BroomComp != nullptr,
    TEXT("[%s] BroomComponent missing on %s"), *GetName(), *Agent->GetName()))
{
    return;
}
```

### IsValid() vs nullptr Check

```cpp
// ❌ Only checks pointer
if (MyActor != nullptr) { /* could be pending kill */ }

// ✅ Checks pointer AND not pending destroy
if (IsValid(MyActor)) { /* safe to use */ }

// ✅ For UObjects in containers
if (MyWeakPtr.IsValid()) { /* safe to use */ }
```

### Conditional Macro Compilation

```cpp
// Debug code that compiles out in Shipping
#if !UE_BUILD_SHIPPING
    DrawDebugSphere(GetWorld(), InterceptPoint, 50.0f, 12, FColor::Yellow, false, 0.5f);
#endif

// Editor-only code
#if WITH_EDITOR
    UE_LOG(LogQuidditchAI, Verbose, TEXT("Role assigned: %s"), *RoleName.ToString());
#endif
```

---

## Unit Test Cases

### Test 1: Ternary Correctness
```cpp
// Verify ternary produces same result as if/else
float TernaryResult = (Distance > 1000.0f) ? BoostSpeed : NormalSpeed;

float IfResult;
if (Distance > 1000.0f) IfResult = BoostSpeed;
else IfResult = NormalSpeed;

check(TernaryResult == IfResult);
```

### Test 2: Switch Exhaustiveness
```cpp
// Test that all enum values are handled
for (int32 i = 0; i < static_cast<int32>(EQuidditchRole::MAX); i++)
{
    EQuidditchRole Role = static_cast<EQuidditchRole>(i);
    // ConfigureRoleBehavior should not hit default: case for any valid value
    ConfigureRoleBehavior(Role, TestSteering);
    // Verify TestSteering was actually modified
}
```

### Test 3: While Loop Bounded
```cpp
// Verify that solver terminates within bounds
int32 MaxIter = 100;
float Result = SolveForParameter(1.0f, 0.001f, MaxIter);
// If this returns, the loop terminated (it didn't hang)
```

---

## Gamified AI Task

**Scenario:** "Decision Tree Audit"

Review the Quidditch AI controller and identify:

1. Find 3 places where a ternary `? :` is used correctly
2. Find 1 place where a ternary could replace a verbose if/else
3. Find the switch statement for match state transitions
4. Verify all switch cases have `break;` and a `default:` case
5. Find any while loops and verify they have bounded termination

**Scoring:**
- All 5 items verified = Gold
- 3-4 items = Silver
- At least 1 fix suggested = Bronze

---

## Quick Reference Card

```
TERNARY:
  Use:   value = condition ? A : B
  Don't: nested ternaries, side effects

SWITCH:
  Use:   enum dispatch, state machines
  Don't: FName (use if/else chain)
  Rules: always default:, always break:

WHILE:
  Use:   bounded convergence, small queues
  Don't: unbounded conditions, per-frame polling
  Rules: ALWAYS have a max iteration guard

UE5 SAFETY:
  check()      -- crash in dev, silent in ship
  ensure()     -- log + continue
  IsValid()    -- pointer + not pending kill
  #if !UE_BUILD_SHIPPING -- debug-only code
```

---

## Related MRC Cards

| MRC Number | Title | Relationship |
|------------|-------|--------------|
| MRC-MATH-S001 | Vector Intercept | Uses while loop for quadratic solver convergence |
| MRC-MATH-S002 | 3D Steering | Uses ternary for direction selection |
| MRC-MATH-S005 | Flocking | Uses switch for role-specific flock weights |

---

## Key Takeaways

1. **Ternary for simple assignments only** -- If it needs more than one line, use if/else
2. **Switch for enums, if/else for FName** -- UE5 uses FName for designer flexibility
3. **While loops need a safety valve** -- Always add `MaxIterations` or `Count < Bound`
4. **IsValid() > nullptr** -- Catches pending-kill actors
5. **ensure > check for production** -- Don't crash the game, log and recover
6. **No polling in Tick** -- Per-frame state checks violate Observer Pattern (use delegates)

---

**END OF MRC-MATH-S004**
