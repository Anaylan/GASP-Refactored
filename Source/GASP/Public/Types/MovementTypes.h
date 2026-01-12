#pragma once

#include "MoverDataModelTypes.h"
#include "TagTypes.h"
#include "Utils/GASPMath.h"
#include "MovementTypes.generated.h"

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

	FGASPMoverInputs()
		: Gait(GaitTags::Run)
		  , RotationMode(RotationTags::OrientToMovement)
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag DesiredRotationMode{RotationTags::OrientToMovement};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag DesiredGait{GaitTags::Run};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag DesiredStance{StanceTags::Standing};
};
