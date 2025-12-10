#include "Animation/Notifies/AnimNotifyState_MontageBlendOut.h"
#include "Actors/GASPCharacter.h"
#include "Kismet/KismetMathLibrary.h"
#include "Types/EnumTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNotifyState_MontageBlendOut)

UAnimNotifyState_MontageBlendOut::UAnimNotifyState_MontageBlendOut()
{
#if WITH_EDITORONLY_DATA
	bShouldFireInEditor = false;
#endif
}

FString UAnimNotifyState_MontageBlendOut::GetNotifyName_Implementation() const
{
	FString Name{TEXT("Early Blend out - ")};
	return Name.Append(GetNameStringByValue(BlendOutCondition));
}

void UAnimNotifyState_MontageBlendOut::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                                  float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (!IsValid(MeshComp))
	{
		return;
	}

	const auto Character = Cast<AGASPCharacter>(MeshComp->GetOwner());
	if (!IsValid(Character))
	{
		return;
	}

	auto* AnimInstance = MeshComp->GetAnimInstance();
	if (!IsValid(AnimInstance))
	{
		return;
	}

	const bool ShouldBlendOut = [&]()
	{
		switch (BlendOutCondition)
		{
		case ETraversalBlendOutCondition::WithMovementInput:
			return !Character->GetMoverInputs_PostSim().GetMoveInput().Equals(FVector::ZeroVector, .1f);
		case ETraversalBlendOutCondition::IfFalling:
			return Character->GetMovementMode() == MovementModeTags::InAir;
		default:
			return true;
		}
	}();

	if (const auto* AnimMontage = Cast<UAnimMontage>(Animation); ShouldBlendOut)
	{
		FMontageBlendSettings BlendOutSettings{};
		BlendOutSettings.Blend.BlendTime = BlendOutTime;
		BlendOutSettings.Blend.BlendOption = EAlphaBlendOption::HermiteCubic;
		BlendOutSettings.BlendMode = EMontageBlendMode::Standard;
		BlendOutSettings.BlendProfile = const_cast<UBlendProfile*>(AnimInstance->GetBlendProfileByName(BlendProfile));

		AnimInstance->Montage_StopWithBlendSettings(BlendOutSettings, AnimMontage);
	}
}
