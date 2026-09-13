#pragma once

#include "MoverDataModelTypes.h"
#include "TagTypes.h"
#include "Utils/GASPMath.h"
#include "MovementTypes.generated.h"

/**
 * Tolerances used when comparing a locally predicted input against the authority copy.
 * Values that travel over the wire are quantized, so a bit-exact comparison would
 * reconcile on every single frame. Each tolerance is at least one quantization step.
 */
namespace GASPInputTolerance
{
	/** Degrees per second. */
	inline constexpr float ControlRotationRate = 1.f;
	/** Degrees. */
	inline constexpr float RotationOffset = .5f;
	/** Centimetres; FVector_NetQuantize keeps whole centimetres. */
	inline constexpr float FloorLocation = 1.f;
	/** FVector_NetQuantizeNormal keeps 16 bits per component. */
	inline constexpr float FloorNormal = .01f;
	/** Degrees; FRotator::SerializeCompressedShort keeps 16 bits per axis (~.0055 degrees). */
	inline constexpr float AimingRotation = .5f;
}

USTRUCT(BlueprintType)
struct FGASPMoverInputs : public FCharacterDefaultInputs
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag Gait;
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag RotationMode;
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

	UPROPERTY(BlueprintReadOnly)
	FTransform RagdollTransform;

	FGASPMoverInputs()
		: Gait(GaitTags::Run)
		  , RotationMode(RotationTags::OrientToMovement)
		  , Stance(StanceTags::Standing)
		  , ControlRotationRate(0.f)
		  , RotationOffset(0.f)
		  , MovementDirection(EMovementDirection::F)
		  , FloorLocation(ForceInitToZero)
		  , FloorNormal(ForceInitToZero)
		  , AimingRotation(ForceInitToZero)
		  , RagdollTransform(FTransform::Identity)
	{
	}

	bool operator==(const FGASPMoverInputs& Other) const
	{
		return Super::operator==(Other)
			&& Gait == Other.Gait
			&& RotationMode == Other.RotationMode
			&& Stance == Other.Stance
			&& ControlRotationRate == Other.ControlRotationRate
			&& RotationOffset == Other.RotationOffset
			&& MovementDirection == Other.MovementDirection
			&& FloorLocation == Other.FloorLocation
			&& FloorNormal == Other.FloorNormal
			&& AimingRotation == Other.AimingRotation
			&& RagdollTransform.Equals(Other.RagdollTransform);
	}

	virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override;
	virtual UScriptStruct* GetScriptStruct() const override { return StaticStruct(); }
	virtual void ToString(FAnsiStringBuilderBase& Out) const override;
	virtual bool ShouldReconcile(const FMoverDataStructBase& AuthorityState) const override;
	virtual void Interpolate(const FMoverDataStructBase& From, const FMoverDataStructBase& To, float Pct) override;

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		Super::AddReferencedObjects(Collector);
	}

	virtual FMoverDataStructBase* Clone() const override
	{
		return new FGASPMoverInputs(*this);
	}
};

template <>
struct TStructOpsTypeTraits<FGASPMoverInputs> : public TStructOpsTypeTraitsBase2<FGASPMoverInputs>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};

USTRUCT(BlueprintType)
struct GASP_API FGASPInputState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag DesiredRotationMode{RotationTags::OrientToMovement};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag DesiredGait{GaitTags::Run};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag DesiredStance{StanceTags::Standing};
};

USTRUCT()
struct GASP_API FGASPMoverSyncState : public FMoverDataStructBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag Gait;
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag RotationMode;
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag Stance;

	FGASPMoverSyncState()
		: Gait(GaitTags::Run)
		  , RotationMode(RotationTags::OrientToMovement)
		  , Stance(StanceTags::Standing)
	{
	}

	bool operator==(const FGASPMoverSyncState& Other) const
	{
		return RotationMode == Other.RotationMode && Stance == Other.Stance && Gait == Other.Gait;
	}

	bool operator!=(const FGASPMoverSyncState& Other) const
	{
		return !operator==(Other);
	}

	// @return newly allocated copy of this FCharacterDefaultInputs. Must be overridden by child classes
	virtual FMoverDataStructBase* Clone() const override
	{
		return new FGASPMoverSyncState(*this);
	}

	virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override;

	virtual UScriptStruct* GetScriptStruct() const override { return StaticStruct(); }

	virtual void ToString(FAnsiStringBuilderBase& Out) const override;

	virtual bool ShouldReconcile(const FMoverDataStructBase& AuthorityState) const override;

	virtual void Interpolate(const FMoverDataStructBase& From, const FMoverDataStructBase& To, float Pct) override;

	virtual void Merge(const FMoverDataStructBase& From) override;
};

template <>
struct TStructOpsTypeTraits<FGASPMoverSyncState> : public TStructOpsTypeTraitsBase2<FGASPMoverSyncState>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};
