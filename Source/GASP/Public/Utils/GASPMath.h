#pragma once

#include "GameplayTagContainer.h"
#include "Types/EnumTypes.h"

DECLARE_STATS_GROUP(TEXT("GASP"), STATGROUP_GASP, STATCAT_Advanced)

struct GASP_API FGASPMath
{
	FGASPMath() = default;

	static float CalculateDirection(const FVector& Velocity, const FRotator& ActorRotation);

	static EMovementDirection GetMovementDirection(const float Angle, const float ForwardHalfAngle,
	                                               const float AngleThreshold);
	
	static EMovementDirection GetMovementDirectionFromThreshold(const FVector4f& Thresholds, const float Direction);

	static float GetForwardAngle(const EMovementDirection& Direction,
	                             const int32 StyleIndex = 0);
};
