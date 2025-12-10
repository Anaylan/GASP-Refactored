#include "Types/MovementTypes.h"

bool FGASPMoverInputs::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	RotationMode.NetSerialize(Ar, Map, bOutSuccess);
	Gait.NetSerialize(Ar, Map, bOutSuccess);
	Stance.NetSerialize(Ar, Map, bOutSuccess);

	Ar << ControlRotationRate;
	Ar << RotationOffset;
	Ar << MovementDirection;
	Ar << FloorLocation;
	Ar << FloorNormal;
	Ar << AimingRotation;

	return FCharacterDefaultInputs::NetSerialize(Ar, Map, bOutSuccess);
}

void FGASPMoverInputs::ToString(FAnsiStringBuilderBase& Out) const
{
	FCharacterDefaultInputs::ToString(Out);
}
