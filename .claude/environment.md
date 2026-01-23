# WizardJam Development Environment

**Last Updated:** January 23, 2026
**Developer:** Marcus Daley
**Project:** WizardJam (Module: END2507)
**Engine:** Unreal Engine 5.4

---

## 1. Project Identity

### Developer Background
Marcus Daley is a Navy veteran with nine years of leadership experience completing his bachelor's degree in Online Game Development at Full Sail University (graduating February 2026). He builds portfolio-quality Unreal Engine 5 projects demonstrating AAA-level coding practices under instructor Nick Penney's professional development standards.

### Development Philosophy
- **Quality over speed** - Portfolio pieces, not rushed prototypes
- **95/5 Rule** - 95% of code should work across any project with only 5% being project-specific configuration
- **Learning over shortcuts** - Understanding the "why" behind implementations
- **Reusable systems** - Every system designed for extraction and reuse

### Current Project Phase
Post-Development Structural Refinement with working feature demos every Tuesday and Thursday.

---

## 2. Hardware Specifications

### Primary Workstation: PowerSpec G759

| Component | Specification |
|-----------|---------------|
| **Processor** | AMD Ryzen 7 9800X3D (8 cores, 16 threads, 4.7GHz base, 5.2GHz boost) |
| **Graphics** | NVIDIA GeForce RTX 5080 (16GB GDDR7) |
| **Memory** | 64GB DDR5-6000 RAM |
| **Storage** | NVMe SSD for projects |

### Build Performance Expectations

| Operation | Expected Time |
|-----------|---------------|
| Full project rebuild | 15-20 minutes |
| Incremental (single file) | 30 seconds - 2 minutes |
| Header change (cascading) | 5-10 minutes |
| Blueprint-only changes | Instant |
| Editor startup | 30-45 seconds |
| PIE launch | 10-20 seconds |

### Memory Usage During Development

| State | RAM Usage |
|-------|-----------|
| Editor idle | 8-12 GB |
| Active development | 15-20 GB |
| Compilation | 28-32 GB |

---

## 3. Development Environment

### IDE Configuration

**Visual Studio 2022** (Community or Professional)
- Platform Toolset: v143
- C++ Standard: C++20 (enforced by UE5.4)

**Required Workloads:**
- Game development with C++
- .NET desktop development
- Desktop development with C++

### Unreal Engine 5.4 Specifics

- Enhanced Input System is standard (legacy input deprecated)
- World Partition and Data Layers built-in
- Lumen global illumination and Nanite enabled by default
- C++20 standard enforced

### Build Configuration Pattern

**NEVER modify Target.cs with compiler arguments** - breaks editor builds.

**Always use Build.cs for project settings:**

```csharp
// END2507.Build.cs - Correct pattern
public class END2507 : ModuleRules
{
    public END2507(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine", "InputCore",
            "EnhancedInput", "AIModule", "GameplayTasks",
            "NavigationSystem", "UMG", "AnimGraphRuntime"
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "Slate", "SlateCore"
        });

        // Suppress benign Windows SDK warnings
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            bEnableUndefinedIdentifierWarnings = false;
            PrivateDefinitions.Add("_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS=1");
        }
    }
}
```

### BuildConfiguration.xml

**Location:** `%APPDATA%\Unreal Engine\UnrealBuildTool\BuildConfiguration.xml`

```xml
<?xml version="1.0" encoding="utf-8" ?>
<Configuration xmlns="https://www.unrealengine.com/BuildConfiguration">
    <BuildConfiguration>
        <MaxParallelActions>16</MaxParallelActions>
    </BuildConfiguration>
</Configuration>
```

---

## 4. Non-Negotiable Coding Standards

These rules come from instructor Nick Penney and represent AAA studio practices. **Never violate these rules.**

### Property Exposure Rules (CRITICAL)

| Specifier | When to Use | Examples |
|-----------|-------------|----------|
| **EditDefaultsOnly** | Designer configures once for all instances | Base damage, movement speed, particle systems |
| **EditInstanceOnly** | Different values per placed instance | Spawn channel, team ID, teleport destination |
| **VisibleAnywhere** | Runtime state (read-only in editor) | Current health, ammo count, active spells |
| **NEVER EditAnywhere** | Creates confusion about where values should be set | - |

### Initialization Rules (CRITICAL)

**All default values MUST be set in constructor initialization lists:**

```cpp
// Header - NO default values
UPROPERTY(EditDefaultsOnly, Category = "Combat")
float BaseDamage;

UPROPERTY(EditDefaultsOnly, Category = "Movement")
float MoveSpeed;

// Constructor - ALL defaults here
AMyActor::AMyActor()
    : BaseDamage(25.0f)
    , MoveSpeed(600.0f)
{
    // Component creation only in body
}
```

### Communication Pattern (CRITICAL)

**Systems communicate through delegates, NEVER through polling:**

```cpp
// Component declares delegate
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, float, NewHealth);

UPROPERTY(BlueprintAssignable)
FOnHealthChanged OnHealthChanged;

// Component broadcasts
OnHealthChanged.Broadcast(CurrentHealth);

// Widget binds in NativeConstruct
HealthComp->OnHealthChanged.AddDynamic(this, &UMyWidget::HandleHealthChanged);
```

