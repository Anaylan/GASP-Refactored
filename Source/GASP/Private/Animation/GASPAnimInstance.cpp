#include "Animation/GASPAnimInstance.h"
#include "Actors/GASPCharacter.h"
#include "PoseSearch/PoseSearchDatabase.h"
#include "Utils/GASPMath.h"
#include "PoseSearch/MotionMatchingAnimNodeLibrary.h"
#include "ChooserFunctionLibrary.h"
#include "PoseSearch/PoseSearchLibrary.h"
#include "AnimationWarpingLibrary.h"
#include "BlendStack/BlendStackAnimNodeLibrary.h"
#include "Interfaces/GASPHeldObjectInterface.h"
#include "MoverPoseSearchTrajectoryPredictor.h"
#include "MovementSet/GASPMoverComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPAnimInstance)

namespace AnimVars
{
	int32 LocomotionSetup{false};
	FAutoConsoleVariableRef CVarLocomotionStyleStruct(
		TEXT("gasp.locomotion.style"), LocomotionSetup, TEXT("locomotion style: \n"
			"0 - MotionMatching\n"
			"1 - StateMachine"),
		ECVF_Default);

	bool bOffsetRootBoneEnabled{true};
	FAutoConsoleVariableRef CVarOffsetRootBoneEnabledStruct(
		TEXT("gasp.offsetrootbone.enabled"), bOffsetRootBoneEnabled, TEXT("enable offset root bone"),
		ECVF_Default);

	bool bFootPlacementEnabled{true};
	FAutoConsoleVariableRef CVarFootPlacementEnabled(
		TEXT("gasp.footplacement.enabled"), bFootPlacementEnabled, TEXT("enable foot placement"),
		ECVF_Default);

	int32 MMDatabaseLOD{0};
	FAutoConsoleVariableRef CVarMMDatabaseLODStruct(
		TEXT("gasp.motionmatching.LOD"), MMDatabaseLOD, TEXT("LOD for motion matching database"),
		ECVF_Default);
}

EPoseSearchInterruptMode UGASPAnimInstance::GetMatchingInterruptMode() const
{
	return MovementMode.Current != MovementMode.LastFrame || MovementMode.Current == MovementModeTags::Grounded && (
		       MovementState.Current != MovementState.LastFrame || (Gait.Current != Gait.LastFrame && MovementState.
			       Current == MovementStateTags::Moving) || StanceMode.Current != StanceMode.LastFrame) || (
		       MovementDirection.Current != MovementDirection.LastFrame && MovementState.Current ==
		       MovementStateTags::Moving)
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

	return MovementMode.Current == MovementModeTags::Grounded && MovementState.Current == MovementStateTags::Moving
		       ? EOffsetRootBoneMode::Interpolate
		       : EOffsetRootBoneMode::Release;
}

float UGASPAnimInstance::GetOffsetRootTranslationHalfLife() const
{
	return MovementState.Current == MovementStateTags::Moving ? .3f : .1f;
}

EOrientationWarpingSpace UGASPAnimInstance::GetOrientationWarpingSpace() const
{
	return OffsetRootBoneEnabled
		       ? EOrientationWarpingSpace::RootBoneTransform
		       : EOrientationWarpingSpace::ComponentTransform;
}

float UGASPAnimInstance::GetAOYaw() const
{
	return RotationMode.Current == RotationTags::OrientToMovement ? 0.f : GetAOValue().X;
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
	return ((BlendStackInputs.bLoop || BlendStack.bActive) && IsMoving()) || MovementMode.Current ==
		MovementModeTags::InAir || MovementMode.Current == MovementModeTags::Slide;
}

bool UGASPAnimInstance::JustTeleported() const
{
	return FVector::DistSquared(PreviousCharacterInfo.ActorTransform.GetTranslation(),
	                            CharacterInfo.ActorTransform.GetTranslation()) > AnimConfiguration.TeleportThreshold *
		AnimConfiguration.TeleportThreshold;
}

bool UGASPAnimInstance::AllowFootPinning() const
{
	return MovementMode.Current == MovementModeTags::Grounded;
}

