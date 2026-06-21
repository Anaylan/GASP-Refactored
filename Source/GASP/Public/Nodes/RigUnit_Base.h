#pragma once

#include "RigVMFunctions/RigVMFunction_ControlFlow.h"
#include "Units/RigUnit.h"
#include "RigUnit_Base.generated.h"

/**
 * Calculates the slope angle of a surface relative to specified axes.
 * Used to determine the steepness of the terrain for gait and posture adjustments.
 */
USTRUCT(DisplayName = "Calculate Slope Angle",
	meta = (Category = "GASP|Math", Keywords = "Slope, Angle, Ground, Floor, Normal, Incline", NodeColor = "1.0 1.0 1.0"
	))
struct GASP_API FRigUnit_CalculateSlopeAngle : public FRigUnit
{
	GENERATED_BODY()

public:
	UPROPERTY(meta=(Input))
	FVector PlaneNormal{ForceInit};
	UPROPERTY(meta=(Input))
	FVector ForwardAxis{ForceInit};
	UPROPERTY(meta=(Input))
	FVector UpAxis{ForceInit};
	UPROPERTY(meta=(Input))
	FQuat Rotation{ForceInit};

	UPROPERTY(meta=(Output))
	float SlopeAngleDegrees{0.f};

public:
	RIGVM_METHOD()
	virtual void Execute() override;
};

/**
 * Computes the final transform for a pinned toe.
 * Blends animated motion with a fixed world-space pin position to prevent foot sliding.
 */
USTRUCT(DisplayName = "Compute Pinned Toe Transform",
	meta = (Category = "GASP|Foot Placement", Keywords = "Toe, Pin, Lock, Transform, Foot, Ground, IK", NodeColor =
		"1.0 0.36 0.0"))
struct GASP_API FRigUnit_ComputePinnedToeTransform : public FRigUnit
{
	GENERATED_BODY()

public:
	UPROPERTY(meta=(Input))
	FTransform ToeAnimatedTransformGlobal{FTransform::Identity};
	UPROPERTY(meta=(Input))
	FTransform ToePinTransformWorld{FTransform::Identity};

	UPROPERTY(meta=(Output))
	FTransform World{FTransform::Identity};

public:
	RIGVM_METHOD()
	virtual void Execute() override;
};

/**
 * Adjusts a position by projecting it onto a plane while accounting for an animated height offset.
 * Used for procedural foot placement on uneven terrain.
 */
USTRUCT(DisplayName = "Adjust Position To Plane",
	meta = (Category = "GASP|Math", Keywords = "Adjust, Position, Plane, Ground, Project, Height, Offset", NodeColor =
		"1.0 1.0 1.0"))
struct GASP_API FRigUnit_AdjustPositionToPlane : public FRigUnit
{
	GENERATED_BODY()

public:
	UPROPERTY(meta=(Input))
	FVector PositionOnFlat{ForceInit};
	UPROPERTY(meta=(Input))
	float AnimatedHeightOffset{ForceInit};
	UPROPERTY(meta=(Input))
	FVector PlanePoint{ForceInit};
	UPROPERTY(meta=(Input))
	FVector PlaneNormal{ForceInit};

	UPROPERTY(meta=(Output))
	FVector AdjustedPosition{ForceInit};

public:
	RIGVM_METHOD()
	virtual void Execute() override;
};

/**
 * Computes the Pole Vector (hinge direction) for a 3-bone limb chain.
 * Defines which way the knee or elbow points during IK solving.
 */
USTRUCT(DisplayName = "Get Pole Vector",
	meta = (Category = "GASP|IK", Keywords = "Pole, Vector, Knee, Elbow, Limb, Direction, IK", NodeColor =
		"0.0 0.36 1.0"))
struct GASP_API FRigUnit_GetPoleVector : public FRigUnit
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (Input))
	FRigElementKey UpperLeg;
	UPROPERTY(meta = (Input))
	FRigElementKey LowerLeg;
	UPROPERTY(meta = (Input))
	FRigElementKey Target;

	UPROPERTY(meta = (Input))
	FVector PrimaryAxis{ForceInit};
	UPROPERTY(meta = (Input))
	FVector SecondaryAxis{ForceInit};

	UPROPERTY(meta = (Output))
	FVector Result{ForceInit};

	UPROPERTY(Transient)
	FCachedRigElement CachedUpperLeg;
	UPROPERTY(Transient)
	FCachedRigElement CachedLowerLeg;
	UPROPERTY(Transient)
	FCachedRigElement CachedTarget;

