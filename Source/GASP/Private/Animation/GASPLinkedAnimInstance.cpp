#include "Animation/GASPLinkedAnimInstance.h"
#include "Actors/GASPCharacter.h"
#include "Animation/GASPAnimInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPLinkedAnimInstance)

UGASPLinkedAnimInstance::UGASPLinkedAnimInstance()
{
	bUseMainInstanceMontageEvaluationData = true;
}

void UGASPLinkedAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Parent = Cast<UGASPAnimInstance>(GetSkelMeshComponent()->GetAnimInstance());
	Character = Cast<AGASPCharacter>(TryGetPawnOwner());

#if WITH_EDITOR
	const auto* World{GetWorld()};

	if (IsValid(World) && !World->IsGameWorld())
	{
		// Use default objects for editor preview.
		if (!IsValid(Parent))
		{
			Parent = GetMutableDefault<UGASPAnimInstance>();
		}

		if (!IsValid(Character))
		{
			Character = GetMutableDefault<AGASPCharacter>();
		}
	}
#endif
}

UGASPAnimInstance* UGASPLinkedAnimInstance::GetParent() const
{
	return Parent.Get();
}

AGASPCharacter* UGASPLinkedAnimInstance::GetCharacter() const
{
	return Character;
}

FGameplayTag UGASPLinkedAnimInstance::GetGait() const
{
	if (IsValid(Parent))
	{
		return Parent->GetGait();
	}

	return FGameplayTag::EmptyTag;
}

FGameplayTag UGASPLinkedAnimInstance::GetMovementState() const
{
	if (IsValid(Parent))
	{
		return Parent->GetMovementState();
	}

	return FGameplayTag::EmptyTag;
}

FGameplayTag UGASPLinkedAnimInstance::GetMovementMode() const
{
	if (IsValid(Parent))
	{
		return Parent->GetMovementMode();
	}

	return FGameplayTag::EmptyTag;
}

FGameplayTag UGASPLinkedAnimInstance::GetStanceMode() const
{
	if (IsValid(Parent))
	{
		return Parent->GetStanceMode();
	}

	return FGameplayTag::EmptyTag;
}

FGameplayTag UGASPLinkedAnimInstance::GetRotationMode() const
{
	if (IsValid(Parent))
	{
		return Parent->GetRotationMode();
	}

	return FGameplayTag::EmptyTag;
}

FCharacterInfo UGASPLinkedAnimInstance::GetCharacterInfo() const
{
	if (IsValid(Parent))
	{
		return Parent->GetCharacterInfo();
	}

	return FCharacterInfo();
}

float UGASPLinkedAnimInstance::GetAimSweep() const
{
	return FMath::GetMappedRangeValueClamped<float, float>({-90.f, 90.f},
	                                                       {1.f, 0.f},
	                                                       GetParent()->GetAOValue().Y);
}