bool UGASPAnimInstance::AllowSlopeWarping() const
{
	return MovementMode.Current == MovementModeTags::Grounded || MovementMode.Current == MovementModeTags::Slide;
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

	CachedCharacter->OverlayModeChanged.AddUniqueDynamic(this, &ThisClass::OnOverlayModeChanged);
	CachedCharacter->PoseModeChanged.AddUniqueDynamic(this, &ThisClass::OnPoseModeChanged);
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

	Predictor = NewObject<UMoverTrajectoryPredictor>(CachedMovement.Get());
	Predictor->Setup(CachedMovement.Get());

	LocomotionSetup = AnimVars::LocomotionSetup;
	OffsetRootBoneEnabled = AnimVars::bOffsetRootBoneEnabled;
	MMDatabaseLOD = AnimVars::MMDatabaseLOD;
	FootPlacementEnabled = AnimVars::bFootPlacementEnabled;

	AnimVars::CVarLocomotionStyleStruct->OnChangedDelegate().AddLambda([this](const IConsoleVariable* ICVar)
	{
		LocomotionSetup = ICVar ? ICVar->GetInt() : false;
	});
	AnimVars::CVarOffsetRootBoneEnabledStruct->OnChangedDelegate().AddLambda([this](const IConsoleVariable* ICVar)
	{
		OffsetRootBoneEnabled = ICVar ? ICVar->GetBool() : false;
	});
	AnimVars::CVarMMDatabaseLODStruct->OnChangedDelegate().AddLambda([this](const IConsoleVariable* ICVar)
	{
		MMDatabaseLOD = ICVar ? ICVar->GetInt() : false;
	});
	AnimVars::CVarLocomotionStyleStruct->OnChangedDelegate().AddLambda([this](const IConsoleVariable* ICVar)
	{
		FootPlacementEnabled = ICVar ? ICVar->GetBool() : false;
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

	const auto& InputState{CachedCharacter->GetMoverState()};
	const FGameplayTag NewGait = InputState.Gait == GaitTags::Sprint && IsCircling() ? GaitTags::Run : InputState.Gait;

	Gait.Update(NewGait, DeltaSeconds, .1f);
	RotationMode.Update(InputState.RotationMode, DeltaSeconds, .1f);
	MovementState.Update(IsMoving() ? MovementStateTags::Moving : MovementStateTags::Idle, DeltaSeconds, .1f);
	MovementMode.Update(CachedCharacter->GetMovementMode(), DeltaSeconds, .1f);
	StanceMode.Update(InputState.Stance, DeltaSeconds, .1f);
	MovementDirection.Update(InputState.MovementDirection, DeltaSeconds, .1f);

	RefreshStateContainer();
	RefreshEssentialValues(DeltaSeconds);
	RefreshTrajectory(DeltaSeconds);
	RefreshOverlaySettings(DeltaSeconds);
	RefreshLayering(DeltaSeconds);

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

void UGASPAnimInstance::NativePostEvaluateAnimation()
{
	Super::NativePostEvaluateAnimation();

	if (ForceFootPlacementReset)
	{
		ForceFootPlacementReset = false;
	}
}

void UGASPAnimInstance::PreUpdateAnimation(float DeltaSeconds)
{
	PreviousCharacterInfo = CharacterInfo;
	PreviousLocomotionAction = LocomotionAction;

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
		GetWorld(), this, OutTrajectory, true, .01f,
		Trajectory, TrajectoryCollision, UEngineTypes::ConvertToTraceType(ECC_Visibility), false,
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

	if (FMath::Abs(TrajectoryInfo.FutureFacingDelta - TrajectoryInfo.PreviousFutureFacingDelta) > 200.f && RotationMode.
		Current != RotationTags::OrientToMovement)
	{
		MovementDirection.Current = MovementDirection.Recent = EMovementDirection::B;
	}

	UPoseSearchTrajectoryLibrary::GetTransformTrajectoryAngularVelocity(
		Trajectory, -.4f, -.3f, TrajectoryInfo.PastAngularVelocity);
	UPoseSearchTrajectoryLibrary::GetTransformTrajectoryAngularVelocity(
		Trajectory, 0.f, .1f, TrajectoryInfo.CurrentAngularVelocity);

	TrajectoryInfo.CirclingTime = IsCircling() ? TrajectoryInfo.CirclingTime + DeltaSeconds : 0.f;
}

void UGASPAnimInstance::RefreshStateContainer()
{
	RecentStateContainer.Reset();
	PreviousStateContainer.Reset();
	StateContainer.Reset();

	RecentStateContainer.AddTag(MovementMode.Recent);
	RecentStateContainer.AddTag(Gait.Recent);
	RecentStateContainer.AddTag(MovementState.Recent);
	RecentStateContainer.AddTag(RotationMode.Recent);
	RecentStateContainer.AddTag(StanceMode.Recent);

	PreviousStateContainer.AddTag(MovementMode.LastFrame);
	PreviousStateContainer.AddTag(Gait.LastFrame);
	PreviousStateContainer.AddTag(MovementState.LastFrame);
	PreviousStateContainer.AddTag(RotationMode.LastFrame);
	PreviousStateContainer.AddTag(StanceMode.LastFrame);

	StateContainer.AddTag(MovementMode.Current);
	StateContainer.AddTag(Gait.Current);
	StateContainer.AddTag(MovementState.Current);
	StateContainer.AddTag(RotationMode.Current);
	StateContainer.AddTag(StanceMode.Current);
}

float UGASPAnimInstance::GetTotalFacingDelta(TArray<float> Times) const
{
	TArray<float> Rotations{};
	for (const float Time : Times)
	{
		Rotations.Add(Trajectory.GetSampleAtTime(Time).Facing.Rotator().Yaw);
	}

	float AngleSum{static_cast<float>(Rotations[0] - CharacterInfo.RootOffsetRotation.Yaw)};
	for (int32 Index = 0; Index < Rotations.Num() - 2; ++Index)
	{
		AngleSum += Rotations[Index + 1] - Rotations[Index];
	}

	return AngleSum;
}

float UGASPAnimInstance::GetMatchingBlendTime() const
{
	if (MovementMode.Current == MovementModeTags::InAir)
	{
		return CharacterInfo.Velocity.Z > 100.f ? .15f : .5f;
	}

	return MovementMode.LastFrame == MovementModeTags::Grounded ? .5f : .2f;
}

FFloatInterval UGASPAnimInstance::GetMatchingPlayRate() const
{
	if (MovementMode.Current == MovementModeTags::Grounded)
	{
		return {.75f, 3.f};
	}
	return {.75f, 1.25f};
}

FVector UGASPAnimInstance::GetStrafeWarpDirection() const
{
	return BlendStack.LastNonZeroVector + FMath::GetMappedRangeValueClamped<float, float>(
		{20.f, 100.f}, {0.f, 1.f},
		FMath::Abs(TrajectoryInfo.CurrentAngularVelocity.Z)) * (TrajectoryInfo.NearFutureVelocity - BlendStack.
		LastNonZeroVector);
}

FVector UGASPAnimInstance::GetSlideSlopeOffset() const
{
	const FVector RootLocation = CharacterInfo.RootTransform.GetTranslation();

	return FVector::PointPlaneProject(RootLocation, CharacterInfo.FloorLocation, CharacterInfo.SmoothedGroundNormal) -
		RootLocation;
}

FRotator UGASPAnimInstance::GetSlideSlopeRotation() const
{
	const auto RootQuat{CharacterInfo.RootTransform.GetRotation()};
	const auto ActorQuat{CharacterInfo.ActorTransform.GetRotation()};

	const auto RightVector{RootQuat.GetRightVector()};
	const auto UpVector{ActorQuat.GetUpVector()};
	const auto FloorNormal{CharacterInfo.SmoothedGroundNormal};

	const auto FloorXAxis{FVector::CrossProduct(RightVector, FloorNormal).GetSafeNormal()};
	const auto FloorYAxis{FVector::CrossProduct(FloorNormal, FloorXAxis).GetSafeNormal()};

	const float PitchAngle{
		static_cast<float>(90.f - FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(FloorYAxis, UpVector))))
	};

	const float RollAngle{
		static_cast<float>(90.f - FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(FloorXAxis, UpVector))))
	};

	return {-PitchAngle, 0.f, -RollAngle};
}

