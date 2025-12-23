#pragma once

#include "Animation/AnimInstance.h"
#include "Animation/GASPAnimInstanceProxy.h"
#include "Animation/AnimExecutionContext.h"
#include "Animation/AnimNodeReference.h"
#include "BoneControllers/AnimNode_OffsetRootBone.h"
#include "BoneControllers/AnimNode_OrientationWarping.h"
#include "PoseSearch/PoseSearchLibrary.h"
#include "PoseSearch/PoseSearchTrajectoryLibrary.h"
#include "Types/EnumTypes.h"
#include "Types/StructTypes.h"
#include "GASPAnimInstance.generated.h"

class UMoverTrajectoryPredictor;
class UChooserTable;
class AGASPCharacter;
class UGASPMoverComponent;

/**
 * GASP Anim Instance
 * Handles Motion Matching, Trajectory Prediction, and Advanced State Machine Logic.
 */
UCLASS()
class GASP_API UGASPAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UGASPAnimInstance() = default;

	// --- UAnimInstance Interface ---
	virtual void NativeBeginPlay() override;
	virtual void NativeInitializeAnimation() override;
	virtual void PreUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativePostEvaluateAnimation() override;

	// Creates a snapshot for the final ragdoll pose
	FPoseSnapshot& SnapshotFinalRagdollPose();

	// --- Movement Analysis ---
	UFUNCTION(BlueprintPure, Category = "Movement|Analysis", meta = (BlueprintThreadSafe))
	bool IsStarting() const;
	UFUNCTION(BlueprintPure, Category = "Movement|Analysis", meta = (BlueprintThreadSafe))
	bool IsPivoting() const;
	UFUNCTION(BlueprintPure, Category = "Movement|Analysis", meta = (BlueprintThreadSafe))
	bool IsMoving() const;
	UFUNCTION(BlueprintPure, Category = "Movement|Analysis", meta = (BlueprintThreadSafe))
	bool ShouldTurnInPlace() const;
	UFUNCTION(BlueprintPure, Category = "Movement|Analysis", meta = (BlueprintThreadSafe))
	bool ShouldSpinTransition() const;
	UFUNCTION(BlueprintPure, Category = "Movement|Analysis", meta = (BlueprintThreadSafe))
	bool JustLanded_Light() const;
	UFUNCTION(BlueprintPure, Category = "Movement|Analysis", meta = (BlueprintThreadSafe))
	bool JustLanded_Heavy() const;
	UFUNCTION(BlueprintPure, Category = "Movement|Analysis", meta = (BlueprintThreadSafe))
	bool JustTraversed() const;
	UFUNCTION(BlueprintPure, Category = "Movement|Analysis", meta = (BlueprintThreadSafe))
	bool PlayLand() const;
	UFUNCTION(BlueprintPure, Category = "Movement|Analysis", meta = (BlueprintThreadSafe))
	bool PlayMovingLand() const;
	UFUNCTION(BlueprintPure, Category = "Movement|Analysis", meta = (BlueprintThreadSafe))
	bool IsEnableSteering() const;
	UFUNCTION(BlueprintPure, Category = "Movement|Analysis", meta = (BlueprintThreadSafe))
	bool JustTeleported() const;
	UFUNCTION(BlueprintPure, Category = "Movement|Analysis", meta = (BlueprintThreadSafe))
	float GetTrajectoryTurnAngle() const;
	UFUNCTION(BlueprintPure, Category = "Movement|Lean", meta = (BlueprintThreadSafe))
	FVector2D GetLeanAmount() const;

	// --- Procedural & IK ---
	UFUNCTION(BlueprintPure, Category = "Procedural", meta = (BlueprintThreadSafe))
	bool AllowFootPinning() const;
	UFUNCTION(BlueprintPure, Category = "Procedural", meta = (BlueprintThreadSafe))
	bool AllowSlopeWarping() const;
	UFUNCTION(BlueprintPure, Category = "Procedural", meta = (BlueprintThreadSafe))
	FVector GetStrafeWarpDirection() const;
	UFUNCTION(BlueprintPure, Category = "Procedural", meta = (BlueprintThreadSafe))
	FVector GetSlideSlopeOffset() const;
	UFUNCTION(BlueprintPure, Category = "Procedural", meta = (BlueprintThreadSafe))
	FRotator GetSlideSlopeRotation() const;
	UFUNCTION(BlueprintPure, Category = "Procedural", meta = (BlueprintThreadSafe))
	FTransform GetHandIKTransform(const FName HandIKSocketName, const FName ObjectIKSocketName,
	                              const FVector& SocketOffset) const;

	// --- AimOffset & Overlay ---
	UFUNCTION(BlueprintPure, Category = "AimOffset", meta = (BlueprintThreadSafe))
	bool IsEnabledAO() const;
	UFUNCTION(BlueprintPure, Category = "AimOffset", meta = (BlueprintThreadSafe))
	FVector2D GetAOValue() const;
	UFUNCTION(BlueprintPure, Category = "AimOffset", meta = (BlueprintThreadSafe))
	bool IsCircling() const;
	UFUNCTION(BlueprintPure, Category = "AimOffset", meta = (BlueprintThreadSafe))
	float GetAOYaw() const;
	UFUNCTION(BlueprintPure, Category = "Overlay", meta = (BlueprintThreadSafe))
	bool CanOverlayTransition() const;
	UFUNCTION(BlueprintImplementableEvent, Category = "Overlay")
	void OnOverlayModeChanged(FGameplayTag OldGameplayTag, FGameplayTag NewGameplayTag);
	UFUNCTION(BlueprintImplementableEvent, Category = "Overlay")
	void OnPoseModeChanged(FGameplayTag OldGameplayTag, FGameplayTag NewGameplayTag);

	// --- Motion Matching & BlendStack ---
	UFUNCTION(BlueprintPure, Category = "BlendStack", meta = (BlueprintThreadSafe))
	FQuat GetDesiredFacing() const;
	UFUNCTION(BlueprintPure, Category = "BlendStack", meta = (BlueprintThreadSafe))
	EPoseSearchInterruptMode GetMatchingInterruptMode() const;
	UFUNCTION(BlueprintPure, Category = "BlendStack", meta = (BlueprintThreadSafe))
	EOffsetRootBoneMode GetOffsetRootRotationMode() const;
	UFUNCTION(BlueprintPure, Category = "BlendStack", meta = (BlueprintThreadSafe))
	EOffsetRootBoneMode GetOffsetRootTranslationMode() const;
	UFUNCTION(BlueprintPure, Category = "BlendStack", meta = (BlueprintThreadSafe))
	EOrientationWarpingSpace GetOrientationWarpingSpace() const;
	UFUNCTION(BlueprintPure, Category = "BlendStack", meta = (BlueprintThreadSafe))
	float GetOffsetRootTranslationHalfLife() const;
	UFUNCTION(BlueprintPure, Category = "BlendStack", meta = (BlueprintThreadSafe))
	float GetMatchingNotifyRecencyTimeOut() const;
	UFUNCTION(BlueprintPure, Category = "BlendStack", meta = (BlueprintThreadSafe))
	float GetMatchingBlendTime() const;
	UFUNCTION(BlueprintPure, Category = "BlendStack", meta = (BlueprintThreadSafe))
	FFloatInterval GetMatchingPlayRate() const;

