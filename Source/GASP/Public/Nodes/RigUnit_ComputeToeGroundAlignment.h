#pragma once

#include "Units/RigUnit.h"
#include "RigUnit_ComputeToeGroundAlignment.generated.h"

/**
 * 
 */
USTRUCT(DisplayName = "Compute Toe Ground Alignment",
	meta = (Category = "GASP|Foot Placement", Keywords = "Compute, Toe, Ground, Alignment, Floor, Normal, Impact", NodeColor = "1.0 0.36 0.0"))
struct GASP_API FRigUnit_ComputeToeGroundAlignment : public FRigUnit
{
	GENERATED_BODY()
	
public:
	UPROPERTY(meta=(Input))
	FRigElementKey ToeItem;
	UPROPERTY(meta=(Input))
	FRigElementKey FootItem;
	UPROPERTY(meta=(Input))
	FVector FloorNormal{ForceInit};
	UPROPERTY(meta=(Input))
	FVector ToeImpactPoint{ForceInit};
	
	UPROPERTY(Transient)
	FCachedRigElement CachedToeItem;
	UPROPERTY(Transient)
	FCachedRigElement CachedFootItem;
	
	UPROPERTY(meta=(Output))
	FVector Global{ForceInit};
public:
	RIGVM_METHOD()
	virtual void Execute() override;
};
