#include "Animation/GASPAnimInstance.h"
#include "Actors/GASPCharacter.h"
#include "PoseSearch/PoseSearchDatabase.h"
#include "Utils/GASPMath.h"
#include "PoseSearch/MotionMatchingAnimNodeLibrary.h"
#include "ChooserFunctionLibrary.h"
#include "PoseSearch/PoseSearchLibrary.h"
#include "AnimationWarpingLibrary.h"
#include "BlendStack/BlendStackAnimNodeLibrary.h"
#include "BoneControllers/AnimNode_FootPlacement.h"
#include "Interfaces/GASPHeldObjectInterface.h"
#include "MoverPoseSearchTrajectoryPredictor.h"
#include "MovementSet/GASPMoverComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPAnimInstance)

namespace AnimVars
{
	int32 LocomotionSetup{false};
	FAutoConsoleVariableRef CVarLocomotionStyleStruct(
		TEXT("gasp.statemachine.style"), LocomotionSetup, TEXT("enabled state machine"), ECVF_Default);

	bool bOffsetRootBoneEnabled{true};
	FAutoConsoleVariableRef CVarOffsetRootBoneEnabledStruct(
		TEXT("gasp.offsetrootbone.enabled"), bOffsetRootBoneEnabled, TEXT("enabled offset root bone"),
		ECVF_Default);

	int32 MMDatabaseLOD{0};
	FAutoConsoleVariableRef CVarMMDatabaseLODStruct(
		TEXT("gasp.motionmatching.LOD"), MMDatabaseLOD, TEXT("LOD for motion matching database"),
		ECVF_Default);
}

EPoseSearchInterruptMode UGASPAnimInstance::GetMatchingInterruptMode() const
{
	return MovementMode != PreviousMovementMode || (MovementMode == MovementModeTags::Grounded && (
		       MovementState != PreviousMovementState || (Gait != PreviousGait && IsMoving()) || StanceMode !=
		       PreviousStanceMode) || (MovementDirection != PreviousMovementDirection && IsMoving()))
		       ? EPoseSearchInterruptMode::InterruptOnDatabaseChange
		       : EPoseSearchInterruptMode::DoNotInterrupt;
}

EOffsetRootBoneMode UGASPAnimInstance::GetOffsetRootRotationMode() const
{
	if (IsSlotActive(FName{TEXT("DefaultSlot")}))
	{
		return EOffsetRootBoneMode::Release;
	}

	return EOffsetRootBoneMode::Accumulate;
}

EOffsetRootBoneMode UGASPAnimInstance::GetOffsetRootTranslationMode() const
{
	if (IsSlotActive(FName{TEXT("DefaultSlot")}))
	{
		return EOffsetRootBoneMode::Release;
	}

	return MovementMode == MovementModeTags::Grounded && IsMoving()
		       ? EOffsetRootBoneMode::Interpolate
		       : EOffsetRootBoneMode::Release;
}

float UGASPAnimInstance::GetOffsetRootTranslationHalfLife() const
{
	return IsMoving() ? .3f : .1f;
}

EOrientationWarpingSpace UGASPAnimInstance::GetOrientationWarpingSpace() const
{
	return bOffsetRootBoneEnabled
		       ? EOrientationWarpingSpace::RootBoneTransform
		       : EOrientationWarpingSpace::ComponentTransform;
}

float UGASPAnimInstance::GetAOYaw() const
{
	return RotationMode == RotationTags::OrientToMovement ? 0.f : GetAOValue().X;
}

FTransform UGASPAnimInstance::GetHandIKTransform(const FName HandIKSocketName, const FName ObjectIKSocketName,
                                                 const FVector& SocketOffset) const
{
	const auto* SkelMeshComp = GetSkelMeshComponent();
	if (!SkelMeshComp || !CachedCharacter.IsValid())
	{
		return FTransform::Identity;
	}

	const FTransform SocketTransform = SkelMeshComp->GetSocketTransform(HandIKSocketName);
	const auto* Interface = Cast<IGASPHeldObjectInterface>(CachedCharacter.Get());
	if (!Interface && !CachedCharacter->Implements<UGASPHeldObjectInterface>())
	{
		return FTransform::Identity;
	}

	const auto* HeldObject = Interface->Execute_GetHeldObject(CachedCharacter.Get());
	if (!IsValid(HeldObject) || !HeldObject->DoesSocketExist(ObjectIKSocketName))
	{
		return FTransform::Identity;
	}

	const FTransform ObjectTransform = HeldObject->GetSocketTransform(ObjectIKSocketName);
	return ObjectTransform.GetRelativeTransform(SocketTransform * FTransform(SocketOffset));
}

bool UGASPAnimInstance::IsEnableSteering() const
{
	return (BlendStackInputs.bLoop && IsMoving()) || MovementMode == MovementModeTags::InAir || MovementMode ==
		MovementModeTags::Slide;
}

void UGASPAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();

	CachedCharacter = Cast<AGASPCharacter>(TryGetPawnOwner());
	if (!CachedCharacter.IsValid())
	{
		return;
	}

	CachedMovement = CachedCharacter->GetMoverComponent();
	if (!CachedMovement.IsValid())
	{
		return;
	}
}

void UGASPAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	CachedCharacter = Cast<AGASPCharacter>(TryGetPawnOwner());
	if (!CachedCharacter.IsValid())
	{
		return;
	}

	CachedMovement = CachedCharacter->GetMoverComponent();
	if (!CachedMovement.IsValid())
	{
		return;
	}
	
	CachedCharacter->OverlayModeChanged.AddUniqueDynamic(this, &ThisClass::OnOverlayModeChanged);
	CachedCharacter->PoseModeChanged.AddUniqueDynamic(this, &ThisClass::OnPoseModeChanged);

	Predictor = NewObject<UMoverTrajectoryPredictor>(CachedMovement.Get());
	Predictor->Setup(CachedMovement.Get());

	LocomotionSetup = AnimVars::LocomotionSetup;
	bOffsetRootBoneEnabled = AnimVars::bOffsetRootBoneEnabled;
	MMDatabaseLOD = AnimVars::MMDatabaseLOD;

	AnimVars::CVarLocomotionStyleStruct->OnChangedDelegate().AddWeakLambda(this, [this](const IConsoleVariable* ICVar)
	{
		LocomotionSetup = ICVar ? ICVar->GetInt() : false;
	});
	AnimVars::CVarOffsetRootBoneEnabledStruct->OnChangedDelegate().AddWeakLambda(
		this, [this](const IConsoleVariable* ICVar)
		{
			bOffsetRootBoneEnabled = ICVar ? ICVar->GetBool() : false;
		});
	AnimVars::CVarMMDatabaseLODStruct->OnChangedDelegate().AddWeakLambda(this, [this](const IConsoleVariable* ICVar)
	{
		MMDatabaseLOD = ICVar ? ICVar->GetInt() : false;
	});
}

void UGASPAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UGASPAnimInstance::NativeThreadSafeUpdateAnimation"),
	                            STAT_UGASPAnimInstance_NativeThreadSafeUpdateAnimation, STATGROUP_GASP)
	TRACE_CPUPROFILER_EVENT_SCOPE(__FUNCTION__);

	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);

	if (!CachedCharacter.IsValid() || !CachedMovement.IsValid())
	{
		return;
	}

	Gait = CachedCharacter->GetGait();
	RotationMode = CachedCharacter->GetRotationMode();
	MovementState = CachedCharacter->GetMovementState();
	LocomotionAction = CachedCharacter->GetLocomotionAction();
	MovementMode = CachedCharacter->GetMovementMode();
	StanceMode = CachedCharacter->GetStanceMode();

	StateContainer.Reset();
	StateContainer.AddTagFast(MovementMode);
	StateContainer.AddTagFast(StanceMode);
	StateContainer.AddTagFast(MovementState);
	StateContainer.AddTagFast(Gait);

	RefreshEssentialValues(DeltaSeconds);
	RefreshTrajectory(DeltaSeconds);
	RefreshOverlaySettings(DeltaSeconds);
	RefreshLayering(DeltaSeconds);
	RefreshTargetRotation();

	if (LocomotionAction == LocomotionActionTags::Ragdoll)
	{
		RefreshRagdollValues(DeltaSeconds);
	}
}

void UGASPAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UGASPAnimInstance::NativeUpdateAnimation"),
	                            STAT_UGASPAnimInstance_NativeUpdateAnimation, STATGROUP_GASP)
	TRACE_CPUPROFILER_EVENT_SCOPE(__FUNCTION__);

	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!CachedCharacter.IsValid() || !CachedMovement.IsValid())
	{
		return;
	}
}

void UGASPAnimInstance::PreUpdateAnimation(float DeltaSeconds)
{
	PreviousCharacterInfo = CharacterInfo;
	PreviousGait = Gait;
	PreviousRotationMode = RotationMode;
	PreviousMovementState = MovementState;
	PreviousLocomotionAction = LocomotionAction;
	PreviousMovementMode = MovementMode;
	PreviousStanceMode = StanceMode;
	PreviousMovementDirection = MovementDirection;

	Super::PreUpdateAnimation(DeltaSeconds);
}

void UGASPAnimInstance::RefreshTrajectory(const float DeltaSeconds)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UGASPAnimInstance::RefreshTrajectory"),
	                            STAT_UGASPAnimInstance_RefreshTrajectory, STATGROUP_GASP)
	TRACE_CPUPROFILER_EVENT_SCOPE(__FUNCTION__);

	FTransformTrajectory OutTrajectory{};
	UPoseSearchTrajectoryLibrary::PoseSearchGenerateTransformTrajectoryWithPredictor(
		Predictor, DeltaSeconds, Trajectory, BlendStack.PreviousDesiredYawRotation,
		OutTrajectory, .033f, 15, .1f, 15);

	const TArray<AActor*> IgnoredActors{};
	UPoseSearchTrajectoryLibrary::HandleTransformTrajectoryWorldCollisions(
		CachedCharacter.Get(), this, OutTrajectory, true, .01f,
		Trajectory, CollisionResults, UEngineTypes::ConvertToTraceType(ECC_Visibility), false,
		IgnoredActors, EDrawDebugTrace::None, true, 150.f);

	UPoseSearchTrajectoryLibrary::GetTransformTrajectoryVelocity(Trajectory, -.3f, -.2f,
	                                                             TrajectoryInfo.PreviousVelocity,
	                                                             false);
	UPoseSearchTrajectoryLibrary::GetTransformTrajectoryVelocity(Trajectory, .1f, .2f,
	                                                             TrajectoryInfo.NearFutureVelocity,
	                                                             false);
	
	TrajectoryInfo.PreviousFutureVelocity = TrajectoryInfo.FutureVelocity;
	UPoseSearchTrajectoryLibrary::GetTransformTrajectoryVelocity(Trajectory, .4f, .5f, TrajectoryInfo.FutureVelocity,
	                                                             false);

	TrajectoryInfo.FutureFacing = Trajectory.GetSampleAtTime(1.5).Facing.Rotator();

	TrajectoryInfo.PreviousFutureFacingDelta = TrajectoryInfo.FutureFacingDelta;
	TrajectoryInfo.FutureFacingDelta = GetTotalFacingDelta({0.f, 0.25f, 0.75f, 1.5f});

	if (FMath::Abs(TrajectoryInfo.FutureFacingDelta - TrajectoryInfo.PreviousFutureFacingDelta) > 200.f && RotationMode
		!= RotationTags::OrientToMovement)
	{
		MovementDirection = PreviousMovementDirection = EMovementDirection::B;
	}

	UPoseSearchTrajectoryLibrary::GetTransformTrajectoryAngularVelocity(
		Trajectory, -.4f, -.3f, TrajectoryInfo.PastAngularVelocity);
	UPoseSearchTrajectoryLibrary::GetTransformTrajectoryAngularVelocity(
		Trajectory, 0.f, .1f, TrajectoryInfo.CurrentAngularVelocity);

	TrajectoryInfo.CirclingTime = IsCircling() ? TrajectoryInfo.CirclingTime + DeltaSeconds : 0.f;
}

