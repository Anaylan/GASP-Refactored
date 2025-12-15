#include "Types/MovementTypes.h"

bool FGASPMoverInputs::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	Super::NetSerialize(Ar, Map, bOutSuccess);
	RotationMode.NetSerialize(Ar, Map, bOutSuccess);
	Gait.NetSerialize(Ar, Map, bOutSuccess);
	Stance.NetSerialize(Ar, Map, bOutSuccess);

	Ar << ControlRotationRate;
	Ar << RotationOffset;
	Ar << MovementDirection;

	SerializeFixedVector<1, 16>(FloorLocation, Ar);
	SerializeFixedVector<1, 16>(FloorNormal, Ar);
	AimingRotation.SerializeCompressedShort(Ar);

	return bOutSuccess;
}

void FGASPMoverInputs::ToString(FAnsiStringBuilderBase& Out) const
{
	Super::ToString(Out);
}
