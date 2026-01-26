#include "MovementSet/Settings/GASPGaitSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPGaitSettings)

UGASPGaitSettings::UGASPGaitSettings()
{
}

UCurveVector* UGASPGaitSettings::GetMovementCurve() const
{
	return MovementCurve.Get();
}
