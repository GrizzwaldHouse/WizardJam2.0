# WizardJam Quidditch AI - Maintenance Requirement Cards (MRC)

**Developer:** Marcus Daley
**Date:** February 15, 2026
**Project:** WizardJam 2.0 (END2507)
**Engine:** Unreal Engine 5.4

---

## Purpose

This document set provides Navy-style Maintenance Requirement Cards (MRCs) for the WizardJam Quidditch AI system. Each card is a structured, step-by-step procedure that can be followed independently to set up, verify, or extend the AI system.

The MRC format ensures:
- **Reproducibility** -- Any developer can follow the steps and get the same result
- **Traceability** -- Each card references specific source files and line numbers
- **Safety** -- Precautions and verification steps prevent silent failures
- **Cross-referencing** -- Cards reference each other for complete coverage

---

## Card Index

### Phase 1: Seeker Vertical Slice

| MRC # | Title | Status | Prerequisites |
|-------|-------|--------|---------------|
| MRC-001 | [Initialize Staging Zone and Agent Positions](MRC-001_Initialize_Staging_Zone.md) | Ready | None |
| MRC-002 | [Bind Pawn Overlap Events with Delegate Handling](MRC-002_Bind_Overlap_Events.md) | Ready | MRC-001 |
| MRC-003 | [Validate RequiredAgentOverride Logic](MRC-003_Validate_RequiredAgentOverride.md) | Ready | MRC-001 |
| MRC-004 | [Test Seeker Pathing and Snitch Acquisition](MRC-004_Test_Seeker_Pathing.md) | Ready | MRC-001, MRC-002, MRC-003 |
| MRC-005 | [Finalize End-of-Vertical-Slice Signals](MRC-005_Finalize_Vertical_Slice.md) | Ready | MRC-004 |

### Math Skills Cards (Study + Validation)

| MRC # | Title | Level | Prerequisites |
|-------|-------|-------|---------------|
| MRC-MATH-S001 | [Vector Intercept (Lead Target Prediction)](MRC-MATH-S001_Vector_Intercept.md) | Intermediate | Vectors, Velocity, Quadratic Formula |
| MRC-MATH-S002 | [3D Steering: Seek, Arrive, Flee](MRC-MATH-S002_3D_Steering.md) | Foundational | Vector normalization, Dot product, Clamp |
| MRC-MATH-S003 | [Collision Avoidance (Obstacle Avoidance)](MRC-MATH-S003_Collision_Avoidance.md) | Intermediate | Line traces, Dot product, Cross product |
| MRC-MATH-S004 | [Discrete Logic Patterns (Ternary/Switch/While)](MRC-MATH-S004_Discrete_Logic_Patterns.md) | Foundational | C++ control flow |
| MRC-MATH-S005 | [Flocking (Alignment/Separation/Cohesion)](MRC-MATH-S005_Flocking.md) | Intermediate | Vectors, Aggregation |

### Templates and Reference

| Document | Purpose |
|----------|---------|
| [MRC-TEMPLATE: New AI Role](MRC-TEMPLATE_New_AI_Role.md) | Reusable template for adding Chaser, Beater, or Keeper roles |
| [AAA Standards Reference](Reference_AAA_Standards.md) | Design rationale and industry standard alignment |
| [Visual Diagrams](Diagrams/README.md) | FigJam links and Mermaid diagram index |

---

## Visual Diagrams (FigJam)

