#pragma once

#include "Types/EnumTypes.h"

DECLARE_STATS_GROUP(TEXT("GASP"), STATGROUP_GASP, STATCAT_Advanced)

struct GASP_API FGASPMath
{
	FGASPMath() = default;

	static float CalculateDirection(const FVector& Velocity, const FRotator& ActorRotation);

	static EMovementDirection GetMovementDirection(const float Angle, const float ForwardHalfAngle,
	                                               const float AngleThreshold);
	
	static EMovementDirection GetMovementDirectionFromThreshold(const FVector4& Thresholds, const float Direction);
	
	static FVector4 GetDirectionThresholds(EMovementDirection MovementDirection, int32 Style);
};
