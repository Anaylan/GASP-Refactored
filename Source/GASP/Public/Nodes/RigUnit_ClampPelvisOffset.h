#pragma once

#include "Units/RigUnit.h"
#include "RigUnit_ClampPelvisOffset.generated.h"

/**
 * 
 */
USTRUCT(DisplayName = "Clamp Pelvis Offset", meta = (Category = "", Keywords = "", NodeColor = "1.0 1.0 1.0"))
struct GASP_API FRigUnit_ClampPelvisOffset : public FRigUnitMutable
{
	GENERATED_BODY()
	
public:
	
	UPROPERTY(Transient)
	FCachedRigElement CachedFootControl;
	UPROPERTY(Transient)
	FCachedRigElement CachedUpperLegBone;
	UPROPERTY(Transient)
	FCachedRigElement CachedFootBone;
	UPROPERTY(Transient)
	FCachedRigElement CachedPelvisItem;

	UPROPERTY(meta=(Input))
	FRigElementKey FootBone;
	UPROPERTY(meta=(Input))
	FRigElementKey FootControl;
	UPROPERTY(meta=(Input))
	FRigElementKey UpperLegBone;
	UPROPERTY(meta=(Input))
	FRigElementKey PelvisItem;
	
	UPROPERTY(meta=(Input))
	FVector PelvisPosition{ForceInit};
	UPROPERTY(meta=(Input))
	float LimitFactor{ForceInit};
	UPROPERTY(meta=(Input))
	float LegLength{ForceInit};
	
	UPROPERTY(meta=(Output))
	FVector ClampedPosition{ForceInit};
	
public:
	RIGVM_METHOD()
	virtual void Execute() override;
};
