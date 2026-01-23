# WizardJam Project Context

**Developer:** Marcus Daley | **Engine:** Unreal Engine 5.4 | **Module:** END2507

---

## Quick Start

This is a wizard-themed arena combat game built as a portfolio project demonstrating AAA-level coding practices. The project follows strict standards: no EditAnywhere, all defaults in constructor initialization lists, delegate-driven communication, and forward declarations in headers. Quality and learning are prioritized over speed. Every system should be reusable across projects (95/5 Rule).

---

## Critical Rules

**Never do these:**
- Use `EditAnywhere` - Use `EditDefaultsOnly` or `EditInstanceOnly` instead
- Set default values in header property declarations
- Poll in Tick() - Use delegates and Observer pattern
- Include unnecessary headers - Use forward declarations
- Use `/** */` docblocks - Use `//` comments only
- Hardcode gameplay types - Use FName for designer-extensible types

**Always do these:**
- Initialize all defaults in constructor initialization lists
- Broadcast state changes via delegates
- Bind delegates in BeginPlay/NativeConstruct
- Unbind delegates in EndPlay/NativeDestruct
- Include `.generated.h` as LAST include in UCLASS headers

---

## File Header Template

Use this for ALL new C++ files:

```cpp
// =============================================================================
// File: [Filename].h/.cpp
// Author: Marcus Daley
// Date: [YYYY-MM-DD]
//
// Purpose:
//   [Brief description of what this class/file does]
//
// Usage:
//   [How to use this class, key methods to call, setup requirements]
//
// Dependencies:
//   [Key classes or systems this depends on]
// =============================================================================
```

---

## Behavior Tree Node Requirement

Every `FBlackboardKeySelector` needs BOTH:

1. **Constructor:** `MyKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(...), AActor::StaticClass());`
2. **Override:** `InitializeFromAsset()` calling `MyKey.ResolveSelectedKey(*BBAsset);`

Missing either causes silent failures where `IsSet()` returns false despite editor configuration.

---

## Current Focus

**Working:** AI perceives and collects broom collectible, Quidditch game mode registers agents

**Next:** Mount broom system, flight navigation to staging zone, HUD delegate bindings

---

## Property Exposure Quick Reference

| Specifier | Use When |
|-----------|----------|
| `EditDefaultsOnly` | Designer sets once for all instances (damage, speed) |
| `EditInstanceOnly` | Different per placed instance (spawn channel, team ID) |
| `VisibleAnywhere` | Runtime state, read-only (current health, ammo) |

---

## Full Documentation

See `.claude/environment.md` for comprehensive details including:
- Hardware specifications and build times
- Complete coding standards with examples
- Common issues and solutions
- Workflow guidelines
- Session memory and lessons learned
