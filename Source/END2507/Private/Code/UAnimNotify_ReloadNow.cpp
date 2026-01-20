// Fill out your copyright notice in the Description page of Project Settings.


#include "Code/UAnimNotify_ReloadNow.h"
#include "Both/CharacterAnimation.h"
#include "Components/SkeletalMeshComponent.h"
#include "../END2507.h"


DEFINE_LOG_CATEGORY_STATIC(LogAnimNotifyReload, Log, All);

void UUAnimNotify_ReloadNow::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	if (!MeshComp)
	{
		UE_LOG(LogAnimNotifyReload, Error, TEXT("AnimNotify_ReloadNow — MeshComp is null!"));
		return;
	}
	// Get animation instance and call OnReloadNow delegate
	UCharacterAnimation* CharAnim = Cast<UCharacterAnimation>(MeshComp->GetAnimInstance());
	if (CharAnim)
	{
		CharAnim->CallOnReloadNow();
		UE_LOG(LogAnimNotifyReload, Log, TEXT("OnReloadNow delegate broadcasted"));
	}
	else
	{
		UE_LOG(LogAnimNotifyReload, Warning, TEXT("AnimNotify_ReloadNow — AnimInstance is not UCharacterAnimation"));
	}
}
