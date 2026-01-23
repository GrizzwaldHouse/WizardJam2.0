Project: WizardJam (Portfolio Capstone Project)
Developer: Marcus Daley
Architecture Lead: Nick Penney AAA Coding Standards
Engine: Unreal Engine 5.4
Start Date: January 5, 2025
Current Phase: Post-Development Structural Refinement
Presentation Schedule: Working features demo every Tuesday & Thursday

🎯 DEVELOPMENT PHILOSOPHY
Quality-first, reusable systems, proper architecture. Speed is NOT a factor.
Correctness, maintainability, and clean architecture ARE factors.
We are building production-quality systems for portfolio demonstration and future reuse.
One week left in class, then personal project development continues indefinitely.

🎯 AGENT ROLE & IDENTITY
You are the WizardJam Development Agent - an expert Unreal Engine 5 C++ game developer assisting Marcus Daley with building a wizard-themed arena combat game. Your role is to:

Provide step-by-step implementation guidance with exact file paths and code
Reference existing working code from Marcus's Island Escape and GAR_MarcusDaley projects
Follow strict coding standards (no hardcoded values, constructor initialization, observer patterns)
Track progress through daily milestones
Point to specific YouTube timestamps when video reference would help. always analyze the transcripts that I have given you first before responding so that you can properly give me industry standards for how we should set up this game and this project to make this as quick as possible for me as a solo developer working on this game you should also be saving all those analysis and adding the rules and standards we want to code this project to your memory folder so that you can avoid making the same mistakes in the next chat I want you to think of how fast I can get different task done with your assistance as an agent. We need to make sure that my headers are set up to display my name as the developer of this project and the date and what the function of the class file is for and how to use it for future developers that might want to use this to set up their game
Prevent scope creep by parking non-essential features


📋 PROJECT SCOPE DEFINITION
✅ IN-SCOPE FEATURES (Priority Order)
PriorityFeatureEst. HoursSource Code Reference1Elemental Spell Channels (Flame, Ice, Lightning, Arcane)8CollectiblePickup.cpp - NEW child class, NOT rename2Elemental Projectiles with VFX10BaseProjectile.cpp, VoxelProjectile.cpp3ElementalHideWall (color-coded spinning obstacles)12HideWall.cpp - Extend with element matching4Wave Defense Spawning System8BaseSpawner.cpp, TrapTrigger.cpp from GAR project5MCP + Voxel Arena Generator6VoxelGenerator.cpp, MapGenerator.cpp, MCP tools6Wizard Duel Boss14BatAgent.cpp, BossAIController.cpp7Dog Companion (AI Mode)16NEW - Based on JMercer's architecture8Dog Bark Distraction6AI Perception sound sense9Channel-Gated Arenas (Teleport)8TeleportPoint.cpp, SenderActor.cpp already working10Teleport Visual Feedback (Fade)4NEW - Screen fade widget11Interactable System (UI popups)6NEW - Requires Interactable interface files12UI Polish (Menu, Results, HUD)6MainMenuWidget.cpp, ResultsWidget.cpp, PlayerHUDWidget.cpp13Win Conditions (Multiple Options)4IslandEscapeGameMode.cpp14Build & Installer4Packaging guide
Total Estimated: ~112 hours
❌ OUT-OF-SCOPE (PARKING LOT)

Spell Combination System (save for post-jam)
Dog Possession/Switching Mode (placeholder code only)
Companion Fetch Quest (separate project)
VoxelWeapon Capture for Rifle (evaluate later)
Full GAS Integration (keep existing channel system)
Damage Over Time System (stretch goal only if time permits)

⚠️ CRITICAL CONSTRAINTS

NO EditAnywhere - Use EditDefaultsOnly or EditInstanceOnly
NO hardcoded values in headers - Initialize in constructor or constructor initialization list
Observer Pattern (Delegates) - All system communication via broadcasts
Blueprint Exposure - Designer configures in BP, not code
Behavior Trees for AI - No hardcoded AI logic in Tick()
Separate Spell Channels from Teleport Channels - Child class, not rename
Modular Systems - Reusable across future projects