| Diagram | Type | Link |
|---------|------|------|
| BT_QuidditchAI Behavior Tree | Flowchart | [Open](https://www.figma.com/online-whiteboard/create-diagram/9acce08d-f6bc-45bc-83e0-1c5182611957?utm_source=claude&utm_content=edit_in_figjam) |
| Seeker Vertical Slice Pipeline | Flowchart | [Open](https://www.figma.com/online-whiteboard/create-diagram/c3fc8c6d-3859-453e-b553-e004fd817b92?utm_source=claude&utm_content=edit_in_figjam) |
| Match State Machine | State Diagram | [Open](https://www.figma.com/online-whiteboard/create-diagram/4abeec4b-39ce-4a39-a523-83b3232fdc75?utm_source=claude&utm_content=edit_in_figjam) |
| Delegate Event Flow | Sequence Diagram | [Open](https://www.figma.com/online-whiteboard/create-diagram/34f907b6-1649-40ce-a157-eed8a6164b64?utm_source=claude&utm_content=edit_in_figjam) |

---

## Execution Order

For first-time setup of the Seeker vertical slice, follow cards in this order:

```
MRC-001 (Place staging zone + agent)
    |
    +---> MRC-002 (Verify delegate bindings -- code review only)
    |
    +---> MRC-003 (Set RequiredAgentOverride in BP)
    |
    v
MRC-004 (PIE test: full Seeker pipeline)
    |
    v
MRC-005 (Verify match end signals)
```

MRC-002 and MRC-003 can be done in parallel since they are independent verification steps. MRC-004 requires all three preceding cards to be complete.

---

## Math Skills Card Dependency Map

```
MRC-MATH-S002 (Seek/Arrive/Flee)  ─── Foundational
    |
    +──→ MRC-MATH-S001 (Intercept) ──── Uses Seek on intercept output
    |       |
    |       +──→ MRC-MATH-S003 (Avoidance) ── Blends avoidance with intercept path
    |
    +──→ MRC-MATH-S005 (Flocking) ──── Builds on Seek/Flee primitives
            |
            +──→ MRC-MATH-S003 (Avoidance) ── Supplements Separation

MRC-MATH-S004 (Discrete Logic) ──── Independent, used across all cards
```

Study order for a new developer:
1. S002 (Steering fundamentals)
2. S004 (C++ patterns for UE5)
3. S001 (Intercept math)
4. S005 (Flocking)
5. S003 (Avoidance -- ties everything together)

---

## Source File Quick Reference

| File | Role |
|------|------|
| `Source/END2507/Public/Code/AI/AIC_QuidditchController.h` | Main AI controller (257 lines) |
| `Source/END2507/Private/Code/AI/AIC_QuidditchController.cpp` | Controller implementation (~1080 lines) |
| `Source/END2507/Public/Code/GameModes/QuidditchGameMode.h` | Match orchestration (456 lines) |
| `Source/END2507/Private/Code/GameModes/QuidditchGameMode.cpp` | GameMode implementation (~840 lines) |
| `Source/END2507/Public/Code/Flight/AC_BroomComponent.h` | Shared flight component (264 lines) |
| `Source/END2507/Private/Code/Flight/AC_BroomComponent.cpp` | Flight implementation (801 lines) |
| `Source/END2507/Public/Code/Quidditch/SnitchBall.h` | Snitch pawn (272 lines) |
| `Source/END2507/Public/Code/Quidditch/QuidditchStagingZone.h` | Staging zone actor (145 lines) |
| `Source/END2507/Public/Code/Quidditch/QuidditchTypes.h` | Shared enums/structs (249 lines) |
| `Source/END2507/Public/Code/AI/Quidditch/BTTask_PredictIntercept.h` | Intercept prediction (S001) |
| `Source/END2507/Private/Code/AI/Quidditch/BTTask_PredictIntercept.cpp` | Quadratic solver implementation |
| `Source/END2507/Public/Code/Utilities/FlyingSteeringComponent.h` | Flock.cs port (S005) |
| `Source/END2507/Public/Code/Flight/AC_FlightSteeringComponent.h` | Obstacle-aware steering (S002, S003) |
| `Source/END2507/Public/Code/AI/Quidditch/BTTask_FlockWithTeam.h` | BT flock integration (S005) |

---

## Review History

| Date | Change | Author |
|------|--------|--------|
| Feb 15, 2026 | Initial MRC creation (5 cards + template + reference) | Marcus Daley |
| Feb 15, 2026 | Code fixes applied: staging zone exit handler, BroomComponent delegate unbind, RequiredAgentOverride | Claude Code Agent |
| Feb 16, 2026 | Added 5 Math Skills Cards (S001-S005) with C++ templates, BP nodes, unit tests, gamified tasks | Marcus Daley |
