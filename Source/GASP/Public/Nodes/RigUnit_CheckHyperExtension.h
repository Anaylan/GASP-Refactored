#pragma once

#include "Units/RigUnit.h"
#include "RigUnit_CheckHyperExtension.generated.h"

/**
 * 
 */
USTRUCT(DisplayName = "Check Hyper Extension",
	meta = (Category = "GASP|IK", Keywords = "Check, Hyper Extension, Hyperextension, Leg, Clamp, Stretch, IK", NodeColor = "0.0 0.36 1.0"))
struct GASP_API FRigUnit_CheckHyperExtension : public FRigUnit
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient)
	FCachedRigElement CachedChildControl;
	UPROPERTY(Transient)
	FCachedRigElement CachedParent;
	UPROPERTY(Transient)
	FCachedRigElement CachedChildBone;

	UPROPERTY(meta=(Input))
	FRigElementKey ChildBone;
	UPROPERTY(meta=(Input))
	FRigElementKey ChildControl;
	UPROPERTY(meta=(Input))
	FRigElementKey Parent;

	UPROPERTY(meta=(Input))
	float LegLength{ForceInit};
	UPROPERTY(meta=(Input))
	float LimitFactor{ForceInit};

	UPROPERTY(meta=(Output))
	bool OutResult{false};
	UPROPERTY(meta=(Output))
	FTransform Global{FTransform::Identity};

public:
	RIGVM_METHOD()
	virtual void Execute() override;
};