float UGASPAnimInstance::GetTotalFacingDelta(TArray<float> Times) const
{
	TArray<float> Rotations{};
	for (const auto Time : Times)
	{
		Rotations.Add(Trajectory.GetSampleAtTime(Time).Facing.Rotator().Yaw);
	}

	float AngleSum{static_cast<float>(Rotations[0] - (CharacterInfo.RootTransform.Rotator().Yaw - 90.f)) };
	for (int32 Index = 0; Index < Rotations.Num() - 2; ++Index)
	{
		AngleSum += Rotations[Index + 1] - Rotations[Index];
	}

	return AngleSum;
}

float UGASPAnimInstance::GetMatchingBlendTime() const
{
	if (MovementMode == MovementModeTags::InAir)
	{
		return CharacterInfo.Velocity.Z > 100.f ? .15f : .5f;
	}

	return PreviousMovementMode == MovementModeTags::Grounded ? .5f : .2f;
}

FFloatInterval UGASPAnimInstance::GetMatchingPlayRate() const
{
	if (MovementMode == MovementModeTags::Grounded)
	{
		return {.75f, 3.f};
	}
	return {.75f, 1.25f};
}

FFootPlacementPlantSettings UGASPAnimInstance::GetPlantSettings() const
{
	return BlendStack.DatabaseTags.Contains(AnimNames.StopsTag)
		       ? PlantSettings_Stops
		       : PlantSettings_Default;
}

FFootPlacementInterpolationSettings UGASPAnimInstance::GetPlantInterpolationSettings() const
{
	return BlendStack.DatabaseTags.Contains(AnimNames.StopsTag)
		       ? InterpolationSettings_Stops
		       : InterpolationSettings_Default;
}

float UGASPAnimInstance::GetMatchingNotifyRecencyTimeOut() const
{
	if (Gait == GaitTags::Sprint)
	{
		return .16f;
	}

	return .2f;
}

FPoseSnapshot& UGASPAnimInstance::SnapshotFinalRagdollPose()
{
	check(IsInGameThread())

	// Save a snapshot of the current ragdoll pose for use in animation graph to blend out of the ragdoll.
	SnapshotPose(RagdollingState.FinalRagdollPose);

	return RagdollingState.FinalRagdollPose;
}

bool UGASPAnimInstance::IsStarting() const
{
	return TrajectoryInfo.FutureVelocity.Size2D() > CharacterInfo.Velocity.Size2D() + 100.f && !BlendStack.
		DatabaseTags.Contains(AnimNames.PivotsTag) && IsMoving() && CharacterInfo.Speed <= 100.f;
}

bool UGASPAnimInstance::IsPivoting() const
{
	if (!IsMoving() || TrajectoryInfo.FutureVelocity.IsNearlyZero())
	{
		return false;
	}

	if (LocomotionSetup == 0)
	{
		return FMath::Abs(GetTrajectoryTurnAngle()) >= 75.f && !IsCircling();
	}

	const auto* FoundSettings = PivotSettings.Find(Gait);
	const auto& Settings = FoundSettings
		                       ? *FoundSettings
		                       : FPivotSettings{{200.f, 700.f}, FVector4f{300.f, 700.f, 60.f, 50.f}};

	const float& Speed = CharacterInfo.Speed;
	if (Speed <= Settings.SpeedRange.X || Speed >= Settings.SpeedRange.Y)
	{
		return false;
	}

	const auto& ClampParams = Settings.RangeParams;
	const float AngleThreshold = FMath::GetMappedRangeValueClamped(
		FVector2f{ClampParams.X, ClampParams.Y},
		FVector2f{ClampParams.Z, ClampParams.W},
		Speed
	);

	return FMath::Abs(GetTrajectoryTurnAngle()) >= AngleThreshold;
}

bool UGASPAnimInstance::IsMoving() const
{
	return MovementState == MovementStateTags::Moving;
}

bool UGASPAnimInstance::ShouldTurnInPlace() const
{
	return CharacterInfo.Speed < 50.f && FMath::Abs(TrajectoryInfo.FutureFacingDelta) >= CharacterInfo.
		MaxTurnInPlaceAngle && !IsMoving();
	// float YawDifference = CharacterInfo.ActorTransform.Rotator().Yaw - CharacterInfo.RootTransform.Rotator().Yaw;
	// YawDifference = FRotator::NormalizeAxis(YawDifference);
	//
	// return FMath::Abs(YawDifference) >= CharacterInfo.MaxTurnInPlaceAngle && (RotationMode == RotationTags::Aim || (
	// 	!IsMoving() && PreviousMovementState == MovementStateTags::Moving));
}

