#include "Nodes/RigUnit_ComputeToeGroundAlignment.h"
#include "Utils/GASPMath.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RigUnit_ComputeToeGroundAlignment)

FRigUnit_ComputeToeGroundAlignment_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	auto* Hierarchy{ExecuteContext.Hierarchy};
	if (!IsValid(Hierarchy) || !CachedFootItem.UpdateCache(FootItem, Hierarchy) || !CachedToeItem.UpdateCache(
		ToeItem, Hierarchy))
	{
		return;
	}
	
	const auto FootPos{Hierarchy->GetGlobalTransform(CachedFootItem).GetTranslation()};
	const auto ToePos{Hierarchy->GetGlobalTransform(CachedToeItem).GetTranslation()};
	const auto FootToToeCurrent{ToePos - FootPos};

	const auto InitialFoot{Hierarchy->GetInitialGlobalTransform(CachedFootItem)};
	const auto InitialToe{Hierarchy->GetInitialGlobalTransform(CachedToeItem)};
	const float FootLengthSq{UE_REAL_TO_FLOAT((InitialToe.GetTranslation() - InitialFoot.GetTranslation()).SizeSquared())};

	const float VerticalDist{UE_REAL_TO_FLOAT((ToeImpactPoint - FootPos).Dot(FloorNormal))};
	const auto VerticalOffset{VerticalDist * FloorNormal};

	const auto CurrentVerticalPart{FootToToeCurrent.Dot(FloorNormal) * FloorNormal};
	const auto HorizontalDir{(FootToToeCurrent - CurrentVerticalPart).GetSafeNormal()};

	const float HorizontalDist{FMath::Sqrt(FMath::Max(0.0f, FootLengthSq - (VerticalDist * VerticalDist)))};
	Global = FootPos + VerticalOffset + (HorizontalDir * HorizontalDist);
}