float UGASPAnimInstance::GetMatchingNotifyRecencyTimeOut() const
{
	if (Gait.Current == GaitTags::Sprint)
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
	return TrajectoryInfo.FutureVelocity.Size2D() >= CharacterInfo.Velocity.Size2D() + 100.f && !BlendStack.
		DatabaseTags.Contains(AnimNames.PivotsTag) && IsMoving() && CharacterInfo.Speed <= 100.f;
}

bool UGASPAnimInstance::IsPivoting() const
{
	if (LocomotionSetup == 0)
	{
		return FMath::Abs(GetTrajectoryTurnAngle()) >= 75.f && !IsCircling() && MovementState.Current ==
			MovementStateTags::Moving;
	}

	float MinSpeed{175.f}, MaxSpeed{600.f}, AngleThreshold{75.f};

	if (StanceMode.Current == StanceTags::Crouching)
	{
		MinSpeed = 50.f;
		MaxSpeed = 200.f;
	}
	else if (Gait.Current == GaitTags::Sprint)
	{
		MinSpeed = 200.f;
		MaxSpeed = 700.f;
		AngleThreshold = 60.f;
	}
	else if (Gait.Current == GaitTags::Walk)
	{
		MinSpeed = 50.f;
		MaxSpeed = 300.f;
	}

	if (Gait.Current != GaitTags::Sprint && (MovementMode.Recent == MovementModeTags::InAir || MovementMode.Recent ==
		MovementModeTags::Slide))
	{
		AngleThreshold = 100.f;
	}

	return FMath::Abs(GetTrajectoryTurnAngle()) >= AngleThreshold && FMath::IsWithinInclusive(
		CharacterInfo.Speed, MinSpeed, MaxSpeed) && MovementState.Current == MovementStateTags::Moving;
}

