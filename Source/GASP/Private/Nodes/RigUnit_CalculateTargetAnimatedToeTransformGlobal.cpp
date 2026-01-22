#include "Nodes/RigUnit_CalculateTargetAnimatedToeTransformGlobal.h"
#include "Utils/GASPMath.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RigUnit_CalculateTargetAnimatedToeTransformGlobal)

FRigUnit_CalculateTargetAnimatedToeTransformGlobal_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	auto* Hierarchy{ExecuteContext.Hierarchy};
	if (!IsValid(Hierarchy) || !CachedFootBone.UpdateCache(FootBone, Hierarchy) || !CachedFootExternalIKTargetBoneBone.
		UpdateCache(FootExternalIKTargetBoneBone, Hierarchy) || !CachedToeBone.UpdateCache(ToeBone, Hierarchy))
	{
		return;
	}

	auto ToeFootRelative{
		UGASPMath::GetRelativeTransform(Hierarchy->GetGlobalTransform(CachedToeBone),
		                                Hierarchy->GetGlobalTransform(CachedFootBone))
	};

	auto IKFootRelative{
		UGASPMath::GetRelativeTransform(Hierarchy->GetGlobalTransform(CachedFootExternalIKTargetBoneBone),
		                                Hierarchy->GetGlobalTransform(CachedFootBone))
	};

	Global = ToeFootRelative * IKFootRelative * Hierarchy->GetGlobalTransform(CachedFootBone);
}
