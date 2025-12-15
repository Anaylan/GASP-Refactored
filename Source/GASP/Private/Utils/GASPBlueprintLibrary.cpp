#include "Utils/GASPBlueprintLibrary.h"

#include "GameplayTagsManager.h"
#include "GameFramework/Character.h"
#include "Types/MovementTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPBlueprintLibrary)

float UGASPBlueprintLibrary::GetAnimationCurveValueFromCharacter(const ACharacter* Character, const FName& CurveName)
{
	const auto* Mesh{IsValid(Character) ? Character->GetMesh() : nullptr};
	const auto* AnimationInstance{IsValid(Mesh) ? Mesh->GetAnimInstance() : nullptr};

	return IsValid(AnimationInstance) ? AnimationInstance->GetCurveValue(CurveName) : 0.0f;
}

FName UGASPBlueprintLibrary::GetShortTagName(const FGameplayTag& GameplayTag)
{
	const auto TagNode{UGameplayTagsManager::Get().FindTagNode(GameplayTag)};

	return TagNode.IsValid() ? TagNode->GetSimpleTagName() : NAME_None;
}

FGameplayTagContainer UGASPBlueprintLibrary::GetAllChildTags(const FGameplayTag& GameplayTag)
{
	return UGameplayTagsManager::Get().RequestGameplayTagChildren(GameplayTag);
}

FVector4 UGASPBlueprintLibrary::GetDirectionThresholds(const EMovementDirection MovementDirection, int32 Style)
{
	switch (Style)
	{
	case 0:
		if (MovementDirection == EMovementDirection::B || MovementDirection == EMovementDirection::F)
		{
			return FVector4{-60.f, 60.f, -120.f, 120.f};
		}
		return FVector4{-40.f, 40.f, -140.f, 140.f};
	case 1:
		if (MovementDirection == EMovementDirection::B)
		{
			return FVector4{-120.f, 120.f, -120.f, 120.f};
		}
		return FVector4{-140.f, 140.f, -140.f, 140.f};
	default:
		return FVector4{-180.f, 180.f, -180.f, 180.f};
	}
}