🌳 BEHAVIOR TREE CODE REVIEW CHECKLIST (MANDATORY)
When reviewing or creating ANY custom BT Task, Service, or Decorator that uses blackboard keys:

⚠️ FBlackboardKeySelector INITIALIZATION REQUIREMENTS
Every FBlackboardKeySelector property MUST have TWO pieces of initialization or it will silently fail:

1. KEY TYPE FILTER IN CONSTRUCTOR (tells editor what key types are valid):
```cpp
// In constructor - REQUIRED for the selector to work
MyKeySelector.AddObjectFilter(this, 
    GET_MEMBER_NAME_CHECKED(UMyBTNode, MyKeySelector), 
    AActor::StaticClass());  // Or UObject for any object

// For vector keys:
MyVectorKey.AddVectorFilter(this,
    GET_MEMBER_NAME_CHECKED(UMyBTNode, MyVectorKey));

// For bool keys:
MyBoolKey.AddBoolFilter(this,
    GET_MEMBER_NAME_CHECKED(UMyBTNode, MyBoolKey));
```

2. RUNTIME KEY RESOLUTION (resolves string key name to actual blackboard slot):
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

⚠️ SYMPTOMS OF MISSING INITIALIZATION:
- "OutputKey is not set!" warnings despite key being selected in editor
- FBlackboardKeySelector.IsSet() returns false at runtime
- Blackboard values never update despite logic running correctly
- AI perceives targets but doesn't act on them (Move To has null target)

⚠️ CODE REVIEW GATE:
BEFORE approving any BT node with FBlackboardKeySelector, verify:
[ ] Constructor has AddObjectFilter/AddVectorFilter/AddBoolFilter call
[ ] InitializeFromAsset override exists and calls ResolveSelectedKey
[ ] Both header AND cpp file have the required code

This is a SILENT FAILURE - code compiles, runs, logs correctly, but blackboard writes fail.
Discovered: January 21, 2026 - BTService_FindCollectible appeared to work but never moved AI.


🏗️ PROJECT ARCHITECTURE
New Project Structure
WizardJam/
├── Source/WizardJam/
│   ├── Code/
│   │   ├── Actors/
│   │   │   ├── BaseCharacter.h/.cpp          (FROM Island Escape)
│   │   │   ├── BasePlayer.h/.cpp             (FROM Island Escape)
│   │   │   ├── BaseAgent.h/.cpp              (FROM GAR - better faction)
│   │   │   ├── BasePickup.h/.cpp             (FROM Island Escape)
│   │   │   ├── CollectiblePickup.h/.cpp      (FROM Island Escape)
│   │   │   ├── SpellCollectible.h/.cpp       (NEW - child of Collectible)
│   │   │   ├── HealthPickup.h/.cpp           (FROM Island Escape)
│   │   │   ├── BaseProjectile.h/.cpp         (FROM Island Escape)
│   │   │   ├── ElementalProjectile.h/.cpp    (NEW - child with element type)
│   │   │   ├── BaseSpawner.h/.cpp            (FROM Island Escape)
│   │   │   ├── TrapTrigger.h/.cpp            (FROM Island Escape)
│   │   │   ├── HideWall.h/.cpp               (FROM GAR)
│   │   │   ├── ElementalWall.h/.cpp          (NEW - child with element matching)
│   │   │   ├── TeleportPoint.h/.cpp          (FROM Island Escape)
│   │   │   ├── SenderActor.h/.cpp            (FROM Island Escape)
│   │   │   ├── ReceiverActor.h/.cpp          (FROM Island Escape)
│   │   │   ├── BatAgent.h/.cpp               (FROM Island Escape - reskin as WizardBoss)
│   │   │   ├── CompanionDog.h/.cpp           (NEW)
│   │   │   └── BaseRifle.h/.cpp              (FROM GAR - evaluate for capture)
│   │   │
│   │   ├── AI/
│   │   │   ├── AIC_CodeBaseAgentController.h/.cpp  (FROM GAR - better perception)
│   │   │   ├── AIC_CompanionController.h/.cpp      (NEW)
│   │   │   ├── BossAIController.h/.cpp             (FROM Island Escape)
│   │   │   ├── BTTask_CodeFindLocation.h/.cpp      (FROM Island Escape)
│   │   │   ├── BTTask_CodeEnemyAttack.h/.cpp       (FROM Island Escape)
│   │   │   ├── BTTask_EnemyFlee.h/.cpp             (FROM GAR)
│   │   │   ├── BTTask_SummonMinions.h/.cpp         (FROM Island Escape)
│   │   │   ├── BTTask_FollowPlayer.h/.cpp          (NEW)
│   │   │   ├── BTTask_StayCommand.h/.cpp           (NEW)
│   │   │   └── BTTask_BarkDistraction.h/.cpp       (NEW)


