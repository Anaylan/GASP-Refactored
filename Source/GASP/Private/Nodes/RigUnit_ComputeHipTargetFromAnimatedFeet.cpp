#include "Nodes/RigUnit_ComputeHipTargetFromAnimatedFeet.h"
#include "Math/UnitConversion.h"

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
		}
	}

	const auto AverageCentroid{SumCentroids / UE_REAL_TO_FLOAT(Feet.Num())};
	Result = {AverageCentroid.X, AverageCentroid.Y, MaxZ};
}
