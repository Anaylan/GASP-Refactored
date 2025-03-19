// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Types/EnumTypes.h"

DECLARE_STATS_GROUP(TEXT("GASP"), STATGROUP_GASP, STATCAT_Advanced)

struct GASP_API FGASPMath
{
	FGASPMath() = default;

	static float CalculateDirection(const FVector& Velocity, const FRotator& ActorRotation);

	static EMovementDirection GetMovementDirection(const float Angle, const float ForwardHalfAngle,
	                                               const float AngleThreshold);
};
