#include "Utils/GASPRagdollLibrary.h"

void FGASPRagdollLibrary::IntegrateBodyState(FRagdollingState& State, const FVector& CurrentCenterOfMass,
                                             const FVector& SpineVelocity, const float DeltaTime)
{
	State.CenterOfMass = CurrentCenterOfMass;
	State.CenterOfMass_Velocity = (State.CenterOfMass - State.CenterOfMass_LastFrame) /
		FMath::Max(DeltaTime, UE_KINDA_SMALL_NUMBER);
	State.CenterOfMass_LastFrame = State.CenterOfMass;

	State.SpineVelocity = SpineVelocity;
	State.Speed = UE_REAL_TO_FLOAT(State.SpineVelocity.Size());
}

FVector2D FGASPRagdollLibrary::ComputeImpactDirection(const FTransform& TraceOrigin, const FVector& ImpactLocation)
{
	const auto RelImpact{TraceOrigin.InverseTransformVectorNoScale(ImpactLocation - TraceOrigin.GetLocation())};

	const float Size2D{static_cast<float>(RelImpact.Size2D())};

	return {
		FMath::RadiansToDegrees(FMath::Atan2(RelImpact.Y, RelImpact.X)),
		FMath::RadiansToDegrees(FMath::Atan2(RelImpact.Z, Size2D))
	};
}

void FGASPRagdollLibrary::AdvanceRollForces(FRagdollingState& State, const FVector& FloorNormal, const float DeltaTime)
{
	static const float CosFloorAngleMin{FMath::Cos(FMath::DegreesToRadians(FloorAngleMinDegrees))};
	static const float CosFloorAngleMax{FMath::Cos(FMath::DegreesToRadians(FloorAngleMaxDegrees))};

	const float FloorDot{static_cast<float>(FloorNormal.Z)};

	const float Alpha{FMath::Clamp((FloorDot - CosFloorAngleMin) / (CosFloorAngleMax - CosFloorAngleMin), 0.f, 1.f)};
	const auto TargetAutoRollForce{FloorNormal * Alpha};

	const float TargetSizeSq{Alpha * Alpha};
	const float CurrentSizeSq{static_cast<float>(State.AutoRollForce.SizeSquared())};
	const float InterpSpeed{(TargetSizeSq < CurrentSizeSq) ? AutoRollReleaseSpeed : AutoRollBuildSpeed};

	State.AutoRollForce = FMath::VInterpConstantTo(State.AutoRollForce, TargetAutoRollForce, DeltaTime, InterpSpeed);
	State.RollForce = State.AutoRollForce.GetClampedToMaxSize(1.f);
}

FVector FGASPRagdollLibrary::ComputeAutoRollTorque(const FRagdollingState& State)
{
	// Analytical CrossProduct with UpVector (0, 0, 1): (-Y, X, 0)
	return FVector{-State.RollForce.Y, State.RollForce.X, 0.f} * AutoRollTorqueScale;
}

float FGASPRagdollLibrary::ComputeGravityMultiplier(const FRagdollingState& State)
{
	return State.CenterOfMass_Velocity.Z < FreeFallGravityCutoffZ ? 0.f : 1.f;
}

bool FGASPRagdollLibrary::ShouldRollingGetUp(const FRagdollingState& State)
{
	return State.CenterOfMass_Velocity.SizeSquared2D() > RollingGetUpSpeedSqThreshold;
}