bool UGASPAnimInstance::IsMoving() const
{
	return !TrajectoryInfo.FutureVelocity.IsNearlyZero(10.f) && !CharacterInfo.Acceleration.IsZero();
}

bool UGASPAnimInstance::ShouldTurnInPlace() const
{
	return CharacterInfo.Speed < 50.f && FMath::Abs(TrajectoryInfo.FutureFacingDelta) >= AnimConfiguration.
		MaxTurnInPlaceAngle && MovementState.Current == MovementStateTags::Idle;
}

bool UGASPAnimInstance::ShouldSpinTransition() const
{
	return FMath::Abs(TrajectoryInfo.FutureFacingDelta) >= AnimConfiguration.SpinTransitionAngle && CharacterInfo.Speed
		>= 150.f && !BlendStack.DatabaseTags.Contains(AnimNames.PivotsTag);
}

bool UGASPAnimInstance::JustLanded_Light() const
{
	return FMath::Abs(PreviousCharacterInfo.Velocity.Z) < FMath::Abs(HeavyLandSpeedThreshold) && MovementMode.Current
		== MovementModeTags::Grounded && MovementMode.LastFrame == MovementModeTags::InAir;
}

bool UGASPAnimInstance::JustLanded_Heavy() const
{
	return FMath::Abs(PreviousCharacterInfo.Velocity.Z) >= FMath::Abs(HeavyLandSpeedThreshold) && MovementMode.Current
		== MovementModeTags::Grounded && MovementMode.LastFrame == MovementModeTags::InAir;
}

bool UGASPAnimInstance::JustTraversed() const
{
	return !IsSlotActive(FName{TEXT("DefaultSlot")}) && GetCurveValue(AnimNames.MovingTraversalCurveName) > 0.f &&
		FMath::Abs(GetTrajectoryTurnAngle()) <= 50.f;
}

bool UGASPAnimInstance::PlayLand() const
{
	return MovementMode.Current == MovementModeTags::Grounded && MovementMode.LastFrame ==
		MovementModeTags::InAir;
}

bool UGASPAnimInstance::PlayMovingLand() const
{
	return MovementMode.Current == MovementModeTags::Grounded && MovementMode.LastFrame ==
		MovementModeTags::InAir &&
		FMath::Abs(GetTrajectoryTurnAngle()) <= 120.f;
}

