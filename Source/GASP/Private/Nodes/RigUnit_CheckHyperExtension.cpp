#include "Nodes/RigUnit_CheckHyperExtension.h"
#include "Utils/GASPMath.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RigUnit_CheckHyperExtension)

FRigUnit_CheckHyperExtension_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	auto Hierarchy{ExecuteContext.Hierarchy};
	if (!IsValid(Hierarchy) || !CachedChildBone.UpdateCache(ChildBone, Hierarchy) || !CachedChildControl.UpdateCache(
		ChildControl, Hierarchy) || !CachedParent.UpdateCache(Parent, Hierarchy))
	{
		return;
	}

	const auto BoneRelTransform{
		UGASPMath::GetRelativeTransform(Hierarchy->GetGlobalTransform(CachedChildBone),
		                                Hierarchy->GetGlobalTransform(CachedParent))
	};


	const auto ControlRelTransform{
		UGASPMath::GetRelativeTransform(Hierarchy->GetGlobalTransform(CachedChildControl),
		                                Hierarchy->GetGlobalTransform(CachedParent))
	};

	const float HyperExtensionLength{
		UE_REAL_TO_FLOAT(FMath::Max(LegLength * LimitFactor, BoneRelTransform.GetTranslation().Size()) + .1f)
	};

	OutResult = ControlRelTransform.GetTranslation().SizeSquared() > HyperExtensionLength * HyperExtensionLength;
	
	auto ClampedVector{UGASPMath::ClampVectorLength(ControlRelTransform.GetTranslation(), 0.f, HyperExtensionLength)};
	Global = {ControlRelTransform.GetRotation(), ClampedVector, ControlRelTransform.GetScale3D()};
}
