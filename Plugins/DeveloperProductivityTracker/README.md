# Developer Productivity Tracker

A comprehensive Unreal Engine 5 editor plugin that helps developers track work sessions, monitor external development applications, manage breaks and wellness, and visualize productivity through an atmospheric sky system.

**Developer:** Marcus Daley
**Version:** 1.0.0
**Engine:** Unreal Engine 5.x
**Platform:** Windows 64-bit

## Features

### Session Tracking
- Clock in/out functionality for work sessions
- Elapsed time tracking with persistence across editor restarts
- Activity state detection (Active, Thinking, Away, Paused)
- Session history with daily summaries
- Task linking for project management integration

### External Application Monitoring
- Detects development tools: Visual Studio, VS Code, Rider, Blender, Photoshop, etc.
- Monitors source file changes to track coding activity
- Process detection for running development applications
- Configurable application database with productivity weights

### Break & Wellness System
- **Pomodoro Timer**: Configurable work/break cycles (default 25/5/15)
- **Smart Break Detection**: Automatically detects when you step away
- **Break Quality Evaluation**: Scores breaks based on duration and disengagement
- **Stretch Reminders**: Scheduled exercise suggestions for developer health

### Atmospheric Visualization
- Dynamic sky that transitions based on work session time
- Sun and dual moon system with orbital motion
- Star visibility based on time of day
- Wellness-aware atmosphere tinting

### Data Security
- Checksum verification for tamper detection
- Installation-specific salt for data integrity
- Local-only data processing
- GDPR-compliant data export functionality

## Installation

1. Copy the `DeveloperProductivityTracker` folder to your project's `Plugins` directory
2. Regenerate project files
3. Build the project
4. Enable the plugin in Edit > Plugins if not auto-enabled

## Configuration

Access settings through **Edit > Project Settings > Plugins > Developer Productivity Tracker**

### Key Settings

| Setting | Default | Description |
|---------|---------|-------------|
| Auto Start Session | true | Start tracking when editor opens |
| Snapshot Interval | 30s | How often to capture activity state |
| Enable External Monitoring | false | Monitor VS, VS Code, etc. |
| Enable Wellness Features | true | Breaks, Pomodoro, stretches |
| Enable Sky Visualization | true | Atmospheric time-of-day system |

## Usage

### Toolbar Button
Click the Productivity button in the editor toolbar to:
- Toggle session tracking
- Access the dashboard
- View quick statistics

### Dashboard
Open via **Window > Productivity Tracker** or toolbar menu:
- View current session statistics
- Control Pomodoro timer
- Monitor wellness status
- Access session history

### Keyboard Shortcuts
(Configure in Editor Preferences > Keyboard Shortcuts)
- Toggle Session: Unbound by default
- Open Dashboard: Unbound by default

## Architecture

```
DeveloperProductivityTracker/
├── Source/
│   └── DeveloperProductivityTracker/
│       ├── Public/
│       │   ├── Core/           # Session tracking, settings, storage
│       │   ├── External/       # App monitoring, file detection
│       │   ├── Wellness/       # Pomodoro, breaks, stretches
│       │   ├── Visualization/  # Sky system, celestial bodies
│       │   └── UI/             # Dashboard, notifications
│       └── Private/
│           └── [Implementations]
├── Content/
│   ├── Materials/
│   └── Config/
└── Scripts/
    └── [Protection scripts]
```

## API Reference

### Session Tracking

```cpp
// Get the session subsystem
USessionTrackingSubsystem* Session = GEditor->GetEditorSubsystem<USessionTrackingSubsystem>();

// Start/end sessions
Session->StartSession();
Session->EndSession();

// Query session state
bool bActive = Session->IsSessionActive();
float Elapsed = Session->GetElapsedSeconds();
EActivityState State = Session->GetCurrentActivityState();
```

### Wellness

```cpp
// Get the wellness subsystem
UBreakWellnessSubsystem* Wellness = GEditor->GetEditorSubsystem<UBreakWellnessSubsystem>();

// Access Pomodoro
UPomodoroManager* Pomodoro = Wellness->GetPomodoroManager();
Pomodoro->StartPomodoro();

// Check wellness status
EWellnessStatus Status = Wellness->GetCurrentWellnessStatus();
```

## Building

### Requirements
- Unreal Engine 5.x
- Visual Studio 2022 (Windows)
- Windows SDK 10.0

### Build Steps
1. Generate project files: Right-click `.uproject` > Generate Visual Studio Files
2. Open solution and build
3. Or use: `UnrealBuildTool.exe <ProjectName> Win64 Development`

## Testing Checklist

- [ ] Session starts/stops correctly
- [ ] Activity states transition properly
- [ ] Session persists across editor restart
- [ ] External app detection works (VS, VS Code)
- [ ] File change detection triggers activity
- [ ] Pomodoro timer cycles correctly
- [ ] Break detection activates on inactivity
- [ ] Notifications display properly
- [ ] Sky visualization updates with time
- [ ] Settings save and load correctly
- [ ] Data export functions work

## License

Copyright 2026 Marcus Daley. All rights reserved.

This plugin is provided for portfolio demonstration purposes.

## Contact

Marcus Daley
Game Development Student
Full Sail University