float UGASPAnimInstance::GetTrajectoryTurnAngle() const
{
	const FVector2D CurrentVelocity2D(CharacterInfo.Velocity.X, CharacterInfo.Velocity.Y);
	const FVector2D FutureVelocity2D(TrajectoryInfo.FutureVelocity.X, TrajectoryInfo.FutureVelocity.Y);

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

	switch (MovementDirection.Current)
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
}

void UGASPAnimInstance::RefreshMotionMatchingMovement(const FAnimUpdateContext& Context,
                                                      const FAnimNodeReference& Node)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UGASPAnimInstance::MotionMatching"),
	                            STAT_UGASPAnimInstance_MotionMatching, STATGROUP_GASP)
	TRACE_CPUPROFILER_EVENT_SCOPE(__FUNCTION__);

	EAnimNodeReferenceConversionResult Result{};
	const auto Reference{UMotionMatchingAnimNodeLibrary::ConvertToMotionMatchingNode(Node, Result)};
	if (Result == EAnimNodeReferenceConversionResult::Failed)
	{
		return;
	}

	BlendStack.Databases = UChooserFunctionLibrary::EvaluateChooserMulti(
		this, MotionMatchingTable, UPoseSearchDatabase::StaticClass());
	if (BlendStack.Databases.IsEmpty())
	{
		return;
	}
	
	TArray<UPoseSearchDatabase*> Databases;
	Algo::Transform(BlendStack.Databases, Databases, [](UObject* Object)
	{
		return static_cast<UPoseSearchDatabase*>(Object);
	});

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
	auto TargetTransform{UAnimationWarpingLibrary::GetOffsetRootTransform(Node)};
	CharacterInfo.RootOffsetRotation = TargetTransform.Rotator();
	if (!OffsetRootBoneEnabled)
	{
		return;
	}

	auto OffsetRotation{CharacterInfo.RootOffsetRotation};
	OffsetRotation.Yaw += 90.f;

	CharacterInfo.RootTransform = {OffsetRotation, TargetTransform.GetLocation(), FVector::OneVector};
}

FQuat UGASPAnimInstance::GetDesiredFacing() const
{
	return Trajectory.GetSampleAtTime(BlendStack.DesiredFacingTime).Facing;
}

void UGASPAnimInstance::RefreshBlendStack(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UGASPAnimInstance::RefreshBlendStack"),
	                            STAT_UGASPAnimInstance_RefreshBlendStack, STATGROUP_GASP)
	TRACE_CPUPROFILER_EVENT_SCOPE(__FUNCTION__);

	BlendStack.AnimTime = UBlendStackAnimNodeLibrary::GetCurrentBlendStackAnimAssetTime(Node);
	BlendStack.AnimAsset = UBlendStackAnimNodeLibrary::GetCurrentBlendStackAnimAsset(Node);
	BlendStack.PlayRate = GetDynamicPlayRate(Node);
	BlendStack.bActive = UBlendStackAnimNodeLibrary::GetCurrentBlendStackAnimIsActive(Node);

	const auto* NewAnimSequence{static_cast<UAnimSequence*>(BlendStack.AnimAsset.Get())};

	float ProceduralTargetTime;
	UAnimationWarpingLibrary::GetCurveValueFromAnimation(NewAnimSequence, AnimNames.SteeringTargetTime,
	                                                     BlendStack.AnimTime, ProceduralTargetTime);

	BlendStack.DesiredFacingTime = FMath::GetMappedRangeValueClamped<float, float>(
		{0.f, 1.f}, {.1f, 1.5f}, ProceduralTargetTime);
	BlendStack.ProceduralTargetTime = FMath::GetMappedRangeValueClamped<float, float>(
		{0.f, 1.f}, {.1f, .3f}, ProceduralTargetTime);


	float WarpingValue;
	UAnimationWarpingLibrary::GetCurveValueFromAnimation(NewAnimSequence, AnimNames.EnableWarpingCurveName,
	                                                     BlendStack.AnimTime, BlendStack.OrientationAlpha);
	UAnimationWarpingLibrary::GetCurveValueFromAnimation(NewAnimSequence, AnimNames.EnableStrafeWarpingName,
	                                                     BlendStack.AnimTime, WarpingValue);
	BlendStack.StrafeWarpAlpha = FMath::Clamp(BlendStack.OrientationAlpha + WarpingValue, 0.f, 1.f);
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

