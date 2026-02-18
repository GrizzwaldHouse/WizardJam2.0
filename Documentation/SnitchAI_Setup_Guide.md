# Golden Snitch AI Setup Guide

**Developer:** Marcus Daley
**Date:** January 26, 2026
**Project:** WizardJam / END2507

---

## Overview

This guide covers setting up the Golden Snitch AI system using Behavior Trees. The system consists of:

1. **BTService_TrackNearestSeeker** - Tracks nearest threat via AI perception
2. **BTTask_EvadeSeeker** - Executes evasive movement away from threats
3. **Blackboard Asset** - Stores runtime AI state
4. **Behavior Tree Asset** - Defines AI decision logic

---

## Step 1: Create Blackboard Asset

1. In Content Browser, navigate to `Content/Code/AI/`
2. Right-click > **Artificial Intelligence > Blackboard**
3. Name it `BB_SnitchAI`
4. Double-click to open

### Add Required Keys:

| Key Name | Key Type | Base Class |
|----------|----------|------------|
| `NearestSeeker` | Object | AActor |
| `bIsEvading` | Bool | - |
| `HomeLocation` | Vector | - |

**To add a key:**
1. Click **New Key** button
2. Select key type (Object, Bool, or Vector)
3. Enter the key name
4. For Object keys: Set **Base Class** to `Actor`

5. Save the Blackboard asset

---

## Step 2: Create Behavior Tree Asset

1. In Content Browser, navigate to `Content/Code/AI/`
2. Right-click > **Artificial Intelligence > Behavior Tree**
3. Name it `BT_SnitchAI`
4. Double-click to open
5. In Details panel, set **Blackboard Asset** to `BB_SnitchAI`

### Build the Tree Structure:

```
Root
└── Selector
    ├── Sequence [Evade When Threatened]
    │   ├── [Service] Track Nearest Seeker
    │   ├── [Decorator] Blackboard Based (NearestSeeker Is Set)
    │   └── [Task] Evade Seeker
    │
    └── Sequence [Default Wander]
        └── [Task] Wait (or custom wander task)
```

### Configure the Evade Sequence:

**1. Add Track Nearest Seeker Service:**
- Right-click on the Sequence node > **Add Service > Track Nearest Seeker**
- Configure in Details panel:
  - **Nearest Seeker Key:** `NearestSeeker`
  - **Valid Threat Tags:** `Seeker`, `Player`
  - **Max Tracking Distance:** `0` (unlimited) or set a range

**2. Add Blackboard Decorator:**
- Right-click on the Sequence node > **Add Decorator > Blackboard Based**
- Configure:
  - **Blackboard Key:** `NearestSeeker`
  - **Key Query:** `Is Set`
  - **Notify Observer:** `On Value Change`

**3. Add Evade Seeker Task:**
- Drag from the Sequence node > **Tasks > Evade Seeker**
- Configure in Details panel:
  - **Threat Actor Key:** `NearestSeeker`
  - **Is Evading Key:** `bIsEvading` (optional)
  - **Safe Distance:** `1500.0`
  - **Evasion Speed Multiplier:** `1.5`
  - **Max Evasion Time:** `5.0`
  - **Include Vertical Evasion:** `true`
  - **Direction Randomization:** `15.0`

5. Save the Behavior Tree

---

## Step 3: Configure AIC_SnitchController (Optional BT Mode)

The Snitch controller already uses a tick-based movement system. To use Behavior Trees instead:

### Option A: Add BT Support to Existing Controller

Edit `AIC_SnitchController.h`:

```cpp
// Add property after existing properties
UPROPERTY(EditDefaultsOnly, Category = "AI|Behavior")
UBehaviorTree* SnitchBehaviorTree;

UPROPERTY(EditDefaultsOnly, Category = "AI|Behavior")
bool bUseBehaviorTree;
```

Edit `AIC_SnitchController.cpp` in `BeginPlay()`:

```cpp
void AAIC_SnitchController::BeginPlay()
{
    Super::BeginPlay();
    SetupPerception();

    // Run behavior tree if configured
    if (bUseBehaviorTree && SnitchBehaviorTree)
    {
        RunBehaviorTree(SnitchBehaviorTree);
    }
}
```

### Option B: Use Existing Tick-Based Movement

The current `SnitchBall::UpdateMovement()` system works without BT. The BT nodes are available for alternative AI or future expansion.

---

## Step 4: Create BP_GoldenSnitch Blueprint

1. In Content Browser, navigate to `Content/Code/Actors/`
2. Right-click > **Blueprint Class**
3. Search for and select `SnitchBall` as parent
4. Name it `BP_GoldenSnitch`
5. Double-click to open

### Configure Components:

**Collision Sphere (Root):**
- Sphere Radius: `30.0`
- Collision Enabled: Query and Physics
- Generate Overlap Events: `true`

**Snitch Mesh:**
- Assign your golden snitch mesh
- Assign golden material

**Perception Source:**
- Auto Register: `true`
- Register for Sense: Sight

### Configure Movement Properties:

