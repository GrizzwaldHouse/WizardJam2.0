# BT_QuidditchAI Setup Guide

**Project:** WizardJam (END2507)
**Developer:** Marcus Daley
**Date:** January 22, 2026
**Engine:** Unreal Engine 5.4

---

## Table of Contents

1. [Overview](#1-overview)
2. [Behavior Tree Flow Diagram](#2-behavior-tree-flow-diagram)
3. [Blackboard Setup (BB_QuidditchAI)](#3-blackboard-setup-bb_quidditchai)
4. [AI Controller Setup](#4-ai-controller-setup)
5. [Node-by-Node Configuration](#5-node-by-node-configuration)
6. [New Custom Tasks](#6-new-custom-tasks)
7. [Testing Checklist](#7-testing-checklist)

---

## 1. Overview

The Quidditch AI system enables AI agents to:
1. **Acquire a broom** - Find and collect the BroomCollectible to unlock flight
2. **Mount the broom** - Interact with BroomActor to enter flight mode
3. **Fly to destination** - Navigate in 3D space using BTTask_ControlFlight

### AI Flow Summary

```
START
  │
  ▼
┌─────────────────────────────┐
│  Has "Broom" Channel?       │
└─────────────────────────────┘
      │ NO                │ YES
      ▼                   ▼
┌─────────────┐    ┌─────────────────────┐
│ Acquisition │    │   Flight Path       │
│    Path     │    └─────────────────────┘
└─────────────┘              │
      │                      ▼
      ▼               ┌─────────────┐
 Find Collectible     │ Is Flying?  │
      │               └─────────────┘
      ▼                 │ NO    │ YES
 Walk to Collectible    ▼       ▼
      │            Find Broom   Control Flight
      ▼                 │       (fly to target)
 Pickup (auto)          ▼            │
      │            Walk to Broom     ▼
      ▼                 │           END
 Loop back             ▼
                  Interact (Mount)
                       │
                       ▼
                  Control Flight
```

---

## 2. Behavior Tree Flow Diagram

### Visual Structure

```
BT_QuidditchAI
│
└── ROOT (BB_QuidditchAI)
    │
    └── [Selector] Mount Broom
        │
        ├── LEFT BRANCH: Has Broom Channel
        │   │
        │   └── [Decorator] Has Channel: Broom
        │       │
        │       └── [Sequence] FlightPath
        │           │
        │           ├── [Selector] MountOrFly
        │           │   │
        │           │   ├── [Decorator] Blackboard: IsFlying == true
        │           │   │   │
        │           │   │   └── [Task] Control Flight
        │           │   │             Target: TargetLocation
        │           │   │
        │           │   └── [Sequence] FindAndMountBroom
        │           │       │
        │           │       ├── [Service] Find Collectible
        │           │       │             Class: BP_BroomActor (NOT collectible!)
        │           │       │             Output: PerceivedCollectible
        │           │       │
        │           │       ├── [Task] Move To
        │           │       │           Target: PerceivedCollectible
        │           │       │
        │           │       └── [Task] Interact With Target  ← NEW TASK
        │           │                   Target: PerceivedCollectible
        │           │                   Range: 200
        │           │                   Success Key: IsFlying
        │           │
        │           └── [Task] Control Flight
        │                     Target: TargetLocation
        │                     Arrival: 100 units
        │                     Boost: >500 away
        │
        └── RIGHT BRANCH: Missing Broom Channel
            │
            └── [Decorator] Has Channel: Broom (Inverted)
                │
                └── [Sequence] AcquisitionPath
                    │
                    ├── [Service] Find Collectible
                    │             Class: BP_BroomCollectible_C
                    │             Output: PerceivedCollectible
                    │
                    ├── [Task] Move To
                    │           Target: PerceivedCollectible
                    │
                    └── [Task] Wait (0.5s)
```

### Alternative Simplified Structure (Using Existing Mount Task)

If you prefer using the existing `BTTask_MountBroom`:

```
BT_QuidditchAI
│
└── ROOT (BB_QuidditchAI)
    │
    └── [Selector] Mount Broom
        │
        ├── LEFT BRANCH: Has Broom Channel
        │   │
        │   └── [Decorator] Has Channel: Broom
        │       │
        │       └── [Sequence] FlightPath
        │           │
        │           ├── [Decorator] Blackboard: IsFlying == false
        │           │   │
        │           │   └── [Task] Mount/Dismount Broom
        │           │
        │           └── [Task] Control Flight
        │
        └── RIGHT BRANCH: Missing Broom Channel
            │
            └── [Decorator] Has Channel: Broom (Inverted)
                │
                └── [Sequence] AcquisitionPath
                    │
                    ├── [Service] Find Collectible
                    ├── [Task] Move To
                    └── [Task] Wait (0.5s)
```

### Execution Order

| Step | Node | Action |
|------|------|--------|
| 1 | Selector | Evaluates children left-to-right |
| 2a | Has Channel (Broom) | If TRUE → FlightPath |
| 2b | Has Channel (Missing) | If TRUE → AcquisitionPath |
| 3a | FlightPath | Mount if needed, then fly |
| 3b | AcquisitionPath | Find, walk to, pickup collectible |

---

## 3. Blackboard Setup (BB_QuidditchAI)

### Create New Blackboard

1. **Content Browser** → Right-click → **Artificial Intelligence** → **Blackboard**
2. Name it: `BB_QuidditchAI`
3. Double-click to open

### Add Keys

Click **New Key** and add each of these:

| Key Name | Key Type | Description |
|----------|----------|-------------|
| `SelfActor` | Object (Base Class: Actor) | Reference to AI pawn itself |
| `TargetLocation` | Vector | 3D destination for flight |
| `IsFlying` | Bool | Current flight state (true/false) |
| `TargetActor` | Object (Base Class: Actor) | Generic actor target |
| `PerceivedCollectible` | Object (Base Class: Actor) | Detected collectible or broom |
| `HomeLocation` | Vector | AI starting position |
| `TeamID` | Int | Faction ID (0=Neutral, 1=Team1, 2=Team2) |
| `QuidditchRole` | Name | Role: Chaser, Keeper, Seeker, Beater |

### Key Configuration Details

**SelfActor**
- Type: `Object`
- Base Class: `Actor`
- Instance Synced: `true`

**PerceivedCollectible**
- Type: `Object`
- Base Class: `Actor`
- Instance Synced: `false`

---

## 4. AI Controller Setup

### Create AI Controller Blueprint

1. **Content Browser** → Right-click → **Blueprint Class**
2. Search for: `AIC_QuidditchController`
3. Name it: `BP_CodeQuidditchAIController`

### Details Panel Settings

Open the Blueprint and set these in the **Class Defaults**:

| Property | Value | Location |
|----------|-------|----------|
| Behavior Tree Asset | `BT_QuidditchAI` | AI section |
| Blackboard Asset | `BB_QuidditchAI` | AI section |
| Sight Radius | `2000` | Perception |
| Lose Sight Radius | `2500` | Perception |
| Peripheral Vision Angle | `90` | Perception |

### Blackboard Key Names (in AI Controller)

| Property | Value |
|----------|-------|
| Target Location Key Name | `TargetLocation` |
| Target Actor Key Name | `TargetActor` |
| Is Flying Key Name | `IsFlying` |
| Self Actor Key Name | `SelfActor` |
| Perceived Collectible Key Name | `PerceivedCollectible` |

---

## 5. Node-by-Node Configuration

### 5.1 ROOT Node

| Property | Value |
|----------|-------|
| Blackboard Asset | `BB_QuidditchAI` |

---

### 5.2 Mount Broom (Selector)

**Type:** Selector (built-in)

| Property | Value |
|----------|-------|
| Node Name | `Mount Broom` |

*No additional configuration needed - Selector evaluates children left-to-right until one succeeds.*

---

### 5.3 Has Channel Decorator (LEFT - Has Broom)

**Type:** `BTDecorator_HasChannel`

| Property | Value | Notes |
|----------|-------|-------|
| Channel Name | `Broom` | Must match collectible's granted channel |
| Invert Result | `false` | Pass when AI HAS the channel |
| Observer Aborts | `Both` | Re-evaluate when condition changes |

**How to Add:**
1. Right-click the Sequence node
2. **Add Decorator** → **Has Channel**
3. Configure in Details panel

---

### 5.4 FlightPath (Sequence)

**Type:** Sequence (built-in)

| Property | Value |
|----------|-------|
| Node Name | `FlightPath` |

---

### 5.5 Blackboard Based Condition (IsFlying Check)

**Type:** `Blackboard Based Condition` (built-in)

| Property | Value |
|----------|-------|
| Blackboard Key | `IsFlying` |
| Key Query | `Is Not Set` |
| Observer Aborts | `Self` |

*This decorator ensures Mount Broom only runs when AI is NOT already flying.*

---

### 5.6 Mount/Dismount Broom Task

**Type:** `BTTask_MountBroom`

| Property | Value | Notes |
|----------|-------|-------|
| Node Name | `Mount Broom` | Display name |
| Broom Key | `PerceivedCollectible` | Blackboard key with BroomActor reference |
| Search Radius | `500` | How far to search for nearby broom |
| bUsePerceivedBroom | `true` | Use blackboard key instead of searching |

---

### 5.7 Control Flight Task

**Type:** `BTTask_ControlFlight`

| Property | Value | Notes |
|----------|-------|-------|
| **Target Key** | `TargetLocation` | Blackboard key (Vector or Actor) |
| **Arrival Tolerance** | `100` | Horizontal distance to consider "arrived" |
| **Altitude Tolerance** | `50` | Vertical distance tolerance |
| **Use Boost When Far** | `true` | Enable boost for long distances |
| **Boost Distance Threshold** | `500` | Distance before boost activates |
| **Vertical Input Multiplier** | `1.0` | Scale for ascend/descend speed |
| **Direct Flight** | `true` | Fly straight to target in 3D |

**Details Panel Screenshot Reference:**
```
┌─────────────────────────────────────┐
│ Control Flight                      │
├─────────────────────────────────────┤
│ Fly toward: TargetLocation          │
│ Arrival tolerance: 100              │
│ Boost when >500 away                │
│ Direct 3D flight                    │
└─────────────────────────────────────┘
```

---

### 5.8 Has Channel Decorator (RIGHT - Missing Broom)

**Type:** `BTDecorator_HasChannel`

| Property | Value | Notes |
|----------|-------|-------|
| Channel Name | `Broom` | Same channel name |
| **Invert Result** | `true` | Pass when AI is MISSING the channel |
| Observer Aborts | `Both` | Re-evaluate when condition changes |

---

### 5.9 AcquisitionPath (Sequence)

**Type:** Sequence (built-in)

| Property | Value |
|----------|-------|
| Node Name | `AcquisitionPath` |

---

### 5.10 Find Collectible Service

**Type:** `BTService_FindCollectible`

| Property | Value | Notes |
|----------|-------|-------|
| **Collectible Class** | `BP_BroomCollectible_C` | The collectible Blueprint class |
| **Output Key** | `PerceivedCollectible` | Blackboard key to write result |
| **Search Radius** | `2000` | Max distance to search |
| Interval | `0.5` | How often to run (seconds) |
| Random Deviation | `0.1` | Slight randomization |

**How to Add:**
1. Right-click the Sequence node
2. **Add Service** → **Find Collectible**
3. Configure in Details panel

---

### 5.11 Move To Task

**Type:** `Move To` (built-in)

| Property | Value | Notes |
|----------|-------|-------|
| **Blackboard Key** | `PerceivedCollectible` | Target to walk toward |
| **Acceptable Radius** | `50` | How close before success |
| Allow Strafe | `true` | Smoother movement |
| Reach Test Includes Agent Radius | `true` | Account for AI size |
| Reach Test Includes Goal Radius | `true` | Account for target size |

---

### 5.12 Wait Task

**Type:** `Wait` (built-in)

| Property | Value |
|----------|-------|
| Wait Time | `0.5` |
| Random Deviation | `0.0` |

*Brief pause after reaching collectible to allow pickup interaction.*

---

## 6. New Custom Tasks

These are the NEW behavior tree tasks created for the Quidditch AI system. Add them to your behavior tree from **New Task** dropdown.

---

### 6.1 BTTask_Interact (Interact With Target)

**Purpose:** Modular task that allows AI to interact with ANY actor implementing IInteractable interface. Use this to mount brooms, open doors, activate switches, etc.

**Display Name in BT Editor:** `Interact With Target`

**How to Add:**
1. In Behavior Tree editor, click **New Task**
2. Search for **Interact With Target**
3. Drag into your behavior tree

**Details Panel Configuration:**

| Property | Type | Value | Description |
|----------|------|-------|-------------|
| **Target Key** | Blackboard Key | `PerceivedCollectible` | Actor to interact with |
| **Interaction Range** | Float | `200` | Max distance to initiate interaction |
| **Required Channel** | FName | `Broom` or `None` | Channel AI must have (leave empty for no requirement) |
| **Clear Target On Success** | Bool | `true` | Remove target from blackboard after success |
| **Success State Key** | Blackboard Key | `IsFlying` | Key to set TRUE on success (optional) |

**Behavior:**
1. Checks if AI has required channel (if specified)
2. Moves toward target if not in range
3. Calls `IInteractable::CanInteract()` on target
4. If true, calls `IInteractable::OnInteract()` with AI as interactor
5. On success: optionally clears target and sets success key

**Example Use Cases:**
- Mount broom: Target = BroomActor, RequiredChannel = "Broom", SuccessKey = "IsFlying"
- Open door: Target = DoorActor, RequiredChannel = "None"
- Activate switch: Target = SwitchActor

**Details Panel Visual:**
```
┌─────────────────────────────────────────────┐
│ Interact With Target                        │
├─────────────────────────────────────────────┤
│ Interaction                                 │
│   Target Key: PerceivedCollectible     [▼] │
│   Interaction Range: 200                    │
│   Required Channel: Broom                   │
│   Clear Target On Success: ☑               │
│   Success State Key: IsFlying          [▼] │
└─────────────────────────────────────────────┘
```

---

### 6.2 BTTask_ForceMount (DEBUG)

**Purpose:** Debug/testing utility that bypasses normal broom acquisition. Spawns a broom at AI location and mounts immediately. **USE FOR TESTING ONLY.**

**Display Name in BT Editor:** `Force Mount (DEBUG)`

**How to Add:**
1. In Behavior Tree editor, click **New Task**
2. Search for **Force Mount**
3. Add as FIRST task in tree (before any other logic)

**Details Panel Configuration:**

| Property | Type | Value | Description |
|----------|------|-------|-------------|
| **Broom Class** | TSubclassOf<ABroomActor> | `BP_BroomActor` | Blueprint class to spawn |
| **Spawn Offset** | FVector | `(0, 0, 50)` | Offset from AI location |

**Behavior:**
1. Checks if AI already has broom component and is flying → Success
2. Spawns broom from BroomClass at AI location + offset
3. Calls `IInteractable::OnInteract()` on spawned broom
4. Verifies AI is now flying
5. Sets `IsFlying = true` in blackboard

**When to Use:**
- Testing BTTask_ControlFlight in isolation
- Verifying flight mechanics work before full BT is configured
- Quick iteration on flight destination/behavior

**When to Remove:**
- Before final testing
- Before shipping/demo
- Once full BT flow is verified

**Details Panel Visual:**
```
┌─────────────────────────────────────────────┐
│ Force Mount (DEBUG)                         │
├─────────────────────────────────────────────┤
│ Debug                                       │
│   Broom Class: BP_BroomActor           [▼] │
│   Spawn Offset: X=0 Y=0 Z=50               │
└─────────────────────────────────────────────┘
```

**Quick Test Setup:**
```
BT_QuidditchAI (TESTING MODE)
│
└── ROOT
    │
    └── [Sequence] QuickFlightTest
        │
        ├── [Task] Force Mount (DEBUG)   ← Add this first!
        │           Broom Class: BP_BroomActor
        │
        └── [Task] Control Flight
                    Target: TargetLocation
```

---

### 6.3 Complete Flow with New Tasks

Here's how to wire up the complete AI flow using BTTask_Interact:

```
BT_QuidditchAI (COMPLETE VERSION)
│
└── ROOT (BB_QuidditchAI)
    │
    └── [Selector] Main
        │
        │ ┌──────────────────────────────────────────────────────┐
        │ │ PHASE 3: Already Flying - Navigate to Target         │
        │ └──────────────────────────────────────────────────────┘
        ├── [Decorator] Blackboard: IsFlying == true
        │   │
        │   └── [Task] Control Flight
        │             Target Key: TargetLocation
        │
        │ ┌──────────────────────────────────────────────────────┐
        │ │ PHASE 2: Has Channel - Find & Mount Broom            │
        │ └──────────────────────────────────────────────────────┘
        ├── [Decorator] Has Channel: Broom
        │   │
        │   └── [Sequence] MountBroomSequence
        │       │
        │       ├── [Service] Find Collectible
        │       │             Class: BP_BroomActor
        │       │             Output: PerceivedCollectible
        │       │
        │       ├── [Task] Move To
        │       │           Target: PerceivedCollectible
        │       │
        │       └── [Task] Interact With Target  ← NEW
        │                   Target: PerceivedCollectible
        │                   Range: 200
        │                   Required Channel: (None)
        │                   Success Key: IsFlying
        │
        │ ┌──────────────────────────────────────────────────────┐
        │ │ PHASE 1: No Channel - Acquire Broom Collectible      │
        │ └──────────────────────────────────────────────────────┘
        └── [Sequence] AcquisitionPath
            │
            ├── [Service] Find Collectible
            │             Class: BP_BroomCollectible_C
            │             Output: PerceivedCollectible
            │
            ├── [Task] Move To
            │           Target: PerceivedCollectible
            │
            └── [Task] Wait (0.5s)
```

**Execution Flow:**
1. **Check Phase 3:** If `IsFlying == true` → Control Flight (already mounted)
2. **Check Phase 2:** If `Has Broom Channel` → Find BroomActor → Walk → Interact → Mount
3. **Fallback Phase 1:** Find BroomCollectible → Walk → Pickup (grants channel)
4. **Loop:** After pickup, re-evaluates → Now has channel → Phase 2 runs

---

## 7. Testing Checklist

### Level Setup

- [ ] Place `BP_BroomCollectible` in level (grants "Broom" channel on pickup)
- [ ] Place `BP_BroomActor` in level (the mountable broom)
- [ ] Place AI agent pawn with `BP_CodeQuidditchAIController`
- [ ] Set a `TargetLocation` vector in the world (or use Nav Mesh goal)

### Quick Flight Test (ForceMount)

1. Add `BTTask_ForceMount` as first task under ROOT
2. Set **Broom Class** to your `BP_Broom` Blueprint
3. Play level
4. **Expected:** AI immediately spawns broom and flies

### Full Flow Test

1. Remove ForceMount task
2. Play level
3. **Expected Sequence:**
   - [ ] AI detects BP_BroomCollectible via perception
   - [ ] AI walks toward collectible
   - [ ] AI picks up collectible (gains "Broom" channel)
   - [ ] AI detects BP_BroomActor
   - [ ] AI walks to broom
   - [ ] AI mounts broom (enters flight mode)
   - [ ] AI flies toward TargetLocation
   - [ ] AI arrives and hovers at destination

### Debug Commands

In Unreal Editor during Play:
- `'` (apostrophe) - Enable AI Debug Display
- Select AI pawn → See Behavior Tree execution
- Check Output Log for `[BTTask_ControlFlight]` messages

### Common Issues

| Symptom | Cause | Fix |
|---------|-------|-----|
| AI doesn't detect collectible | Perception not configured | Verify AI Controller has AIPerceptionComponent |
| AI walks but never flies | Missing InitializeFromAsset | Rebuild after pulling latest code |
| Control Flight fails immediately | Not flying yet | Ensure Mount Broom runs first |
| Blackboard key "not set" | Missing ResolveSelectedKey | Check InitializeFromAsset implementation |

---

## Appendix: File Locations

### C++ Source Files

| File | Path | Description |
|------|------|-------------|
| BTTask_ControlFlight.h/.cpp | `Code/AI/` | 3D flight navigation task |
| BTTask_MountBroom.h/.cpp | `Code/AI/` | Original mount broom task |
| **BTTask_Interact.h/.cpp** | `Code/AI/` | **NEW** - Modular IInteractable interaction |
| **BTTask_ForceMount.h/.cpp** | `Code/AI/` | **NEW** - Debug spawn & mount utility |
| BTService_FindCollectible.h/.cpp | `Code/AI/` | Perception-based collectible finder |
| BTDecorator_HasChannel.h/.cpp | `Code/AI/` | Channel gate decorator |
| AIC_QuidditchController.h/.cpp | `Code/AI/` | AI Controller with perception |

**Full Paths:**
```
Source/END2507/Public/Code/AI/
├── BTTask_ControlFlight.h
├── BTTask_Interact.h         ← NEW
├── BTTask_ForceMount.h       ← NEW
├── BTTask_MountBroom.h
├── BTService_FindCollectible.h
├── BTDecorator_HasChannel.h
└── AIC_QuidditchController.h

Source/END2507/Private/Code/AI/
├── BTTask_ControlFlight.cpp
├── BTTask_Interact.cpp       ← NEW
├── BTTask_ForceMount.cpp     ← NEW
├── BTTask_MountBroom.cpp
├── BTService_FindCollectible.cpp
├── BTDecorator_HasChannel.cpp
└── AIC_QuidditchController.cpp
```

### Blueprint Assets

| Asset | Suggested Location |
|-------|-------------------|
| BB_QuidditchAI | `Content/Blueprints/AI/` |
| BT_QuidditchAI | `Content/Blueprints/AI/` |
| BP_CodeQuidditchAIController | `Content/Blueprints/AI/` |
| BP_BroomCollectible | `Content/Blueprints/Items/` |
| BP_BroomActor | `Content/Blueprints/Items/` |

---

*End of Setup Guide*
