# Connecting Claude Web and Claude Code
## A Guide for Marcus Daley's WizardJam Workflow

---

## The Challenge

Claude Web (claude.ai) and Claude Code (CLI) are separate systems. They don't share memory or context directly. However, there are effective ways to keep them synchronized.

---

## Method 1: Shared Context File (RECOMMENDED)

### How It Works
Keep a single source of truth file that both Claudes can read.

### Setup

1. **Create the context file** (already done):
   ```
   WizardJam2.0/CLAUDE.md           # Project rules
   WizardJam2.0/PROJECT_CONTEXT.md  # Quick reference
   ```

2. **Claude Code reads it automatically:**
   - Claude Code automatically reads `CLAUDE.md` in your project root
   - No setup needed - it's already working

3. **Claude Web needs manual paste:**
   - Copy `CLAUDE_WEB_PROJECT_AGENT_PROMPT.md` into Claude.ai
   - Use Claude's "Projects" feature (see below)

### Keeping Them Synced

When you learn something new or make architecture decisions:

1. Update `CLAUDE.md` in your project
2. Claude Code sees it immediately
3. Periodically update your Claude Web project instructions

---

## Method 2: Claude.ai Projects Feature

### What It Is
Claude.ai has a "Projects" feature that lets you:
- Store persistent instructions
- Upload reference files
- Maintain context across conversations

### Setup Steps

1. **Go to claude.ai**
2. **Click "Projects" in the sidebar** (or create new project)
3. **Create project: "WizardJam Development"**
4. **Add Project Instructions:**
   - Paste the entire contents of `CLAUDE_WEB_PROJECT_AGENT_PROMPT.md`
5. **Upload Reference Files (optional):**
   - Your key header files
   - Architecture diagrams
   - Screenshots of UE5 editor setup

### Benefits
- Every conversation in that project has full context
- No need to re-explain the project each time
- Can upload files for Claude to reference

---

## Method 3: Export/Import Workflow

### From Claude Web → Claude Code

When Claude Web generates useful info:

1. **In Claude Web:** Ask Claude to format output as markdown
2. **Copy the response**
3. **Save to your project:**
   ```
   WizardJam2.0/DOCS/claude_web_session_[date].md
   ```
4. **Claude Code can read it:**
   ```
   "Read the file DOCS/claude_web_session_jan24.md and continue from there"
   ```

### From Claude Code → Claude Web

When Claude Code generates useful info:

1. **Ask Claude Code to write a summary:**
   ```
   "Write a summary of what we implemented to DOCS/session_summary.md"
   ```
2. **Copy that file's contents to Claude Web**

---

## Method 4: Screenshot Bridge

### For UE5 Editor State

Claude Code and Claude Web can both analyze images.

1. **Take screenshot in UE5 Editor**
2. **Save to project:**
   ```
   WizardJam2.0/DOCS/Screenshots/BT_Setup_Jan24.png
   ```
3. **Claude Code can view it:**
   ```
   "Look at the screenshot at DOCS/Screenshots/BT_Setup_Jan24.png"
   ```
4. **Claude Web:** Upload directly to conversation

---

## Method 5: MCP Server (Advanced)

### What Is MCP?
Model Context Protocol - allows Claude Code to connect to external data sources.

### Current Limitation
Claude Web doesn't support MCP connections. This is a Claude Code-only feature.

### Future Possibility
If Anthropic adds MCP to Claude Web, you could:
- Create a shared MCP server
- Both Claudes connect to same knowledge base
- Real-time sync

### For Now
Not practical for Web↔Code sync.

---

## Recommended Workflow for Marcus

### Daily Workflow

```
┌─────────────────────────────────────────────────────────┐
│                    YOUR WORKFLOW                         │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  1. START DAY                                           │
│     └─ Open Claude Code in WizardJam2.0 folder          │
│     └─ Claude Code auto-reads CLAUDE.md                 │
│                                                          │
│  2. CODE IMPLEMENTATION                                  │
│     └─ Use Claude Code for writing C++ code             │
│     └─ Use Claude Code for git operations               │
│     └─ Use Claude Code for file analysis                │
│                                                          │
│  3. QUESTIONS / PLANNING                                │
│     └─ Use Claude Web Project for discussions           │
│     └─ Upload screenshots of UE5 editor                 │
│     └─ Get step-by-step setup guides                    │
│                                                          │
│  4. SYNC KNOWLEDGE                                       │
│     └─ Important learnings → Update CLAUDE.md           │
│     └─ Update Claude Web project instructions monthly   │
│                                                          │
│  5. END DAY                                             │
│     └─ Claude Code: "Summarize what we did today"       │
│     └─ Save summary to DOCS/daily_logs/                 │
│                                                          │
└─────────────────────────────────────────────────────────┘
```

### When to Use Which

| Task | Use Claude Code | Use Claude Web |
|------|-----------------|----------------|
| Write C++ code | YES | No |
| Read project files | YES | No (unless uploaded) |
| Git operations | YES | No |
| Long discussions | No | YES |
| Upload screenshots | Yes (file path) | YES (drag & drop) |
| Planning sessions | Either | YES (better UI) |
| Quick questions | YES | Either |
| File search | YES | No |

---

## Quick Commands

### For Claude Code

```bash
# Start with full context
cd /path/to/WizardJam2.0
claude

# Ask for analysis
"Analyze the AI folder and tell me what BT nodes exist"

# Generate sync file
"Write a summary of current project state to DOCS/current_state.md"
```

### For Claude Web

```
# Start a new conversation in your WizardJam Project
# Project instructions load automatically

# Reference previous work
"Based on the BT architecture we discussed, how do I set up the AcquireBroom sequence?"
```

---

## Files Created for You

| File | Purpose | Where to Use |
|------|---------|--------------|
| `CLAUDE_CODE_ANALYSIS_PROMPT.md` | Paste into Claude Code for feature analysis | Claude Code |
| `CLAUDE_WEB_PROJECT_AGENT_PROMPT.md` | Paste into Claude.ai Project instructions | Claude Web |
| `CLAUDE.md` | Auto-read by Claude Code | Claude Code |
| `PROJECT_CONTEXT.md` | Quick reference | Both |

---

## Summary

There's no direct API connection between Claude Web and Claude Code. The best approach is:

1. **Use CLAUDE.md as single source of truth** (auto-read by Claude Code)
2. **Create a Claude.ai Project** with the Web prompt as instructions
3. **Manually sync** important discoveries by updating CLAUDE.md
4. **Use screenshots** to bridge UE5 editor state to either Claude

This keeps both Claudes informed without complex integrations.