protected:
	// --- Internal Update Logic ---
	UFUNCTION(BlueprintCallable, Category = "Runtime", meta = (BlueprintThreadSafe))
	void RefreshEssentialValues(const float DeltaSeconds);
	UFUNCTION(BlueprintCallable, Category = "Runtime", meta = (BlueprintThreadSafe))
	void RefreshStateContainer();
	UFUNCTION(BlueprintCallable, Category = "Runtime", meta = (BlueprintThreadSafe))
	void RefreshTrajectory(float DeltaSeconds);

	// --- Motion Matching & BlendStack Refresh ---
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

	// --- Systems Refresh ---
	UFUNCTION(BlueprintCallable, Category = "Runtime", meta = (BlueprintThreadSafe))
	void OnBecomeRelevantFootPlacement(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	UFUNCTION(BlueprintCallable, Category = "Runtime", meta = (BlueprintThreadSafe))
	void RefreshRagdollValues(const float DeltaSeconds);
	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
	void RefreshOverlaySettings(float DeltaTime);
	UFUNCTION(BlueprintCallable, meta = (BlueprintThreadSafe))
	void RefreshLayering(float DeltaTime);
	UFUNCTION(BlueprintPure)
	float GetTotalFacingDelta(TArray<float> Times) const;

	// --- State Machine Logic ---
	UFUNCTION(BlueprintCallable, Category = "StateMachine", meta = (BlueprintThreadSafe))
	void SetBlendStackAnimFromChooser(const FAnimNodeReference& Node, const EStateMachineState NewState,
	                                  const bool bForceBlend = false);
	UFUNCTION(BlueprintPure, Category = "StateMachine", meta = (BlueprintThreadSafe))
	bool IsAnimationAlmostComplete() const;
	UFUNCTION(BlueprintPure, Category = "StateMachine", meta = (BlueprintThreadSafe))
	float GetDynamicPlayRate(const FAnimNodeReference& Node) const;

	// --- State Entry/Update Handlers ---
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
	void OnStateEntryTransitionToSlide(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);
	UFUNCTION(BlueprintCallable, Category = "StateMachine", meta = (BlueprintThreadSafe))
	void OnStateEntrySlideLoop(const FAnimUpdateContext& Context, const FAnimNodeReference& Node);

protected:
	// --- References ---
	UPROPERTY(BlueprintReadOnly, Category = "References", Transient)
	TWeakObjectPtr<AGASPCharacter> CachedCharacter{};
	UPROPERTY(BlueprintReadOnly, Category = "References", Transient)
	TWeakObjectPtr<UGASPMoverComponent> CachedMovement{};
	UPROPERTY(BlueprintReadOnly, Category = "References", Transient)
	TObjectPtr<UMoverTrajectoryPredictor> Predictor;

	// --- Character State & Configuration ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CharacterInformation|General")
	FAnimUtilityNames AnimNames{};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CharacterInformation|General")
	FAnimConfiguration AnimConfiguration{};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CharacterInformation|General")
	FCharacterInfo CharacterInfo{};
	UPROPERTY(BlueprintReadOnly, Category = "CharacterInformation|PreviousValues", Transient)
	FCharacterInfo PreviousCharacterInfo;

	// Tags & State Info
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LocomotionAction", Transient)
	FGameplayTag LocomotionAction{FGameplayTag::EmptyTag};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LocomotionAction", Transient)
	FGameplayTag PreviousLocomotionAction{FGameplayTag::EmptyTag};

	// Containers
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CharacterInformation|States", Transient)
	FGameplayTagContainer StateContainer{};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CharacterInformation|States", Transient)
	FGameplayTagContainer PreviousStateContainer{};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CharacterInformation|States", Transient)
	FGameplayTagContainer RecentStateContainer{};

	// State Wrappers
	TStateWrapper<FGameplayTag> Gait{FGameplayTag::EmptyTag};
	TStateWrapper<FGameplayTag> MovementState{FGameplayTag::EmptyTag};
	TStateWrapper<FGameplayTag> RotationMode{FGameplayTag::EmptyTag};
	TStateWrapper<FGameplayTag> MovementMode{FGameplayTag::EmptyTag};
	TStateWrapper<FGameplayTag> StanceMode{FGameplayTag::EmptyTag};
	TStateWrapper<EMovementDirection> MovementDirection{EMovementDirection::F};

	// --- Motion Matching & Trajectory Data ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PoseSearchData|Choosers")
	TObjectPtr<UChooserTable> LocomotionTable{nullptr};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PoseSearchData|Choosers")
	TObjectPtr<UChooserTable> StateMachineTable{nullptr};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PoseSearchData|Trajectory", Transient)
	FTransformTrajectory Trajectory{};
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PoseSearchData|Trajectory", Transient)
	FPoseSearchTrajectory_WorldCollisionResults CollisionResults{};
	UPROPERTY(BlueprintReadOnly, Category = "MotionMatching")
	FTrajectoryInfo TrajectoryInfo;
	UPROPERTY(BlueprintReadOnly, Category = "MotionMatching", Transient)
	FMotionMatchingInfo BlendStack;
	UPROPERTY(BlueprintReadOnly, Category = "CharacterInformation|General", Transient)
	FGASPBlendStackInputs BlendStackInputs{};

	// --- Procedural / Additive / IK Data ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Additive|Layering")
	FLayeringState LayeringState;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Additive|Poses", Transient)
	FGASPBlendPoses BlendPoses;
	UPROPERTY(BlueprintReadOnly, Category = "Additive|Poses", Transient)
	FBlendStackMachine BlendStackMachine;
	UPROPERTY(BlueprintReadOnly, Category = "LocomotionAction|Information", Transient)
	FRagdollingAnimationState RagdollingState;
	UPROPERTY(BlueprintReadOnly, Category = "Additive", Transient)
	FRotator SpineRotation{FRotator::ZeroRotator};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HeldObject", Transient)
	FVector RightHandOffset{FVector::ZeroVector};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HeldObject", Transient)
	FVector LeftHandOffset{FVector::ZeroVector};

	// --- Config & Settings ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MovementInformation|General")
	float HeavyLandSpeedThreshold{700.f};
	UPROPERTY(BlueprintReadOnly, Category = "MovementInformation|General")
	int32 MMDatabaseLOD{0};
	UPROPERTY(BlueprintReadOnly, Category = "MovementInformation|General")
	int32 LocomotionSetup{0};
	UPROPERTY(BlueprintReadOnly, Category = "MovementInformation|General")
	uint8 OffsetRootBoneEnabled : 1 {true};
	UPROPERTY(BlueprintReadOnly, Category = "MovementInformation|General")
	uint8 FootPlacementEnabled : 1 {true};
	UPROPERTY(BlueprintReadOnly, Category = "MovementInformation|General")
	uint8 ForceFootPlacementReset : 1{false};

	// --- State Machine Data ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StateMachine")
	float SearchCost{.0f};
	UPROPERTY(BlueprintReadOnly, Category = "StateMachine", Transient)
	FGASPBlendStackInputs PreviousBlendStackInputs{};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "StateMachine")
	EStateMachineState StateMachineState{EStateMachineState::IdleLoop};

