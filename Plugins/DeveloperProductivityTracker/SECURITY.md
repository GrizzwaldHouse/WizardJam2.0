# Security Policy

## Overview

The Developer Productivity Tracker plugin is designed with security as a core principle. All data is processed and stored locally on your machine with no external transmission.

## Data Integrity

### Checksum Verification
- Every activity snapshot includes an MD5 checksum
- Session records are verified against checksums on load
- Tampering is detected and reported via the UI

### Installation Salt
- Each installation generates a unique random salt
- Salt is stored locally and never transmitted
- Checksums include the salt for installation-specific verification

### Machine Identification
- Machine ID is generated from hardware identifiers
- Used only for multi-device session tracking
- Never transmitted externally

## Secure Storage

### File Locations
All data is stored in your project's Saved directory:
```
[ProjectDir]/Saved/ProductivityTracker/
├── Sessions/          # Individual session records
├── Summaries/         # Daily summaries
├── .salt              # Installation salt (hidden)
├── .machine           # Machine identifier (hidden)
└── active_session.json # Crash recovery
```

### File Protection
The included PowerShell scripts provide optional file protection:

**Protection Levels:**
- Level 1 (+r): Headers, documentation - Read-only
- Level 2 (+h +r): Standard implementations - Hidden + Read-only
- Level 3 (+s +h +r): Core subsystems - System + Hidden + Read-only

## Data Handling

### What We Store
- Session start/end times
- Activity state snapshots
- Application names (if enabled)
- Break duration data
- Productivity statistics

### What We Never Store
- Window titles (may contain private info)
- File contents
- Passwords or credentials
- Personal identification

### Data Retention
- Default retention: 30 days
- Configurable in settings
- Automatic cleanup of old data
- Manual deletion available

## External Monitoring

### Process Detection
- Only checks process names, never memory
- Matches against known application database
- No injection or hooking

### File Monitoring
- Uses Unreal's DirectoryWatcher
- Only monitors file paths, not contents
- Only tracks source code extensions

## Vulnerability Reporting

If you discover a security vulnerability:

1. Do NOT create a public issue
2. Email the developer directly
3. Include:
   - Description of the vulnerability
   - Steps to reproduce
   - Potential impact
   - Suggested fix (if any)

## Best Practices

### For Users
1. Keep the plugin updated
2. Review settings before enabling external monitoring
3. Use file protection scripts for sensitive installations
4. Regularly review stored data
5. Delete data before sharing projects

### For Contributors
1. Never log sensitive information
2. Use checksums for all persisted data
3. Validate all external input
4. Follow the coding standards document
5. Test security features thoroughly

## Threat Model

### Considered Threats
- Data tampering (mitigated by checksums)
- Unauthorized access (mitigated by local-only storage)
- Data leakage (mitigated by privacy-first design)
- Crash data loss (mitigated by recovery system)

### Out of Scope
- Physical machine access
- Malware on the host system
- Unreal Engine vulnerabilities
- Operating system vulnerabilities

## Compliance

### GDPR Considerations
- All data stored locally
- No external transmission
- Full data export available
- Complete deletion supported
- Transparent data collection

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | Jan 2026 | Initial security implementation |

## Contact

Security concerns: Contact Marcus Daley directly
