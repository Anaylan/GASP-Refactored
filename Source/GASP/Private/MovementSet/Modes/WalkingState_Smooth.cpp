// Copyright Epic Games, Inc. All Rights Reserved.

#include "MovementSet/Modes/WalkingState_Smooth.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(WalkingState_Smooth)

namespace WalkingStateErrorTolerance_Smooth
{
	constexpr float VelocityErrorTolerance = 10.f;
	constexpr float AngularVelocityErrorTolerance = 10.f;
	constexpr float AccelerationErrorTolerance = 50.f;
	constexpr float FacingDegreeErrorTolerance = 10.0f;
}

UScriptStruct* FWalkingState_Smooth::GetScriptStruct() const
{
	return StaticStruct();
}

FMoverDataStructBase* FWalkingState_Smooth::Clone() const
{
	return new FWalkingState_Smooth(*this);
}

bool FWalkingState_Smooth::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	bool bSuccess = Super::NetSerialize(Ar, Map, bOutSuccess);

	// Could be quantized to save bandwidth
	Ar << SpringVelocity;
	Ar << SpringAcceleration;
	Ar << IntermediateVelocity;
	Ar << IntermediateFacing;
	Ar << IntermediateAngularVelocity;

	return bSuccess;
}

void FWalkingState_Smooth::ToString(FAnsiStringBuilderBase& Out) const
{
	Super::ToString(Out);

	Out.Appendf("SpringVelocity=%s SpringAcceleration=%s IntVel=%s IntFac=%s IntAng=%s\n",
	            *SpringVelocity.ToCompactString(),
	            *SpringAcceleration.ToCompactString(),
	            *IntermediateVelocity.ToCompactString(),
	            *IntermediateFacing.ToString(),
	            *IntermediateAngularVelocity.ToString());
}

bool FWalkingState_Smooth::ShouldReconcile(const FMoverDataStructBase& AuthorityState) const
{
	const FWalkingState_Smooth* AuthoritySpringState = static_cast<const FWalkingState_Smooth*>(&AuthorityState);

	return (!(SpringVelocity - AuthoritySpringState->SpringVelocity).IsNearlyZero(
			WalkingStateErrorTolerance_Smooth::VelocityErrorTolerance) ||
		!(SpringAcceleration - AuthoritySpringState->SpringAcceleration).IsNearlyZero(
			WalkingStateErrorTolerance_Smooth::AccelerationErrorTolerance) ||
		!(IntermediateVelocity - AuthoritySpringState->IntermediateVelocity).IsNearlyZero(
			WalkingStateErrorTolerance_Smooth::VelocityErrorTolerance) ||
		(IntermediateFacing.AngularDistance(AuthoritySpringState->IntermediateFacing) >
			WalkingStateErrorTolerance_Smooth::FacingDegreeErrorTolerance ||
			!(IntermediateAngularVelocity - AuthoritySpringState->IntermediateAngularVelocity).IsNearlyZero(
				WalkingStateErrorTolerance_Smooth::AngularVelocityErrorTolerance)));
}


void FWalkingState_Smooth::Interpolate(const FMoverDataStructBase& From, const FMoverDataStructBase& To, float Pct)
{
	const FWalkingState_Smooth* FromState = static_cast<const FWalkingState_Smooth*>(&From);
	const FWalkingState_Smooth* ToState = static_cast<const FWalkingState_Smooth*>(&To);

	SpringVelocity = FMath::Lerp(FromState->SpringVelocity, ToState->SpringVelocity, Pct);
	SpringAcceleration = FMath::Lerp(FromState->SpringAcceleration, ToState->SpringAcceleration, Pct);
	IntermediateVelocity = FMath::Lerp(FromState->IntermediateVelocity, ToState->IntermediateVelocity, Pct);
	IntermediateFacing = FQuat::Slerp(FromState->IntermediateFacing, ToState->IntermediateFacing, Pct);
	IntermediateAngularVelocity = FMath::Lerp(FromState->IntermediateAngularVelocity,
	                                          ToState->IntermediateAngularVelocity, Pct);
}