### Header Organization (CRITICAL)

**Minimize includes through forward declarations:**

```cpp
// In header - forward declare
class UParticleSystem;
class UAudioComponent;

// In cpp - full includes
#include "Particles/ParticleSystem.h"
#include "Components/AudioComponent.h"
```

### Comment Style

- Use `//` for ALL comments - **NO `/** */` docblocks**
- Comments should sound like a human explaining to another developer
- Keep comments practical and brief
- Never use asterisk-style block comments (reads as AI-generated)

---

## 5. File Header Template

**Use this template for ALL new C++ files:**

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

**Example:**

```cpp
// =============================================================================
// File: SpellCollectionComponent.h
// Author: Marcus Daley
// Date: 2026-01-15
//
// Purpose:
//   Manages the player's collected spell inventory and broadcasts changes
//   to UI systems via delegates.
//
// Usage:
//   Add to player character. Call CollectSpell(SpellName) when player
//   picks up a spell collectible. Bind to OnSpellCollected for UI updates.
//
// Dependencies:
//   ISpellCollector interface, SpellCollectible actors
// =============================================================================
```

---

## 6. Behavior Tree Checklist (MANDATORY)

**Every FBlackboardKeySelector MUST have TWO pieces of initialization:**

### 1. Key Type Filter in Constructor

```cpp
// In constructor - REQUIRED for selector to work
MyKeySelector.AddObjectFilter(this,
    GET_MEMBER_NAME_CHECKED(UMyBTNode, MyKeySelector),
    AActor::StaticClass());

// For vector keys:
MyVectorKey.AddVectorFilter(this,
    GET_MEMBER_NAME_CHECKED(UMyBTNode, MyVectorKey));

// For bool keys:
MyBoolKey.AddBoolFilter(this,
    GET_MEMBER_NAME_CHECKED(UMyBTNode, MyBoolKey));
```

### 2. Runtime Key Resolution

```cpp
// Override in header:
virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

// Implementation in cpp:
void UMyBTNode::InitializeFromAsset(UBehaviorTree& Asset)
{
    Super::InitializeFromAsset(Asset);
    if (UBlackboardData* BBAsset = GetBlackboardAsset())
    {
        MyKeySelector.ResolveSelectedKey(*BBAsset);
    }
}
```

### Symptoms of Missing Initialization

- "OutputKey is not set!" warnings despite key being selected in editor
- `FBlackboardKeySelector.IsSet()` returns false at runtime
- Blackboard values never update despite logic running correctly
- AI perceives targets but doesn't act (Move To has null target)

**This is a SILENT FAILURE - code compiles, runs, logs correctly, but blackboard writes fail.**

---

## 7. Project Architecture

### Module Structure

```
Source/END2507/
├── Public/Code/
│   ├── Actors/         # Characters, pickups, projectiles, spawners
│   ├── AI/             # Controllers, BT tasks, services, decorators
│   ├── Data/           # Data assets, structs
│   ├── Flight/         # Broom flight mechanics
│   ├── GameModes/      # Game modes, game states
│   ├── Interfaces/     # Interface declarations
│   ├── Quidditch/      # Quidditch-specific systems
│   ├── Spells/         # Spell system components
│   ├── UI/             # Widget classes
│   └── Utilities/      # Helper classes, libraries
├── Private/Code/       # (mirrors Public structure)
└── END2507.Build.cs
```

### Reusable Code Sources

| Project | Systems to Reference |
|---------|---------------------|
| **Island Escape** | Channel-based teleportation, collectible pickups, UI widgets, win conditions |
| **GAR_MarcusDaley** | Faction-aware AI, projectile combat with team ID, spinning wall obstacles |

### Hybrid Architecture Approach

1. **Reuse directly** - Copy existing code when it already solves the problem perfectly
2. **Adapt strategically** - Modify existing systems for WizardJam's needs
3. **Use as reference** - Understand patterns but implement fresh

---

## 8. Common Issues & Solutions

### MSVC C4668/C4067 Warnings

**Symptom:** Hundreds of warnings about undefined preprocessor macros in Windows SDK headers.

**Cause:** Windows SDK headers with MSVC's strict C++20 mode.

**Solution:** Suppress in Build.cs:
```csharp
bEnableUndefinedIdentifierWarnings = false;
PrivateDefinitions.Add("_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS=1");
```

### Enhanced Input Not Working

**Symptom:** Input actions defined but player doesn't respond.

**Cause:** Input Mapping Context not added to player controller.

**Solution:**
```cpp
if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
    ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
{
    Subsystem->AddMappingContext(DefaultMappingContext, 0);
}
```

### Delegate Not Broadcasting

**Symptom:** UI doesn't update when component state changes.

**Cause:** Widget never bound to delegate, or binding happened after component destroyed.

**Solution:** Bind in NativeConstruct, unbind in NativeDestruct:
```cpp
void UMyWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (AMyCharacter* Char = Cast<AMyCharacter>(GetOwningPlayerPawn()))
    {
        if (auto* HealthComp = Char->FindComponentByClass<UAC_HealthComponent>())
        {
            HealthComp->OnHealthChanged.AddDynamic(this, &UMyWidget::HandleHealthChanged);
        }
    }
}
```

