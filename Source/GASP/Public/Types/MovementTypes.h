#pragma once

#include "MoverDataModelTypes.h"
#include "TagTypes.h"
#include "Curves/CurveVector.h"
#include "Utils/GASPMath.h"
#include "MovementTypes.generated.h"

namespace MovementModeNames
{
	const FName Sliding = TEXT("Sliding");
}

/**
 *
 */
USTRUCT(BlueprintType)
struct GASP_API FGaitSettings
{
	GENERATED_BODY()

	float GetSpeed(const FGameplayTag& Gait, const FVector& Velocity,
	               const FRotator& ActorRotation) const
	{
		const auto SpeedRange = SpeedMap.Contains(Gait) ? SpeedMap.FindRef(Gait) : FVector::ZeroVector;

		return UE_REAL_TO_FLOAT(InterpolateSpeedForDirection(SpeedRange, Velocity, ActorRotation));
	}

	UCurveVector* GetMovementCurve() const
	{
		return MovementCurve.Get();
	}

	float InterpolateSpeedForDirection(const FVector& SpeedRange, const FVector& Velocity,
	                                   const FRotator& ActorRotation) const
	{
		const float Dir{FGASPMath::CalculateDirection(Velocity, ActorRotation)};
		const float StrafeSpeedMap{IsValid(StrafeCurve) ? StrafeCurve->GetFloatValue(FMath::Abs(Dir)) : 0.f};

		if (StrafeSpeedMap < 1.f)
		{
			return FMath::Lerp(SpeedRange.X, SpeedRange.Y, StrafeSpeedMap);
		}

		return FMath::Lerp(SpeedRange.Y, SpeedRange.Z, StrafeSpeedMap - 1.f);
	}

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly,
		meta = (Description = "X = Forward Speed, Y = Strafe Speed, Z = Backwards Speed"))
	TMap<FGameplayTag, FVector> SpeedMap{
		{GaitTags::Walk, {200.f, 180.f, 150.f}},
		{GaitTags::Run, {450.f, 400.f, 350.f}},
		{GaitTags::Sprint, {700.f, 0.f, 0.f}}
	};

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UCurveFloat> StrafeCurve{};
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UCurveVector> MovementCurve{};
};

USTRUCT(BlueprintType)
struct FGASPMoverInputs : public FCharacterDefaultInputs
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag RotationMode;

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag Gait;

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag Stance;

	UPROPERTY(BlueprintReadOnly)
	float ControlRotationRate;

	UPROPERTY(BlueprintReadOnly)
	float RotationOffset;

	UPROPERTY(BlueprintReadOnly)
	EMovementDirection MovementDirection;

	UPROPERTY(BlueprintReadOnly)
	FVector_NetQuantize FloorLocation;
	UPROPERTY(BlueprintReadOnly)
	FVector_NetQuantizeNormal FloorNormal;
	UPROPERTY(BlueprintReadOnly)
	FRotator AimingRotation;

	FGASPMoverInputs()
		: RotationMode(RotationTags::OrientToMovement)
		  , Gait(GaitTags::Walk)
		  , Stance(StanceTags::Standing)
		  , ControlRotationRate(ForceInitToZero)
		  , RotationOffset(ForceInitToZero)
		  , MovementDirection(EMovementDirection::F)
		  , FloorLocation(ForceInitToZero)
		  , FloorNormal(ForceInitToZero)
		  , AimingRotation(ForceInitToZero)
	{
	}

	virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override;
	virtual UScriptStruct* GetScriptStruct() const override { return StaticStruct(); }
	virtual void ToString(FAnsiStringBuilderBase& Out) const override;

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		Super::AddReferencedObjects(Collector);
	}

	virtual FMoverDataStructBase* Clone() const override
	{
		return new FGASPMoverInputs(*this);
	}
};

USTRUCT(BlueprintType)
struct GASP_API FGASPInputState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag DesiredRotationMode{RotationTags::OrientToMovement};

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag DesiredGait{GaitTags::Walk};

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag DesiredStance{StanceTags::Standing};
};
