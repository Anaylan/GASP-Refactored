#include "MovementSet/Modifiers/MovementModifier_Ragdoll.h"
#include "MoverComponent.h"
#include "GameFramework/Pawn.h"
#include "MovementSet/Settings/RagdollSettings.h"
#include "Types/TagTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MovementModifier_Ragdoll)

namespace
{
}

FMovementModifier_Ragdoll::FMovementModifier_Ragdoll()
{
	DurationMs = -1.0f;
}

void FMovementModifier_Ragdoll::OnStart(UMoverComponent* MoverComp, const FMoverTimeStep& TimeStep,
                                        const FMoverSyncState& SyncState, const FMoverAuxStateContext& AuxState)
{
}

void FMovementModifier_Ragdoll::OnEnd(UMoverComponent* MoverComp, const FMoverTimeStep& TimeStep,
                                      const FMoverSyncState& SyncState, const FMoverAuxStateContext& AuxState)
{
}

FMovementModifierBase* FMovementModifier_Ragdoll::Clone() const
{
	return new FMovementModifier_Ragdoll(*this);
}

FString FMovementModifier_Ragdoll::ToSimpleString() const
{
	return FString::Printf(TEXT("Ragdoll Modifier"));
}

void FMovementModifier_Ragdoll::GetGameplayTags(FGameplayTagContainer& InOutTags) const
{
	InOutTags.AddTag(MovementModeTags::Ragdoll);
}

bool FMovementModifier_Ragdoll::HasGameplayTag(FGameplayTag TagToFind, bool bExactMatch) const
{
	if (bExactMatch)
	{
		return TagToFind.MatchesTagExact(MovementModeTags::Ragdoll);
	}

	return TagToFind.MatchesTag(MovementModeTags::Ragdoll);
}