bool UGASPAnimInstance::ShouldSpin() const
{
	return FMath::Abs(TrajectoryInfo.FutureFacingDelta) >= CharacterInfo.SpinTransitionAngle && CharacterInfo.Speed >=
		150.f && !BlendStack.DatabaseTags.Contains(AnimNames.PivotsTag);
	// float YawDifference = CharacterInfo.ActorTransform.Rotator().Yaw - CharacterInfo.RootTransform.Rotator().Yaw;
	// YawDifference = FRotator::NormalizeAxis(YawDifference);
	//
	// return FMath::Abs(YawDifference) >= CharacterInfo.SpinTransitionAngle && CharacterInfo.Speed >= 150.f && !BlendStack
	// 	.DatabaseTags.Contains(
	// 		AnimNames.PivotsTag);
}

bool UGASPAnimInstance::JustLanded_Light() const
{
	return FMath::Abs(PreviousCharacterInfo.Velocity.Z) < FMath::Abs(HeavyLandSpeedThreshold) && MovementMode ==
		MovementModeTags::Grounded && PreviousMovementMode == MovementModeTags::InAir;
}

bool UGASPAnimInstance::JustLanded_Heavy() const
{
	return FMath::Abs(PreviousCharacterInfo.Velocity.Z) >= FMath::Abs(HeavyLandSpeedThreshold) && MovementMode ==
		MovementModeTags::Grounded && PreviousMovementMode == MovementModeTags::InAir;
}

bool UGASPAnimInstance::JustTraversed() const
{
	return !IsSlotActive(FName{TEXT("DefaultSlot")}) && GetCurveValue(AnimNames.MovingTraversalCurveName) > 0.f &&
		FMath::Abs(GetTrajectoryTurnAngle()) <= 50.f;
}

bool UGASPAnimInstance::PlayLand() const
{
	return MovementMode == MovementModeTags::Grounded && PreviousMovementMode == MovementModeTags::InAir;
}

bool UGASPAnimInstance::PlayMovingLand() const
{
	return MovementMode == MovementModeTags::Grounded && PreviousMovementMode == MovementModeTags::InAir &&
		FMath::Abs(GetTrajectoryTurnAngle()) <= 120.f;
}

float UGASPAnimInstance::GetTrajectoryTurnAngle() const
{
	const FVector2D CurrentVelocity2D(CharacterInfo.Velocity.X, CharacterInfo.Velocity.Y);
	const FVector2D FutureVelocity2D(TrajectoryInfo.FutureVelocity.X, TrajectoryInfo.FutureVelocity.Y);

	if (CurrentVelocity2D.IsNearlyZero() || FutureVelocity2D.IsNearlyZero())
	{
		return 0.0f;
	}

	const float Dot = FVector2D::DotProduct(CurrentVelocity2D, FutureVelocity2D);
	const float Cross = FVector2D::CrossProduct(CurrentVelocity2D, FutureVelocity2D);

	return FMath::RadiansToDegrees(FMath::Atan2(Cross, Dot));
}

FVector2D UGASPAnimInstance::GetLeanAmount() const
{
	if (!CachedCharacter.IsValid())
	{
		return FVector2D::ZeroVector;
	}


	float LateralAccelerationAmount = FMath::Clamp(
		CharacterInfo.Velocity.ToOrientationRotator().UnrotateVector(CharacterInfo.VelocityAcceleration).Y /
		FMath::GetMappedRangeValueClamped<float, float>({200.f, 320.f}, {500.f, 800.f}, CharacterInfo.Speed), -1.f,
		1.f);

	switch (MovementDirection)
	{
	case EMovementDirection::B:
		return {LateralAccelerationAmount * -1.f, 0.f};
	case EMovementDirection::LL:
	case EMovementDirection::LR:
		return {0.f, LateralAccelerationAmount};
	case EMovementDirection::RR:
	case EMovementDirection::RL:
		return {0.f, LateralAccelerationAmount * -1.f};
	default:
		return {LateralAccelerationAmount, 0.f};
	}

	// const FVector RelAccel{GetRelativeAcceleration()};
	// return FVector2D(
	// 	RelAccel * FMath::GetMappedRangeValueClamped<float, float>({200.f, 500.f}, {.5f, 1.f}, CharacterInfo.Speed));
}

FVector UGASPAnimInstance::GetRelativeAcceleration() const
{
	if (!CachedMovement.IsValid())
	{
		return FVector::ZeroVector;
	}
	return FVector::ZeroVector;

	// const float Dot{UE_REAL_TO_FLOAT(FVector::DotProduct(CharacterInfo.Velocity, CharacterInfo.Acceleration))};
	// const float MaxAccelValue{.0f
	// 	// Dot > 0.f ? CachedMovement->GetMaxAcceleration() : CachedMovement->GetMaxBrakingDeceleration()
	// };
	//
	// const FVector ClampedAcceleration = CharacterInfo.VelocityAcceleration.GetClampedToMaxSize(MaxAccelValue) /
	// 	MaxAccelValue;
	// return CharacterInfo.ActorTransform.GetRotation().UnrotateVector(ClampedAcceleration);
}

