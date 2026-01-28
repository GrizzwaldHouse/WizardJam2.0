#!/usr/bin/env python3
"""
LogServer.py
Real-time HTTP server for StructuredLogging plugin
Developer: Marcus Daley
Date: January 25, 2026

Purpose:
Provides REST API for querying structured log files in real-time.
Enables Claude Code to analyze logs during active PIE sessions via WebFetch.

Usage:
    python LogServer.py [--port 8765] [--logs-dir path/to/logs]

API Endpoints:
    GET /                          - Server status and available endpoints
    GET /api/sessions              - List all session GUIDs
    GET /api/sessions/latest       - Get most recent session metadata
    GET /api/sessions/<guid>       - Get session metadata
    GET /api/sessions/<guid>/events - Get all events for session
    GET /api/sessions/<guid>/errors - Get only errors/warnings
    GET /api/channels/<name>       - Filter latest session by channel
    GET /api/health                - Health check endpoint
"""

import json
import argparse
from pathlib import Path
from datetime import datetime
from typing import List, Dict, Optional
from flask import Flask, jsonify, request
from flask_cors import CORS
import re

app = Flask(__name__)
CORS(app)  # Enable CORS for browser access

# Global configuration
LOGS_BASE_DIR = None
WATCHED_SESSIONS = {}  # Cache for session metadata


class LogParser:
    """Parse .jsonl log files and extract structured events."""

    @staticmethod
    def parse_jsonl_file(file_path: Path) -> List[Dict]:
        """Parse a JSON Lines file and return list of events."""
        events = []
        if not file_path.exists():
            return events

        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                for line_num, line in enumerate(f, 1):
                    line = line.strip()
                    if not line:
                        continue
                    try:
                        event = json.loads(line)
                        events.append(event)
                    except json.JSONDecodeError as e:
                        print(f"Warning: Invalid JSON at {file_path}:{line_num}: {e}")
        except Exception as e:
            print(f"Error reading {file_path}: {e}")

        return events

    @staticmethod
    def get_session_metadata(session_dir: Path) -> Optional[Dict]:
        """Read session metadata from session_metadata.json."""
        metadata_file = session_dir / "session_metadata.json"
        if metadata_file.exists():
            try:
                with open(metadata_file, 'r', encoding='utf-8') as f:
                    return json.load(f)
            except Exception as e:
                print(f"Error reading metadata: {e}")

        # Fallback: create metadata from directory name
        return {
            "session_guid": session_dir.name,
            "start_time": datetime.fromtimestamp(session_dir.stat().st_ctime).isoformat(),
            "project_name": "WizardJam",
            "platform": "Windows"
        }

    @staticmethod
    def get_all_events(session_dir: Path) -> List[Dict]:
        """Get all events from a session directory."""
        events = []

        # Find all events_*.jsonl files
        for events_file in session_dir.glob("events_*.jsonl"):
            events.extend(LogParser.parse_jsonl_file(events_file))

        # Sort by timestamp
        events.sort(key=lambda e: e.get('timestamp', ''))
        return events


class SessionManager:
    """Manage session discovery and caching."""

    def __init__(self, logs_dir: Path):
        self.logs_dir = logs_dir

    def get_all_sessions(self) -> List[Dict]:
        """List all available sessions with metadata."""
        sessions = []

        if not self.logs_dir.exists():
            return sessions

        for session_dir in self.logs_dir.iterdir():
            if not session_dir.is_dir():
                continue

            # Skip if not a valid GUID format
            if not self._is_valid_guid(session_dir.name):
                continue

            metadata = LogParser.get_session_metadata(session_dir)
            metadata['path'] = str(session_dir)
            sessions.append(metadata)

        # Sort by start time (newest first)
        sessions.sort(key=lambda s: s.get('start_time', ''), reverse=True)
        return sessions

    def get_latest_session(self) -> Optional[Dict]:
        """Get the most recent session."""
        sessions = self.get_all_sessions()
        return sessions[0] if sessions else None

    def get_session(self, guid: str) -> Optional[Path]:
        """Get session directory by GUID."""
        session_dir = self.logs_dir / guid
        if session_dir.exists() and session_dir.is_dir():
            return session_dir
        return None

    @staticmethod
    def _is_valid_guid(name: str) -> bool:
        """Check if string looks like a GUID."""
        # Simple GUID pattern: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
        guid_pattern = re.compile(r'^[a-f0-9]{8}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{4}-[a-f0-9]{12}$', re.IGNORECASE)
        return bool(guid_pattern.match(name))


