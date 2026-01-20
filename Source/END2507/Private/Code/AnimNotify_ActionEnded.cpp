// UAnimNotify_ActionEnded - Signals end of action animations
// Author: Marcus Daley
// Date: 10/6/2025

#include "Code/AnimNotify_ActionEnded.h"
#include "Both/CharacterAnimation.h"
#include "Components/SkeletalMeshComponent.h"
#include "../END2507.h"
DEFINE_LOG_CATEGORY_STATIC(LogAnimNotifyAction, Log, All);

void UAnimNotify_ActionEnded::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{

	if (!MeshComp)
	{
		UE_LOG(LogAnimNotifyAction, Error, TEXT("AnimNotify_ActionEnded — MeshComp is null!"));
		return;
	}
	//Get animation instance and broadcast action ended
		UCharacterAnimation * CharAnim = Cast<UCharacterAnimation>(MeshComp->GetAnimInstance());
	if (CharAnim)
	{
		CharAnim->CallOnActionEnded();
		UE_LOG(LogAnimNotifyAction, Log, TEXT(" OnActionEnded delegate broadcasted"));
	}
	else
	{
		UE_LOG(LogAnimNotifyAction, Warning, TEXT("AnimNotify_ActionEnded — AnimInstance is not UCharacterAnimation"));
	}
}