void UGASPAnimInstance::RefreshMotionMatchingMovement(const FAnimUpdateContext& Context,
                                                      const FAnimNodeReference& Node)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UGASPAnimInstance::MotionMatching"),
	                            STAT_UGASPAnimInstance_MotionMatching, STATGROUP_GASP)
	TRACE_CPUPROFILER_EVENT_SCOPE(__FUNCTION__);
	if (!LocomotionTable)
	{
		return;
	}

	EAnimNodeReferenceConversionResult Result{};
	const auto Reference{UMotionMatchingAnimNodeLibrary::ConvertToMotionMatchingNode(Node, Result)};
	if (Result == EAnimNodeReferenceConversionResult::Failed)
	{
		return;
	}

	const auto Objects{
		UChooserFunctionLibrary::EvaluateChooserMulti(this, LocomotionTable, UPoseSearchDatabase::StaticClass())
	};
	TArray<UPoseSearchDatabase*> Databases{};
	Algo::Transform(Objects, Databases, [](UObject* Object) { return Cast<UPoseSearchDatabase>(Object); });

	UMotionMatchingAnimNodeLibrary::SetDatabasesToSearch(Reference, Databases, GetMatchingInterruptMode());
}

void UGASPAnimInstance::RefreshMatchingPostSelection(const FAnimUpdateContext& Context,
                                                     const FAnimNodeReference& Node)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UGASPAnimInstance::RefreshMotionMatchingPostSelection"),
	                            STAT_UGASPAnimInstance_RefreshMotionMatchingPostSelection, STATGROUP_GASP)
	TRACE_CPUPROFILER_EVENT_SCOPE(__FUNCTION__);

	EAnimNodeReferenceConversionResult Result{};
	const auto Reference{UMotionMatchingAnimNodeLibrary::ConvertToMotionMatchingNode(Node, Result)};
	if (Result == EAnimNodeReferenceConversionResult::Failed)
	{
		return;
	}

	FPoseSearchBlueprintResult OutResult{};
	bool bIsValidResult{};

	UMotionMatchingAnimNodeLibrary::GetMotionMatchingSearchResult(Reference, OutResult, bIsValidResult);
	BlendStack.PoseSearchDatabase = OutResult.SelectedDatabase;
	UPoseSearchLibrary::GetDatabaseTags(OutResult.SelectedDatabase, BlendStack.DatabaseTags);
	SearchCost = OutResult.SearchCost;
}

void UGASPAnimInstance::RefreshOffsetRoot(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	if (!bOffsetRootBoneEnabled)
	{
		return;
	}

	const auto TargetTransform{UAnimationWarpingLibrary::GetOffsetRootTransform(Node)};
	auto OffsetRotation{TargetTransform.Rotator()};
	OffsetRotation.Yaw += 90.f;

	CharacterInfo.RootTransform = {OffsetRotation, TargetTransform.GetLocation(), FVector::OneVector};
}

FQuat UGASPAnimInstance::GetDesiredFacing() const
{
	return Trajectory.GetSampleAtTime(.5).Facing;
	// UPoseSearchTrajectoryLibrary::GetTransformTrajectorySampleAtTime()
	// const auto DesiredFacing = FQuat(TargetRotation);
	// const auto Offset = FQuat({0.f, -90.f, 0.f});
	// return DesiredFacing * Offset;
}

void UGASPAnimInstance::RefreshBlendStack(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UGASPAnimInstance::RefreshBlendStack"),
	                            STAT_UGASPAnimInstance_RefreshBlendStack, STATGROUP_GASP)
	TRACE_CPUPROFILER_EVENT_SCOPE(__FUNCTION__);

	BlendStack.AnimTime = UBlendStackAnimNodeLibrary::GetCurrentBlendStackAnimAssetTime(Node);
	BlendStack.AnimAsset = UBlendStackAnimNodeLibrary::GetCurrentBlendStackAnimAsset(Node);
	BlendStack.PlayRate = GetDynamicPlayRate(Node);

	const auto* NewAnimSequence{static_cast<UAnimSequence*>(BlendStack.AnimAsset.Get())};
	UAnimationWarpingLibrary::GetCurveValueFromAnimation(NewAnimSequence, AnimNames.EnableMotionWarpingCurveName,
	                                                     BlendStack.AnimTime, BlendStack.OrientationAlpha);

	UAnimationWarpingLibrary::GetCurveValueFromAnimation(NewAnimSequence, AnimNames.SteeringTargetTime,
	                                                     BlendStack.AnimTime, BlendStack.ProceduralTargetTime);
	BlendStack.ProceduralTargetTime = FMath::GetMappedRangeValueClamped<float, float>(
		{0.f, 1.f}, {.1f, .3f}, BlendStack.ProceduralTargetTime);
}

void UGASPAnimInstance::RefreshBlendStackMachine(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UGASPAnimInstance::RefreshBlendStackMachine"),
	                            STAT_UGASPAnimInstance_RefreshBlendStackMachine, STATGROUP_GASP)
	TRACE_CPUPROFILER_EVENT_SCOPE(__FUNCTION__);

	EAnimNodeReferenceConversionResult Result{};
	const auto Reference{UBlendStackAnimNodeLibrary::ConvertToBlendStackNode(Node, Result)};
	if (Result == EAnimNodeReferenceConversionResult::Failed)
	{
		return;
	}

	BlendStackMachine.bLoop = UBlendStackAnimNodeLibrary::IsCurrentAssetLooping(Reference);
	BlendStackMachine.AssetTimeRemaining = UBlendStackAnimNodeLibrary::GetCurrentAssetTimeRemaining(Reference);
}