📝 LESSONS LEARNED & RULES

UE5 builds: Never add AdditionalCompilerArguments to Target.cs - breaks shared build environment. Use Build.cs PrivateDefinitions instead.
Day 5: SpellCollectible has auto-color system (tries mesh/project/engine materials then emissive fallback). SpellMaterialFactory creates M_SpellCollectible_Colorable via C++. HUD files created.
PowerSpec G759 build performance: 32GB RAM during UE5 compilation is normal (system has 64GB). BuildConfiguration.xml placed at %APPDATA%\Unreal Engine\UnrealBuildTool\ for 16-thread optimization
Cross-class communication: Use static delegates (Observer pattern), never direct GameMode calls or Kismet/GameplayStatics. Broadcaster emits, listener binds at BeginPlay
RULE: Use FName for designer-configurable types (spells, items, etc.) - NOT enums. Allows designer expansion without C++ changes.
RULE: Apply designer-set colors to ALL material slots using GAR BaseAgent::SetupAgentAppearance() pattern - create dynamic material per slot at BeginPlay.
PRINCIPLE: Never hardcode gameplay types or colors - always designer-driven via EditDefaultsOnly properties. System must be expandable without C++ changes.
UI Architecture: C++ broadcasts state via delegates, Blueprint creates/styles widgets. Verify function names in headers before use.
Day 8 WizardJam: Spell slot HUD with texture-swapping complete. FSlateBrush requires SlateCore module in Build.cs. UMG Image sizing uses parent slot settings (Size=Fill/Auto), not "Override" checkbox.
UE5 Enhanced Input: Using UInputMappingContext requires #include "InputMappingContext.h" in .cpp files - forward declarations from EnhancedInputSubsystemInterface.h are insufficient
UE5 UHT Rule: The .generated.h include MUST be the LAST include in any UCLASS header - placing it first causes "must appear at top following all other includes" error
Day 17 BT Fix: FBlackboardKeySelector requires AddObjectFilter() in constructor AND InitializeFromAsset() override with ResolveSelectedKey() call. Without both, IsSet() returns false even when key is configured in editor. This is a SILENT FAILURE that causes AI to not act on perceived targets.
Day 21 AI Broom Collection SUCCESS: AI agent now perceives, navigates to, and collects BroomCollectible. Key fixes: (1) BTService_FindCollectible with proper blackboard key initialization, (2) AI perception registration on collectibles, (3) Team ID restrictions removed from pickup logic - only IPickupInterface check required.


📊 CURRENT SESSION: AI BROOM FLIGHT SYSTEM
Status: AI successfully moves to and picks up broom collectible

✅ WORKING SYSTEMS (January 21, 2026):
- AI perception detects BP_BroomCollectible_C at 500 units
- BTService_FindCollectible writes to PerceivedCollectible Blackboard key
- BroomCollectible has AI perception registration
- Health/Stamina/SpellCollection components initialized on player
- QuidditchGameMode registers agents correctly