public:
	RIGVM_METHOD()
	virtual void Execute() override;
};

/**
 * Clamps the minimum height of a transform relative to a reference bone.
 * Ensures that a bone cannot descend below the height of another bone (like a floor proxy).
 */
USTRUCT(DisplayName = "Clamp Transform Min Height From Reference",
	meta = (Category = "GASP|Math", Keywords = "Clamp, Height, Minimum, Floor, Reference, Transform", NodeColor =
		"1.0 1.0 1.0"))
struct GASP_API FRigUnit_ClampTransformMinHeightFromReference : public FRigUnit
{
	GENERATED_BODY()

public:
	UPROPERTY(meta=(Input))
	FTransform Transform{FTransform::Identity};
	UPROPERTY(meta=(Input))
	FRigElementKey Bone;
	UPROPERTY(Transient)
	FCachedRigElement CachedBone;

	UPROPERTY(meta=(Output))
	FTransform Global{FTransform::Identity};

public:
	RIGVM_METHOD()
	virtual void Execute() override;
};

/**
 * Limits the rotation of an item so it doesn't tilt beyond a specific slope angle.
 * Prevents extreme foot/ankle tilting on very steep inclines.
 */
USTRUCT(DisplayName = "Limit Rotation to Max Slope Angle",
	meta = (Category = "GASP|Math", Keywords = "Limit, Rotation, Clamp, Angle, Slope, Ground, Normal", NodeColor =
		"0.0 0.36 1.0"))
struct GASP_API FRigUnit_LimitRotationToMaxSlopeAngle : public FRigUnit
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (Input))
	FRigElementKey Item;
	UPROPERTY(meta = (Input))
	float AngleLimitDegrees{ForceInit};
	UPROPERTY(meta = (Output))
	FQuat Result{FQuat::Identity};

	UPROPERTY(Transient)
	FCachedRigElement CachedItem;

public:
	RIGVM_METHOD()
	virtual void Execute() override;
};

/**
 * Calculates the total length of a limb by summing the distance between joints.
 * Calculated as: distance(Upper, Mid) + distance(Mid, End).
 */
USTRUCT(DisplayName = "Calculate Limb Length",
	meta = (Category = "GASP|IK", Keywords = "Limb, Length, Distance, Bone, Chain, Leg, Arm", NodeColor = "0.0 0.36 1.0"
	))
struct GASP_API FRigUnit_CalculateLimbLength : public FRigUnit
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (Input))
	FRigElementKey UpperBone;
	UPROPERTY(meta = (Input))
	FRigElementKey MidBone;
	UPROPERTY(meta = (Input))
	FRigElementKey EndBone;

	UPROPERTY(meta = (Output))
	float Result{ForceInit};

	UPROPERTY(Transient)
	FCachedRigElement CachedUpperBone;
	UPROPERTY(Transient)
	FCachedRigElement CachedMidBone;
	UPROPERTY(Transient)
	FCachedRigElement CachedEndBone;

public:
	RIGVM_METHOD()
	virtual void Execute() override;
};

/**
 * 
 */
USTRUCT(DisplayName = "Compute Unpinning Transform",
	meta = (Category = "GASP|Foot Placement", Keywords = "Unpin, Translation, Toe, Pin, Release, Foot Placement",
		NodeColor = "1.0 0.36 0.0"))
struct GASP_API FRigUnit_ComputeUnpinningTranslation : public FRigUnit
{
	GENERATED_BODY()

public:
	UPROPERTY(meta=(Input))
	FTransform AnimatedToeTargetGlobal{FTransform::Identity};
	UPROPERTY(meta=(Input))
	FTransform ToePinAnimTransformWorld{FTransform::Identity};
	UPROPERTY(meta=(Input))
	FTransform ToePinTransformWorld{FTransform::Identity};

	UPROPERTY(meta=(Output))
	FVector Global{ForceInit};

public:
	RIGVM_METHOD()
	virtual void Execute() override;
};

/**
 * 
 */