void UGASPAnimInstance::RefreshEssentialValues(const float DeltaSeconds)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UGASPAnimInstance::RefreshEssentialValues"),
	                            STAT_UGASPAnimInstance_RefreshEssentialValues, STATGROUP_GASP)
	TRACE_CPUPROFILER_EVENT_SCOPE(__FUNCTION__);
	CharacterInfo.ActorTransform = CachedCharacter->GetActorTransform();

	if (!bOffsetRootBoneEnabled)
	{
		CharacterInfo.RootTransform = CharacterInfo.ActorTransform;
	}

	const auto& InputState = CachedCharacter->GetMoverInputs_PostSim();

	MovementDirection = InputState.MovementDirection;
	CharacterInfo.Acceleration = InputState.GetMoveInput();

	// Refresh velocity variables
	CharacterInfo.Velocity = CachedMovement->GetVelocity();
	CharacterInfo.Speed = CharacterInfo.Velocity.Size2D();

	// Calculate rate of change velocity
	CharacterInfo.VelocityAcceleration = (CharacterInfo.Velocity - PreviousCharacterInfo.Velocity) / FMath::Max(
		DeltaSeconds, .001f);
	CharacterInfo.RelativeAcceleration = CharacterInfo.RootTransform.Rotator().UnrotateVector(
		CharacterInfo.VelocityAcceleration);

	if (IsMoving())
	{
		BlendStack.LastNonZeroVector = CharacterInfo.Velocity;
	}

	CharacterInfo.SmoothedGroundNormal = FMath::VInterpTo(CharacterInfo.SmoothedGroundNormal, InputState.FloorNormal,
	                                                      DeltaSeconds, 5.f);
}

void UGASPAnimInstance::RefreshRagdollValues(const float DeltaSeconds)
{
	static constexpr auto ReferenceSpeed{1000.0f};
	RagdollingState.FlailPlayRate = FMath::Clamp(
		UE_REAL_TO_FLOAT(CachedCharacter->GetRagdollingState().Velocity.Size() / ReferenceSpeed), 0.f, 1.f);
}

bool UGASPAnimInstance::IsEnabledAO() const
{
	return FMath::Abs(GetAOValue().X) <= 115.f && RotationMode != RotationTags::OrientToMovement &&
		GetSlotMontageLocalWeight(FName{TEXT("DefaultSlot")}) < .5f;
}

FVector2D UGASPAnimInstance::GetAOValue() const
{
	if (!CachedCharacter.IsValid())
	{
		return FVector2D::ZeroVector;
	}

	const auto ControlRot = CachedCharacter->GetMoverInputs_PostSim().AimingRotation;

	const FRotator RootRot = CharacterInfo.RootTransform.Rotator();
	FRotator DeltaRot = (ControlRot - RootRot).GetNormalized();

	const float DisableBlend = GetCurveValue(AnimNames.DisableAOCurveName);
	return FMath::Lerp({DeltaRot.Yaw, DeltaRot.Pitch}, FVector2D::ZeroVector, DisableBlend);
}

bool UGASPAnimInstance::IsCircling() const
{
	return FMath::Abs(GetTrajectoryTurnAngle()) > 50.f && ((TrajectoryInfo.PastAngularVelocity.Z <
		-200.f && TrajectoryInfo.CurrentAngularVelocity.Z < -200.f) || (TrajectoryInfo.PastAngularVelocity.Z >
		200.f && TrajectoryInfo.CurrentAngularVelocity.Z > 200.f));
}

bool UGASPAnimInstance::CanOverlayTransition() const
{
	return StanceMode == StanceTags::Standing && !IsMoving();
}

void UGASPAnimInstance::RefreshOverlaySettings(float DeltaTime)
{
	const float ClampedYawAxis = FMath::ClampAngle(GetAOValue().X, -90.f, 90.f) / 6.f;
	SpineRotation.Yaw = FMath::FInterpTo(SpineRotation.Yaw, ClampedYawAxis, DeltaTime, 60.f);
}

void UGASPAnimInstance::RefreshLayering(float DeltaTime)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UGASPAnimInstance::RefreshLayering"),
	                            STAT_UGASPAnimInstance_RefreshLayering, STATGROUP_GASP)
	TRACE_CPUPROFILER_EVENT_SCOPE(__FUNCTION__);

	LayeringState.SpineAdditiveBlendAmount = GetCurveValue(AnimNames.LayeringSpineAdditiveName);
	LayeringState.HeadAdditiveBlendAmount = GetCurveValue(AnimNames.LayeringHeadAdditiveName);

	LayeringState.ArmLeftAdditiveBlendAmount = GetCurveValue(AnimNames.LayeringArmLeftAdditiveName);
	LayeringState.ArmRightAdditiveBlendAmount = GetCurveValue(AnimNames.LayeringArmRightAdditiveName);

	LayeringState.HandLeftBlendAmount = GetCurveValue(AnimNames.LayeringHandLeftName);
	LayeringState.HandRightBlendAmount = GetCurveValue(AnimNames.LayeringHandRightName);

	LayeringState.EnableHandLeftIKBlend = FMath::Lerp(0.f, GetCurveValue(AnimNames.LayeringHandLeftIKName),
	                                                  GetCurveValue(AnimNames.LayeringArmLeftName));
	LayeringState.EnableHandRightIKBlend = FMath::Lerp(0.f, GetCurveValue(AnimNames.LayeringHandRightIKName),
	                                                   GetCurveValue(AnimNames.LayeringArmRightName));

	LayeringState.ArmLeftLocalSpaceBlendAmount = GetCurveValue(AnimNames.LayeringArmLeftLocalSpaceName);
	LayeringState.ArmLeftMeshSpaceBlendAmount = UE_REAL_TO_FLOAT(
		1.f - FMath::FloorToInt(LayeringState.ArmLeftLocalSpaceBlendAmount));

	LayeringState.ArmRightLocalSpaceBlendAmount = GetCurveValue(AnimNames.LayeringArmRightLocalSpaceName);
	LayeringState.ArmRightMeshSpaceBlendAmount = UE_REAL_TO_FLOAT(
		1.f - FMath::FloorToInt(LayeringState.ArmRightLocalSpaceBlendAmount));

	BlendPoses.BasePoseN = FMath::FInterpTo(BlendPoses.BasePoseN,
	                                        StanceMode == StanceTags::Standing ? 1.f : 0.f,
	                                        DeltaTime, 15.f);
	BlendPoses.BasePoseCLF = FMath::GetMappedRangeValueClamped<float, float>(
		{0.f, 1.f}, {1.f, 0.f}, BlendPoses.BasePoseN);
}

