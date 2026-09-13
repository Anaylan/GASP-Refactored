#pragma once


#include "MovementModifier.h"
#include "MovementModifier_Ragdoll.generated.h"

/**
 * Owns the capsule shape for the duration of a ragdoll episode: shrinks it to URagdollSettings on
 * start and restores the owner's CDO shape on end, so the full-height capsule does not drag through
 * the floor while UMovementMode_Ragdolling chases the ragdoll's top bone.
 *
 * The resize lives here rather than in the movement mode because only modifiers take part in
 * FMovementModifierGroup::ShouldReconcile - a mode-driven resize would drift out of sync with the
 * simulation after a rollback.
 */
USTRUCT(BlueprintType)
struct GASP_API FMovementModifier_Ragdoll : public FMovementModifierBase
{
	GENERATED_BODY()

public:
	FMovementModifier_Ragdoll();
	virtual ~FMovementModifier_Ragdoll() override = default;

	virtual void OnStart(UMoverComponent* MoverComp, const FMoverTimeStep& TimeStep,
	                     const FMoverSyncState& SyncState, const FMoverAuxStateContext& AuxState) override;
	virtual void OnEnd(UMoverComponent* MoverComp, const FMoverTimeStep& TimeStep,
	                   const FMoverSyncState& SyncState, const FMoverAuxStateContext& AuxState) override;

	virtual FMovementModifierBase* Clone() const override;

	virtual UScriptStruct* GetScriptStruct() const override { return StaticStruct(); }

	virtual FString ToSimpleString() const override;

	virtual void GetGameplayTags(FGameplayTagContainer& InOutTags) const override;
	virtual bool HasGameplayTag(FGameplayTag TagToFind, bool bExactMatch) const override;
};

template <>
struct TStructOpsTypeTraits<FMovementModifier_Ragdoll> : public TStructOpsTypeTraitsBase2<FMovementModifier_Ragdoll>
{
	enum
	{
		WithCopy = true
	};
};