USTRUCT(DisplayName = "Project Z Damper To Ground Plane",
	meta = (Category = "GASP|Foot Placement", Keywords = "Project, Z, Damper, Ground, Plane, Floor, Foot Placement",
		NodeColor = "1.0 0.36 0.0"))
struct GASP_API FRigUnit_ProjectZDamperToGroundPlane : public FRigUnit
{
	GENERATED_BODY()

public:
	UPROPERTY(meta=(Input))
	FVector WorldZPrev{ForceInit};
	UPROPERTY(meta=(Input))
	FVector GroundNormal{ForceInit};
	UPROPERTY(meta=(Input))
	float WorldZDamper{ForceInit};

	UPROPERTY(meta=(Output))
	float Result{ForceInit};

public:
	RIGVM_METHOD()
	virtual void Execute() override;
};

/**
 * 
 */
USTRUCT(DisplayName = "Alpha Linear Interp",
	meta = (Category = "GASP|Math", Keywords = "Alpha, Linear, Interp, Interpolate, Lerp, Blend, Scalar", NodeColor =
		"1.0 1.0 1.0"))
struct GASP_API FRigUnit_AlphaLinearInterp : public FRigUnit
{
	GENERATED_BODY()

public:
	UPROPERTY(meta=(Input))
	float Value{ForceInit};
	UPROPERTY(meta=(Input))
	float Speed{ForceInit};
	UPROPERTY(meta=(Input))
	float Target{ForceInit};

	UPROPERTY(meta=(Output))
	float Result{ForceInit};

public:
	RIGVM_METHOD()
	virtual void Execute() override;
};

/**
 * 
 */
USTRUCT(DisplayName = "Clamp Pin Yaw",
	meta = (Category = "GASP|Foot Placement", Keywords = "Clamp, Pin, Yaw, Rotation, Foot Placement, Lock", NodeColor =
		"1.0 0.36 0.0"))
struct GASP_API FRigUnit_ClampPinYaw : public FRigUnit
{
	GENERATED_BODY()

public:
	UPROPERTY(meta=(Input))
	FTransform UpdatedPinTransform{FTransform::Identity};
	UPROPERTY(meta=(Input))
	FQuat Rotation{ForceInit};
	UPROPERTY(meta=(Input))
	float PinYawLimitDegrees{ForceInit};

	UPROPERTY(meta=(Output))
	FQuat World{ForceInit};

public:
	RIGVM_METHOD()
	virtual void Execute() override;
};

/**
 *
 */
USTRUCT(DisplayName = "Clamp Pin Distance",
	meta = (Category = "GASP|Foot Placement", Keywords = "Clamp, Pin, Distance, Translation, Foot Placement, Lock",
		NodeColor = "1.0 0.36 0.0"))
struct GASP_API FRigUnit_ClampPinDistance : public FRigUnit
{
	GENERATED_BODY()

public:
	UPROPERTY(meta=(Input))
	FTransform UpdatedPinTransform{FTransform::Identity};
	UPROPERTY(meta=(Input))
	FVector Translation{ForceInit};
	UPROPERTY(meta=(Input))
	float MaxFootPinDistance{ForceInit};

	UPROPERTY(meta=(Output))
	FVector World{ForceInit};

public:
	RIGVM_METHOD()
	virtual void Execute() override;
};

/**
 * 
 */
USTRUCT(DisplayName = "Is Game World", DocumentationPolicy = "None",
	meta = (Category = "GASP|Utility", Keywords = "Is, Game, World, Editor, Preview, PIE", NodeColor = "1.0 1.0 1.0"))
struct GASP_API FRigVMFunction_IsGameWorld : public FRigVMFunction_ControlFlowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, DisplayName = "Execute", Meta = (Input))
	FRigVMExecuteContext ExecuteContext;

	UPROPERTY(Transient, Meta = (Output))
	FRigVMExecuteContext True;

	UPROPERTY(Transient, Meta = (Output))
	FRigVMExecuteContext False;

	UPROPERTY(meta=(Output))
	FRigVMExecuteContext Completed;

	UPROPERTY(Transient, Meta = (Singleton))
	FName BlockToRun;

public:
	RIGVM_METHOD()
	virtual void Execute() override;

#if WITH_EDITORONLY_DATA
	virtual const TArray<FName>& GetControlFlowBlocks_Impl() const override;
#endif
};
