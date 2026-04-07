#pragma once

#include "Units/RigUnit.h"
#include "RigUnit_ComputeHipTargetFromAnimatedFeet.generated.h"

USTRUCT(BlueprintType)
struct FComputeHipTarget_FootData
{
	GENERATED_BODY()

	UPROPERTY(meta=(Input))
	FRigElementKey ToeTarget;

	UPROPERTY(meta=(Input))
	FRigElementKey ToeControl;
};

/**
 * 
 */
USTRUCT(DisplayName = "Compute Hip Target From Animated Feet",
	meta = (Category = "GASP|Foot Placement", Keywords = "Compute, Hip, Pelvis, Target, Animated, Feet, Foot Placement", NodeColor = "1.0 0.36 0.0"))
struct GASP_API FRigUnit_ComputeHipTargetFromAnimatedFeet : public FRigUnit
{
	GENERATED_BODY()
	
public:
	UPROPERTY(Transient)
	TArray<FCachedRigElement> CachedToeTargets;
	UPROPERTY(Transient)
	TArray<FCachedRigElement> CachedToeControls;
	UPROPERTY(Transient)
	FCachedRigElement CachedPelvisControl;
	
	UPROPERTY(meta=(Input))
	FRigElementKey PelvisControl;
	UPROPERTY(meta=(Input))
	TArray<FComputeHipTarget_FootData> Feet{};
	
	UPROPERTY(meta=(Output))
	FVector Result{ForceInit};
	
public:
	RIGVM_METHOD()
	virtual void Execute() override;
};
