# WizardJam 2.0

**Developer:** Marcus Daley
**Engine:** Unreal Engine 5.4
**Language:** C++

A wizard-themed Quidditch arena game built in Unreal Engine 5. AI-driven agents fly on brooms, pursue the Golden Snitch, and compete in team-based matches using a modular behavior tree architecture.

---

## Features

- **AI Quidditch System** - Behavior tree-driven agents with role assignment (Seeker, Chaser, Beater, Keeper)
- **Broom Flight** - Component-based flight system with stamina management and mount/dismount mechanics
- **Golden Snitch AI** - Autonomous evasion controller with pursuer tracking and perception-driven flight
- **Match State Machine** - 8-state game flow (Initialization through Ended) with delegate-only communication
- **Staging Zone System** - Overlap-based ready-up with team/role validation and countdown management
- **Modular BT Architecture** - 28 custom behavior tree nodes (Tasks, Services, Decorators) with proper blackboard key initialization

## Architecture

All cross-system communication uses the **Observer Pattern** (delegates and broadcasts). No GameplayStatics polling or direct GameMode calls.

```
QuidditchGameMode (Broadcaster)
  -> OnMatchStarted, OnMatchEnded, OnSnitchCaught, OnGoalScored
  -> OnCountdownTick, OnTeamScoreChanged

AIC_QuidditchController (Listener)
  -> Binds at OnPossess, unbinds at OnUnPossess
  -> Updates Blackboard keys from delegate callbacks
  -> BT Decorators read Blackboard -> drive AI behavior
```

## Project Structure

```
Source/END2507/
  Public/Code/
    AI/              - Controllers, BT nodes, Quidditch-specific services
    Actors/          - Characters, agents, pickups, projectiles
    Flight/          - BroomComponent, flight input
    GameModes/       - QuidditchGameMode, match state management
    Interfaces/      - Pickup, Interactable interfaces
  Private/Code/      - Implementation files (mirrors Public layout)

Plugins/
  StructuredLogging/              - Production logging subsystem
  DeveloperProductivityTracker/   - Editor productivity metrics
  UnrealMCP/                      - Model Context Protocol bridge
```

## Building

1. Open `WizardJam2.0.sln` in Visual Studio or Rider
2. Set configuration to `Development Editor | Win64`
3. Build solution (F5 or Ctrl+B)
4. Open `WizardJam2.0.uproject` in Unreal Editor

## Current Phase

**Phase 1: Vertical Slice** - Seeker + Snitch chase loop
- AI agents acquire brooms, fly to staging zones, and pursue the Golden Snitch
- Critical bug fixes complete (delegate unbinds, staging zone exit handling, GameplayStatics removal)

**Phase 2: Modularization Hardening** (upcoming)
**Phase 3: Role Expansion** - Chaser, Keeper, Beater gameplay loops

## License

All rights reserved. Portfolio project by Marcus Daley.