🔄 NEXT IMPLEMENTATION BATCHES:
1. Fix Collectible Pickup Interface - Remove team restrictions, only check IPickupInterface
2. Create QuidditchStagingZone Actor - Target location for AI flight navigation
3. Implement BTTask_MountBroom - Command AI to mount broom via IInteractable
4. Implement BTTask_ControlFlight + BTService_UpdateFlightTarget - Navigation to staging zone
5. HUD Widget Integration - Delegate binding for health, stamina, spells, broom state


🎮 QUIDDITCH AI BEHAVIOR TREE STRUCTURE
Root: BT_QuidditchAI
├── AcquireBroom (exit: HasBroom == true)
│   ├── BTService_FindCollectible
│   ├── BTTask_MoveToCollectible
│   └── BTTask_InteractWithBroom
├── PrepareForFlight (exit: IsFlying == true)
│   ├── BTTask_MountBroom
│   ├── BTTask_ValidateFlightMode
│   └── BTDecorator_IsFlying
├── FlyToMatchStart (exit: DistanceToTarget <= AcceptableRadius)
│   ├── BTService_UpdateFlightTarget
│   ├── BTTask_ControlFlight
│   └── BTDecorator_ReachedFlightTarget
└── PlayQuidditchMatch (placeholder)


📝 CODE STYLE REQUIREMENTS (Nick Penney AAA Standards)
Comment Standards:
- Use // for ALL comments - NO /** */ docblocks
- Comments should sound like a human explaining to another developer
- Keep comments practical and brief

Constructor Initialization:
- ALWAYS use constructor initialization lists
- Example: AMyClass::AMyClass() : PropertyA(DefaultValue), PropertyB(OtherValue) { }
- NEVER assign in header or BeginPlay unless absolutely necessary

Delegate Pattern:
- Use Observer pattern - NO polling in Tick()
- Components broadcast state changes
- UI widgets listen and update via bound delegates


📚 LESSONS LEARNED COMPENDIUM (January 2026)
============================================

🔴 CRITICAL FAILURES & SOLUTIONS (12+ hours debugging each):

1. FBlackboardKeySelector Silent Failure (Day 17-21)
   SYMPTOM: Key selected in editor, code runs, logs show success, but blackboard never updates
   ROOT CAUSE: Missing BOTH AddObjectFilter() in constructor AND InitializeFromAsset() override
   SOLUTION: Every FBlackboardKeySelector needs constructor filter + InitializeFromAsset with ResolveSelectedKey()
   TIME LOST: ~12 hours across multiple sessions
   PREVENTION: Added mandatory code review checklist to CLAUDE.md

2. AI Perception Not Detecting Actors
   SYMPTOM: AI walks past brooms/collectibles without seeing them
   ROOT CAUSE: Missing UAIPerceptionStimuliSourceComponent on perceivable actors
   SOLUTION: Add perception source component and register for sight/hearing
   RELATED: Team ID filtering can block perception - check GetTeamAttitudeTowards()

3. Team ID Blocking Pickups
   SYMPTOM: AI detects collectible but CanPickup() returns false
   ROOT CAUSE: Faction/team restrictions in pickup code (copied from GAR combat)
   SOLUTION: Pickups should only check IPickupInterface, not team allegiance

4. Enhanced Input Missing Context
   SYMPTOM: Input actions don't fire despite proper binding
   ROOT CAUSE: #include "InputMappingContext.h" missing in .cpp
   SOLUTION: Forward declarations insufficient for UInputMappingContext - need full include

5. Generated.h Placement
   SYMPTOM: "must appear at top following all other includes" compiler error
   ROOT CAUSE: .generated.h placed first instead of last
   SOLUTION: .generated.h MUST be the LAST include in UCLASS headers

🟡 ARCHITECTURE DECISIONS THAT WORKED:

1. Component Composition over Inheritance
   AC_BroomComponent attaches to ANY actor (player, AI) without base class changes
   AC_SpellCollectionComponent same pattern - attach and use
   AC_StaminaComponent - shared stamina system
   BENEFIT: Add flight to any character with one component addition

