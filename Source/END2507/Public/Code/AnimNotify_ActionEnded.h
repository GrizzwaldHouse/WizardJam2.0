// Place this notify at the last frame of reload/fire/hit animations
// Author: Marcus Daley
// Date: 10/6/2025
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_ActionEnded.generated.h"

/**
 * 
 */
UCLASS()
class END2507_API UAnimNotify_ActionEnded : public UAnimNotify
{
	GENERATED_BODY()
public:
	// Override Notify
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

};
