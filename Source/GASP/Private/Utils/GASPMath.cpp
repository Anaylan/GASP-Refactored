#include "Utils/GASPMath.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPMath)
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
float UGASPMath::CalculateDirection(const FVector& Vector, const FRotator& Rotation)
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
	const float ForwardDot = NormalizedVelocity.X * ForwardVector.X + NormalizedVelocity.Y * ForwardVector.Y;
	const float RightDot = NormalizedVelocity.X * RightVector.X + NormalizedVelocity.Y * RightVector.Y;

	// Calculate angle and clamp between -180 and 180 degrees
	return FMath::UnwindDegrees(FMath::RadiansToDegrees(FMath::Atan2(RightDot, ForwardDot)));
}

EMovementDirection UGASPMath::GetMovementDirection(const float Angle, const float ForwardHalfAngle,
                                                   const float AngleThreshold)
{
	if (Angle >= -ForwardHalfAngle - AngleThreshold && Angle <= ForwardHalfAngle + AngleThreshold)
	{
		return EMovementDirection::F;
	}

	if (Angle >= ForwardHalfAngle - AngleThreshold && Angle <= 180.0f - ForwardHalfAngle + AngleThreshold)
	{
		return EMovementDirection::RR;
	}

	if (Angle <= -(ForwardHalfAngle - AngleThreshold) && Angle >= -(180.0f - ForwardHalfAngle + AngleThreshold))
	{
		return EMovementDirection::LL;
	}

	return EMovementDirection::B;
}

EMovementDirection UGASPMath::GetMovementDirectionFromThreshold(const FVector4& Thresholds, const float Direction)
{
	const auto LeftDiagLimit{(Thresholds.Z + Thresholds.X) / 2.0f};
	const auto RightDiagLimit{(Thresholds.W + Thresholds.Y) / 2.0f};

	if (FMath::IsWithinInclusive(Direction, Thresholds.X, Thresholds.Y))
	{
		return EMovementDirection::F;
	}

	if (FMath::IsWithinInclusive(Direction, LeftDiagLimit, Thresholds.X))
	{
		return EMovementDirection::FL;
	}

	if (FMath::IsWithinInclusive(Direction, Thresholds.Z, LeftDiagLimit))
	{
		return EMovementDirection::LL;
	}

	if (FMath::IsWithinInclusive(Direction, Thresholds.Y, RightDiagLimit))
	{
		return EMovementDirection::FR;
	}

	if (FMath::IsWithinInclusive(Direction, RightDiagLimit, Thresholds.W))
	{
		return EMovementDirection::RR;
	}

	return EMovementDirection::B;
}

FVector4 UGASPMath::GetDirectionThresholds(const EMovementDirection MovementDirection, int32 Style)
{
	switch (Style)
	{
	case 0:
		if (MovementDirection == EMovementDirection::B || MovementDirection == EMovementDirection::F)
		{
			return FVector4{-35.f, 35.f, -125.f, 125.f};
		}
		return FVector4{-25.f, 25.f, -115.f, 115.f};

	case 1:
		if (MovementDirection == EMovementDirection::B)
		{
			return FVector4{-120.f, 120.f, -120.f, 120.f};
		}
		return FVector4{-140.f, 140.f, -140.f, 140.f};

	default:
		return FVector4{-180.f, 180.f, -180.f, 180.f};
	}
}
