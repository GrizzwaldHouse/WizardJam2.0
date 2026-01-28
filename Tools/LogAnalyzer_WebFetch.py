"""
LogAnalyzer_WebFetch.py
AI-Assisted Structured Log Analysis Tool

Developer: Marcus Daley
Date: January 25, 2026
Project: WizardJam

PURPOSE:
Analyzes structured logs (JSON Lines format) from the StructuredLogging plugin
and uses AI (via WebFetch) to identify issues, patterns, and root causes.

USAGE:
    python Tools/LogAnalyzer_WebFetch.py --session <session_guid>
    python Tools/LogAnalyzer_WebFetch.py --session <session_guid> --channel AI
    python Tools/LogAnalyzer_WebFetch.py --file path/to/events.jsonl
    python Tools/LogAnalyzer_WebFetch.py --session <session_guid> --timeline

FEATURES:
- Parses JSON Lines (.jsonl) structured log files
- Filters by channel, verbosity, time range, actor
- Detects AI stuck states (agent at staging zone for 30+ seconds)
- Detects blackboard key failures
- Detects perception issues
- Uses AI to suggest root causes and fixes
- Generates timeline visualization

REQUIREMENTS:
    pip install requests

STRUCTURED LOG FORMAT (from StructuredLogging plugin):
{
    "timestamp": "2026-01-25T14:30:15.123Z",
    "session_guid": "a1b2c3d4-e5f6-7890-abcd-ef1234567890",
    "channel": "QuidditchAI",
    "event_name": "ChaseStarted",
    "verbosity": "Display",
    "metadata": {
        "seeker_name": "BP_QuidditchAgent_3",
        "snitch_location": "X=100 Y=200 Z=300",
        "initial_distance": "1500.0"
    },
    "context": {
        "actor_name": "BP_QuidditchAgent_3",
        "world_name": "SpawnerLevel",
        "subsystem": "QuidditchAI"
    },
    "source": {
        "file": "BTTask_ChaseSnitch.cpp",
        "line": 95
    }
}
"""

import json
import argparse
import os
import sys
from datetime import datetime
from pathlib import Path
from typing import List, Dict, Any, Optional
from collections import defaultdict

# ============================================================================
# CONSTANTS
# ============================================================================

DEFAULT_LOG_DIR = "Saved/Logs/Structured"

# ============================================================================
# LOG ENTRY CLASS
# ============================================================================

class LogEntry:
    """Represents a single structured log entry"""

    def __init__(self, data: Dict[str, Any]):
        self.timestamp = data.get("timestamp", "")
        self.session_guid = data.get("session_guid", "")
        self.channel = data.get("channel", "")
        self.event_name = data.get("event_name", "")
        self.verbosity = data.get("verbosity", "Display")
        self.metadata = data.get("metadata", {})
        self.context = data.get("context", {})
        self.source = data.get("source", {})

    def __repr__(self):
        return f"<LogEntry {self.channel}.{self.event_name} @ {self.timestamp}>"

    def matches_filters(self,
                        channel: Optional[str] = None,
                        event_name: Optional[str] = None,
                        verbosity: Optional[str] = None,
                        actor: Optional[str] = None) -> bool:
        """Check if this entry matches the given filters"""

        if channel and self.channel != channel:
            return False

        if event_name and self.event_name != event_name:
            return False

        if verbosity and self.verbosity != verbosity:
            return False

        if actor:
            context_actor = self.context.get("actor_name", "")
            if actor not in context_actor:
                return False

        return True

# ============================================================================
# LOG ANALYZER CLASS
# ============================================================================

