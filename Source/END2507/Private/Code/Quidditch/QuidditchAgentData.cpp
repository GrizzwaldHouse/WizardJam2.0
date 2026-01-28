// ============================================================================
// QuidditchAgentData.cpp
// Implementation of Primary Data Asset for Quidditch agent configuration
//
// Developer: Marcus Daley
// Date: January 25, 2026
// Project: WizardJam / GAR (END2507)
// ============================================================================

#include "Code/Quidditch/QuidditchAgentData.h"

// ============================================================================
// GetPrimaryAssetId
// Returns the primary asset ID for this data asset
//
// ARCHITECTURE:
// Primary Asset IDs allow Unreal's Asset Manager to track and load assets
// by name. Format: "PrimaryAssetType:AssetName"
//
// EXAMPLE:
// Asset named "DA_SeekerAggressive" returns ID "QuidditchAgent:DA_SeekerAggressive"
// ============================================================================

FPrimaryAssetId UQuidditchAgentData::GetPrimaryAssetId() const
{
	// Return primary asset ID with type "QuidditchAgent"
	// This allows Asset Manager to categorize and load these assets
	return FPrimaryAssetId(TEXT("QuidditchAgent"), GetFName());
}
