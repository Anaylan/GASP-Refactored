#include "Utils/GASPBlueprintLibrary.h"
#include "GameplayTagsManager.h"
#include "Types/MovementTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPBlueprintLibrary)

FName UGASPBlueprintLibrary::GetShortTagName(const FGameplayTag& GameplayTag)
{
	const auto TagNode{UGameplayTagsManager::Get().FindTagNode(GameplayTag)};

	return TagNode.IsValid() ? TagNode->GetSimpleTagName() : NAME_None;
}

FGameplayTagContainer UGASPBlueprintLibrary::GetAllChildTags(const FGameplayTag& GameplayTag)
{
	return UGameplayTagsManager::Get().RequestGameplayTagChildren(GameplayTag);
}

float UGASPBlueprintLibrary::GetAnimationCurveValueFromPawn(const APawn* Pawn, const FName& CurveName)
{
	const auto* Mesh{IsValid(Pawn) ? Pawn->FindComponentByClass<USkeletalMeshComponent>() : nullptr};
	const auto* AnimationInstance{IsValid(Mesh) ? Mesh->GetAnimInstance() : nullptr};

	return IsValid(AnimationInstance) ? AnimationInstance->GetCurveValue(CurveName) : 0.0f;
}