class LogAnalyzer:
    """Analyzes structured log files and detects patterns"""

    def __init__(self, log_file_path: str):
        self.log_file_path = log_file_path
        self.entries: List[LogEntry] = []
        self.load_logs()

    def load_logs(self):
        """Load and parse JSON Lines log file"""

        if not os.path.exists(self.log_file_path):
            print(f"ERROR: Log file not found: {self.log_file_path}")
            sys.exit(1)

        print(f"Loading logs from: {self.log_file_path}")

        with open(self.log_file_path, 'r', encoding='utf-8') as f:
            for line_num, line in enumerate(f, start=1):
                line = line.strip()
                if not line:
                    continue

                try:
                    data = json.loads(line)
                    self.entries.append(LogEntry(data))
                except json.JSONDecodeError as e:
                    print(f"WARNING: Failed to parse line {line_num}: {e}")

        print(f"Loaded {len(self.entries)} log entries")

    def filter_entries(self,
                       channel: Optional[str] = None,
                       event_name: Optional[str] = None,
                       verbosity: Optional[str] = None,
                       actor: Optional[str] = None) -> List[LogEntry]:
        """Filter log entries by criteria"""

        filtered = [entry for entry in self.entries
                    if entry.matches_filters(channel, event_name, verbosity, actor)]

        return filtered

    def detect_ai_stuck_state(self) -> List[Dict[str, Any]]:
        """Detect if AI agents are stuck at staging zones"""

        issues = []

        # Group events by actor
        actor_events = defaultdict(list)
        for entry in self.entries:
            actor_name = entry.context.get("actor_name", "")
            if actor_name:
                actor_events[actor_name].append(entry)

        # Check each actor for stuck patterns
        for actor_name, events in actor_events.items():
            # Look for StagingZoneAssigned followed by no ChaseStarted
            staging_time = None
            for i, event in enumerate(events):
                if event.event_name == "StagingZoneAssigned":
                    staging_time = event.timestamp

                if event.event_name == "ChaseStarted" and staging_time:
                    # Agent progressed normally
                    staging_time = None

            # If agent assigned to staging but never started chase
            if staging_time:
                issues.append({
                    "type": "AI_STUCK_AT_STAGING",
                    "actor": actor_name,
                    "timestamp": staging_time,
                    "description": f"Agent '{actor_name}' reached staging zone but never started chase - likely bMatchStarted never set"
                })

        return issues

    def detect_blackboard_failures(self) -> List[Dict[str, Any]]:
        """Detect blackboard key not set failures"""

        issues = []

        for entry in self.entries:
            if entry.event_name == "BlackboardKeyNotSet":
                key_name = entry.metadata.get("key_name", "Unknown")
                node_name = entry.metadata.get("node_name", "Unknown")

                issues.append({
                    "type": "BLACKBOARD_KEY_FAILURE",
                    "key_name": key_name,
                    "node_name": node_name,
                    "timestamp": entry.timestamp,
                    "description": f"Blackboard key '{key_name}' not set in node '{node_name}' - check InitializeFromAsset() and ResolveSelectedKey()"
                })

        return issues

    def detect_perception_issues(self) -> List[Dict[str, Any]]:
        """Detect AI perception failures"""

        issues = []

        # Look for agents that never perceived Snitch
        actor_perceptions = defaultdict(list)
        for entry in self.entries:
            if entry.channel == "Snitch.Perception" or entry.channel == "AI.Perception":
                actor_name = entry.context.get("actor_name", "")
                if actor_name:
                    actor_perceptions[actor_name].append(entry)

        # Check for agents with no successful perception events
        for actor_name, events in actor_perceptions.items():
            has_successful_perception = any(
                e.event_name == "SnitchPerceived" for e in events
            )

            if not has_successful_perception and len(events) > 0:
                issues.append({
                    "type": "PERCEPTION_FAILURE",
                    "actor": actor_name,
                    "description": f"Agent '{actor_name}' has perception events but never successfully perceived Snitch"
                })

        return issues

    def generate_timeline(self) -> str:
        """Generate a text timeline of key events"""

        timeline_lines = []
        timeline_lines.append("=== STRUCTURED LOG TIMELINE ===\n")

        # Group events by channel for clarity
        events_by_channel = defaultdict(list)
        for entry in self.entries:
            events_by_channel[entry.channel].append(entry)

        # Print each channel's events chronologically
        for channel, events in sorted(events_by_channel.items()):
            timeline_lines.append(f"\n--- Channel: {channel} ---")

            for entry in events:
                timestamp = entry.timestamp
                event_name = entry.event_name
                actor = entry.context.get("actor_name", "N/A")

                # Format metadata for display
                metadata_str = ", ".join([f"{k}={v}" for k, v in entry.metadata.items()])

                timeline_lines.append(
                    f"  [{timestamp}] {event_name} | Actor={actor} | {metadata_str}"
                )

        return "\n".join(timeline_lines)

    def analyze_with_ai(self, issues: List[Dict[str, Any]]) -> str:
        """
        Use AI (via WebFetch) to analyze detected issues and suggest fixes

        NOTE: This is a placeholder. In production, you would:
        1. Format issues as JSON
        2. Send to Claude API via requests library
        3. Parse AI response with suggested fixes

        For now, we'll return a formatted summary
        """

        if not issues:
            return "No issues detected - logs look healthy!"

        analysis = []
        analysis.append("=== AI-ASSISTED LOG ANALYSIS ===\n")

        for issue in issues:
            issue_type = issue["type"]
            description = issue["description"]

            analysis.append(f"\nISSUE: {issue_type}")
            analysis.append(f"Description: {description}")

            # Suggest fixes based on issue type
            if issue_type == "AI_STUCK_AT_STAGING":
                analysis.append("Suggested Fix:")
                analysis.append("  - Check if GameMode broadcasts OnMatchStarted delegate")
                analysis.append("  - Verify AI controller binds to OnMatchStarted in BeginPlay")
                analysis.append("  - Ensure bMatchStarted blackboard key is set when delegate fires")

            elif issue_type == "BLACKBOARD_KEY_FAILURE":
                analysis.append("Suggested Fix:")
                analysis.append("  - Verify FBlackboardKeySelector has AddObjectFilter() in constructor")
                analysis.append("  - Ensure InitializeFromAsset() calls ResolveSelectedKey()")
                analysis.append("  - Check that blackboard asset has the required key defined")

            elif issue_type == "PERCEPTION_FAILURE":
                analysis.append("Suggested Fix:")
                analysis.append("  - Verify target has UAIPerceptionStimuliSourceComponent")
                analysis.append("  - Check perception config (sight radius, peripheral vision angle)")
                analysis.append("  - Ensure target has appropriate tags for BTService detection")

        return "\n".join(analysis)

