#include "Nodes/RigUnit_DistributeRotationSimple.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RigUnit_DistributeRotationSimple)

FRigUnit_DistributeRotationSimple_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	auto* Hierarchy{ExecuteContext.Hierarchy};
	if (!IsValid(Hierarchy))
	{
		return;
	}

	if (Items.IsEmpty() || Rotation.IsIdentity(UE_KINDA_SMALL_NUMBER))
	{
		return;
	}

	auto NormalizedRotation{Rotation};
	NormalizedRotation.Normalize();

	// Match the original shortest-path distribution without using quaternion interpolation.
	if (NormalizedRotation.W < 0.0)
	{
		NormalizedRotation = NormalizedRotation * -1.0;
	}

	FVector RotationAxis{FVector::ForwardVector};
	double RotationAngle{0.0};
	NormalizedRotation.ToAxisAndAngle(RotationAxis, RotationAngle);

	const auto DeltaRotation{FQuat{RotationAxis, RotationAngle / static_cast<double>(Items.Num())}};
	if (DeltaRotation.IsIdentity(UE_KINDA_SMALL_NUMBER))
	{
		return;
	}

	if (CachedItems.Num() != Items.Num())
	{
		CachedItems.Reset();
		CachedItems.SetNum(Items.Num());
	}

	for (int32 i{0}; i < Items.Num(); i++)
	{
		if (CachedItems[i].UpdateCache(Items[i], Hierarchy))
		{
			auto NewTransform{Hierarchy->GetGlobalTransform(CachedItems[i])};
			NewTransform.SetRotation(DeltaRotation * NewTransform.GetRotation());

			Hierarchy->SetGlobalTransform(CachedItems[i], NewTransform);
		}
	}
}