2. Configuration Structs (FBroomConfiguration pattern)
   All gameplay values in one designer-editable struct
   Component reads struct at runtime - zero hardcoded values
   Preset factory functions for common configurations
   BENEFIT: Designers create new broom types without C++ changes

3. Input-Agnostic Core API
   SetFlightEnabled(), SetVerticalInput(), SetBoostEnabled()
   Players call through input handlers, AI calls directly
   BENEFIT: Identical flight mechanics for all actors

4. Static Delegates for Global Events
   OnAnySpellCollected - GameMode binds once, receives from all components
   BENEFIT: No direct references, clean decoupling

🟢 PATTERNS FROM NICK PENNEY GAR CLASS TO REUSE:

1. EQS for Target Finding (instead of services with perception)
   Nick's GAR agents used EQS to find players, cover, patrol points
   EQS queries are designer-configurable in editor
   BENEFIT: Avoids the FBlackboardKeySelector initialization pitfalls

2. Decorator-Based BT Structure
   Nick used decorators to check conditions, not services
   Services ran continuously and sometimes had race conditions
   Decorators are cleaner: check condition → run child or abort

3. Separate Attack/Patrol Behavior Trees
   BT_Standard for patrol/roam until player detected
   BT_Attack for combat sequences (shoot, reload, flee to cover)
   Switch trees on perception state change
   BENEFIT: Cleaner separation of concerns


🎮 QUIDDITCH AI BEHAVIORS BRAINSTORM (20+ Behaviors)
====================================================

PRE-MATCH ACQUISITION PHASE:
1. BTTask_FindBroomChannel - Locate and collect "Broom" channel collectible
2. BTTask_FindBroomActor - Locate interactable broom in world
3. BTTask_InteractWithBroom - Mount broom via IInteractable interface
4. BTService_MonitorChannel - Watch for channel state changes
5. BTDecorator_HasChannel - Gate behaviors on channel possession

FLIGHT CONTROL BEHAVIORS:
6. BTTask_TakeOff - Initial ascent to minimum flight altitude
7. BTTask_Land - Controlled descent to ground
8. BTTask_HoverInPlace - Maintain position (idle state)
9. BTTask_FlyToLocation - Navigate to world position
10. BTTask_FlyToActor - Navigate to moving target (player, ball)
11. BTTask_FollowFormation - Maintain position relative to team
12. BTService_UpdateFlightTarget - Continuously update target location
13. BTService_MonitorStamina - Watch stamina for rest decisions
14. BTDecorator_IsFlying - Gate ground vs flight behaviors
15. BTDecorator_HasStamina - Gate movement on stamina availability

QUIDDITCH ROLE BEHAVIORS (SEEKER):
16. BTTask_SearchForSnitch - Patrol pattern while scanning
17. BTTask_ChaseSnitch - High-speed pursuit of snitch
18. BTTask_CatchSnitch - Final grab attempt
19. BTService_TrackSnitch - Update snitch position continuously

QUIDDITCH ROLE BEHAVIORS (CHASER):
20. BTTask_FindQuaffle - Locate the quaffle
21. BTTask_CatchQuaffle - Intercept thrown/dropped quaffle
22. BTTask_CarryQuaffle - Hold while flying
23. BTTask_ThrowToTeammate - Pass to another chaser
24. BTTask_ThrowAtGoal - Score attempt
25. BTService_EvaluatePassOptions - Find open teammates
26. BTDecorator_HasBall - Gate behaviors on ball possession

QUIDDITCH ROLE BEHAVIORS (BEATER):
27. BTTask_FindBludger - Locate nearest bludger
28. BTTask_HitBludgerAtTarget - Aim and strike bludger at enemy
29. BTTask_InterceptBludger - Block bludger heading for teammate
30. BTService_TrackBludgers - Monitor all bludgers

QUIDDITCH ROLE BEHAVIORS (KEEPER):
31. BTTask_GuardGoal - Position between threat and goal
32. BTTask_BlockShot - Intercept incoming quaffle
33. BTTask_ClearQuaffle - Throw quaffle away from goal area
34. BTService_EvaluateThreat - Determine most dangerous attacker

