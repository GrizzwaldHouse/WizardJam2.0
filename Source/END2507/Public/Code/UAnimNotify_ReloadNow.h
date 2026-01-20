// Place this notify at the frame where clip inserts into rifle
// Author: Marcus Daley
// Date: 10/6/2025

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "UAnimNotify_ReloadNow.generated.h"

/**
 * 
 */
UCLASS()
class END2507_API UUAnimNotify_ReloadNow : public UAnimNotify
{
	GENERATED_BODY()
	

public:
	// Override Notify 
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

};