void UGASPAnimInstance::SetBlendStackAnimFromChooser(const FAnimNodeReference& Node, const EStateMachineState NewState,
                                                     const bool bForceBlend)
{
	StateMachineState = NewState;
	PreviousBlendStackInputs = BlendStackInputs;

	bNoValidAnim = bNotifyTransition_ReTransition = bNotifyTransition_ToLoop = false;

	if (!StateMachineTable)
	{
		return;
	}

	FGASPChooserOutputs ChooserOutputs;
	auto Context = UChooserFunctionLibrary::MakeChooserEvaluationContext();
	Context.AddObjectParam(this);
	Context.AddStructParam(ChooserOutputs);

	const auto Objects{
		UChooserFunctionLibrary::EvaluateObjectChooserBaseMulti(
			Context, UChooserFunctionLibrary::MakeEvaluateChooser(StateMachineTable),
			UAnimationAsset::StaticClass())
	};

	if (Objects.IsEmpty())
	{
		bNoValidAnim = true;
		return;
	}

	// Update blend stack inputs
	BlendStackInputs.AnimationAsset = static_cast<UAnimationAsset*>(Objects[0]);
	UPoseSearchLibrary::IsAnimationAssetLooping(Objects[0], BlendStackInputs.bLoop);
	BlendStackInputs.StartTime = ChooserOutputs.StartTime;
	BlendStackInputs.BlendTime = ChooserOutputs.BlendTime;
	BlendStackInputs.BlendProfile = GetBlendProfileByName(ChooserOutputs.BlendProfile);
	BlendStack.DatabaseTags = ChooserOutputs.Tags;

	if (ChooserOutputs.bUseMotionMatching)
	{
		FPoseSearchBlueprintResult PoseSearchResult;
		UPoseSearchLibrary::MotionMatch(this, Objects, AnimNames.PoseHistoryTag, FPoseSearchContinuingProperties(),
		                                FPoseSearchFutureProperties(), PoseSearchResult);

		SearchCost = PoseSearchResult.SearchCost;

		auto AnimationAsset = static_cast<UAnimationAsset*>(PoseSearchResult.SelectedAnim);

		const bool bIsCostAcceptable = ChooserOutputs.MMCostLimit > 0.f
			                               ? PoseSearchResult.SearchCost <= ChooserOutputs.MMCostLimit
			                               : true;
		if (!IsValid(AnimationAsset) && bIsCostAcceptable)
		{
			bNoValidAnim = true;
			return;
		}

		BlendStackInputs.AnimationAsset = AnimationAsset;
		UPoseSearchLibrary::IsAnimationAssetLooping(BlendStackInputs.AnimationAsset.Get(), BlendStackInputs.bLoop);
		BlendStackInputs.StartTime = PoseSearchResult.SelectedTime;
	}

	if (bForceBlend)
	{
		EAnimNodeReferenceConversionResult Result{};
		const auto Reference{UBlendStackAnimNodeLibrary::ConvertToBlendStackNode(Node, Result)};
		if (Result == EAnimNodeReferenceConversionResult::Succeeded)
		{
			UBlendStackAnimNodeLibrary::ForceBlendNextUpdate(Reference);
		}
	}
}

bool UGASPAnimInstance::IsAnimationAlmostComplete() const
{
	return !BlendStackMachine.bLoop && BlendStackMachine.AssetTimeRemaining <= .75f;
}