# ============================================================================
# API ROUTES
# ============================================================================

@app.route('/')
def index():
    """Server status and API documentation."""
    return jsonify({
        "status": "running",
        "version": "1.0.0",
        "logs_directory": str(LOGS_BASE_DIR),
        "endpoints": {
            "/": "This help page",
            "/api/health": "Health check",
            "/api/sessions": "List all sessions",
            "/api/sessions/latest": "Get latest session metadata",
            "/api/sessions/<guid>": "Get session metadata",
            "/api/sessions/<guid>/events": "Get all events (optional ?limit=N)",
            "/api/sessions/<guid>/errors": "Get errors and warnings only",
            "/api/channels/<name>": "Filter latest session by channel (e.g., 'AI.Perception')"
        }
    })


@app.route('/api/health')
def health():
    """Health check endpoint."""
    return jsonify({"status": "healthy", "timestamp": datetime.utcnow().isoformat()})


@app.route('/api/sessions')
def list_sessions():
    """List all available sessions."""
    manager = SessionManager(LOGS_BASE_DIR)
    sessions = manager.get_all_sessions()

    return jsonify({
        "count": len(sessions),
        "sessions": sessions
    })


@app.route('/api/sessions/latest')
def get_latest_session():
    """Get the most recent session metadata."""
    manager = SessionManager(LOGS_BASE_DIR)
    session = manager.get_latest_session()

    if not session:
        return jsonify({"error": "No sessions found"}), 404

    return jsonify(session)


@app.route('/api/sessions/<guid>')
def get_session_metadata(guid: str):
    """Get metadata for a specific session."""
    manager = SessionManager(LOGS_BASE_DIR)
    session_dir = manager.get_session(guid)

    if not session_dir:
        return jsonify({"error": f"Session {guid} not found"}), 404

    metadata = LogParser.get_session_metadata(session_dir)
    return jsonify(metadata)


@app.route('/api/sessions/<guid>/events')
def get_session_events(guid: str):
    """Get all events for a session."""
    manager = SessionManager(LOGS_BASE_DIR)
    session_dir = manager.get_session(guid)

    if not session_dir:
        return jsonify({"error": f"Session {guid} not found"}), 404

    events = LogParser.get_all_events(session_dir)

    # Apply limit if requested
    limit = request.args.get('limit', type=int)
    if limit and limit > 0:
        events = events[-limit:]  # Get last N events

    # Apply channel filter if requested
    channel = request.args.get('channel')
    if channel:
        events = [e for e in events if e.get('channel') == channel]

    # Apply verbosity filter if requested
    verbosity = request.args.get('verbosity')
    if verbosity:
        events = [e for e in events if e.get('verbosity') == verbosity]

    return jsonify({
        "session_guid": guid,
        "count": len(events),
        "events": events
    })


@app.route('/api/sessions/<guid>/errors')
def get_session_errors(guid: str):
    """Get only errors and warnings for a session."""
    manager = SessionManager(LOGS_BASE_DIR)
    session_dir = manager.get_session(guid)

    if not session_dir:
        return jsonify({"error": f"Session {guid} not found"}), 404

    events = LogParser.get_all_events(session_dir)

    # Filter for errors and warnings
    error_events = [e for e in events if e.get('verbosity') in ['Error', 'Warning', 'Fatal']]

    return jsonify({
        "session_guid": guid,
        "count": len(error_events),
        "errors": error_events
    })