public:
	UPROPERTY(BlueprintReadOnly, Category = "StateMachine", Transient)
	bool bNotifyTransition_ReTransition{false};
	UPROPERTY(BlueprintReadOnly, Category = "StateMachine", Transient)
	bool bNotifyTransition_ToLoop{false};
	UPROPERTY(BlueprintReadOnly, Category = "StateMachine", Transient)
	bool bNoValidAnim{true};

public:
	UFUNCTION(BlueprintGetter, Category = "Movement|Analysis", meta = (BlueprintThreadSafe))
	FORCEINLINE FCharacterInfo GetCharacterInfo() const { return CharacterInfo; }

	UFUNCTION(BlueprintPure, Category = "Movement|States", meta = (BlueprintThreadSafe))
	FORCEINLINE FGameplayTag GetGait() const { return Gait.Current; }

	UFUNCTION(BlueprintPure, Category = "Movement|States", meta = (BlueprintThreadSafe))
	FORCEINLINE FGameplayTag GetPreviousGait() const { return Gait.LastFrame; }

	UFUNCTION(BlueprintPure, Category = "Movement|States", meta = (BlueprintThreadSafe))
	FORCEINLINE FGameplayTag GetRecentGait() const { return Gait.Recent; }

	UFUNCTION(BlueprintPure, Category = "Movement|States", meta = (BlueprintThreadSafe))
	FORCEINLINE float GetGaitTime() const { return Gait.TimeInState; }

	UFUNCTION(BlueprintPure, Category = "Movement|States", meta = (BlueprintThreadSafe))
	FORCEINLINE float GetGaitLastTime() const { return Gait.LastStateTime; }

	UFUNCTION(BlueprintPure, Category = "Movement|States", meta = (BlueprintThreadSafe))
	FORCEINLINE FGameplayTag GetStanceMode() const { return StanceMode.Current; }

	UFUNCTION(BlueprintPure, Category = "Movement|States", meta = (BlueprintThreadSafe))
	FORCEINLINE FGameplayTag GetPreviousStanceMode() const { return StanceMode.LastFrame; }

	UFUNCTION(BlueprintPure, Category = "Movement|States", meta = (BlueprintThreadSafe))
	FORCEINLINE FGameplayTag GetRecentStanceMode() const { return StanceMode.Recent; }

	UFUNCTION(BlueprintPure, Category = "Movement|States", meta = (BlueprintThreadSafe))
	FORCEINLINE float GetStanceModeTime() const { return StanceMode.TimeInState; }

	UFUNCTION(BlueprintPure, Category = "Movement|States", meta = (BlueprintThreadSafe))
	FORCEINLINE float GetStanceModeLastTime() const { return StanceMode.LastStateTime; }

	UFUNCTION(BlueprintPure, Category = "Movement|States", meta = (BlueprintThreadSafe))
	FORCEINLINE FGameplayTag GetMovementState() const { return MovementState.Current; }

	UFUNCTION(BlueprintPure, Category = "Movement|States", meta = (BlueprintThreadSafe))
	FORCEINLINE FGameplayTag GetPreviousMovementState() const { return MovementState.LastFrame; }

	UFUNCTION(BlueprintPure, Category = "Movement|States", meta = (BlueprintThreadSafe))
	FORCEINLINE FGameplayTag GetRecentMovementState() const { return MovementState.Recent; }

	UFUNCTION(BlueprintPure, Category = "Movement|States", meta = (BlueprintThreadSafe))
	FORCEINLINE float GetMovementStateTime() const { return MovementState.TimeInState; }

	UFUNCTION(BlueprintPure, Category = "Movement|States", meta = (BlueprintThreadSafe))
	FORCEINLINE float GetMovementStateLastTime() const { return MovementState.LastStateTime; }

	UFUNCTION(BlueprintPure, Category = "Movement|States", meta = (BlueprintThreadSafe))
	FORCEINLINE FGameplayTag GetRotationMode() const { return RotationMode.Current; }

	UFUNCTION(BlueprintPure, Category = "Movement|States", meta = (BlueprintThreadSafe))
	FORCEINLINE FGameplayTag GetPreviousRotationMode() const { return RotationMode.LastFrame; }

	UFUNCTION(BlueprintPure, Category = "Movement|States", meta = (BlueprintThreadSafe))
	FORCEINLINE FGameplayTag GetRecentRotationMode() const { return RotationMode.Recent; }

	UFUNCTION(BlueprintPure, Category = "Movement|States", meta = (BlueprintThreadSafe))
	FORCEINLINE float GetRotationModeTime() const { return RotationMode.TimeInState; }

	UFUNCTION(BlueprintPure, Category = "Movement|States", meta = (BlueprintThreadSafe))
	FORCEINLINE float GetRotationModeLastTime() const { return RotationMode.LastStateTime; }

	UFUNCTION(BlueprintPure, Category = "Movement|States", meta = (BlueprintThreadSafe))
	FORCEINLINE FGameplayTag GetMovementMode() const { return MovementMode.Current; }

	UFUNCTION(BlueprintPure, Category = "Movement|States", meta = (BlueprintThreadSafe))
	FORCEINLINE FGameplayTag GetPreviousMovementMode() const { return MovementMode.LastFrame; }

	UFUNCTION(BlueprintPure, Category = "Movement|States", meta = (BlueprintThreadSafe))
	FORCEINLINE FGameplayTag GetRecentMovementMode() const { return MovementMode.Recent; }

	UFUNCTION(BlueprintPure, Category = "Movement|States", meta = (BlueprintThreadSafe))
	FORCEINLINE float GetMovementModeTime() const { return MovementMode.TimeInState; }

	UFUNCTION(BlueprintPure, Category = "Movement|States", meta = (BlueprintThreadSafe))
	FORCEINLINE float GetMovementModeLastTime() const { return MovementMode.LastStateTime; }

	UFUNCTION(BlueprintPure, Category = "Movement|States", meta = (BlueprintThreadSafe))
	FORCEINLINE EMovementDirection GetMovementDirection() const { return MovementDirection.Current; }

	UFUNCTION(BlueprintPure, Category = "Movement|States", meta = (BlueprintThreadSafe))
	FORCEINLINE EMovementDirection GetPreviousMovementDirection() const { return MovementDirection.LastFrame; }

	UFUNCTION(BlueprintPure, Category = "Movement|States", meta = (BlueprintThreadSafe))
	FORCEINLINE EMovementDirection GetRecentMovementDirection() const { return MovementDirection.Recent; }

	UFUNCTION(BlueprintPure, Category = "Movement|States", meta = (BlueprintThreadSafe))
	FORCEINLINE float GetMovementDirectionTime() const { return MovementDirection.TimeInState; }

	UFUNCTION(BlueprintPure, Category = "Movement|States", meta = (BlueprintThreadSafe))
	FORCEINLINE float GetMovementDirectionLastTime() const { return MovementDirection.LastStateTime; }
};
