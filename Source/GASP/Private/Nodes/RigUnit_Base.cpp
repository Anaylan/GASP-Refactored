#include "Nodes/RigUnit_Base.h"
#include "Utils/GASPMath.h"
#include "RigVMFunctions/Math/RigVMMathLibrary.h"
#include "RigVMFunctions/Math/RigVMFunction_MathVector.h"
#include "RigVMFunctions/Math/RigVMFunction_MathQuaternion.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RigUnit_Base)

FRigUnit_AdjustPositionToPlane_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	FVector PlaneIntersection;
	float Distance;
	FRigVMFunction_MathIntersectPlane::StaticExecute(ExecuteContext, PositionOnFlat, -FVector::UpVector, PlanePoint,
	                                                 PlaneNormal, PlaneIntersection, Distance);

	AdjustedPosition = PlaneIntersection + FVector::UpVector * AnimatedHeightOffset;
}

FRigUnit_CalculateLimbLength_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	auto* Hierarchy{ExecuteContext.Hierarchy};
	if (!IsValid(Hierarchy) || !CachedUpperBone.UpdateCache(UpperBone, Hierarchy) || !CachedEndBone.
		UpdateCache(EndBone, Hierarchy) || !CachedMidBone.UpdateCache(MidBone, Hierarchy))
	{
		return;
	}

	const auto MidUpperRelative{
		UGASPMath::GetRelativeTransform(Hierarchy->GetInitialGlobalTransform(CachedMidBone),
		                                Hierarchy->GetInitialGlobalTransform(CachedUpperBone))
	};

	const auto EndMidRelative{
		UGASPMath::GetRelativeTransform(Hierarchy->GetInitialGlobalTransform(CachedEndBone),
		                                Hierarchy->GetInitialGlobalTransform(CachedMidBone))
	};

	Result = MidUpperRelative.GetTranslation().Size() + EndMidRelative.GetTranslation().Size();
}


FRigUnit_CalculateSlopeAngle_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	const auto RotatedVector{Rotation.RotateVector(ForwardAxis).GetUnsafeNormal()};
	const float DegreesVector{
		UE_REAL_TO_FLOAT(FMath::RadiansToDegrees(FMath::Asin(PlaneNormal.Dot(RotatedVector))))
	};

	SlopeAngleDegrees = Rotation.RotateVector(UpAxis).Z > 0.f ? DegreesVector : -DegreesVector;
}


FRigUnit_ClampTransformMinHeightFromReference_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	auto* Hierarchy{ExecuteContext.Hierarchy};

	if (!IsValid(Hierarchy))
	{
		return;
	}

	auto Translation{Transform.GetTranslation()};
	Translation.Z = FMath::Max(Translation.Z, Hierarchy->GetGlobalTransform(Bone, true).GetTranslation().Z);

	Global = FTransform{Transform.GetRotation(), Translation, Transform.GetScale3D()};
}


FRigUnit_ComputePinnedToeTransform_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	auto* Hierarchy{ExecuteContext.Hierarchy};
	if (!IsValid(Hierarchy))
	{
		return;
	}

	auto ToeWorldSpace{ExecuteContext.ToWorldSpace(ToeAnimatedTransformGlobal)};
	auto ToeAnimatedQuatAxisZ{ToeWorldSpace.GetRotation().GetAxisZ()};
	auto ToePinQuatAxisZ{ToePinTransformWorld.GetRotation().GetAxisZ()};

	World = FTransform{
		FRigVMMathLibrary::FindQuatBetweenVectors({ToeAnimatedQuatAxisZ.X, ToeAnimatedQuatAxisZ.Y, 0.f},
		                                          {
			                                          ToePinQuatAxisZ.X, ToePinQuatAxisZ.Y, 0.f
		                                          }) * ToeWorldSpace.GetRotation(),
		FVector{
			ToePinTransformWorld.GetTranslation().X, ToePinTransformWorld.GetTranslation().Y,
			ToeWorldSpace.GetTranslation().Z
		},
		ToeAnimatedTransformGlobal.GetScale3D()
	};
}


