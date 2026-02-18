# Quidditch Agent Setup Guide

**Project**: WizardJam
**Developer**: Marcus Daley
**Date**: January 26, 2026
**Engine**: Unreal Engine 5.4

---

## Table of Contents

1. [Overview](#overview)
2. [Quidditch Positions](#quidditch-positions)
3. [Team Configuration](#team-configuration)
4. [Prerequisites Checklist](#prerequisites-checklist)
5. [Step-by-Step Setup](#step-by-step-setup)
   - [Part A: Create Data Assets Folder](#part-a-create-data-assets-folder)
   - [Part B: Create Data Assets for Each Position](#part-b-create-data-assets-for-each-position)
   - [Part C: Configure Agents in Level](#part-c-configure-agents-in-level)
   - [Part D: Set Up Staging Zones](#part-d-set-up-staging-zones)
   - [Part E: Configure GameMode](#part-e-configure-gamemode)
6. [Quick Demo Setup (2 Seekers)](#quick-demo-setup-2-seekers)
7. [Full Team Setup (14 Agents)](#full-team-setup-14-agents)
8. [Testing and Verification](#testing-and-verification)
9. [Troubleshooting](#troubleshooting)

---

## Overview

The Quidditch AI system uses a **Data Asset-Driven Architecture**:

```
[UQuidditchAgentData]     <-- Designer creates data assets per position/team
        |
        v
    [ABaseAgent]          <-- Agent in level has AgentDataAsset property
        |                      Getters read from data asset (or fallback)
        v
[AAIC_QuidditchController] <-- Controller reads Team/Role FROM Agent
        |
        v
  [QuidditchGameMode]     <-- Registers agent with correct Team/Role
```

**Key Principle**: One Controller class, many Agent configurations via Data Assets.

---

## Quidditch Positions

| Position | Per Team | Role Description | AI Behavior |
|----------|----------|------------------|-------------|
| **Seeker** | 1 | Catches the Golden Snitch (ends game) | Intercept prediction, chase snitch |
| **Chaser** | 3 | Handles Quaffle, scores goals (10 pts) | Flocking, passing, goal-seeking |
| **Beater** | 2 | Hits Bludgers at opponents | Inverted safety scoring, target selection |
| **Keeper** | 1 | Guards the team's goal hoops | Cohesion to goal point, intercept |

**Total per team**: 7 positions
**Full match**: 14 AI agents (or 13 AI + 1 Player)

---

## Team Configuration

| Team | Enum Value | Default Color |
|------|------------|---------------|
| Team A | `EQuidditchTeam::TeamA` | Blue (0.0, 0.4, 1.0) |
| Team B | `EQuidditchTeam::TeamB` | Red (1.0, 0.2, 0.2) |

Colors are configurable in `BP_QuidditchGameMode` under **Quidditch > Teams**.

---

## Prerequisites Checklist

Before starting, verify these are in place:

- [ ] **C++ Build Complete**: Project builds without errors
- [ ] **BP_QuidditchAgent Blueprint**: Child of `ABaseAgent` with AI Controller set to `BP_CodeQuidditchAIController`
- [ ] **BP_CodeQuidditchAIController Blueprint**: Child of `AAIC_QuidditchController`
- [ ] **BP_QuidditchGameMode Blueprint**: Child of `AQuidditchGameMode`
- [ ] **QuidditchLevel Map**: Your test level with staging zones
- [ ] **BB_QuidditchAI Blackboard**: Blackboard asset with `QuidditchRole` (Name type) key
- [ ] **BT_QuidditchAI Behavior Tree**: Assigned to the AI Controller

---

## Step-by-Step Setup

### Part A: Create Data Assets Folder

1. Open **Content Browser**
2. Navigate to `Content/Code/`
3. Right-click > **New Folder** > Name it `DataAssets`
4. Open `DataAssets` folder
5. Right-click > **New Folder** > Name it `QuidditchAgents`

**Final Path**: `Content/Code/DataAssets/QuidditchAgents/`

---

### Part B: Create Data Assets for Each Position

For each agent configuration, you need to create a Data Asset.

#### Creating a Data Asset:

1. In `Content/Code/DataAssets/QuidditchAgents/`, right-click
2. Select **Miscellaneous** > **Data Asset**
3. In the popup, search for and select **QuidditchAgentData**
4. Click **Select**
5. Name the asset (see naming convention below)
6. Double-click to open and configure

#### Data Asset Naming Convention:
```
DA_[Role]_[Team]
```

Examples:
- `DA_Seeker_TeamA`
- `DA_Seeker_TeamB`
- `DA_Chaser1_TeamA`
- `DA_Keeper_TeamB`

#### Data Asset Properties to Configure:

| Property | Category | Description | Example Value |
|----------|----------|-------------|---------------|
| **Display Name** | Agent Identity | Name shown in UI/logs | "Team A Seeker" |
| **Agent Role** | Agent Identity | Quidditch position | `Seeker` |
| **Team Affiliation** | Agent Identity | Which team | `TeamA` |
| **Flight Speed** | Performance | Units/second | 1000.0 |
| **Acceleration** | Performance | How fast to reach speed | 600.0 |
| **Max Stamina** | Performance | Stamina pool size | 100.0 |
| **Stamina Regen Rate** | Performance | Per second recovery | 10.0 |
| **Aggression Level** | Performance | 0.0-1.0 (passive to aggressive) | 0.5 |
| **Behavior Tree Asset** | AI | Which BT to use | `BT_QuidditchAI` |
| **Sight Radius** | AI Perception | Detection range | 2000.0 |
| **Risk Tolerance** | AI Behavior | 0.0-1.0 (safe to risky) | 0.5 |

---

### Part C: Configure Agents in Level

1. Open your **QuidditchLevel** map
2. In the **Place Actors** panel, search for `BP_QuidditchAgent`
3. Drag `BP_QuidditchAgent` into the level at desired spawn location
4. With the agent selected, look at the **Details Panel**
5. Find the **Quidditch** category
6. **Agent Data Asset**: Click the dropdown and select your data asset (e.g., `DA_Seeker_TeamA`)

#### Alternative: Manual Configuration (No Data Asset)

If you don't assign a Data Asset, you can configure manually:

1. Select the agent in the level
2. In **Details Panel** > **Quidditch** section:
   - **Placed Quidditch Team**: Select `TeamA` or `TeamB`
   - **Placed Preferred Role**: Select `Seeker`, `Chaser`, `Beater`, or `Keeper`

**Note**: Data Asset takes priority over manual settings if both are configured.

---

### Part D: Set Up Staging Zones

Staging zones are the locations where agents fly to before the match starts.

1. In **Content Browser**, search for `BP_QuidditchStagingZone`
2. Drag one into the level for each role/team combination you need
3. Select each staging zone and configure:
   - **Staging Team**: `TeamA` or `TeamB`
   - **Staging Role**: `Seeker`, `Chaser`, `Beater`, or `Keeper`
   - **Slot Index**: 0 for single-slot roles, 0-2 for Chasers, 0-1 for Beaters

#### Staging Zone Placement Guide:

```
                    [TEAM B SIDE]

    Keeper Zone                    Seeker Zone
         |                              |
    [KB]                           [SB]

         [CB0]  [CB1]  [CB2]
              Chaser Zones

         [BB0]        [BB1]
            Beater Zones

    =========== CENTER LINE ===========

         [BA0]        [BA1]
            Beater Zones

         [CA0]  [CA1]  [CA2]
              Chaser Zones

    [KA]                           [SA]
         |                              |
    Keeper Zone                    Seeker Zone

                    [TEAM A SIDE]
```

---

### Part E: Configure GameMode

1. Open **World Settings** (Window > World Settings)
2. Under **Game Mode**, set **GameMode Override** to `BP_QuidditchGameMode`
3. Open `BP_QuidditchGameMode` Blueprint
4. Configure these settings:

| Property | Category | Description | Default |
|----------|----------|-------------|---------|
| **Snitch Catch Points** | Scoring | Points for catching snitch | 150 |
| **Quaffle Goal Points** | Scoring | Points per goal | 10 |
| **Max Seekers Per Team** | Teams | Role slots | 1 |
| **Max Chasers Per Team** | Teams | Role slots | 3 |
| **Max Beaters Per Team** | Teams | Role slots | 2 |
| **Max Keepers Per Team** | Teams | Role slots | 1 |
| **Team A Color** | Teams | Visual color | Blue |
| **Team B Color** | Teams | Visual color | Red |
| **Match Start Countdown** | Sync | Seconds before match | 3.0 |

---

## Quick Demo Setup (2 Seekers)

For tomorrow's demo with just two Seekers chasing the Snitch:

### Step 1: Create Data Assets

1. Navigate to `Content/Code/DataAssets/QuidditchAgents/`
2. Create **DA_Seeker_TeamA**:
   - Display Name: "Team A Seeker"
   - Agent Role: **Seeker**
   - Team Affiliation: **TeamA**
3. Create **DA_Seeker_TeamB**:
   - Display Name: "Team B Seeker"
   - Agent Role: **Seeker**
   - Team Affiliation: **TeamB**

### Step 2: Place Agents in QuidditchLevel

1. Open `QuidditchLevel`
2. Place two `BP_QuidditchAgent` instances
3. First agent: Set **Agent Data Asset** = `DA_Seeker_TeamA`
4. Second agent: Set **Agent Data Asset** = `DA_Seeker_TeamB`

### Step 3: Place Staging Zones

1. Place two `BP_QuidditchStagingZone` instances
2. First zone: Team = TeamA, Role = Seeker, Slot = 0
3. Second zone: Team = TeamB, Role = Seeker, Slot = 0

### Step 4: Place the Golden Snitch

1. Search for `BP_GoldenSnitch` in Content Browser
2. Drag into level in center of play area

### Step 5: Verify GameMode

1. World Settings > GameMode Override = `BP_QuidditchGameMode`

### Step 6: Test

1. PIE (Play In Editor)
2. Watch Output Log for:
   ```
   Read from Agent: Team=TeamA, Role=Seeker, HasDataAsset=Yes
   Read from Agent: Team=TeamB, Role=Seeker, HasDataAsset=Yes
   ```
3. Verify agents fly to their staging zones
4. Verify agents chase the Snitch

---

## Full Team Setup (14 Agents)

### Complete Data Asset List:

| Asset Name | Team | Role | Slot |
|------------|------|------|------|
| DA_Seeker_TeamA | TeamA | Seeker | - |
| DA_Seeker_TeamB | TeamB | Seeker | - |
| DA_Chaser1_TeamA | TeamA | Chaser | 0 |
| DA_Chaser2_TeamA | TeamA | Chaser | 1 |
| DA_Chaser3_TeamA | TeamA | Chaser | 2 |
| DA_Chaser1_TeamB | TeamB | Chaser | 0 |
| DA_Chaser2_TeamB | TeamB | Chaser | 1 |
| DA_Chaser3_TeamB | TeamB | Chaser | 2 |
| DA_Beater1_TeamA | TeamA | Beater | 0 |
| DA_Beater2_TeamA | TeamA | Beater | 1 |
| DA_Beater1_TeamB | TeamB | Beater | 0 |
| DA_Beater2_TeamB | TeamB | Beater | 1 |
| DA_Keeper_TeamA | TeamA | Keeper | - |
| DA_Keeper_TeamB | TeamB | Keeper | - |

### Complete Staging Zone List:

| Zone | Team | Role | Slot Index |
|------|------|------|------------|
| SZ_Seeker_TeamA | TeamA | Seeker | 0 |
| SZ_Seeker_TeamB | TeamB | Seeker | 0 |
| SZ_Chaser0_TeamA | TeamA | Chaser | 0 |
| SZ_Chaser1_TeamA | TeamA | Chaser | 1 |
| SZ_Chaser2_TeamA | TeamA | Chaser | 2 |
| SZ_Chaser0_TeamB | TeamB | Chaser | 0 |
| SZ_Chaser1_TeamB | TeamB | Chaser | 1 |
| SZ_Chaser2_TeamB | TeamB | Chaser | 2 |
| SZ_Beater0_TeamA | TeamA | Beater | 0 |
| SZ_Beater1_TeamA | TeamA | Beater | 1 |
| SZ_Beater0_TeamB | TeamB | Beater | 0 |
| SZ_Beater1_TeamB | TeamB | Beater | 1 |
| SZ_Keeper_TeamA | TeamA | Keeper | 0 |
| SZ_Keeper_TeamB | TeamB | Keeper | 0 |

---

## Testing and Verification

### Log Messages to Look For:

**Success indicators**:
```
[AIC_QuidditchController] Read from Agent: Team=TeamA, Role=Seeker, HasDataAsset=Yes
[AIC_QuidditchController] RegisterAgentWithGameMode SUCCESS | Team=TeamA | Preferred=Seeker | Assigned=Seeker
[AIC_QuidditchController] Direct BB write: QuidditchRole = Seeker
```

**Failure indicators**:
```
[AIC_QuidditchController] Pawn is not BaseAgent - using deprecated controller properties
[AIC_QuidditchController] Team is None! Assign AgentDataAsset or set PlacedQuidditchTeam
[BTTask_FlyToStagingZone] No staging zone found for Team=0 Role=0
```

### Blackboard Verification:

1. PIE
2. Select an AI agent in World Outliner
3. Open **Behavior Tree Debugger** (Window > AI > Behavior Tree)
4. Check Blackboard values:
   - `QuidditchRole` should be `Seeker` (not `None`)
   - `HasBroom` should be `true` after broom collection
   - `IsFlying` should be `true` when mounted

---

## Troubleshooting

### Problem: Agent shows QuidditchRole = None

**Causes**:
1. No Data Asset assigned AND no manual properties set
2. Data Asset has Team/Role set to None
3. Registration failed (check logs)

**Fix**:
1. Select agent in level
2. Assign Data Asset OR set `PlacedQuidditchTeam` and `PlacedPreferredRole`

---

### Problem: Agent doesn't fly to staging zone

**Causes**:
1. No staging zone for agent's Team+Role combination
2. Staging zone not configured correctly
3. `BTTask_FlyToStagingZone` returning failed

**Fix**:
1. Verify staging zone exists for the Team+Role
2. Check staging zone's Team, Role, and Slot Index settings
3. Check logs for `No staging zone found`

---

### Problem: "Pawn is not BaseAgent" warning

**Cause**: Agent Blueprint doesn't inherit from `ABaseAgent`

**Fix**: Ensure `BP_QuidditchAgent` parent class is `ABaseAgent` (or child)

---

### Problem: Data Asset dropdown is empty

**Cause**: No `QuidditchAgentData` assets created yet

**Fix**: Create data assets in `Content/Code/DataAssets/QuidditchAgents/`

---

### Problem: Build errors after code changes

**Fix**: Full rebuild required for UPROPERTY changes
1. Close Editor
2. Delete `Intermediate` folder
3. Rebuild from IDE

---

## Architecture Reference

### File Locations:

| File | Purpose |
|------|---------|
| `Source/END2507/Public/Code/Quidditch/QuidditchAgentData.h` | Data Asset class definition |
| `Source/END2507/Public/Code/Actors/BaseAgent.h` | Agent with AgentDataAsset property |
| `Source/END2507/Public/Code/AI/AIC_QuidditchController.h` | AI Controller header |
| `Source/END2507/Private/Code/AI/AIC_QuidditchController.cpp` | Controller reads from Agent |
| `Source/END2507/Public/Code/GameModes/QuidditchGameMode.h` | GameMode with registration |

### Data Flow:

```
1. Designer creates DA_Seeker_TeamA (QuidditchAgentData)
2. Designer places BP_QuidditchAgent in level
3. Designer assigns DA_Seeker_TeamA to agent's AgentDataAsset
4. At runtime, controller calls Agent->GetQuidditchTeam() and Agent->GetPreferredQuidditchRole()
5. Agent's getters check data asset first, return values
6. Controller registers with GameMode using those values
7. GameMode assigns role and broadcasts delegate
8. Controller writes role to Blackboard
9. Behavior Tree reads Blackboard and executes correct behaviors
```

---

## Quick Reference Card

### Create Data Asset:
```
Content Browser > Right-click > Miscellaneous > Data Asset > QuidditchAgentData
```

### Configure Agent in Level:
```
Select BP_QuidditchAgent > Details > Quidditch > Agent Data Asset > [Select Asset]
```

### Configure Staging Zone:
```
Select BP_QuidditchStagingZone > Details > Staging Team + Staging Role + Slot Index
```

### Set GameMode:
```
World Settings > Game Mode > GameMode Override > BP_QuidditchGameMode
```

---

*End of Setup Guide*
