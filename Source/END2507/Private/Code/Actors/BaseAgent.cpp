// Fill out your copyright notice in the Description page of Project Settings.


#include "Code/Actors/BaseAgent.h"
#include "Code/AC_HealthComponent.h"
#include "AIController.h"
#include "Code/Actors/AIC_CodeBaseAgentController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h" 
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Code/Actors/BaseRifle.h"
#include "../END2507.h"

// Logging category for this class
DEFINE_LOG_CATEGORY_STATIC(LogCodeAgent, Log, All);
ABaseAgent::ABaseAgent()
{
	PrimaryActorTick.bCanEverTick = true;
	//Set default values for the agent appearance and behavior
	AgentColor = FLinearColor::Red;
	MaterialParameterName = TEXT("Tint");
	PlacedAgentFactionID = 1;  // Enemy team
	PlacedAgentFactionColor = FLinearColor::Red;
	//Configure AI possession
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	

}

void ABaseAgent::EnemyAttack(AActor* Target)
{
	// Validate target exists
	if (!Target)
	{
		UE_LOG(LogCodeAgent, Warning, TEXT("%s: EnemyAttack called with null target"), *GetName());
		return;
	}
	// Validate rifle exists
	if (!EquippedRifle)
	{
		UE_LOG(LogCodeAgent, Warning, TEXT("%s: FATAL - NO RIFLE IN EnemyAttack!"), *GetName());
		return;
	}
// Aim at target
	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController)
	{
		AIController->SetFocus(Target);
	}
	EquippedRifle->Fire();
	UE_LOG(LogCodeAgent, Log, TEXT("[%s] Fired at %s"), *GetName(), *Target->GetName());


}

void ABaseAgent::EnemyReload(AActor* Target)
{
	if (!EquippedRifle)
	{
		UE_LOG(LogCodeAgent, Error, TEXT("[%s] EnemyReload failed — No rifle equipped!"), *GetName());
		return;
	}

	// Request reload from rifle (will trigger OnReloadStart delegate)
	EquippedRifle->RequestReload();

	UE_LOG(LogCodeAgent, Warning, TEXT("[%s] EnemyReload — Submarine torpedo reload sequence initiated"), *GetName());

}

void ABaseAgent::SetAgentColor(const FLinearColor& NewColor)
{
	AgentColor = NewColor;
	if (DynamicMaterials.Num() == 0)
	{
		UE_LOG(LogCodeAgent, Warning, TEXT("%s: No dynamic materials to set color — SetupAgentAppearance() may not have run yet"), *GetName());
		return;
	}

	// Loop through ALL stored dynamic materials and update their color
	for (int32 i = 0; i < DynamicMaterials.Num(); i++)
	{
		if (DynamicMaterials[i])
		{
			DynamicMaterials[i]->SetVectorParameterValue(MaterialParameterName, NewColor);
		}
	}

	UE_LOG(LogCodeAgent, Log, TEXT("[%s] SetAgentColor: Updated %d material slots to (%.2f, %.2f, %.2f)"),
		*GetName(), DynamicMaterials.Num(), NewColor.R, NewColor.G, NewColor.B);
}
void ABaseAgent::UpdateBlackboardHealth(float HealthRatio)
{
	AAIController* AIController = Cast<AAIController>(GetController());
	if (!AIController)
	{
		UE_LOG(LogCodeAgent, Warning, TEXT("%s: No AIController"), *GetName());
		return;
	}

	UBlackboardComponent* BlackboardComp = AIController->GetBlackboardComponent();
	if (!BlackboardComp)
	{
		UE_LOG(LogCodeAgent, Warning, TEXT("%s: No Blackboard"), *GetName());
		return;
	}
	
	// Update health value in blackboard
	BlackboardComp->SetValueAsFloat(FName("HealthRatio"), HealthRatio);

	UE_LOG(LogCodeAgent, Log, TEXT("%s: Blackboard Health=%.1f"), *GetName(), HealthRatio * 100.f);


}
void ABaseAgent::SetupAgentAppearance()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		UE_LOG(LogCodeAgent, Error, TEXT("%s: No mesh component"), *GetName());
		return;
	}

	// Get total number of material slots
	int32 NumMaterials = MeshComp->GetNumMaterials();
	if (NumMaterials == 0)
	{
		UE_LOG(LogCodeAgent, Error, TEXT("%s: No materials on mesh"), *GetName());
		return;
	}
	DynamicMaterials.Empty();

	// Apply dynamic material to ALL slots
	for (int32 i = 0; i < NumMaterials; i++)
	{
		UMaterialInterface* BaseMaterial = MeshComp->GetMaterial(i);
		if (BaseMaterial)
		{
			UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			if (DynMat)
			{
				MeshComp->SetMaterial(i, DynMat);
				// Set initial color
				DynMat->SetVectorParameterValue(MaterialParameterName, AgentColor);

				
				DynamicMaterials.Add(DynMat);

				UE_LOG(LogCodeAgent, Log, TEXT("%s: Created dynamic material for slot %d"), *GetName(), i);
			}
		}
	}

	UE_LOG(LogCodeAgent, Log, TEXT("%s: Applied faction color to %d material slots — ALL slots stored for SetAgentColor()"),
		*GetName(), DynamicMaterials.Num());
}
void ABaseAgent::HandleActionFinished()
{
	UE_LOG(LogCodeAgent, Warning, TEXT("===== %s HandleActionFinished CALLED ====="), *GetName());

	AAIController* AIController = Cast<AAIController>(GetController());
	if (!AIController) {
		UE_LOG(LogCodeAgent, Error, TEXT("No AIController in HandleActionFinished"));
		return;
	}
	UBlackboardComponent* BlackboardComp = AIController->GetBlackboardComponent();
	if (!BlackboardComp)
	{
		UE_LOG(LogCodeAgent, Error, TEXT("No BlackboardComponent in HandleActionFinished"));
		return;
		
	}
	BlackboardComp->SetValueAsBool(TEXT("ActionFinished"), true);
	UE_LOG(LogCodeAgent, Warning, TEXT("%s: ActionFinished set to TRUE in Blackboard"), *GetName());
}