void UGASPAnimInstance::OnBecomeRelevantFootPlacement(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	ForceFootPlacementReset = true;
}

void UGASPAnimInstance::RefreshEssentialValues(const float DeltaSeconds)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UGASPAnimInstance::RefreshEssentialValues"),
	                            STAT_UGASPAnimInstance_RefreshEssentialValues, STATGROUP_GASP)
	TRACE_CPUPROFILER_EVENT_SCOPE(__FUNCTION__);
	CharacterInfo.ActorTransform = CachedCharacter->GetActorTransform();

	if (!OffsetRootBoneEnabled)
	{
		CharacterInfo.RootTransform = CharacterInfo.ActorTransform;
	}

	const auto InputState = CachedCharacter->GetMoverState();
	CharacterInfo.Acceleration = CachedMovement->GetMovementIntent();
	CharacterInfo.FloorLocation = InputState.FloorLocation;
	CharacterInfo.FloorNormal = InputState.FloorNormal;

	// Refresh velocity variables
	CharacterInfo.Velocity = CachedMovement->GetVelocity();
	CharacterInfo.Speed = CharacterInfo.Velocity.Size2D();

	// Calculate rate of change velocity
	CharacterInfo.VelocityAcceleration = (CharacterInfo.Velocity - PreviousCharacterInfo.Velocity) / FMath::Max(
		DeltaSeconds, .001f);
	CharacterInfo.RelativeAcceleration = CharacterInfo.RootTransform.Rotator().UnrotateVector(
		CharacterInfo.VelocityAcceleration);

	if (CharacterInfo.Velocity.Size() > 0.f)
	{
		BlendStack.LastNonZeroVector = CharacterInfo.Velocity;
	}

	CharacterInfo.SmoothedGroundNormal = FMath::VInterpTo(CharacterInfo.SmoothedGroundNormal, CharacterInfo.FloorNormal,
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
	return FMath::Abs(GetAOValue().X) <= 115.f && RotationMode.Current != RotationTags::OrientToMovement &&
		GetSlotMontageLocalWeight(FName{TEXT("DefaultSlot")}) < .5f;
}

FVector2D UGASPAnimInstance::GetAOValue() const
{
	if (!CachedCharacter.IsValid())
	{
		return FVector2D::ZeroVector;
	}

	const auto ControlRot{CachedCharacter->GetMoverState().ControlRotation};
	const auto RootRot{CharacterInfo.RootTransform.Rotator()};
	auto DeltaRot{(ControlRot - RootRot).GetNormalized()};

	const float DisableBlend = GetCurveValue(AnimNames.DisableAOCurveName);
	return FMath::Lerp({DeltaRot.Yaw, DeltaRot.Pitch}, FVector2D::ZeroVector, DisableBlend);
}

bool UGASPAnimInstance::IsCircling() const
{
	return FMath::Abs(GetTrajectoryTurnAngle()) > 50.f && ((TrajectoryInfo.PastAngularVelocity.Z < -200.f &&
		TrajectoryInfo.CurrentAngularVelocity.Z < -200.f) || (TrajectoryInfo.PastAngularVelocity.Z > 200.f &&
		TrajectoryInfo.CurrentAngularVelocity.Z > 200.f));
}

bool UGASPAnimInstance::CanOverlayTransition() const
{
	return StanceMode.Current == StanceTags::Standing && MovementState.Current == MovementStateTags::Idle;
}

void UGASPAnimInstance::RefreshOverlaySettings(float DeltaTime)
{
	// TODO
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
	                                        StanceMode.Current == StanceTags::Standing ? 1.f : 0.f, DeltaTime, 15.f);
	BlendPoses.BasePoseCLF = FMath::GetMappedRangeValueClamped<float, float>(
		{0.f, 1.f}, {1.f, 0.f}, BlendPoses.BasePoseN);
}

