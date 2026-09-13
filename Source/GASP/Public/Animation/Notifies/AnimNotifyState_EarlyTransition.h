#pragma once

#include "GameplayTagContainer.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Types/EnumTypes.h"
#include "Types/TagTypes.h"
#include "AnimNotifyState_EarlyTransition.generated.h"

UCLASS()
class GASP_API UAnimNotifyState_EarlyTransition : public UAnimNotifyState
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EarlyTransition")
	EEarlyTransitionDestination TransitionDestination{EEarlyTransitionDestination::ReTransition};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EarlyTransition")
	EEarlyTransitionCondition TransitionCondition{EEarlyTransitionCondition::Always};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EarlyTransition")
	FGameplayTag GaitNotEqual{GaitTags::Walk};

public:
	UAnimNotifyState_EarlyTransition();

	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime,
	                        const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