In Class Defaults:
- **Base Speed:** `600.0`
- **Max Evade Speed:** `1200.0`
- **Turn Rate:** `180.0`
- **Direction Change Interval:** `2.0`
- **Direction Change Variance:** `1.0`
- **Detection Radius:** `2000.0`
- **Evade Radius:** `800.0`
- **Evade Strength:** `1.5`

### Configure Bounds:

- **Play Area Volume Ref:** Assign a volume actor in your level
- OR set manual **Play Area Center** and **Play Area Extent**

5. Compile and Save

---

## Step 5: Place in Level

1. Open your Quidditch level
2. Drag `BP_GoldenSnitch` into the level
3. Position at desired spawn location
4. If using manual bounds:
   - Set **Play Area Center** to center of arena
   - Set **Play Area Extent** to half-dimensions (e.g., `5000, 5000, 2000`)

### Optional: Create Play Area Volume

1. Place a **Box Collision** or **Trigger Box** actor
2. Scale to match your arena dimensions
3. On BP_GoldenSnitch, set **Play Area Volume Ref** to this actor

---

## Step 6: Configure Seekers

For AI agents that chase the Snitch:

1. Ensure Seeker pawns have the **"Seeker"** tag
2. Add tag in Blueprint: **Tags > Add > "Seeker"**
3. Or in C++ constructor: `Tags.Add(TEXT("Seeker"));`

For the Player:
1. Add **"Player"** tag to your player pawn
2. The Snitch will evade both AI Seekers and the Player

---

## Testing Checklist

### Perception Test:
- [ ] Place Snitch and a Seeker in level
- [ ] PIE (Play In Editor)
- [ ] Snitch should detect Seeker when within DetectionRadius (2000 units)
- [ ] Check Output Log for `[Snitch] Pursuer detected: [SeekerName]`

### Evasion Test:
- [ ] Move Seeker within EvadeRadius (800 units)
- [ ] Snitch should increase speed and flee
- [ ] Check Output Log for `[Snitch] Evasion started`
- [ ] Snitch should slow down when Seeker leaves range

### Boundary Test:
- [ ] Let Snitch wander
- [ ] Verify it stays within PlayAreaExtent
- [ ] Check for soft push-back at 80% of boundary

### Debug Visualization:
- [ ] On BP_GoldenSnitch, enable **Show Debug**
- [ ] Green arrow: Current velocity
- [ ] Yellow arrow: Wander direction
- [ ] Cyan lines: Obstacle check rays
- [ ] Red sphere: Evade radius (when pursuers detected)
- [ ] Cyan box: Play area boundary

---

## Behavior Tree Node Reference

### BTService_TrackNearestSeeker

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| NearestSeekerKey | FBlackboardKeySelector | - | BB key to write nearest threat |
| ValidThreatTags | TArray<FName> | Seeker, Player | Tags that identify threats |
| MaxTrackingDistance | float | 0 (unlimited) | Max range to track |

### BTTask_EvadeSeeker

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| ThreatActorKey | FBlackboardKeySelector | - | BB key containing threat actor |
| IsEvadingKey | FBlackboardKeySelector | - | Optional: BB key for evading state |
| SafeDistance | float | 1500 | Distance before task succeeds |
| EvasionSpeedMultiplier | float | 1.5 | Movement speed multiplier |
| MaxEvasionTime | float | 5.0 | Timeout (0 = no limit) |
| bIncludeVerticalEvasion | bool | true | 3D evasion vs horizontal only |
| DirectionRandomization | float | 15.0 | Random angle variance (degrees) |

---

## Troubleshooting

### "NearestSeekerKey not set!" Error
- Full rebuild required after code changes
- Close editor, delete Intermediate/Binaries folders, rebuild

### Snitch Not Detecting Seekers
- Verify Seeker has "Seeker" or "Player" tag
- Check DetectionRadius on SnitchController (default: 2000)
- Verify Seeker pawn is registered with AI Perception

### Snitch Not Evading
- Check EvadeRadius (must be < DetectionRadius)
- Verify pursuer is within EvadeRadius (default: 800)
- Enable debug visualization to see evade radius sphere

### Snitch Flying Through Walls
- Verify ObstacleChannel is set correctly (ECC_WorldStatic)
- Check that walls have collision enabled
- Increase ObstacleCheckDistance if needed

---

## File Locations

| File | Path |
|------|------|
| BTService_TrackNearestSeeker.h | Source/END2507/Public/Code/AI/ |
| BTService_TrackNearestSeeker.cpp | Source/END2507/Private/Code/AI/ |
| BTTask_EvadeSeeker.h | Source/END2507/Public/Code/AI/ |
| BTTask_EvadeSeeker.cpp | Source/END2507/Private/Code/AI/ |
| AIC_SnitchController.h | Source/END2507/Public/Code/AI/ |
| AIC_SnitchController.cpp | Source/END2507/Private/Code/AI/ |
| SnitchBall.h | Source/END2507/Public/Code/Quidditch/ |
| SnitchBall.cpp | Source/END2507/Private/Code/Quidditch/ |
