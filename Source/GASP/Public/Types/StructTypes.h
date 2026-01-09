#pragma once

#include "GameplayTags.h"
#include "StructTypes.generated.h"

template <typename T>
struct TStateWrapper
{
	T Current{};
	T LastFrame{};
	T Recent{};

	float TimeInState{0.0f};
	float LastStateTime{0.0f};

	explicit TStateWrapper(T DefaultState)
	{
		Current = LastFrame = Recent = DefaultState;
	}

	void Update(const T& NewState, const float DeltaTime, const float RecentLimit)
	{
		LastFrame = Current;
		Current = NewState;

		if (Current != LastFrame)
		{
			LastStateTime = TimeInState;
			TimeInState = 0.0f;
		}
		else
		{
			TimeInState += DeltaTime;
			if (TimeInState >= RecentLimit)
			{
				Recent = Current;
			}
		}
	}
};

USTRUCT(BlueprintType)
struct FAnimConfiguration
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Configuration")
	float MaxTurnInPlaceAngle{50.f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Configuration")
	float SpinTransitionAngle{130.f};
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Configuration")
	float TeleportThreshold{50.f};
};

/**
 *
 */
USTRUCT(BlueprintType)
struct GASP_API FCharacterInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Configuration")
	float FlailRate{0.f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Meta = (ClampMin = 0, ForceUnits = "s"))
	float Speed{0.f};
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float AngleVelocity{0.f};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Information")
	FVector Velocity{FVector::ZeroVector};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Information")
	FVector Acceleration{FVector::ZeroVector};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Information")
	FVector VelocityAcceleration{FVector::ZeroVector};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Information")
	FTransform RootTransform{FTransform::Identity};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Information")
	FTransform ActorTransform{FTransform::Identity};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Information")
	FVector SmoothedGroundNormal{FVector::ZeroVector};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Information")
	FVector RelativeAcceleration{FVector::ZeroVector};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Information")
	FVector FloorLocation{FVector::ZeroVector};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Information")
	FVector FloorNormal{FVector::ZeroVector};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Information")
	FRotator OrientationIntent{FRotator::ZeroRotator};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Information")
	FRotator AimingRotation{FRotator::ZeroRotator};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Information")
	FRotator RootOffsetRotation{FRotator::ZeroRotator};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Information")
	FRotator ViewRotation;
};

/**
 *
 */
USTRUCT(BlueprintType)
struct GASP_API FGASPSpineState
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "")
	uint8 bSpineRotationAllowed : 1 {false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "", meta = (ClampMin = 0, ClampMax = 1))
	float SpineAmount{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "", meta = (ClampMin = 0, ForceUnits = "x"))
	float SpineAmountScale{1.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "")
	float SpineAmountBias{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "", meta = (ClampMin = -180, ClampMax = 180, ForceUnits = "deg"))
	float LastYawAngle{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "", meta = (ClampMin = -180, ClampMax = 180, ForceUnits = "deg"))
	float CurrentYawAngle{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "", meta = (ClampMin = -180, ClampMax = 180, ForceUnits = "deg"))
	float YawAngle{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "", meta = (ClampMin = -180, ClampMax = 180, ForceUnits = "deg"))
	float LastActorYawAngle{0.0f};
};

USTRUCT(BlueprintType)
struct GASP_API FTrajectoryInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FVector FutureVelocity{FVector::ZeroVector};
	UPROPERTY(BlueprintReadOnly)
	FVector PreviousVelocity{FVector::ZeroVector};
	UPROPERTY(BlueprintReadOnly)
	FVector PreviousFutureVelocity{FVector::ZeroVector};
	UPROPERTY(BlueprintReadOnly)
	FVector NearFutureVelocity{FVector::ZeroVector};
	UPROPERTY(BlueprintReadOnly)
	FVector PastAngularVelocity{FVector::ZeroVector};
	UPROPERTY(BlueprintReadOnly)
	FVector CurrentAngularVelocity{FVector::ZeroVector};

	UPROPERTY(BlueprintReadOnly)
	float PreviousFutureFacingDelta{0.f};
	UPROPERTY(BlueprintReadOnly)
	float FutureFacingDelta{0.f};
	UPROPERTY(BlueprintReadOnly)
	float CirclingTime{0.f};

	UPROPERTY(BlueprintReadOnly)
	uint8 bIsCircling : 1{false};

	UPROPERTY(BlueprintReadOnly)
	FRotator FutureFacing{FRotator::ZeroRotator};
	UPROPERTY(BlueprintReadOnly)
	FRotator FutureFacingOnTransitionStart{FRotator::ZeroRotator};
};