TEAM COORDINATION BEHAVIORS:
35. BTTask_FormOnLeader - Move to formation position
36. BTTask_CallForPass - Signal desire to receive quaffle
37. BTTask_SetPlay - Execute named formation
38. BTService_TeamCommunication - Share information with teammates

DEFENSIVE/EVASIVE BEHAVIORS:
39. BTTask_DodgeBludger - Evade incoming bludger
40. BTTask_BlockOpponent - Body-block without fouling
41. BTTask_Retreat - Fall back to defensive position
42. BTDecorator_UnderAttack - React to incoming threats

STAMINA MANAGEMENT:
43. BTTask_RestInPlace - Stop and regenerate stamina
44. BTTask_GlideDownward - Conserve stamina while descending
45. BTDecorator_LowStamina - Trigger rest behaviors


📊 UPDATED SESSION STATUS (January 22, 2026)
============================================

✅ FULLY WORKING:
- AI perceives BP_BroomCollectible at 500 units
- BTService_FindCollectible writes to PerceivedCollectible blackboard key
- BroomCollectible has AI perception registration
- AI collects "Broom" channel from BroomCollectible
- AI waits for broom channel, then navigates to BroomActor
- AI interacts with broom to mount
- AI flies (short duration due to stamina drain on current broom type)
- Health/Stamina/SpellCollection components initialized
- QuidditchGameMode registers agents correctly

🔄 NEXT PRIORITY TASKS:
1. Create BP_Broom_Firebolt using GetQuidditchPreset() configuration
   - Infinite duration, drain only when moving, regen when idle
   - Place in level to test longer flight

2. Create QuidditchStagingZone actor
   - C++ class: AQuidditchStagingZone
   - Team formation volumes
   - Target point for AI flight navigation
   - "Ready" detection for match start

3. Create BTService_UpdateFlightTarget
   - Reads StagingZone reference from blackboard
   - Writes target location for BTTask_ControlFlight

4. Test extended flight with Firebolt broom
   - Verify stamina regen when idle
   - Verify no dismount on depletion
   - Verify AI can reach staging zone

5. HUD Integration (consider migrating from original WizardJam)
   - Spell slot UI already working in original project
   - Evaluate migration vs rebuild effort


🎯 RECOMMENDED APPROACH: USE BASEAGENT FOR QUIDDITCH
====================================================

The question of whether to use BaseAgent's AI controller for Quidditch:

RECOMMENDATION: YES - Extend AIC_CodeBaseAgentController for Quidditch

RATIONALE:
- BaseAgent already has shooting, reload, flee behaviors
- AIC_CodeBaseAgentController has perception setup (sight + hearing)
- Can add QuidditchStagingZone detection to existing perception
- Add flight-specific blackboard keys without losing combat keys
- Wand attacks reuse existing EnemyAttack() with different animations

IMPLEMENTATION:
1. Create AIC_QuidditchController as child of AIC_CodeBaseAgentController
2. Add flight-specific blackboard keys (IsFlying, FlightTarget, etc.)
3. Add BroomComponent to BaseAgent (or QuidditchAgent child)
4. Override perception update to include brooms/balls
5. Create BT_Quidditch that includes combat behaviors

This avoids rebuilding 12+ hours of tested AI controller code.


🏗️ HUD MIGRATION DECISION
==========================

Original WizardJam has:
- Spell slot UI with texture swapping
- Health bar
- Working delegate bindings

Current WizardJam2.0 (this project) has:
- AC_SpellCollectionComponent with delegates
- AC_StaminaComponent with delegates
- AC_BroomComponent with flight state delegates
- No working HUD widget yet

RECOMMENDATION: MIGRATE the spell slot widget from original project
- Faster than rebuilding
- Already tested and styled
- Connect to new components via delegates

STEPS:
1. Copy WBP_SpellSlotHUD from original project
2. Update delegate bindings to new component names
3. Add flight stamina bar using existing AC_BroomComponent delegates
4. Test in QuidditchGameMode
