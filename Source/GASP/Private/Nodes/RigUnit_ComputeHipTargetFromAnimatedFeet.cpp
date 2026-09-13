#include "Nodes/RigUnit_ComputeHipTargetFromAnimatedFeet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RigUnit_ComputeHipTargetFromAnimatedFeet)

FRigUnit_ComputeHipTargetFromAnimatedFeet_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	auto* Hierarchy{ExecuteContext.Hierarchy};
	if (!IsValid(Hierarchy) || Feet.IsEmpty() || !CachedPelvisControl.UpdateCache(PelvisControl, Hierarchy))
	{
		return;
	}

	auto PelvisPos{Hierarchy->GetGlobalTransform(CachedPelvisControl).GetTranslation()};

	auto SumCentroids{FVector::ZeroVector};
	float MaxZ{-TNumericLimits<float>::Max()};
	int32 ValidFootCount{0};

	if (CachedToeControls.Num() != Feet.Num() || CachedToeTargets.Num() != Feet.Num())
	{
		CachedToeControls.Reset();
		CachedToeControls.SetNum(Feet.Num());

		CachedToeTargets.Reset();
		CachedToeTargets.SetNum(Feet.Num());
	}

	for (int32 i{0}; i < Feet.Num(); i++)
	{
		if (CachedToeControls[i].UpdateCache(Feet[i].ToeControl, Hierarchy) && CachedToeTargets[i].UpdateCache(
			Feet[i].ToeTarget, Hierarchy))
		{
			auto TargetPos{Hierarchy->GetGlobalTransform(CachedToeTargets[i]).GetTranslation()};
			auto ControlPos{Hierarchy->GetGlobalTransform(CachedToeControls[i]).GetTranslation()};

			auto FootCentroid{ControlPos + (PelvisPos - TargetPos)};

			SumCentroids += FootCentroid;
			MaxZ = FMath::Max(MaxZ, FootCentroid.Z);
			++ValidFootCount;
		}
	}

	if (ValidFootCount <= 0)
	{
		// No foot resolved against the hierarchy, so MaxZ is still its sentinel. Leaving Result
		// untouched keeps the previous hip target instead of writing -FLT_MAX into the rig.
		return;
	}

	// Divide by the feet that contributed, not by the configured count: an unresolved entry would
	// otherwise pull the centroid towards the origin.
	Result = SumCentroids / static_cast<float>(ValidFootCount);
}

// Touch for LiveCoding compilation
