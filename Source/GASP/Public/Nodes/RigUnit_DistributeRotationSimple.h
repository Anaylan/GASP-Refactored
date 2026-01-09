#pragma once

#include "Units/RigUnit.h"
#include "RigUnit_DistributeRotationSimple.generated.h"

USTRUCT(DisplayName = "Distribute Rotation Simple", Meta = (Category = "", NodeColor = "0.0 0.36 1.0"))
struct GASP_API FRigUnit_DistributeRotationSimple : public FRigUnitMutable
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (Input))
	TArray<FRigElementKey> Items;

	UPROPERTY(meta = (Input))
	FQuat Rotation{ForceInit};

	UPROPERTY(Transient)
	TArray<FCachedRigElement> CachedItems;

public:
	RIGVM_METHOD()
	virtual void Execute() override;
};