#pragma once

#include "Units/RigUnit.h"
#include "RigUnit_CalculateTargetAnimatedToeTransformGlobal.generated.h"

USTRUCT(DisplayName = "Calculate Target Animated Toe Transform Global",
	meta = (Category = "GASP|Foot Placement", Keywords = "Calculate, Toe, Animated, Target, Global, Foot, IK", NodeColor = "1.0 0.36 0.0"))
struct GASP_API FRigUnit_CalculateTargetAnimatedToeTransformGlobal : public FRigUnit
{
	GENERATED_BODY()

public:
	UPROPERTY(meta=(Input))
	FRigElementKey FootBone;
	UPROPERTY(meta=(Input))
	FRigElementKey FootExternalIKTargetBoneBone;
	UPROPERTY(meta=(Input))
	FRigElementKey ToeBone;

	UPROPERTY(Transient)
	FCachedRigElement CachedFootBone;
	UPROPERTY(Transient)
	FCachedRigElement CachedFootExternalIKTargetBoneBone;
	UPROPERTY(Transient)
	FCachedRigElement CachedToeBone;
	
	UPROPERTY(meta=(Output))
	FTransform Global{FTransform::Identity};

public:
	RIGVM_METHOD()
	virtual void Execute() override;
};