/**
 *
 */
USTRUCT(BlueprintType)
struct GASP_API FMotionMatchingInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<const class UPoseSearchDatabase> PoseSearchDatabase{};
	UPROPERTY(BlueprintReadOnly)
	TArray<TObjectPtr<UObject>> Databases{};
	UPROPERTY(BlueprintReadOnly)
	FVector LastNonZeroVector{FVector::ZeroVector};
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FName> DatabaseTags{};
	UPROPERTY(BlueprintReadOnly, meta = (ClampMin = 0))
	float OrientationAlpha{.2f};
	UPROPERTY(BlueprintReadOnly, meta = (ClampMin = 0))
	float ProceduralTargetTime{.2f};
	UPROPERTY(BlueprintReadOnly, meta = (ClampMin = 0))
	float DesiredFacingTime{.5f};
	UPROPERTY(BlueprintReadOnly)
	float PreviousDesiredYawRotation{0.f};
	UPROPERTY(BlueprintReadOnly, meta = (ClampMin = 0))
	float AnimTime{0.f};
	UPROPERTY(BlueprintReadOnly, meta = (ClampMin = 0))
	float PlayRate{0.f};
	UPROPERTY(BlueprintReadOnly, meta = (ClampMin = 0))
	float StrafeWarpAlpha{0.f};
	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<class UAnimationAsset> AnimAsset;
	UPROPERTY(BlueprintReadOnly)
	uint8 bActive : 1{false};
};

/**
 *
 */
USTRUCT(BlueprintType)
struct GASP_API FAnimUtilityNames
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName MovingTraversalCurveName{TEXT("MovingTraversal")};
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName EnableWarpingCurveName{TEXT("Enable_Warping")};
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName EnableStrafeWarpingName{TEXT("Enable_StrafeWarping")};
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName SteeringTargetTime{TEXT("SteeringTargetTime")};
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName DisableAOCurveName{TEXT("Disable_AO")};
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName StopsTag{TEXT("Stops")};
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName TurnInPlaceTag{TEXT("TurnInPlace")};
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName PivotsTag{TEXT("Pivots")};
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName PoseHistoryTag{TEXT("PoseHistory")};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layering|Names")
	FName LayeringLegsSlotName{TEXT("Layering_Legs")};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layering|Names")
	FName LayeringPelvisSlotName{TEXT("Layering_Pelvis")};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layering|Names")
	FName LayeringHeadSlotName{TEXT("Layering_Head")};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layering|Names")
	FName LayeringSpineName{TEXT("Layering_Spine")};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layering|Names")
	FName LayeringSpineAdditiveName{TEXT("Layering_Spine_Add")};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layering|Names")
	FName LayeringHeadAdditiveName{TEXT("Layering_Head_Add")};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layering|Names")
	FName LayeringArmLeftAdditiveName{TEXT("Layering_Arm_L_Add")};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layering|Names")
	FName LayeringArmRightAdditiveName{TEXT("Layering_Arm_R_Add")};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layering|Names")
	FName LayeringHandLeftName{TEXT("Layering_Hand_L")};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layering|Names")
	FName LayeringHandRightName{TEXT("Layering_Hand_R")};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layering|Names")
	FName LayeringArmLeftName{TEXT("Layering_Arm_L")};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layering|Names")
	FName LayeringArmRightName{TEXT("Layering_Arm_R")};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layering|Names")
	FName LayeringHandLeftIKName{TEXT("Enable_HandIK_L")};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layering|Names")
	FName LayeringHandRightIKName{TEXT("Enable_HandIK_R")};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layering|Names")
	FName LayeringArmLeftLocalSpaceName{TEXT("Layering_Arm_L_LS")};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layering|Names")
	FName LayeringArmRightLocalSpaceName{TEXT("Layering_Arm_R_LS")};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layering|Names")
	FName EnableSpineRotationName{TEXT("Enable_SpineRotation")};
};

