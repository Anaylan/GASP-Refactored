#include "Types/MovementTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MovementTypes)

bool FGASPMoverInputs::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	RotationMode.NetSerialize(Ar, Map, bOutSuccess);
	Gait.NetSerialize(Ar, Map, bOutSuccess);
	Stance.NetSerialize(Ar, Map, bOutSuccess);

	Ar << ControlRotationRate;
	Ar << RotationOffset;
	Ar << MovementDirection;

	FloorLocation.NetSerialize(Ar, Map, bOutSuccess);
	FloorNormal.NetSerialize(Ar, Map, bOutSuccess);
	AimingRotation.SerializeCompressedShort(Ar);

	return FCharacterDefaultInputs::NetSerialize(Ar, Map, bOutSuccess);
}

void FGASPMoverInputs::ToString(FAnsiStringBuilderBase& Out) const
{
	FCharacterDefaultInputs::ToString(Out);
}