FRigUnit_GetPoleVector_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	auto* Hierarchy{ExecuteContext.Hierarchy};
	if (!IsValid(Hierarchy) || !CachedTarget.UpdateCache(Target, Hierarchy) || !CachedUpperLeg.
		UpdateCache(UpperLeg, Hierarchy) || !CachedLowerLeg.UpdateCache(LowerLeg, Hierarchy))
	{
		return;
	}

	const auto TargetTranslation{Hierarchy->GetGlobalTransform(CachedTarget).GetTranslation()};
	const auto UpperLegTranslation{Hierarchy->GetGlobalTransform(CachedUpperLeg).GetTranslation()};
	const auto LowerLegRotation{Hierarchy->GetGlobalTransform(CachedLowerLeg).GetRotation()};

	Result = (TargetTranslation - UpperLegTranslation).Cross(
		LowerLegRotation.RotateVector(SecondaryAxis.Cross(PrimaryAxis))) + UpperLegTranslation;
}


FRigUnit_LimitRotationToMaxSlopeAngle_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	auto* Hierarchy{ExecuteContext.Hierarchy};
	if (!IsValid(Hierarchy) || !CachedItem.UpdateCache(Item, Hierarchy))
	{
		return;
	}

	auto TargetVec{Hierarchy->GetGlobalTransform(CachedItem).GetRotation().GetAxisZ()};
	const float AngleBetween{
		static_cast<float>(FRigVMMathLibrary::FindQuatBetweenVectors(FVector::UpVector, TargetVec).GetAngle())
	};

	const float AngleLimitRadians{FMath::DegreesToRadians(AngleLimitDegrees)};

	Result = FRigVMMathLibrary::FindQuatBetweenVectors(FVector::UpVector,
	                                                   AngleBetween > AngleLimitRadians
		                                                   ? FVector{TargetVec.X, TargetVec.Y, 0.f}.GetUnsafeNormal() *
		                                                   FMath::Sin(AngleLimitRadians) +
		                                                   FVector::UpVector * FMath::Cos(AngleLimitRadians)
		                                                   : TargetVec);
}

FRigUnit_ComputeUnpinningTranslation_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	auto DeltaCurrent{
		ExecuteContext.ToWorldSpace(AnimatedToeTargetGlobal).GetTranslation() - ToePinAnimTransformWorld.
		GetTranslation()
	};
	Global = ExecuteContext.ToVMSpace(ToePinTransformWorld.GetTranslation() + DeltaCurrent);
}

FRigUnit_ProjectZDamperToGroundPlane_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	auto WorldDisplacement{ExecuteContext.ToWorldSpace(FVector::ZeroVector) - WorldZPrev};
	auto ZMovementGroundNormal{GroundNormal * WorldDisplacement.Dot(GroundNormal)};
	Result = WorldZDamper + (WorldDisplacement - ZMovementGroundNormal).Z;
}

FRigUnit_AlphaLinearInterp_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	if (Speed > 0.f || FMath::IsNearlyEqual(Value, Target, .001f))
	{
		Result = Target;
		return;
	}

	Result = Value + FMath::Clamp(ExecuteContext.GetDeltaTime() * Speed, 0.f, 1.f) * (Target - Value);
}

FRigUnit_ClampPinYaw_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	float Angle;
	FVector Axis;
	FRigVMFunction_MathQuaternionToAxisAndAngle::StaticExecute(ExecuteContext,
	                                                           FRigVMMathLibrary::FindQuatBetweenVectors(
		                                                           Rotation.GetAxisZ(),
		                                                           ExecuteContext.ToVMSpace(UpdatedPinTransform).
		                                                           GetRotation().GetAxisZ()),
	                                                           Axis, Angle);

	const FQuat AxisRotation{
		Axis.GetUnsafeNormal(),
		FMath::DegreesToRadians(FMath::Clamp(FMath::RadiansToDegrees(Angle), -PinYawLimitDegrees, PinYawLimitDegrees))
	};

	World = ExecuteContext.ToWorldSpace(AxisRotation * Rotation);
}

FRigUnit_ClampPinDistance_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	auto Delta{ExecuteContext.ToVMSpace(UpdatedPinTransform.GetTranslation()) - Translation};

	auto ClampedDelta{UGASPMath::ClampVectorLength({Delta.X, Delta.Y, 0.f}, 0.f, MaxFootPinDistance)};
	
	World = ExecuteContext.ToWorldSpace(ClampedDelta + Translation);
}
