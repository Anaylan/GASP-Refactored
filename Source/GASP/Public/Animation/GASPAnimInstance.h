#pragma once

#include "Animation/AnimInstance.h"
#include "Animation/GASPAnimInstanceProxy.h"
#include "Animation/AnimExecutionContext.h"
#include "Animation/AnimNodeReference.h"
#include "BoneControllers/AnimNode_FootPlacement.h"
#include "BoneControllers/AnimNode_OffsetRootBone.h"
#include "BoneControllers/AnimNode_OrientationWarping.h"
#include "PoseSearch/PoseSearchLibrary.h"
#include "PoseSearch/PoseSearchTrajectoryLibrary.h"
#include "Types/EnumTypes.h"
#include "Types/StructTypes.h"
#include "Types/TagTypes.h"
#include "GASPAnimInstance.generated.h"

class UMoverTrajectoryPredictor;

/**
 *
 */
UCLASS()
class GASP_API UGASPAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

	FTimerHandle LandedHandle;

protected:
	/**************
	 * References *
	 *************/
	UPROPERTY(BlueprintReadOnly, Category = "References", Transient)
	TWeakObjectPtr<class AGASPCharacter> CachedCharacter{};

	UPROPERTY(BlueprintReadOnly, Category = "References", Transient)
	TWeakObjectPtr<class UGASPMoverComponent> CachedMovement{};
	
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UMoverTrajectoryPredictor> Predictor;
	
	/******************
	 * Character state *
	 ******************/
	UPROPERTY(EditAnywhere, Category = "CharacterInformation|General", BlueprintReadOnly, Transient)
	FAnimUtilityNames AnimNames{};

	UPROPERTY(EditAnywhere, Category = "CharacterInformation|General", BlueprintReadOnly, Transient)
	EMovementDirection MovementDirection{EMovementDirection::F};
	UPROPERTY(EditAnywhere, Category = "LocomotionAction", BlueprintReadOnly, Transient)
	FGameplayTag LocomotionAction{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, Category = "CharacterInfromation|States", BlueprintReadOnly, Transient)
	FGameplayTag Gait{};
	UPROPERTY(EditAnywhere, Category = "CharacterInfromation|States", BlueprintReadOnly, Transient)
	FGameplayTag MovementState{};
	UPROPERTY(EditAnywhere, Category = "CharacterInfromation|States", BlueprintReadOnly, Transient)
	FGameplayTag RotationMode{};
	UPROPERTY(EditAnywhere, Category = "CharacterInfromation|States", BlueprintReadOnly, Transient)
	FGameplayTag MovementMode{};
	UPROPERTY(EditAnywhere, Category = "CharacterInfromation|States", BlueprintReadOnly, Transient)
	FGameplayTag StanceMode{};
	UPROPERTY(EditAnywhere, Category = "CharacterInfromation|States", BlueprintReadOnly, Transient)
	FGameplayTagContainer StateContainer{};
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CharacterInformation|General", Transient)
	FCharacterInfo CharacterInfo{};
	UPROPERTY(BlueprintReadOnly, Category = "CharacterInformation|General")
	FGASPBlendStackInputs BlendStackInputs{};
	UPROPERTY(BlueprintReadOnly, Category = "MotionMatching")
	FMotionMatchingInfo BlendStack;
	UPROPERTY(BlueprintReadOnly, Category = "LocomotionAction|Information", Transient)
	FRagdollingAnimationState RagdollingState;
	UPROPERTY(EditAnywhere, Category = "Additive|Poses", BlueprintReadOnly, Transient)
	FGASPBlendPoses BlendPoses;
	UPROPERTY(BlueprintReadOnly, Category = "Additive|Poses", Transient)
	FBlendStackMachine BlendStackMachine;

	UPROPERTY(BlueprintReadOnly, Category = "MotionMatching")
	FTrajectoryInfo TrajectoryInfo;
	
	UPROPERTY(BlueprintReadOnly, Category = "CharacterInformation|PreviousValues", Transient)
	FGameplayTag PreviousStanceMode{FGameplayTag::EmptyTag};
	UPROPERTY(BlueprintReadOnly, Category = "CharacterInformation|PreviousValues", Transient)
	FGameplayTag PreviousGait{FGameplayTag::EmptyTag};
	UPROPERTY(BlueprintReadOnly, Category = "CharacterInformation|PreviousValues", Transient)
	FGameplayTag PreviousMovementState{FGameplayTag::EmptyTag};
	UPROPERTY(BlueprintReadOnly, Category = "CharacterInformation|PreviousValues", Transient)
	FGameplayTag PreviousRotationMode{FGameplayTag::EmptyTag};
	UPROPERTY(BlueprintReadOnly, Category = "CharacterInformation|PreviousValues", Transient)
	FGameplayTag PreviousMovementMode{MovementModeTags::Grounded};
	UPROPERTY(BlueprintReadOnly, Category = "CharacterInformation|PreviousValues", Transient)
	FCharacterInfo PreviousCharacterInfo;
	UPROPERTY(BlueprintReadOnly, Category = "CharacterInformation|PreviousValues", Transient)
	FGameplayTag PreviousLocomotionAction{FGameplayTag::EmptyTag};
	UPROPERTY(BlueprintReadOnly, Category = "CharacterInformation|PreviousValues", Transient)
	FGameplayTagContainer PreviousStateContainer{};
	
	UPROPERTY(EditAnywhere, Category="PoseSearchData|Choosers", BlueprintReadOnly)
	TObjectPtr<class UChooserTable> LocomotionTable{nullptr};
	UPROPERTY(EditAnywhere, Category="PoseSearchData|Choosers", BlueprintReadOnly)
	TObjectPtr<UChooserTable> StateMachineTable{nullptr};

	UPROPERTY(EditAnywhere, Category="PoseSearchData|Trajectory", BlueprintReadOnly, Transient)
	FTransformTrajectory Trajectory{};
	
	UPROPERTY(EditAnywhere, Category="Movement|FootPlacement", BlueprintReadOnly)
	FFootPlacementPlantSettings PlantSettings_Default{};
	UPROPERTY(EditAnywhere, Category="Movement|FootPlacement", BlueprintReadOnly)
	FFootPlacementPlantSettings PlantSettings_Stops{};
	UPROPERTY(EditAnywhere, Category="Movement|FootPlacement", BlueprintReadOnly)
	FFootPlacementInterpolationSettings InterpolationSettings_Default{};
	UPROPERTY(EditAnywhere, Category="Movement|FootPlacement", BlueprintReadOnly)
	FFootPlacementInterpolationSettings InterpolationSettings_Stops{};
	UPROPERTY(BlueprintReadOnly, Category="Additive", Transient)
	FRotator SpineRotation{FRotator::ZeroRotator};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="HeldObject", Transient)
	FVector RightHandOffset{FVector::ZeroVector};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="HeldObject", Transient)
	FVector LeftHandOffset{FVector::ZeroVector};

