# Privacy Policy

## Overview

The Developer Productivity Tracker respects your privacy. This document explains what data is collected, how it's used, and your rights regarding that data.

**Last Updated:** January 2026
**Version:** 1.0.0

## Key Principles

1. **Local Processing**: All data is processed on your machine
2. **No Transmission**: No data is ever sent to external servers
3. **Opt-In Features**: External monitoring is disabled by default
4. **User Control**: Full access to view, export, and delete your data
5. **Transparency**: Clear documentation of all data collection

## Data Collection

### Session Data (Always Collected When Tracking)
| Data | Purpose | Retention |
|------|---------|-----------|
| Session start/end times | Track work duration | Configurable (default 30 days) |
| Activity state | Measure active vs. idle time | Configurable (default 30 days) |
| Input timestamps | Detect activity state changes | Not stored (processed in memory) |
| Editor focus state | Determine active work | Per session |

### External Monitoring Data (Opt-In)
| Data | Purpose | Retention |
|------|---------|-----------|
| Process names | Identify development tools | Per session |
| File modification times | Detect coding activity | Per session |
| File paths | Track active files | Optional, per session |

### What We Never Collect
- Window titles (may contain private content)
- File contents
- Keyboard input / keystrokes
- Mouse movement patterns
- Screen captures
- Network activity
- Personal information
- Browsing history

## Data Storage

### Location
All data is stored locally in:
```
[YourProject]/Saved/ProductivityTracker/
```

### Format
- JSON files for session records
- Plain text for configuration
- No encryption by default (data is not sensitive)

### Retention
- Default: 30 days
- Configurable: 7-365 days
- Automatic cleanup of expired data
- Manual deletion always available

## Your Rights

### Access
You can view all collected data through:
- The productivity dashboard
- Direct file access in the Saved directory
- Data export functionality

### Export
Export your data in multiple formats:
- JSON (complete data)
- CSV (spreadsheet compatible)
- Markdown (human readable report)

Use: Settings > Export Data

### Deletion
Delete your data at any time:
- Individual sessions
- Date ranges
- All data (complete wipe)

Use: Settings > Data Management > Delete

### Control
You control what's collected:
- Enable/disable session tracking
- Enable/disable external monitoring
- Enable/disable file monitoring
- Choose what to store (app names, file paths)

## External Monitoring Details

When enabled, external monitoring:

### Does
- Check which processes are running
- Match process names against known applications
- Track which development tools are focused
- Monitor source directories for file changes
- Record application focus times

### Does Not
- Read process memory
- Inject code into applications
- Monitor non-development applications
- Track personal applications
- Access application data

### Known Applications
The plugin recognizes these categories:
- IDEs (Visual Studio, VS Code, Rider, etc.)
- Version Control (Git GUIs, P4V, etc.)
- Asset Creation (Blender, Photoshop, Maya, etc.)
- Communication (Slack, Discord, Teams)
- Project Management (Notion, Trello)

## File Monitoring Details

When enabled, file monitoring:

### Monitors
- File modification timestamps
- File creation/deletion events
- Source code extensions only (.cpp, .h, .cs, etc.)

### Does Not Monitor
- File contents
- Non-source files
- Files outside configured directories
- System directories

## Third-Party Integration

### Task Linking
You can optionally link sessions to external task IDs (Jira, Trello, etc.):
- Only the task ID is stored
- No connection to external services
- Manual entry only

### No Analytics
- No telemetry
- No usage statistics
- No crash reporting to external services
- No third-party SDKs

## Children's Privacy

This plugin is a development tool not intended for children under 13. No personal information is collected from any users.

## Changes to This Policy

Policy changes will be documented in:
- This file
- Release notes
- README changelog

## Contact

Questions about privacy:
- Review this document
- Check the SECURITY.md file
- Contact Marcus Daley

## Summary

| Category | Status |
|----------|--------|
| Data Location | Local only |
| External Transmission | None |
| Personal Data | Not collected |
| Third-Party Access | None |
| User Control | Full |
| Data Export | Available |
| Data Deletion | Available |