ABaseRifle* ABaseAgent::GetEquippedRifle() const
{
	return EquippedRifle;
}

void ABaseAgent::OnFactionAssigned_Implementation(int32 FactionID, const FLinearColor& FactionColor)
{
	UE_LOG(LogTemp, Display, TEXT("[%s] Faction assigned: ID=%d, Color=%s"),
		*GetName(), FactionID, *FactionColor.ToString());

	// Update visual appearance
	SetAgentColor(FactionColor);

	// Update controller's team ID and Blackboard
	AAIC_CodeBaseAgentController* AgentController = Cast<AAIC_CodeBaseAgentController>(GetController());
	if (AgentController)
	{
		// Set team via interface method
		FGenericTeamId NewTeamID = FGenericTeamId(static_cast<uint8>(FactionID));
		AgentController->SetGenericTeamId(NewTeamID);

		// Update Blackboard with faction data for BehaviorTree
		AgentController->UpdateFactionFromPawn(FactionID, FactionColor);

		UE_LOG(LogTemp, Display, TEXT("[%s] Notified controller of faction change"), *GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] No valid AgentController to notify"), *GetName());
	}
}

FGenericTeamId ABaseAgent::GetGenericTeamId() const
{	// Delegate to controller
	AAIC_CodeBaseAgentController* AgentController = Cast<AAIC_CodeBaseAgentController>(GetController());
	if (AgentController)
	{
		return AgentController->GetGenericTeamId();
	}
	return FGenericTeamId::NoTeam;
}
void ABaseAgent::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	AAIC_CodeBaseAgentController* AgentController = Cast<AAIC_CodeBaseAgentController>(GetController());
	if (AgentController)
	{
		AgentController->SetGenericTeamId(NewTeamID);
		UE_LOG(LogTemp, Display, TEXT("[%s] Set team ID to %d via controller"),
			*GetName(), NewTeamID.GetId());
	}
}
bool ABaseAgent::CanPickAmmo() const
{
	return false;
}
void ABaseAgent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner() == nullptr)  // No owner = placed in level
	{
		UE_LOG(LogCodeAgent, Log, TEXT("[%s] Placed agent — applying faction %d"),
			*GetName(), PlacedAgentFactionID);

		OnFactionAssigned_Implementation(PlacedAgentFactionID, PlacedAgentFactionColor);
	}
	else
	{
		UE_LOG(LogCodeAgent, Log, TEXT("[%s] Spawned agent — faction set by spawner"), *GetName());
	}
	if (!EquippedRifle)
	{
		UE_LOG(LogCodeAgent, Error, TEXT("%s: NO RIFLE EQUIPPED AFTER SPAWN!"), *GetName());
		return; // Don't continue if no rifle
	}

	// ===== BIND DELEGATE =====
	EquippedRifle->OnRifleAttack.AddDynamic(this, &ABaseAgent::HandleActionFinished);
	UE_LOG(LogCodeAgent, Warning, TEXT("[%s] DELEGATE BOUND: OnRifleAttack -> HandleActionFinished"), *GetName());
	if (EquippedRifle->OnRifleAttack.IsBound())
	{
		UE_LOG(LogCodeAgent, Warning, TEXT("[%s]  Delegate binding VERIFIED"), *GetName());
	}
	else
	{
		UE_LOG(LogCodeAgent, Error, TEXT("[%s]  Delegate binding FAILED"), *GetName());
	}
	EquippedRifle->OnAmmoChanged.AddDynamic(this, &ABaseAgent::HandleAmmoChanged);
	UE_LOG(LogCodeAgent, Warning, TEXT("[%s] DELEGATE BOUND: OnAmmoChanged -> UpdateBlackboardAmmo"), *GetName());

	// ===== COLLISION SETUP =====
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionObjectType(ECC_Pawn);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);
		GetCapsuleComponent()->SetGenerateOverlapEvents(true);
	}

	SetupAgentAppearance();
	UpdateBlackboardHealth(1.0f);
	UpdateBlackboardAmmo();
	UE_LOG(LogCodeAgent, Log, TEXT("%s: Agent initialized with rifle %s"),
		*GetName(), *EquippedRifle->GetName());
	
}
void ABaseAgent::HandleHurt(float HealthRatio)
{
	// Call parent implementation for hit animation
	Super::HandleHurt(HealthRatio);

	// Update blackboard with new health ratio
	if (HealthComponent)
	{
		float CurrentHealthRatio = HealthComponent->GetHealthRatio();
		UpdateBlackboardHealth(CurrentHealthRatio);

		UE_LOG(LogCodeAgent, Warning, TEXT("%s: Hurt! HealthRatio=%.2f"), *GetName(), HealthRatio);

	}

}

