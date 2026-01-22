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
6. [Testing Checklist](#6-testing-checklist)

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
┌─────────────┐    ┌─────────────┐
│ Acquisition │    │   Flight    │
│    Path     │    │    Path     │
└─────────────┘    └─────────────┘
      │                   │
      ▼                   ▼
 Find Collectible    Mount Broom
      │              (if not flying)
      ▼                   │
 Walk to Collectible      ▼
      │              Control Flight
      ▼              (fly to target)
 Pickup (auto)            │
      │                   ▼
      ▼                  END
 Loop back to check
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
        │           ├── [Decorator] Blackboard: IsFlying == false
        │           │   │
        │           │   └── [Task] Mount/Dismount Broom
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

## 6. Testing Checklist

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

| File | Path |
|------|------|
| BTTask_ControlFlight.h | `Source/END2507/Public/Code/AI/` |
| BTTask_ControlFlight.cpp | `Source/END2507/Private/Code/AI/` |
| BTTask_MountBroom.h | `Source/END2507/Public/Code/AI/` |
| BTTask_MountBroom.cpp | `Source/END2507/Private/Code/AI/` |
| BTService_FindCollectible.h | `Source/END2507/Public/Code/AI/` |
| BTService_FindCollectible.cpp | `Source/END2507/Private/Code/AI/` |
| BTDecorator_HasChannel.h | `Source/END2507/Public/Code/AI/` |
| BTDecorator_HasChannel.cpp | `Source/END2507/Private/Code/AI/` |
| AIC_QuidditchController.h | `Source/END2507/Public/Code/AI/` |
| AIC_QuidditchController.cpp | `Source/END2507/Private/Code/AI/` |

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