@app.route('/api/channels/<channel_name>')
def get_channel_events(channel_name: str):
    """Get events from latest session filtered by channel."""
    manager = SessionManager(LOGS_BASE_DIR)
    session = manager.get_latest_session()

    if not session:
        return jsonify({"error": "No sessions found"}), 404

    session_dir = manager.get_session(session['session_guid'])
    events = LogParser.get_all_events(session_dir)

    # Filter by channel
    channel_events = [e for e in events if e.get('channel') == channel_name]

    return jsonify({
        "session_guid": session['session_guid'],
        "channel": channel_name,
        "count": len(channel_events),
        "events": channel_events
    })


@app.route('/api/sessions/latest/events')
def get_latest_session_events():
    """Get all events from the latest session."""
    manager = SessionManager(LOGS_BASE_DIR)
    session = manager.get_latest_session()

    if not session:
        return jsonify({"error": "No sessions found"}), 404

    return get_session_events(session['session_guid'])


@app.route('/api/sessions/latest/errors')
def get_latest_session_errors():
    """Get errors/warnings from the latest session."""
    manager = SessionManager(LOGS_BASE_DIR)
    session = manager.get_latest_session()

    if not session:
        return jsonify({"error": "No sessions found"}), 404

    return get_session_errors(session['session_guid'])


# ============================================================================
# MAIN
# ============================================================================

def main():
    parser = argparse.ArgumentParser(description='StructuredLogging HTTP Server')
    parser.add_argument('--port', type=int, default=8765, help='Port to run server on (default: 8765)')
    parser.add_argument('--logs-dir', type=str, help='Path to Saved/Logs/Structured directory')
    parser.add_argument('--host', type=str, default='127.0.0.1', help='Host to bind to (default: 127.0.0.1)')

    args = parser.parse_args()

    # Determine logs directory
    global LOGS_BASE_DIR
    if args.logs_dir:
        LOGS_BASE_DIR = Path(args.logs_dir)
    else:
        # Auto-detect: assume running from Plugins/StructuredLogging/Tools/
        script_dir = Path(__file__).parent
        project_root = script_dir.parent.parent.parent  # Go up to project root
        LOGS_BASE_DIR = project_root / "Saved" / "Logs" / "Structured"

    print("=" * 70)
    print("StructuredLogging HTTP Server")
    print("=" * 70)
    print(f"Logs Directory: {LOGS_BASE_DIR}")
    print(f"Server URL: http://{args.host}:{args.port}")
    print(f"Dashboard: http://{args.host}:{args.port}/")
    print("=" * 70)
    print("\nAvailable Endpoints:")
    print(f"  - GET http://localhost:{args.port}/api/sessions")
    print(f"  - GET http://localhost:{args.port}/api/sessions/latest")
    print(f"  - GET http://localhost:{args.port}/api/sessions/<guid>/events")
    print(f"  - GET http://localhost:{args.port}/api/sessions/<guid>/errors")
    print(f"  - GET http://localhost:{args.port}/api/channels/<name>")
    print("=" * 70)
    print("\nPress Ctrl+C to stop server\n")

    # Verify logs directory exists
    if not LOGS_BASE_DIR.exists():
        print(f"WARNING: Logs directory does not exist yet: {LOGS_BASE_DIR}")
        print("Server will wait for logs to be created during PIE sessions.\n")
    else:
        # Show existing sessions
        manager = SessionManager(LOGS_BASE_DIR)
        sessions = manager.get_all_sessions()
        if sessions:
            print(f"Found {len(sessions)} existing session(s):")
            for session in sessions[:5]:  # Show first 5
                print(f"  - {session['session_guid']} ({session.get('start_time', 'unknown time')})")
            if len(sessions) > 5:
                print(f"  ... and {len(sessions) - 5} more")
        print()

    # Run Flask server
    try:
        app.run(host=args.host, port=args.port, debug=False)
    except KeyboardInterrupt:
        print("\n\nServer stopped by user.")


if __name__ == '__main__':
    main()
