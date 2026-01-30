#include "Types/MovementTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MovementTypes)

bool FGASPMoverInputs::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	bOutSuccess = FCharacterDefaultInputs::NetSerialize(Ar, Map, bOutSuccess);

	RotationMode.NetSerialize(Ar, Map, bOutSuccess);
	Gait.NetSerialize(Ar, Map, bOutSuccess);
	Stance.NetSerialize(Ar, Map, bOutSuccess);

	Ar << ControlRotationRate;
	Ar << RotationOffset;
	Ar << MovementDirection;

	FloorLocation.NetSerialize(Ar, Map, bOutSuccess);
	FloorNormal.NetSerialize(Ar, Map, bOutSuccess);
	AimingRotation.SerializeCompressedShort(Ar);

	return bOutSuccess;
}

void FGASPMoverInputs::ToString(FAnsiStringBuilderBase& Out) const
{
	FCharacterDefaultInputs::ToString(Out);

	Out.Appendf("Gait: %s\n", TCHAR_TO_ANSI(*Gait.ToString()));
	Out.Appendf("RotationMode: %s\n", TCHAR_TO_ANSI(*RotationMode.ToString()));
	Out.Appendf("Stance: %s\n", TCHAR_TO_ANSI(*Stance.ToString()));
	Out.Appendf("ControlRotationRate: %.3f\n", ControlRotationRate);
	Out.Appendf("RotationOffset: %.3f\n", RotationOffset);
	Out.Appendf("MovementDirection: %i\n", static_cast<int32>(MovementDirection));
	Out.Appendf("FloorLocation: X=%.1f Y=%.1f Z=%.1f\n", FloorLocation.X, FloorLocation.Y, FloorLocation.Z);
	Out.Appendf("FloorNormal: X=%.2f Y=%.2f Z=%.2f\n", FloorNormal.X, FloorNormal.Y, FloorNormal.Z);
	Out.Appendf("AimingRotation: P=%.2f Y=%.2f R=%.2f\n", AimingRotation.Pitch, AimingRotation.Yaw,
	            AimingRotation.Roll);
}

bool FGASPMoverInputs::ShouldReconcile(const FMoverDataStructBase& AuthorityState) const
{
	const auto& TypedAuthority = static_cast<const FGASPMoverInputs&>(AuthorityState);
	return *this != TypedAuthority;
}

void FGASPMoverInputs::Interpolate(const FMoverDataStructBase& From, const FMoverDataStructBase& To, float Pct)
{
	FCharacterDefaultInputs::Interpolate(From, To, Pct);
	
	const auto& TypedFrom = static_cast<const FGASPMoverInputs&>(From);
	const auto& TypedTo = static_cast<const FGASPMoverInputs&>(To);
	
	const auto* ClosestInputs = Pct < 0.5f ? &TypedFrom : &TypedTo;
	MovementDirection = ClosestInputs->MovementDirection;
	Gait = ClosestInputs->Gait;
	RotationMode = ClosestInputs->RotationMode;
	Stance = ClosestInputs->Stance;
	
	ControlRotationRate = FMath::Lerp(TypedFrom.ControlRotationRate, TypedTo.ControlRotationRate, Pct);
	RotationOffset = FMath::Lerp(TypedFrom.RotationOffset, TypedTo.RotationOffset, Pct);
	
	FloorLocation = FMath::Lerp(TypedFrom.FloorLocation, TypedTo.FloorLocation, Pct);
	FloorNormal = FMath::Lerp(TypedFrom.FloorNormal, TypedTo.FloorNormal, Pct);
	
	AimingRotation = FMath::Lerp(TypedFrom.AimingRotation, TypedTo.AimingRotation, Pct);
}