float UGASPAnimInstance::GetDynamicPlayRate(const FAnimNodeReference& Node) const
{
	static const FName EnablePlayRateWarpingCurveName = TEXT("Enable_PlayRateWarping");
	static const FName MoveDataSpeedCurveName = TEXT("MoveData_Speed");
	static const FName MaxDynamicPlayRateCurveName = TEXT("MaxDynamicPlayRate");
	static const FName MinDynamicPlayRateCurveName = TEXT("MinDynamicPlayRate");

	const auto AnimSequence{
		static_cast<UAnimSequence*>(UBlendStackAnimNodeLibrary::GetCurrentBlendStackAnimAsset(Node))
	};
	if (!IsValid(AnimSequence))
	{
		return 1.f;
	}

	const float AnimTime = UBlendStackAnimNodeLibrary::GetCurrentBlendStackAnimAssetTime(Node);

	float AlphaCurve{0.f};
	float SpeedCurve{0.f};
	float MaxDynamicPlayRate;
	float MinDynamicPlayRate;

	if (!UAnimationWarpingLibrary::GetCurveValueFromAnimation(AnimSequence, EnablePlayRateWarpingCurveName, AnimTime,
	                                                          AlphaCurve))
	{
		return 1.f;
	}

	if (!UAnimationWarpingLibrary::GetCurveValueFromAnimation(AnimSequence, MoveDataSpeedCurveName, AnimTime,
	                                                          SpeedCurve))
	{
		return 1.f;
	}

	if (!UAnimationWarpingLibrary::GetCurveValueFromAnimation(AnimSequence, MaxDynamicPlayRateCurveName, AnimTime,
	                                                          MaxDynamicPlayRate))
	{
		MaxDynamicPlayRate = 3.f;
	}

	if (!UAnimationWarpingLibrary::GetCurveValueFromAnimation(AnimSequence, MinDynamicPlayRateCurveName, AnimTime,
	                                                          MinDynamicPlayRate))
	{
		MinDynamicPlayRate = .5f;
	}

	const float SpeedRatio = CharacterInfo.Speed / FMath::Clamp(SpeedCurve, 1.f, UE_MAX_FLT);

	return FMath::Lerp(1.f, FMath::Clamp(SpeedRatio, MinDynamicPlayRate, MaxDynamicPlayRate), AlphaCurve);
}

void UGASPAnimInstance::OnStateEntryIdleLoop(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	SetBlendStackAnimFromChooser(Node, EStateMachineState::IdleLoop);
}

void UGASPAnimInstance::OnStateEntryTransitionToIdleLoop(const FAnimUpdateContext& Context,
                                                         const FAnimNodeReference& Node)
{
	SetBlendStackAnimFromChooser(Node, EStateMachineState::TransitionToIdleLoop, true);
}

void UGASPAnimInstance::OnStateEntryLocomotionLoop(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	TargetRotationOnTransitionStart = TargetRotation;
	SetBlendStackAnimFromChooser(Node, EStateMachineState::LocomotionLoop);
}

void UGASPAnimInstance::OnStateEntryTransitionToLocomotionLoop(const FAnimUpdateContext& Context,
                                                               const FAnimNodeReference& Node)
{
	TargetRotationOnTransitionStart = TargetRotation;
	SetBlendStackAnimFromChooser(Node, EStateMachineState::TransitionToLocomotionLoop, true);
}

void UGASPAnimInstance::OnUpdateTransitionToLocomotionLoop(const FAnimUpdateContext& Context,
                                                           const FAnimNodeReference& Node)
{
	TargetRotationOnTransitionStart = FMath::RInterpTo(TargetRotationOnTransitionStart, TargetRotation,
	                                                   GetDeltaSeconds(), 5.f);
}

void UGASPAnimInstance::OnStateEntryInAirLoop(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	SetBlendStackAnimFromChooser(Node, EStateMachineState::InAirLoop);
}

void UGASPAnimInstance::OnStateEntryTransitionToInAirLoop(const FAnimUpdateContext& Context,
                                                          const FAnimNodeReference& Node)
{
	SetBlendStackAnimFromChooser(Node, EStateMachineState::TransitionToInAirLoop, true);
}

void UGASPAnimInstance::OnStateEntryIdleBreak(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	SetBlendStackAnimFromChooser(Node, EStateMachineState::IdleBreak, true);
}

void UGASPAnimInstance::RefreshTargetRotation()
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UGASPAnimInstance::RefreshTargetRotation"),
	                            STAT_UGASPAnimInstance_RefreshTargetRotation, STATGROUP_GASP)
	TRACE_CPUPROFILER_EVENT_SCOPE(__FUNCTION__);

	if (IsMoving())
	{
		if (RotationMode == RotationTags::OrientToMovement)
		{
			TargetRotation = CharacterInfo.ActorTransform.Rotator();
		}
		else
		{
			TargetRotation = CharacterInfo.ActorTransform.Rotator();
			TargetRotation.Yaw += GetStrafeYawRotationOffset();
		}
	}
	else
	{
		TargetRotation = CharacterInfo.ActorTransform.Rotator();
	}

	TargetRotationDelta = (TargetRotation - CharacterInfo.RootTransform.Rotator()).GetNormalized().Yaw;
}

float UGASPAnimInstance::GetStrafeYawRotationOffset() const
{
	if (!IsValid(StrafeCurveAnimationAsset))
	{
		return 0.f;
	}

	static const TMap<EMovementDirection, FName> CurveNames = {
		{EMovementDirection::B, TEXT("StrafeOffset_B")},
		{EMovementDirection::LL, TEXT("StrafeOffset_LL")},
		{EMovementDirection::LR, TEXT("StrafeOffset_LR")},
		{EMovementDirection::RL, TEXT("StrafeOffset_RL")},
		{EMovementDirection::RR, TEXT("StrafeOffset_RR")},
		{EMovementDirection::F, TEXT("StrafeOffset_F")}
	};

	const float Dir = FGASPMath::CalculateDirection(TrajectoryInfo.FutureVelocity.GetSafeNormal(),
	                                                CharacterInfo.ActorTransform.Rotator());
	const float MappedDirection = FMath::GetMappedRangeValueClamped<float, float>({-180.f, 180.f},
		{0.f, 8.f}, Dir) / 30.f;

	const auto* CurveName{CurveNames.Find(MovementDirection)};
	if (!CurveName)
	{
		CurveName = CurveNames.Find(EMovementDirection::F);
	}

	float CurveValue;
	UAnimationWarpingLibrary::GetCurveValueFromAnimation(StrafeCurveAnimationAsset, *CurveName, MappedDirection,
	                                                     CurveValue);
	return CurveValue;
}