USTRUCT(BlueprintType)
struct GASP_API FGASPControlRigInput
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "", Meta = (ClampMin = -180, ClampMax = 180, ForceUnits = "deg"))
	float SpineYawAngle{0.0f};
};

USTRUCT(BlueprintType)
struct GASP_API FOverlaySettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TWeakObjectPtr<UAnimInstance> OverlayInstance;
};

USTRUCT(BlueprintType)
struct GASP_API FLayeringState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layering|Values", Meta = (ClampMin = 0, ClampMax = 1))
	float HeadAdditiveBlendAmount{0.0f};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layering|Values", Meta = (ClampMin = 0, ClampMax = 1))
	float ArmLeftAdditiveBlendAmount{0.0f};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layering|Values", Meta = (ClampMin = 0, ClampMax = 1))
	float ArmLeftLocalSpaceBlendAmount{0.0f};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layering|Values", Meta = (ClampMin = 0, ClampMax = 1))
	float ArmLeftMeshSpaceBlendAmount{0.0f};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layering|Values", Meta = (ClampMin = 0, ClampMax = 1))
	float ArmRightAdditiveBlendAmount{0.0f};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layering|Values", Meta = (ClampMin = 0, ClampMax = 1))
	float ArmRightLocalSpaceBlendAmount{0.0f};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layering|Values", Meta = (ClampMin = 0, ClampMax = 1))
	float ArmRightMeshSpaceBlendAmount{0.0f};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layering|Values", Meta = (ClampMin = 0, ClampMax = 1))
	float HandLeftBlendAmount{0.0f};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layering|Values", Meta = (ClampMin = 0, ClampMax = 1))
	float HandRightBlendAmount{0.0f};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layering|Values", Meta = (ClampMin = 0, ClampMax = 1))
	float EnableHandLeftIKBlend{0.0f};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layering|Values", Meta = (ClampMin = 0, ClampMax = 1))
	float EnableHandRightIKBlend{0.0f};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layering|Values", Meta = (ClampMin = 0, ClampMax = 1))
	float SpineAdditiveBlendAmount{0.0f};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layering|Values", Meta = (ClampMin = 0, ClampMax = 1))
	float PelvisBlendAmount{0.0f};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layering|Values", Meta = (ClampMin = 0, ClampMax = 1))
	float LegsBlendAmount{0.0f};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layering|Values", Meta = (ClampMin = 0, ClampMax = 1))
	float SpineEnableRotationAmount{0.0f};
};

USTRUCT(BlueprintType)
struct GASP_API FRagdollingState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "State|Character")
	FVector Velocity{ForceInit};

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "State|Character", meta = (ForceUnits = "N"))
	float PullForce{0.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State|Character", meta = (ClampMin = 0))
	int32 SpeedLimitFrameTimeRemaining{0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State|Character",
		meta = (ClampMin = 0, ForceUnits = "cm/s"))
	float SpeedLimit{0.0f};
};

USTRUCT(BlueprintType)
struct GASP_API FRagdollingAnimationState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP")
	FPoseSnapshot FinalRagdollPose;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP", meta = (ClampMin = 0, ClampMax = 1, ForceUnits = "x"))
	float FlailPlayRate{1.0f};
};