void UGASPAnimInstance::SetBlendStackAnimFromChooser(const FAnimNodeReference& Node, const EStateMachineState NewState,
                                                     const bool bForceBlend)
{
	StateMachineState = NewState;
	PreviousBlendStackInputs = BlendStackInputs;

	bNoValidAnim = bNotifyTransition_ReTransition = bNotifyTransition_ToLoop = false;

	FGASPChooserOutputs ChooserOutputs;
	auto Context = UChooserFunctionLibrary::MakeChooserEvaluationContext();
	Context.AddObjectParam(this);
	Context.AddStructParam(ChooserOutputs);

	BlendStack.Databases = UChooserFunctionLibrary::EvaluateObjectChooserBaseMulti(
		Context, UChooserFunctionLibrary::MakeEvaluateChooser(StateMachineTable), UAnimationAsset::StaticClass());

	if (BlendStack.Databases.IsEmpty())
	{
		bNoValidAnim = true;
		return;
	}

	// Update blend stack inputs
	BlendStackInputs.AnimationAsset = static_cast<UAnimationAsset*>(BlendStack.Databases[0]);
	UPoseSearchLibrary::IsAnimationAssetLooping(BlendStack.Databases[0], BlendStackInputs.bLoop);
	BlendStackInputs.StartTime = ChooserOutputs.StartTime;
	BlendStackInputs.BlendTime = ChooserOutputs.BlendTime;
	BlendStackInputs.BlendProfile = GetBlendProfileByName(ChooserOutputs.BlendProfile);
	BlendStack.DatabaseTags = ChooserOutputs.Tags;

	if (ChooserOutputs.bUseMotionMatching)
	{
		FPoseSearchBlueprintResult PoseSearchResult;
		UPoseSearchLibrary::MotionMatch(this, BlendStack.Databases, AnimNames.PoseHistoryTag,
		                                FPoseSearchContinuingProperties(), FPoseSearchFutureProperties(),
		                                PoseSearchResult);

		SearchCost = PoseSearchResult.SearchCost;

		auto AnimationAsset = static_cast<UAnimationAsset*>(PoseSearchResult.SelectedAnim);

		const bool bIsCostAcceptable = ChooserOutputs.MMCostLimit > 0.f
			                               ? SearchCost <= ChooserOutputs.MMCostLimit
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

	const float LerpedAngularVelocity = FMath::Lerp(
		1.f, FMath::GetMappedRangeValueClamped<float, float>({100.f, 400.f}, {1.f, 1.2f},
		                                                     FMath::Abs(TrajectoryInfo.CurrentAngularVelocity.Z)),
		FMath::GetMappedRangeValueClamped<float, float>({0.f, .5f}, {0.f, 1.f}, TrajectoryInfo.CirclingTime));
	return FMath::Lerp(1.f, FMath::Clamp(SpeedRatio, MinDynamicPlayRate, MaxDynamicPlayRate), AlphaCurve) *
		LerpedAngularVelocity;
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
	SetBlendStackAnimFromChooser(Node, EStateMachineState::LocomotionLoop);
}

void UGASPAnimInstance::OnStateEntryTransitionToLocomotionLoop(const FAnimUpdateContext& Context,
                                                               const FAnimNodeReference& Node)
{
	TrajectoryInfo.FutureFacingOnTransitionStart = TrajectoryInfo.FutureFacing;
	SetBlendStackAnimFromChooser(Node, EStateMachineState::TransitionToLocomotionLoop, true);
}

void UGASPAnimInstance::OnUpdateTransitionToLocomotionLoop(const FAnimUpdateContext& Context,
                                                           const FAnimNodeReference& Node)
{
	TrajectoryInfo.FutureFacingOnTransitionStart = FMath::RInterpTo(TrajectoryInfo.FutureFacingOnTransitionStart,
	                                                                TrajectoryInfo.FutureFacing, GetDeltaSeconds(),
	                                                                4.f);
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

void UGASPAnimInstance::OnStateEntryTransitionToSlide(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	SetBlendStackAnimFromChooser(Node, EStateMachineState::TransitionToSlide, true);
}

void UGASPAnimInstance::OnStateEntrySlideLoop(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	SetBlendStackAnimFromChooser(Node, EStateMachineState::SlideLoop);
}
