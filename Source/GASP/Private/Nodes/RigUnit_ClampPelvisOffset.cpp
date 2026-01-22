#include "Nodes/RigUnit_ClampPelvisOffset.h"
#include "Utils/GASPMath.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RigUnit_ClampPelvisOffset)

FRigUnit_ClampPelvisOffset_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	auto* Hierarchy{ExecuteContext.Hierarchy};
	if (!IsValid(Hierarchy) || !CachedFootBone.UpdateCache(FootBone, Hierarchy) || !CachedFootControl.
		UpdateCache(FootControl, Hierarchy) || !CachedPelvisItem.UpdateCache(PelvisItem, Hierarchy) ||
		!CachedUpperLegBone.UpdateCache(UpperLegBone, Hierarchy))
	{
		return;
	}

	auto ProposedPelvisAdjustmentTranslation{
		PelvisPosition - Hierarchy->GetGlobalTransform(CachedPelvisItem).GetTranslation()
	};
	auto ProposedUpperLegTranslation{
		ProposedPelvisAdjustmentTranslation + Hierarchy->GetGlobalTransform(CachedUpperLegBone).GetTranslation()
	};
	auto UpperLegToPelvisTranslation{PelvisPosition - ProposedUpperLegTranslation};

	auto FootControlTranslation{Hierarchy->GetGlobalTransform(CachedFootControl).GetTranslation()};
	const auto FootToUpperLength{
		UGASPMath::GetRelativeTransform(Hierarchy->GetGlobalTransform(CachedFootBone),
		                                Hierarchy->GetGlobalTransform(CachedUpperLegBone)).GetTranslation().Size()
	};
	auto ClampedUpperLegTranslation
	{
		UGASPMath::ClampVectorLength(ProposedUpperLegTranslation - FootControlTranslation, 0.f,
		                             FMath::Max(FootToUpperLength, LimitFactor * LegLength)) + FootControlTranslation
	};

	ClampedPosition = UpperLegToPelvisTranslation + ClampedUpperLegTranslation;
}
