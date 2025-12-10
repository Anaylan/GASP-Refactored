#include "Utils/GASPMath.h"

/**
 * Calculates the angle between a vector and a rotation in degrees.
 * Returns the signed angle between -180 and 180 degrees.
 * 
 * @param Vector - The input vector to calculate direction from (typically velocity or movement vector)
 * @param Rotation - The reference rotation to calculate direction against (typically actor rotation)
 * @return float - The angle in degrees between the vector and rotation:
 *         - 0 degrees: Vector points in the same direction as rotation
 *         - Positive: Vector points to the right of rotation
 *         - Negative: Vector points to the left of rotation
 *         - Returns 0 for zero vectors or zero rotations
 */
float FGASPMath::CalculateDirection(const FVector& Vector, const FRotator& Rotation)
{
	// Early return if either input is invalid
	if (Vector.IsNearlyZero() || Rotation.IsNearlyZero())
	{
		return 0.f;
	}

	// Normalize only X and Y components for 2D calculation
	const float VectorLength2D = FMath::Sqrt(Vector.X * Vector.X + Vector.Y * Vector.Y);
	if (VectorLength2D < SMALL_NUMBER)
	{
		return 0.f;
	}

	const FVector2f NormalizedVelocity{
		UE_REAL_TO_FLOAT(Vector.X / VectorLength2D),
		UE_REAL_TO_FLOAT(Vector.Y / VectorLength2D)
	};

	// Convert Yaw to radians once
	const float YawRad = FMath::DegreesToRadians(Rotation.Yaw);
	const float CosYaw = FMath::Cos(YawRad);
	const float SinYaw = FMath::Sin(YawRad);

	// Calculate forward and right vectors
	// Note: These are already normalized by definition
	const FVector2f ForwardVector{CosYaw, SinYaw};
	const FVector2f RightVector{-SinYaw, CosYaw}; // Optimized cross product for Z-up

	// Calculate dot products
	const float ForwardDot = NormalizedVelocity.X * ForwardVector.X +
		NormalizedVelocity.Y * ForwardVector.Y;
	const float RightDot = NormalizedVelocity.X * RightVector.X +
		NormalizedVelocity.Y * RightVector.Y;

	// Calculate angle and clamp between -180 and 180 degrees
	return FMath::UnwindDegrees(FMath::RadiansToDegrees(FMath::Atan2(RightDot, ForwardDot)));
}

EMovementDirection FGASPMath::GetMovementDirection(const float Angle, const float ForwardHalfAngle,
                                                   const float AngleThreshold)
{
	if (Angle >= -ForwardHalfAngle - AngleThreshold && Angle <= ForwardHalfAngle + AngleThreshold)
	{
		return EMovementDirection::F;
	}

	if (Angle >= ForwardHalfAngle - AngleThreshold && Angle <= 180.0f - ForwardHalfAngle + AngleThreshold)
	{
		return EMovementDirection::RL;
	}

	if (Angle <= -(ForwardHalfAngle - AngleThreshold) && Angle >= -(180.0f - ForwardHalfAngle + AngleThreshold))
	{
		return EMovementDirection::LL;
	}

	return EMovementDirection::B;
}

EMovementDirection FGASPMath::GetMovementDirectionFromThreshold(const FVector4f& Thresholds, const float Direction)
{
	if (Direction >= Thresholds.X && Direction <= Thresholds.Y)
	{
		return EMovementDirection::F;
	}
	if (Direction >= Thresholds.Z && Direction <= Thresholds.X)
	{
		return EMovementDirection::LL;
	}
	if (Direction >= Thresholds.Y && Direction <= Thresholds.W)
	{
		return EMovementDirection::RL;
	}

	return EMovementDirection::B;
}

float FGASPMath::GetForwardAngle(const EMovementDirection& Direction,
                                 const int32 StyleIndex)
{
	switch (StyleIndex)
	{
	case 1:
		if (Direction == EMovementDirection::B)
		{
			return 120.f;
		}
		return 140.f;
	case 2:
		return 180.f;
	default:
		if (Direction == EMovementDirection::F || Direction == EMovementDirection::B)
		{
			return 60.f;
		}
		return 40.f;
	}
}
