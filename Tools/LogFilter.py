#!/usr/bin/env python3
"""
LogFilter.py
Developer: Marcus Daley
Date: January 25, 2026

Purpose:
Filter structured logs by channel, verbosity, time range, or event name.
Reads .jsonl files and outputs matching entries.

Usage:
python LogFilter.py --session <guid> --channel AI --verbosity Error
python LogFilter.py --session <guid> --event BlackboardKeyNotSet
python LogFilter.py --session <guid> --after "2026-01-25T14:00:00Z"
"""

import json
import argparse
import sys
from pathlib import Path
from datetime import datetime
from typing import List, Dict, Optional

class LogFilter:
    def __init__(self, session_guid: str, logs_dir: str = "Saved/Logs/Structured"):
        self.session_guid = session_guid
        self.logs_dir = Path(logs_dir)
        self.session_dir = self.logs_dir / session_guid

        if not self.session_dir.exists():
            raise FileNotFoundError(f"Session directory not found: {self.session_dir}")

    def get_log_files(self) -> List[Path]:
        """Get all .jsonl event log files for this session"""
        return sorted(self.session_dir.glob("events_*.jsonl"))

    def filter_logs(self,
                   channels: Optional[List[str]] = None,
                   verbosity: Optional[str] = None,
                   event_names: Optional[List[str]] = None,
                   after: Optional[str] = None,
                   before: Optional[str] = None,
                   actor: Optional[str] = None) -> List[Dict]:
        """
        Filter logs by specified criteria

        Args:
            channels: List of channel names to include
            verbosity: Minimum verbosity level (Display, Warning, Error, Fatal)
            event_names: List of event names to include
            after: ISO 8601 timestamp - only logs after this time
            before: ISO 8601 timestamp - only logs before this time
            actor: Actor name filter

        Returns:
            List of matching log entries
        """
        verbosity_order = {"Verbose": 0, "Display": 1, "Warning": 2, "Error": 3, "Fatal": 4}
        min_verbosity_level = verbosity_order.get(verbosity, 0) if verbosity else 0

        after_dt = datetime.fromisoformat(after.replace('Z', '+00:00')) if after else None
        before_dt = datetime.fromisoformat(before.replace('Z', '+00:00')) if before else None

        matching_entries = []

        for log_file in self.get_log_files():
            with open(log_file, 'r', encoding='utf-8') as f:
                for line in f:
                    try:
                        entry = json.loads(line.strip())

                        # Channel filter
                        if channels and entry.get('channel') not in channels:
                            continue

                        # Verbosity filter
                        entry_verbosity = entry.get('verbosity', 'Display')
                        if verbosity_order.get(entry_verbosity, 0) < min_verbosity_level:
                            continue

                        # Event name filter
                        if event_names and entry.get('event_name') not in event_names:
                            continue

                        # Time range filter
                        timestamp_str = entry.get('timestamp', '')
                        if timestamp_str:
                            entry_dt = datetime.fromisoformat(timestamp_str.replace('Z', '+00:00'))
                            if after_dt and entry_dt < after_dt:
                                continue
                            if before_dt and entry_dt > before_dt:
                                continue

                        # Actor filter
                        if actor:
                            entry_actor = entry.get('context', {}).get('actor')
                            if not entry_actor or actor not in entry_actor:
                                continue

                        matching_entries.append(entry)

                    except json.JSONDecodeError:
                        print(f"Warning: Skipping invalid JSON line in {log_file}", file=sys.stderr)
                        continue

        return matching_entries

    def print_entries(self, entries: List[Dict], format: str = "human"):
        """
        Print log entries in specified format

        Args:
            entries: List of log entries
            format: Output format ("human", "json", "csv")
        """
        if format == "json":
            for entry in entries:
                print(json.dumps(entry))

        elif format == "csv":
            if entries:
                # Print CSV header
                print("timestamp,channel,event_name,verbosity,actor,metadata")
                for entry in entries:
                    metadata_str = ";".join(f"{k}={v}" for k, v in entry.get('metadata', {}).items())
                    actor = entry.get('context', {}).get('actor', '')
                    print(f"{entry['timestamp']},{entry['channel']},{entry['event_name']},"
                          f"{entry['verbosity']},{actor},\"{metadata_str}\"")

        else:  # human-readable
            for entry in entries:
                timestamp = entry.get('timestamp', 'N/A')
                channel = entry.get('channel', 'N/A')
                event_name = entry.get('event_name', 'N/A')
                verbosity = entry.get('verbosity', 'Display')
                actor = entry.get('context', {}).get('actor', 'N/A')
                metadata = entry.get('metadata', {})

                metadata_str = ", ".join(f"{k}={v}" for k, v in metadata.items())

                print(f"[{timestamp}] [{verbosity}] [{channel}] {event_name}")
                print(f"  Actor: {actor}")
                if metadata_str:
                    print(f"  Metadata: {metadata_str}")
                print()

def main():
    parser = argparse.ArgumentParser(description="Filter structured log files")
    parser.add_argument("--session", required=True, help="Session GUID")
    parser.add_argument("--logs-dir", default="Saved/Logs/Structured", help="Logs directory")
    parser.add_argument("--channel", action="append", help="Filter by channel (can specify multiple)")
    parser.add_argument("--verbosity", choices=["Display", "Warning", "Error", "Fatal", "Verbose"],
                        help="Minimum verbosity level")
    parser.add_argument("--event", action="append", help="Filter by event name (can specify multiple)")
    parser.add_argument("--after", help="Filter logs after this timestamp (ISO 8601)")
    parser.add_argument("--before", help="Filter logs before this timestamp (ISO 8601)")
    parser.add_argument("--actor", help="Filter by actor name (substring match)")
    parser.add_argument("--format", choices=["human", "json", "csv"], default="human",
                        help="Output format")

    args = parser.parse_args()

    try:
        filter = LogFilter(args.session, args.logs_dir)
        entries = filter.filter_logs(
            channels=args.channel,
            verbosity=args.verbosity,
            event_names=args.event,
            after=args.after,
            before=args.before,
            actor=args.actor
        )

        print(f"Found {len(entries)} matching entries", file=sys.stderr)
        filter.print_entries(entries, args.format)

    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
