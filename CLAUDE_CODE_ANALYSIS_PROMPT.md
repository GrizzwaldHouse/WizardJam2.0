# Claude Code Analysis Prompt
# Copy everything below the line and paste into Claude Code terminal
# ====================================================================

You are analyzing the WizardJam2.0 Unreal Engine 5.4 project for developer Marcus Daley.

## Your Task

Generate a comprehensive report on ALL behavioral tree features and AI code implemented between January 22, 2026 and January 24, 2026.

## Analysis Steps

1. **Git History Analysis**
   Run these commands to see what was committed:
   ```
   git log --since="2026-01-22" --until="2026-01-25" --oneline --stat
   git log --since="2026-01-22" --until="2026-01-25" -p --name-only
   ```

2. **Find ALL BT-Related Files**
   Search for behavioral tree tasks, services, and decorators:
   ```
   - Source/END2507/Public/Code/AI/BTTask_*.h
   - Source/END2507/Public/Code/AI/BTService_*.h
   - Source/END2507/Public/Code/AI/BTDecorator_*.h
   - Source/END2507/Private/Code/AI/BTTask_*.cpp
   - Source/END2507/Private/Code/AI/BTService_*.cpp
   - Source/END2507/Private/Code/AI/BTDecorator_*.cpp
   ```

3. **Analyze Each BT Node**
   For each file found, document:
   - File name and path
   - Purpose (from header comments)
   - Blackboard keys used (FBlackboardKeySelector properties)
   - Whether it has proper InitializeFromAsset() override
   - Key methods (ExecuteTask, TickTask, CalculateRawConditionValue, etc.)
   - Dependencies on other classes

4. **Check for AI Controllers**
   Look for:
   - AIC_QuidditchController.h/.cpp
   - Any other AI controller files

5. **Check for Flight/Broom Components**
   Look for:
   - AC_BroomComponent.h/.cpp
   - Any flight-related code

6. **Generate Report in This Format**

```
# WizardJam2.0 AI Feature Report
## Date Range: January 22-24, 2026
## Generated: [current date]

---

## Executive Summary
[Brief overview of what was implemented]

---

## Behavioral Tree Tasks (BTTask_*)

### 1. BTTask_[Name]
- **File:** [path]
- **Purpose:** [what it does]
- **Blackboard Keys:**
  - KeyName (Type) - [usage]
- **InitializeFromAsset:** [Yes/No - CRITICAL]
- **Tick Mode:** [Instant/TickTask]
- **Key Logic:** [brief description]

[Repeat for each task]

---

## Behavioral Tree Services (BTService_*)

### 1. BTService_[Name]
- **File:** [path]
- **Purpose:** [what it does]
- **Tick Interval:** [default interval]
- **Blackboard Keys:**
  - KeyName (Type) - [read/write]
- **InitializeFromAsset:** [Yes/No - CRITICAL]

[Repeat for each service]

---

## Behavioral Tree Decorators (BTDecorator_*)

### 1. BTDecorator_[Name]
- **File:** [path]
- **Purpose:** [condition it checks]
- **Blackboard Keys:**
  - KeyName (Type) - [usage]
- **InitializeFromAsset:** [Yes/No - CRITICAL]

[Repeat for each decorator]

---

## AI Controllers

### 1. AIC_[Name]
- **File:** [path]
- **Perception:** [Yes/No, what senses]
- **Team Interface:** [Yes/No]
- **Blackboard Keys Initialized:**
  - [list keys set in SetupBlackboard]

---

## Content Assets Required

Based on the C++ code, these Blueprint/Content assets need to exist:

| Asset Type | Expected Name | Purpose |
|------------|---------------|---------|
| Behavior Tree | BT_QuidditchAI | Main AI tree |
| Blackboard | BB_QuidditchAI | AI memory |
| [etc] | | |

---

## Blackboard Schema

All blackboard keys referenced in code:

| Key Name | Type | Used By | Read/Write |
|----------|------|---------|------------|
| [KeyName] | Object/Vector/Bool | [Node names] | R/W |

---

## Editor Setup Checklist

Based on code analysis, these need to be configured in UE5 Editor:

### Blackboard (BB_QuidditchAI)
- [ ] Key: [Name] (Type)
- [ ] Key: [Name] (Type)

### Behavior Tree (BT_QuidditchAI)
- [ ] Root node structure
- [ ] Service attachments
- [ ] Decorator conditions

### AI Controller Blueprint
- [ ] Assign BehaviorTreeAsset
- [ ] Assign BlackboardAsset (or use BT's blackboard)

### Agent Blueprint
- [ ] Attach required components
- [ ] Set AI Controller Class

---

## Warnings & Issues Found

1. [Any missing InitializeFromAsset overrides]
2. [Any hardcoded values that should be configurable]
3. [Any missing filter calls in constructors]

---

## Recommended BT Structure

Based on the tasks/services/decorators found:

```
BT_QuidditchAI (Recommended Structure)
├── [Node] [Decorator conditions]
│   ├── [Service attachments]
│   └── [Child tasks]
```
```

7. **Save Report**
   Write the report to: `WizardJam2.0/DOCS/AI_Feature_Report_Jan2026.md`

## Important Notes

- The developer may have uncommitted changes from yesterday. If git shows limited history, also scan the current file system for ALL BTTask_*, BTService_*, and BTDecorator_* files regardless of commit date.
- Pay special attention to the FBlackboardKeySelector initialization pattern - this was a major bug fix discovered on Jan 21.
- Check that EVERY FBlackboardKeySelector has BOTH:
  1. AddObjectFilter/AddVectorFilter/AddBoolFilter in constructor
  2. InitializeFromAsset() override with ResolveSelectedKey() call
