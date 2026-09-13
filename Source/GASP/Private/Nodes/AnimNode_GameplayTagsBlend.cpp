#include "Nodes/AnimNode_GameplayTagsBlend.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNode_GameplayTagsBlend)

int32 FAnimNode_GameplayTagsBlend::GetActiveChildIndex()
{
	const FGameplayTag& CurrentActiveTag{GetActiveTag()};

	// Pose 0 is the default pose: it is used when no tag is set, and Find() returning INDEX_NONE
	// for an unknown tag lands on it too. Tagged poses follow in Tags order.
	const int32 Index{
		CurrentActiveTag.IsValid()
			? GetTags().Find(CurrentActiveTag) + 1
			: 0
	};

	// FAnimNode_BlendListBase indexes PerBlendData and CurrentBlendTimes with this value without
	// bounds checking. The Tags/BlendPose invariant is only maintained by the editor-only
	// RefreshPosePins, so a stale serialized asset would otherwise read out of bounds.
	return FMath::Clamp(Index, 0, FMath::Max(0, BlendPose.Num() - 1));
}

const FGameplayTag& FAnimNode_GameplayTagsBlend::GetActiveTag() const
{
	return GET_ANIM_NODE_DATA(FGameplayTag, ActiveTag);
}

const TArray<FGameplayTag>& FAnimNode_GameplayTagsBlend::GetTags() const
{
	return GET_ANIM_NODE_DATA(TArray<FGameplayTag>, Tags);
}

#if WITH_EDITOR
void FAnimNode_GameplayTagsBlend::RefreshPosePins()
{
	const int Difference = BlendPose.Num() - GetTags().Num() - 1;
	if (Difference == 0)
	{
		return;
	}

	if (Difference > 0)
	{
		for (int i = Difference; i > 0; i--)
		{
			RemovePose(BlendPose.Num() - 1);
		}
	}
	else
	{
		for (int i = Difference; i < 0; i++)
		{
			AddPose();
		}
	}
}
#endif