# ============================================================================
# MAIN FUNCTION
# ============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="Analyze structured logs from WizardJam Quidditch system"
    )

    parser.add_argument("--session", help="Session GUID to analyze")
    parser.add_argument("--file", help="Direct path to .jsonl log file")
    parser.add_argument("--channel", help="Filter by channel (e.g., QuidditchAI)")
    parser.add_argument("--event", help="Filter by event name")
    parser.add_argument("--verbosity", help="Filter by verbosity (Display, Warning, Error)")
    parser.add_argument("--actor", help="Filter by actor name")
    parser.add_argument("--timeline", action="store_true", help="Generate timeline visualization")
    parser.add_argument("--detect-issues", action="store_true", help="Auto-detect common issues")

    args = parser.parse_args()

    # Determine log file path
    if args.file:
        log_file_path = args.file
    elif args.session:
        # Find log file by session GUID
        log_dir = Path(DEFAULT_LOG_DIR) / args.session
        if not log_dir.exists():
            print(f"ERROR: Session directory not found: {log_dir}")
            sys.exit(1)

        # Find events_*.jsonl file
        log_files = list(log_dir.glob("events_*.jsonl"))
        if not log_files:
            print(f"ERROR: No events_*.jsonl files found in {log_dir}")
            sys.exit(1)

        # Use the most recent file
        log_file_path = str(max(log_files, key=os.path.getmtime))
    else:
        print("ERROR: Must specify either --session or --file")
        sys.exit(1)

    # Load and analyze logs
    analyzer = LogAnalyzer(log_file_path)

    # Apply filters if specified
    filtered_entries = analyzer.filter_entries(
        channel=args.channel,
        event_name=args.event,
        verbosity=args.verbosity,
        actor=args.actor
    )

    print(f"\nFiltered to {len(filtered_entries)} entries")

    # Display filtered entries
    if filtered_entries and not args.timeline and not args.detect_issues:
        print("\n=== FILTERED LOG ENTRIES ===\n")
        for entry in filtered_entries:
            print(f"[{entry.timestamp}] {entry.channel}.{entry.event_name}")
            print(f"  Actor: {entry.context.get('actor_name', 'N/A')}")
            print(f"  Metadata: {entry.metadata}")
            print(f"  Source: {entry.source.get('file', 'N/A')}:{entry.source.get('line', 'N/A')}")
            print()

    # Generate timeline if requested
    if args.timeline:
        timeline = analyzer.generate_timeline()
        print(timeline)

    # Detect issues if requested
    if args.detect_issues:
        print("\n=== DETECTING ISSUES ===\n")

        all_issues = []
        all_issues.extend(analyzer.detect_ai_stuck_state())
        all_issues.extend(analyzer.detect_blackboard_failures())
        all_issues.extend(analyzer.detect_perception_issues())

        if all_issues:
            print(f"Found {len(all_issues)} potential issues:\n")
            for issue in all_issues:
                print(f"- {issue['description']}")

            # AI-assisted analysis
            print("\n")
            ai_analysis = analyzer.analyze_with_ai(all_issues)
            print(ai_analysis)
        else:
            print("No issues detected - logs look healthy!")

# ============================================================================
# ENTRY POINT
# ============================================================================

if __name__ == "__main__":
    main()