USTRUCT(BlueprintType, meta=(ToolTip = "Struct used in State Machine to drive Blend Stack inputs"))
struct GASP_API FGASPBlendStackInputs
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP")
	TWeakObjectPtr<class UAnimationAsset> AnimationAsset{};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP")
	bool bLoop{false};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP", meta = (ClampMin = 0))
	float StartTime{.0f};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP", meta = (ClampMin = 0))
	float BlendTime{.0f};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP")
	TWeakObjectPtr<const class UBlendProfile> BlendProfile{};
};

USTRUCT(BlueprintType)
struct GASP_API FGASPChooserOutputs
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP", meta = (ClampMin = 0))
	float StartTime{.0f};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP", meta = (ClampMin = 0))
	float BlendTime{.3f};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP")
	bool bUseMotionMatching{false};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP", meta = (ClampMin = 0))
	float MMCostLimit{.0f};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP")
	FName BlendProfile{NAME_None};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP")
	TArray<FName> Tags;
};

USTRUCT(BlueprintType)
struct GASP_API FGASPTraversalCheckInputs
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP")
	FVector TraceForwardDirection{FVector::ZeroVector};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP")
	float TraceForwardDistance{.0f};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP")
	FVector TraceOriginOffset{FVector::ZeroVector};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP")
	FVector TraceEndOffset{FVector::ZeroVector};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP")
	float TraceRadius{.0f};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP")
	float TraceHalfHeight{.0f};
};

USTRUCT(BlueprintType)
struct GASP_API FGASPBlendPoses
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP", meta = (ClampMin = 0))
	float BasePoseN{.0f};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP", meta = (ClampMin = 0))
	float BasePoseCLF{.0f};
};

USTRUCT(BlueprintType)
struct GASP_API FTraversalCheckResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	FGameplayTag ActionType{FGameplayTag::EmptyTag};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	uint8 bHasFrontLedge : 1{false};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	FVector FrontLedgeLocation{FVector::ZeroVector};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	FVector FrontLedgeNormal{FVector::ZeroVector};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	uint8 bHasBackLedge : 1{false};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	FVector BackLedgeLocation{FVector::ZeroVector};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	FVector BackLedgeNormal{FVector::ZeroVector};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	uint8 bHasBackFloor : 1{false};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	FVector BackFloorLocation{FVector::ZeroVector};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	float ObstacleHeight{0.f};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	float ObstacleDepth{0.f};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	float BackLedgeHeight{0.f};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	TObjectPtr<UPrimitiveComponent> HitComponent{};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	TObjectPtr<const UAnimMontage> ChosenMontage{};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal", meta = (ClampMin = 0))
	float StartTime{0.f};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal", meta = (ClampMin = 0))
	float PlayRate{0.f};

	FORCEINLINE FString ToString() const
	{
		return FString::Printf(
			TEXT(
				"Has Front Ledge: %hhd\nHas Back Ledge: %hhd\nHas Back Floor:%hhd\nObstacle Height: %f\nObstacle Depth: "
				"%f\nBack Ledge Height: %f\nChosen Animation: %s\nAnimation Start Time: %f\nAnimation Play Rate: %f"),
			bHasFrontLedge, bHasBackLedge, bHasBackFloor, ObstacleHeight, ObstacleDepth, BackLedgeHeight,
			IsValid(ChosenMontage) ? *ChosenMontage->GetName() : TEXT("nullptr"), StartTime, PlayRate);
	}
};

USTRUCT(BlueprintType)
struct GASP_API FBlendStackMachine
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP")
	bool bLoop{false};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP", meta = (ClampMin = 0))
	float AssetTimeRemaining{.0f};
};

USTRUCT(BlueprintType)
struct GASP_API FPivotSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Settings")
	FVector2D SpeedRange{FVector2D::ZeroVector};

	UPROPERTY(EditAnywhere, Category = "Settings",
		meta = (Description = "X = Minimum Speed, Y = Maximum Speed, Z = Minimum Angle, W = Maximum Angle"))
	FVector4f RangeParams{FVector4f::Zero()};
};
