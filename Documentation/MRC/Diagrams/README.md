# WizardJam Quidditch AI - Visual Diagrams

Generated: February 15, 2026
All diagrams are interactive FigJam files hosted on Figma.

## Diagram Index

### 1. BT_QuidditchAI - Seeker Behavior Tree
**Type:** Flowchart (LR)
**Shows:** Full behavior tree structure with 4 phases: AcquireBroom, PrepareForFlight, FlyToMatchStart, PlayQuidditchMatch. Color-coded by node type (orange=composites, blue=services, purple=decorators, green=match phase).

[Open in FigJam](https://www.figma.com/online-whiteboard/create-diagram/9acce08d-f6bc-45bc-83e0-1c5182611957?utm_source=claude&utm_content=edit_in_figjam)

---

### 2. Seeker Vertical Slice Pipeline
**Type:** Flowchart (LR)
**Shows:** Complete end-to-end pipeline from agent spawn through match end. Every step labeled with the C++ function or BT node responsible. Highlights key transitions: ground-to-air (green), match start/end (red), catch event (orange).

[Open in FigJam](https://www.figma.com/online-whiteboard/create-diagram/c3fc8c6d-3859-453e-b553-e004fd817b92?utm_source=claude&utm_content=edit_in_figjam)

---

### 3. Quidditch Match State Machine
**Type:** State Diagram
**Shows:** All 8 match states (Initializing, FlyingToStart, WaitingForReady, Countdown, InProgress, PlayerJoining, SnitchCaught, Ended) with transition triggers. Includes the new Countdown -> WaitingForReady transition when an agent leaves their staging zone.

[Open in FigJam](https://www.figma.com/online-whiteboard/create-diagram/4abeec4b-39ce-4a39-a523-83b3232fdc75?utm_source=claude&utm_content=edit_in_figjam)

---

### 4. Delegate Event Flow - Gas Station Pattern
**Type:** Sequence Diagram
**Shows:** Complete delegate binding and event flow between AIC_QuidditchController, AC_BroomComponent, QuidditchStagingZone, QuidditchGameMode, and SnitchBall. Traces the full lifecycle from OnPossess binding through match end broadcast.

[Open in FigJam](https://www.figma.com/online-whiteboard/create-diagram/34f907b6-1649-40ce-a157-eed8a6164b64?utm_source=claude&utm_content=edit_in_figjam)

---

## Inline Mermaid Diagrams

Each MRC card (MRC-001 through MRC-005) contains inline Mermaid diagrams that render in VS Code, GitHub, and any Markdown viewer with Mermaid support. These provide the same visual information in a portable format that doesn't require Figma access.

## Color Legend

| Color | Meaning |
|-------|---------|
| Blue (#4a90d9) | Root / Primary nodes |
| Orange (#e8a838) | Composite / Branch nodes |
| Green (#2ecc71) | Match phase / Success states |
| Purple (#9b59b6) | Decorators (conditional gates) |
| Light Blue (#3498db) | Services (background polling) |
| Red (#e74c3c) | Match start/end events |
| Yellow (#f39c12) | Catch / Score events |
| Gray (#95a5a6) | Setup / Initialization |