void ABaseAgent::HandleDeathStart(float Ratio)
{
	
	UE_LOG(LogCodeAgent, Warning, TEXT("[%s] Death sequence initiated"), *GetName());

	// Destroy rifle when agent dies
	if (EquippedRifle)
	{
		EquippedRifle->Destroy();
		UE_LOG(LogCodeAgent, Log, TEXT("[%s] Rifle destroyed"), *GetName());
	}

	// this handles animation and destruction
	Super::HandleDeathStart(Ratio);
	
}
void ABaseAgent::UpdateBlackboardAmmo()
{
	AAIController* AIController = Cast<AAIController>(GetController());
	if (!AIController)
	{
		UE_LOG(LogCodeAgent, Warning, TEXT("[%s] Cannot update blackboard ammo — No AIController"), *GetName());
		return;
	}

	UBlackboardComponent* BlackboardComp = AIController->GetBlackboardComponent();
	if (!BlackboardComp)
	{
		UE_LOG(LogCodeAgent, Warning, TEXT("[%s] Cannot update blackboard ammo — No Blackboard"), *GetName());
		return;
	}

	if (!EquippedRifle)
	{
		UE_LOG(LogCodeAgent, Error, TEXT("[%s] Cannot update blackboard ammo — No rifle equipped"), *GetName());
		BlackboardComp->SetValueAsFloat(FName("Ammo"), 0.0f);
		return;
	}

	// Calculate ammo ratio for blackboard (0.0 to 1.0)
	const float MaxAmmo = static_cast<float>(EquippedRifle->GetMaxAmmo());
	const float CurrentAmmo = static_cast<float>(EquippedRifle->GetCurrentAmmo());
	const float AmmoRatio = (MaxAmmo > 0.0f) ? (CurrentAmmo / MaxAmmo) : 0.0f;

	BlackboardComp->SetValueAsFloat(FName("Ammo"), AmmoRatio);

	UE_LOG(LogCodeAgent, Log, TEXT("[%s] Blackboard ammo updated: %.2f (Ammo: %d/%d)"),
		*GetName(), AmmoRatio, EquippedRifle->GetCurrentAmmo(), EquippedRifle->GetMaxAmmo());
}
void ABaseAgent::HandleAmmoChanged(float CurrentAmmo, float MaxAmmo)
{
	UpdateBlackboardAmmo();

	UE_LOG(LogCodeAgent, Warning, TEXT("[%s] ammo changed: %.0f/%.0f — Blackboard updated"),
		*GetName(), CurrentAmmo, MaxAmmo);
}