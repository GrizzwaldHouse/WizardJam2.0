// ============================================================================
// QuidditchNames.h
// Centralized FName constants for Blackboard keys, actor tags, and channels
//
// Developer: Marcus Daley
// Date: March 1, 2026
// Project: WizardJam / GAR (END2507)
//
// PURPOSE:
// Eliminates inline TEXT("...") magic strings throughout the codebase.
// All Quidditch-related FName constants live here to prevent typos and
// provide a single source of truth for key names and tag names.
//
// USAGE:
// #include "Code/Quidditch/QuidditchNames.h"
// Blackboard->SetValueAsBool(QuidditchBBKeys::IsFlying, true);
// Actor->Tags.Add(QuidditchTags::Seeker);
// ============================================================================

#pragma once

#include "CoreMinimal.h"

// Blackboard key name constants
// Used with GetValueAs*/SetValueAs* calls on UBlackboardComponent
namespace QuidditchBBKeys
{
    static const FName TargetLocation(TEXT("TargetLocation"));
    static const FName TargetActor(TEXT("TargetActor"));
    static const FName IsFlying(TEXT("IsFlying"));
    static const FName SelfActor(TEXT("SelfActor"));
    static const FName PerceivedCollectible(TEXT("PerceivedCollectible"));
    static const FName MatchStarted(TEXT("MatchStarted"));
    static const FName ShouldSwapTeam(TEXT("ShouldSwapTeam"));
    static const FName QuidditchRole(TEXT("QuidditchRole"));
    static const FName HasBroom(TEXT("HasBroom"));
    static const FName ActionFinished(TEXT("ActionFinished"));
    static const FName HomeLocation(TEXT("HomeLocation"));
    static const FName Player(TEXT("Player"));
    static const FName StageLocation(TEXT("StageLocation"));
    static const FName ReachedStagingZone(TEXT("ReachedStagingZone"));
}

// Actor tag constants
// Used with ActorHasTag() and Tags.Add() calls
namespace QuidditchTags
{
    static const FName Seeker(TEXT("Seeker"));
    static const FName Chaser(TEXT("Chaser"));
    static const FName Keeper(TEXT("Keeper"));
    static const FName Beater(TEXT("Beater"));
    static const FName Flying(TEXT("Flying"));
    static const FName Player(TEXT("Player"));
    static const FName Snitch(TEXT("Snitch"));
    static const FName GoldenSnitch(TEXT("GoldenSnitch"));
    static const FName Bludger(TEXT("Bludger"));
    static const FName Quaffle(TEXT("Quaffle"));
    static const FName Held(TEXT("Held"));
    static const FName StagingZone(TEXT("StagingZone"));
    static const FName LandingZone(TEXT("LandingZone"));
}