public:
	UFUNCTION(BlueprintPure, Category="Movement|Analys", meta = (BlueprintThreadSafe))
	bool IsStarting() const;
	UFUNCTION(BlueprintPure, Category="Movement|Analys", meta = (BlueprintThreadSafe))
	bool IsPivoting() const;
	UFUNCTION(BlueprintPure, Category="Movement|Analys", meta = (BlueprintThreadSafe))
	bool IsMoving() const;
	UFUNCTION(BlueprintPure, Category="Movement|Analys", meta = (BlueprintThreadSafe))
	bool ShouldTurnInPlace() const;
	UFUNCTION(BlueprintPure, Category="Movement|Analys", meta = (BlueprintThreadSafe))
	bool ShouldSpin() const;
	UFUNCTION(BlueprintPure, Category="Movement|Analys", meta = (BlueprintThreadSafe))
	bool JustLanded_Light() const;
	UFUNCTION(BlueprintPure, Category="Movement|Analys", meta = (BlueprintThreadSafe))
	bool JustLanded_Heavy() const;
	UFUNCTION(BlueprintPure, Category="Movement|Analys", meta = (BlueprintThreadSafe))
	bool JustTraversed() const;
	UFUNCTION(BlueprintPure, Category="Movement|Analys", meta = (BlueprintThreadSafe))
	bool PlayLand() const;
	UFUNCTION(BlueprintPure, Category="Movement|Analys", meta = (BlueprintThreadSafe))
	bool PlayMovingLand() const;
	UFUNCTION(BlueprintPure, Category="Movement|Analys", meta = (BlueprintThreadSafe))
	float GetTrajectoryTurnAngle() const;
	UFUNCTION(BlueprintPure, Category = "Movement|Analys", meta = (BlueprintThreadSafe))
	bool IsEnableSteering() const;

	UFUNCTION(BlueprintPure, Category="Movement|Lean", meta = (BlueprintThreadSafe))
	FVector2D GetLeanAmount() const;
	UFUNCTION(BlueprintPure, Category="Movement|Lean", meta = (BlueprintThreadSafe))
	FVector GetRelativeAcceleration() const;
	UFUNCTION(BlueprintPure, Category="BlendStack", meta = (BlueprintThreadSafe))
	FQuat GetDesiredFacing() const;

