#include "Animation/Notifies/AnimNotifyState_EarlyTransition.h"
#include "Animation/GASPAnimInstance.h"
#include "Types/EnumTypes.h"
#include "Utils/GASPBlueprintLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNotifyState_EarlyTransition)

UAnimNotifyState_EarlyTransition::UAnimNotifyState_EarlyTransition()
{
#if WITH_EDITORONLY_DATA
	bShouldFireInEditor = false;
#endif
}

void UAnimNotifyState_EarlyTransition::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                                  float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (!IsValid(MeshComp) || EventReference.IsActiveContext())
	{
		return;
	}

	auto* AnimInstance = static_cast<UGASPAnimInstance*>(MeshComp->GetAnimInstance());
	if (!IsValid(AnimInstance))
	{
		return;
	}

	const bool bTransition = [&]()
	{
		switch (TransitionCondition)
		{
		case EEarlyTransitionCondition::GaitNotEqual:
			return AnimInstance->GetGait() != GaitNotEqual;
		default:
			return true;
		}
	}();

	if (!bTransition)
	{
		return;
	}

	switch (TransitionDestination)
	{
	case EEarlyTransitionDestination::ReTransition:
		AnimInstance->bNotifyTransition_ReTransition = true;
		break;
	case EEarlyTransitionDestination::TransitionToLoop:
		AnimInstance->bNotifyTransition_ToLoop = true;
		break;
	default:
		break;
	}
}

FString UAnimNotifyState_EarlyTransition::GetNotifyName_Implementation() const
{
	FString Output{GetNameStringByValue(TransitionDestination)};

	switch (TransitionCondition)
	{
	case EEarlyTransitionCondition::GaitNotEqual:
		Output.Append(TEXT(" - If - "));
		Output.Append(GetNameStringByValue(TransitionCondition));
		Output.Append(TEXT(" - "));
		Output.Append(TransitionCondition == EEarlyTransitionCondition::GaitNotEqual
			              ? *UGASPBlueprintLibrary::GetShortTagName(GaitNotEqual).ToString()
			              : TEXT(""));
		break;
	default:
		Output.Append(TEXT(" - Always"));
		break;
	}

	return Output;
}
