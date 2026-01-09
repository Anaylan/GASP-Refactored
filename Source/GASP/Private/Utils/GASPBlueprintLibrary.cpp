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