protected:
	UFUNCTION(BlueprintCallable, Category = "Runtime", meta = (BlueprintThreadSafe))
	void RefreshMotionMatchingMovement(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	UFUNCTION(BlueprintCallable, Category = "Runtime", meta = (BlueprintThreadSafe))
	void RefreshMatchingPostSelection(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	UFUNCTION(BlueprintCallable, Category = "Runtime", meta = (BlueprintThreadSafe))
	void RefreshOffsetRoot(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	UFUNCTION(BlueprintCallable, Category = "Runtime", meta = (BlueprintThreadSafe))
	void RefreshBlendStack(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	UFUNCTION(BlueprintCallable, Category = "Runtime", meta = (BlueprintThreadSafe))
	void RefreshBlendStackMachine(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	UFUNCTION(BlueprintCallable, Category = "Runtime", meta = (BlueprintThreadSafe))
	void RefreshEssentialValues(const float DeltaSeconds);
	UFUNCTION(BlueprintCallable, Category = "Runtime", meta = (BlueprintThreadSafe))
	void RefreshRagdollValues(const float DeltaSeconds);
	UFUNCTION(BlueprintCallable, Category = "Runtime", meta = (BlueprintThreadSafe))
	void RefreshTrajectory(float DeltaSeconds);

	UFUNCTION(BlueprintPure)
	[[maybe_unused]] float GetTotalFacingDelta(TArray<float> Times) const;
	
	/**************
	 * Aim Offsets *
	 **************/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MovementInformation|General")
	float HeavyLandSpeedThreshold{700.f};;
	UPROPERTY(BlueprintReadOnly, Category = "MovementInformation|General")
	int32 MMDatabaseLOD{0};
	UPROPERTY(BlueprintReadOnly, Category = "MovementInformation|General")
	int32 LocomotionSetup{0};
	UPROPERTY(BlueprintReadOnly, Category = "MovementInformation|General")
	uint8 bOffsetRootBoneEnabled : 1 {true};
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "StateMachine|Configuration")
	TMap<FGameplayTag, FPivotSettings> PivotSettings{
		{GaitTags::Walk, {{50.f, 200.f}, FVector4f(150.f, 200.f, 70.f, 60.f)}},
		{GaitTags::Run, {{200.f, 550.f}, FVector4f(300.f, 500.f, 70.f, 60.f)}},
		{GaitTags::Sprint, {{200.f, 700.f}, FVector4f(300.f, 700.f, 60.f, 50.f)}}
	};

public:
	UFUNCTION(BlueprintPure, Category = "AimOffset", meta = (BlueprintThreadSafe))
	bool IsEnabledAO() const;
	UFUNCTION(BlueprintPure, Category = "AimOffset", meta = (BlueprintThreadSafe))
	FVector2D GetAOValue() const;
	UFUNCTION(BlueprintPure, Category = "AimOffset", meta = (BlueprintThreadSafe))
	bool IsCircling() const;
	UFUNCTION(BlueprintPure, Category = "Overlay", meta = (BlueprintThreadSafe))
	bool CanOverlayTransition() const;

	UFUNCTION(BlueprintImplementableEvent)
	void OnOverlayModeChanged(FGameplayTag OldOverlayMode, FGameplayTag NewGameplayTag);
	UFUNCTION(BlueprintImplementableEvent)
	void OnPoseModeChanged(FGameplayTag OldGameplayTag, FGameplayTag NewGameplayTag);

	UFUNCTION(BlueprintPure, Category = "BlendStack", meta = (BlueprintThreadSafe))
	EPoseSearchInterruptMode GetMatchingInterruptMode() const;
	UFUNCTION(BlueprintPure, Category = "BlendStack", meta = (BlueprintThreadSafe))
	EOffsetRootBoneMode GetOffsetRootRotationMode() const;
	UFUNCTION(BlueprintPure, Category = "BlendStack", meta = (BlueprintThreadSafe))
	EOffsetRootBoneMode GetOffsetRootTranslationMode() const;
	UFUNCTION(BlueprintPure, Category = "BlendStack", meta = (BlueprintThreadSafe))
	float GetOffsetRootTranslationHalfLife() const;
	UFUNCTION(BlueprintPure, Category = "BlendStack", meta = (BlueprintThreadSafe))
	EOrientationWarpingSpace GetOrientationWarpingSpace() const;
	UFUNCTION(BlueprintPure, Category = "AimOffset", meta = (BlueprintThreadSafe))
	float GetAOYaw() const;

	UFUNCTION(BlueprintPure, Category = "BlendStack", meta = (BlueprintThreadSafe))
	FTransform GetHandIKTransform(const FName HandIKSocketName, const FName ObjectIKSocketName,
	                              const FVector& SocketOffset) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PoseSearchData|Trajectory", Transient)
	FPoseSearchTrajectory_WorldCollisionResults CollisionResults{};

	/********************
	 * Overlay blending *
	 *******************/
	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
	void RefreshOverlaySettings(float DeltaTime);
	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
	void RefreshLayering(float DeltaTime);

	UPROPERTY(EditAnywhere, Category = "Additive|Layering", BlueprintReadOnly)
	FLayeringState LayeringState;

public:
	UGASPAnimInstance() = default;

	virtual void NativeBeginPlay() override;
	virtual void NativeInitializeAnimation() override;
	virtual void PreUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "BlendStack", meta = (BlueprintThreadSafe))
	float GetMatchingNotifyRecencyTimeOut() const;
	UFUNCTION(BlueprintPure, Category = "BlendStack", meta = (BlueprintThreadSafe))
	float GetMatchingBlendTime() const;
	UFUNCTION(BlueprintPure, Category = "BlendStack", meta = (BlueprintThreadSafe))
	FFloatInterval GetMatchingPlayRate() const;
	UFUNCTION(BlueprintGetter, meta = (BlueprintThreadSafe))
	FFootPlacementPlantSettings GetPlantSettings() const;
	UFUNCTION(BlueprintGetter, meta = (BlueprintThreadSafe))
	FFootPlacementInterpolationSettings GetPlantInterpolationSettings() const;

	UFUNCTION(BlueprintGetter, Category = "Movement|Analys", meta = (BlueprintThreadSafe))
	FORCEINLINE FCharacterInfo GetCharacterInfo() const { return CharacterInfo; }

	UFUNCTION(BlueprintGetter, Category = "Movement|Analys", meta = (BlueprintThreadSafe))
	FORCEINLINE FGameplayTag GetGait() const { return Gait; }

	UFUNCTION(BlueprintGetter, Category = "Movement|Analys", meta = (BlueprintThreadSafe))
	FORCEINLINE FGameplayTag GetStanceMode() const { return StanceMode; }

	UFUNCTION(BlueprintGetter, Category = "Movement|Analys", meta = (BlueprintThreadSafe))
	FORCEINLINE FGameplayTag GetMovementState() const { return MovementState; }

	UFUNCTION(BlueprintGetter, Category = "Movement|Analys", meta = (BlueprintThreadSafe))
	FORCEINLINE FGameplayTag GetRotationMode() const { return RotationMode; }

	UFUNCTION(BlueprintGetter, Category = "Movement|Analys", meta = (BlueprintThreadSafe))
	FORCEINLINE FGameplayTag GetMovementMode() const { return MovementMode; }

	FPoseSnapshot& SnapshotFinalRagdollPose();

	UPROPERTY(BlueprintReadOnly, Category = "StateMachine", Transient)
	bool bNotifyTransition_ReTransition{false};
	UPROPERTY(BlueprintReadOnly, Category = "StateMachine", Transient)
	bool bNotifyTransition_ToLoop{false};
	UPROPERTY(BlueprintReadOnly, Category = "StateMachine", Transient)
	bool bNoValidAnim{true};
	
protected:
	UPROPERTY(EditAnywhere, Category = "StateMachine", BlueprintReadOnly)
	TObjectPtr<UAnimSequenceBase> StrafeCurveAnimationAsset;

	UPROPERTY(BlueprintReadOnly, Category = "StateMachine", Transient)
	FGASPBlendStackInputs PreviousBlendStackInputs{};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StateMachine")
	float SearchCost{.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StateMachine")
	EStateMachineState StateMachineState{EStateMachineState::IdleLoop};
	UPROPERTY(BlueprintReadOnly, Category = "StateMachine", Transient)
	EMovementDirection PreviousMovementDirection{};
	[[maybe_unused]] UPROPERTY(VisibleAnywhere, Category = "StateMachine", BlueprintReadOnly, Transient)
	EMovementDirectionBias MovementDirectionBias{EMovementDirectionBias::LeftFootForward};

	UPROPERTY(BlueprintReadOnly, Category = "StateMachine", Transient)
	FRotator TargetRotation{FRotator::ZeroRotator};
	UPROPERTY(BlueprintReadOnly, Category = "StateMachine", Transient)
	FRotator TargetRotationOnTransitionStart{FRotator::ZeroRotator};
	UPROPERTY(BlueprintReadOnly, Category = "StateMachine", Transient)
	float TargetRotationDelta{0.f};

	UFUNCTION(BlueprintCallable, Category = "StateMachine", meta = (BlueprintThreadSafe))
	void SetBlendStackAnimFromChooser(const FAnimNodeReference& Node, const EStateMachineState NewState,
	                                  const bool bForceBlend = false);

	UFUNCTION(BlueprintPure, Category = "StateMachine", meta = (BlueprintThreadSafe))
	bool IsAnimationAlmostComplete() const;
	UFUNCTION(BlueprintPure, Category = "StateMachine", meta = (BlueprintThreadSafe))
	float GetDynamicPlayRate(const FAnimNodeReference& Node) const;

	UFUNCTION(BlueprintCallable, Category = "StateMachine", meta = (BlueprintThreadSafe))
	void OnStateEntryIdleLoop(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	UFUNCTION(BlueprintCallable, Category = "StateMachine", meta = (BlueprintThreadSafe))
	void OnStateEntryTransitionToIdleLoop(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	UFUNCTION(BlueprintCallable, Category = "StateMachine", meta = (BlueprintThreadSafe))
	void OnStateEntryLocomotionLoop(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	UFUNCTION(BlueprintCallable, Category = "StateMachine", meta = (BlueprintThreadSafe))
	void OnStateEntryTransitionToLocomotionLoop(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	UFUNCTION(BlueprintCallable, Category = "StateMachine", meta = (BlueprintThreadSafe))
	void OnUpdateTransitionToLocomotionLoop(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	UFUNCTION(BlueprintCallable, Category = "StateMachine", meta = (BlueprintThreadSafe))
	void OnStateEntryInAirLoop(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	UFUNCTION(BlueprintCallable, Category = "StateMachine", meta = (BlueprintThreadSafe))
	void OnStateEntryTransitionToInAirLoop(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	UFUNCTION(BlueprintCallable, Category = "StateMachine", meta = (BlueprintThreadSafe))
	void OnStateEntryIdleBreak(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);

	UFUNCTION(BlueprintCallable, Category = "StateMachine", meta = (BlueprintThreadSafe))
	void RefreshTargetRotation();
	UFUNCTION(BlueprintPure, Category = "StateMachine", meta = (BlueprintThreadSafe))
	float GetStrafeYawRotationOffset() const;
};
