#pragma once

#include "Types/EnumTypes.h"
#include "GASPMath.generated.h"

DECLARE_STATS_GROUP(TEXT("GASP"), STATGROUP_GASP, STATCAT_Advanced)

UCLASS(meta = (BlueprintThreadSafe))
class GASP_API UGASPMath : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UGASPMath() = default;

	static float CalculateDirection(const FVector& Velocity, const FRotator& ActorRotation);

	UFUNCTION(BlueprintPure, Category = "GASP|Utility")
	static EMovementDirection GetMovementDirection(const float Angle, const float ForwardHalfAngle,
	                                               const float AngleThreshold);

	UFUNCTION(BlueprintPure, Category = "GASP|Utility")
	static EMovementDirection GetMovementDirectionFromThreshold(const FVector4& Thresholds, const float Direction);

	UFUNCTION(BlueprintPure, Category = "GASP|Utility")
	static FVector4 GetDirectionThresholds(EMovementDirection MovementDirection, int32 Style = 0);

	template <typename ValueType> requires UE::CFloatingPoint<ValueType>
	static constexpr ValueType RemapAngleForCounterClockwiseRotation(ValueType Angle);
	
	UFUNCTION(BlueprintPure, Category = "GASP|Utility", Meta = (ReturnDisplayName = "Angle"))
	static float RemapAngleForCounterClockwiseRotation(float Angle);
	
	UFUNCTION(BlueprintPure, Category = "GASP|Math", Meta = (ReturnDisplayName = "Alpha"))
	static float DamperExactAlpha(float DeltaTime, float HalfLife);

	// HalfLife is the time it takes for the distance to the target to be reduced by half.
	template <typename ValueType>
	static ValueType DamperExact(const ValueType& Current, const ValueType& Target, float DeltaTime, float HalfLife);
	
	UFUNCTION(BlueprintPure, Category = "GASP|Utility", Meta = (ReturnDisplayName = "Angle"))
	static float LerpAngle(float From, float To, float Ratio);

	// UFUNCTION(BlueprintPure, Category = "ALS|Rotation Utility", Meta = (AutoCreateRefTerm = "From, To", ReturnDisplayName = "Rotation"))
	// static FRotator LerpRotation(const FRotator& From, const FRotator& To, float Ratio);
};

template <typename ValueType> requires UE::CFloatingPoint<ValueType>
constexpr ValueType UGASPMath::RemapAngleForCounterClockwiseRotation(const ValueType Angle)
{
	if (Angle > 180.0f)
	{
		return Angle - 360.0f;
	}

	return Angle;
}

inline float UGASPMath::DamperExactAlpha(const float DeltaTime, const float HalfLife)
{
	// https://theorangeduck.com/page/spring-roll-call#exactdamper

	return 1.0f - FMath::InvExpApprox(UE_LN2 / (HalfLife + UE_SMALL_NUMBER) * DeltaTime);
}

template <typename ValueType>
ValueType UGASPMath::DamperExact(const ValueType& Current, const ValueType& Target, const float DeltaTime,
                                 const float HalfLife)
{
	return FMath::Lerp(Current, Target, DamperExactAlpha(DeltaTime, HalfLife));
}

inline float UGASPMath::RemapAngleForCounterClockwiseRotation(const float Angle)
{
	return RemapAngleForCounterClockwiseRotation<float>(Angle);
}

inline float UGASPMath::LerpAngle(const float From, const float To, const float Ratio)
{
	auto Delta{FMath::UnwindDegrees(To - From)};
	Delta = RemapAngleForCounterClockwiseRotation(Delta);

	return FMath::UnwindDegrees(From + Delta * Ratio);
}