### Cascading Header Recompilation

**Symptom:** Changing one header triggers recompilation of dozens of files.

**Cause:** Header included by many files creating dependency chain.

**Solution:** Use forward declarations in headers, full includes only in cpp files.

### UE5 Editor Crash on Launch

**Solution Sequence:**
1. Delete Intermediate, Binaries, and Saved folders
2. Right-click .uproject > "Generate Visual Studio project files"
3. Rebuild from Visual Studio
4. Check Output Log for specific error

---

## 9. Workflow Guidelines

### Before Writing Code

1. **Brainstorm 10-15 implementation ideas** tagged by approach:
   - BUILT-IN: Uses Unreal's existing subsystems
   - ADAPT: Modifies code from Island Escape or GAR
   - HYBRID: Combines multiple sources
   - CUSTOM: Builds from scratch

2. **Research first** - Check if Unreal Engine already provides this functionality

3. **Architecture before implementation** - Discuss:
   - Which classes/components are involved
   - What each one owns
   - How they communicate
   - What designers can configure

### Problem Tracker Format

When encountering bugs, document in this format:

```
Date: [YYYY-MM-DD]
Project: WizardJam
Category: [Build System | Architecture | Input | AI | Networking | Delegates]
Severity: [Critical | High | Medium | Low]
Time to Resolve: [Duration]

Symptoms:
[What error or behavior appeared]

Root Cause:
[Why it happened]

Solution:
[What fixed it]

Prevention:
[How to avoid in future]

Reusable: [Yes/No]
```

### Session End Protocol

Before ending a conversation:
1. Summarize progress and logical next step
2. Log any problems in Problem Tracker format
3. Update development notes if new learnings emerged
4. Provide memory keywords: Project, feature, pattern, blocker resolved

---

## 10. Session Memory & Lessons Learned

### Key Discoveries Log

| Date | Discovery | Impact |
|------|-----------|--------|
| Day 5 | SpellCollectible auto-color system (mesh/project/engine materials then emissive fallback) | Reduces manual material setup |
| Day 8 | FSlateBrush requires SlateCore module in Build.cs | Build dependency |
| Day 17 | FBlackboardKeySelector requires both AddObjectFilter AND InitializeFromAsset | Critical AI fix |
| Day 21 | AI broom collection working - team ID restrictions removed from pickup logic | AI progress |

### Persistent Rules

1. **UE5 builds:** Never add AdditionalCompilerArguments to Target.cs
2. **Cross-class communication:** Use static delegates (Observer pattern)
3. **Spell types:** Use FName, not enums - allows designer expansion
4. **Material colors:** Apply to ALL slots using dynamic material per slot at BeginPlay
5. **UE5 UHT Rule:** `.generated.h` include MUST be LAST in UCLASS headers
6. **Enhanced Input:** `#include "InputMappingContext.h"` required in cpp files

### Architecture Decisions Record

| Decision | Rationale |
|----------|-----------|
| FName for spell types | Designer can add new spells without C++ changes |
| Delegate-driven UI | No polling, immediate updates, loose coupling |
| Channel-based teleportation | Metroidvania progression gates, reused from Island Escape |
| Component-based health/stamina | Reusable across any character type |

---

## 11. Current Development Focus

### Working Systems (January 21, 2026)

- AI perception detects BP_BroomCollectible_C at 500 units
- BTService_FindCollectible writes to PerceivedCollectible Blackboard key
- BroomCollectible has AI perception registration
- Health/Stamina/SpellCollection components initialized on player
- QuidditchGameMode registers agents correctly

### Next Implementation Batches

1. Fix Collectible Pickup Interface - Remove team restrictions
2. Create QuidditchStagingZone Actor - Flight navigation target
3. Implement BTTask_MountBroom - AI mounts broom via IInteractable
4. Implement BTTask_ControlFlight + BTService_UpdateFlightTarget
5. HUD Widget Integration - Delegate binding for health, stamina, spells

---

## 12. Quick Reference Cards

### Property Exposure Decision Tree

```
Do designers set this value?
├── No → VisibleAnywhere (runtime state)
└── Yes
    └── Same value for all instances of this Blueprint class?
        ├── Yes → EditDefaultsOnly
        └── No → EditInstanceOnly
```

### Delegate Setup Checklist

```
[ ] Declare delegate type (DECLARE_DYNAMIC_MULTICAST_DELEGATE)
[ ] Declare delegate property (UPROPERTY BlueprintAssignable)
[ ] Broadcast in component when state changes
[ ] Bind in widget NativeConstruct
[ ] Unbind in widget NativeDestruct
[ ] Test: change state, verify widget updates
```

### New BT Node Checklist

```
[ ] FBlackboardKeySelector has AddObjectFilter/AddVectorFilter in constructor
[ ] InitializeFromAsset override exists
[ ] InitializeFromAsset calls Super and ResolveSelectedKey
[ ] Test: verify IsSet() returns true at runtime
```
