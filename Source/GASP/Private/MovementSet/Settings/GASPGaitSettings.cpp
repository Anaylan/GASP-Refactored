#include "MovementSet/Settings/GASPGaitSettings.h"

UGASPGaitSettings::UGASPGaitSettings()
{
}

UCurveVector* UGASPGaitSettings::GetMovementCurve() const
{
	return MovementCurve.Get();
}
