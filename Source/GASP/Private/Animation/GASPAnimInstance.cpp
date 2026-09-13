#include "Animation/GASPAnimInstance.h"
#include "Actors/GASPCharacter.h"
#include "PoseSearch/PoseSearchDatabase.h"
#include "Utils/GASPMath.h"
#include "PoseSearch/MotionMatchingAnimNodeLibrary.h"
#include "ChooserFunctionLibrary.h"
#include "Utils/GASPChooserLibrary.h"
#include "PoseSearch/PoseSearchLibrary.h"
#include "AnimationWarpingLibrary.h"
#include "Animation/AnimClassInterface.h"
#include "BlendStack/BlendStackAnimNodeLibrary.h"
#include "Interfaces/GASPHeldObjectInterface.h"
#include "MoverPoseSearchTrajectoryPredictor.h"
#include "Animation/AnimLayerInterface.h"
#include "Animation/AnimNode_LinkedAnimLayer.h"
#include "Interfaces/GASPAnimContextInterface.h"
#include "MovementSet/GASPMoverComponent.h"
#include "Settings/GASPCharacterSettings.h"
#include "Types/Misc.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPAnimInstance)

#define UPDATE_STATE(Prefix, NewValue, DeltaTime, RecentLimit) \
{ \
Prefix##_LastFrame = Prefix##_Current; \
Prefix##_Current = (NewValue); \
if (Prefix##_Current != Prefix##_LastFrame) \
{ \
Prefix##_LastStateTime = Prefix##_TimeInState; \
Prefix##_TimeInState = 0.0f; \
} \
else \
{ \
Prefix##_TimeInState += (DeltaTime); \
if (Prefix##_TimeInState >= (RecentLimit)) \
{ \
Prefix##_Recent = Prefix##_Current; \
} \
} \
}

#define ADD_STATE_TAGS(Container, Suffix) \
{ \
Container.AddTag(MovementMode_##Suffix); \
Container.AddTag(StanceMode_##Suffix); \
Container.AddTag(Gait_##Suffix); \
Container.AddTag(MovementState_##Suffix); \
Container.AddTag(RotationMode_##Suffix); \
}

namespace AnimVars
{
	int32 LocomotionSetup{1};
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
	return MovementMode_Current != MovementMode_LastFrame || MovementMode_Current == MovementModeTags::Grounded && (
		       MovementState_Current != MovementState_LastFrame || (Gait_Current != Gait_LastFrame &&
			       MovementState_Current == MovementStateTags::Moving) || StanceMode_Current != StanceMode_LastFrame) ||
	       (MovementDirection_Current != MovementDirection_LastFrame && MovementState_Current ==
		       MovementStateTags::Moving)
		       ? EPoseSearchInterruptMode::InterruptOnDatabaseChange
		       : EPoseSearchInterruptMode::DoNotInterrupt;
}

EOffsetRootBoneMode UGASPAnimInstance::GetOffsetRootRotationMode() const
{
	if (MovementMode_Current == MovementModeTags::Ragdoll || MovementMode_Current == MovementModeTags::InAir)
	{
		return EOffsetRootBoneMode::Release;
	}

	if (IsSlotActive(FAnimSlotGroup::DefaultSlotName))
	{
		return EOffsetRootBoneMode::Release;
	}

	return EOffsetRootBoneMode::Accumulate;
}

EOffsetRootBoneMode UGASPAnimInstance::GetOffsetRootTranslationMode() const
{
	if (IsSlotActive(FAnimSlotGroup::DefaultSlotName))
	{
		return EOffsetRootBoneMode::Release;
	}

	return MovementMode_Current == MovementModeTags::Grounded && MovementState_Current == MovementStateTags::Moving
		       ? EOffsetRootBoneMode::Interpolate
		       : EOffsetRootBoneMode::Release;
}

float UGASPAnimInstance::GetOffsetRootTranslationHalfLife() const
{
	return MovementState_Current == MovementStateTags::Moving ? .3f : .1f;
}

EOrientationWarpingSpace UGASPAnimInstance::GetOrientationWarpingSpace() const
{
	return OffsetRootBoneEnabled
		       ? EOrientationWarpingSpace::RootBoneTransform
		       : EOrientationWarpingSpace::ComponentTransform;
}

float UGASPAnimInstance::GetAOYaw() const
{
	return RotationMode_Current == RotationTags::OrientToMovement ? 0.f : GetAOValue().X;
}

FTransform UGASPAnimInstance::GetHandIKTransform(const FName HandIKSocketName, const FName ObjectIKSocketName,
                                                 const FVector& SocketOffset) const
{
	const auto* SkelMeshComp = GetSkelMeshComponent();
	if (!SkelMeshComp || !Proxy.Character.IsValid())
	{
		return FTransform::Identity;
	}

	const FTransform SocketTransform = SkelMeshComp->GetSocketTransform(HandIKSocketName);

	auto* Character = Proxy.Character.Get();
	if (!Character->Implements<UGASPHeldObjectInterface>())
	{
		return FTransform::Identity;
	}

	const auto* HeldObject = IGASPHeldObjectInterface::Execute_GetHeldObject(Character);
	if (!IsValid(HeldObject) || !HeldObject->DoesSocketExist(ObjectIKSocketName))
	{
		return FTransform::Identity;
	}

	const FTransform ObjectTransform = HeldObject->GetSocketTransform(ObjectIKSocketName);
	return ObjectTransform.GetRelativeTransform(SocketTransform * FTransform(SocketOffset));
}

FGASPControlRigInput UGASPAnimInstance::GetControlRigInputs() const
{
	return {.SpineYawAngle = UE_REAL_TO_FLOAT(FMath::ClampAngle(GetAOValue().X, -90.f, 90.f))};
}

bool UGASPAnimInstance::IsEnableSteering() const
{
	return ((BlendStackInputs.bLoop || BlendStack.bActive) && IsMoving()) || MovementMode_Current ==
		MovementModeTags::InAir || MovementMode_Current == MovementModeTags::Slide;
}

bool UGASPAnimInstance::JustTeleported() const
{
	return FVector::DistSquared(PreviousCharacterInfo.ActorTransform.GetTranslation(),
	                            CharacterInfo.ActorTransform.GetTranslation()) > FMath::Square(
		AnimConfiguration.TeleportThreshold);
}

bool UGASPAnimInstance::AllowFootPinning() const
{
	return MovementMode_Current == MovementModeTags::Grounded;
}

bool UGASPAnimInstance::AllowSlopeWarping() const
{
	return MovementMode_Current == MovementModeTags::Grounded || MovementMode_Current == MovementModeTags::Slide;
}

void UGASPAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();

	CachedCharacter = Cast<AGASPCharacter>(TryGetPawnOwner());
	if (CachedCharacter.IsValid())
	{
		CachedMovement = CachedCharacter->GetMoverComponent();
		if (CachedMovement.IsValid())
		{
			Predictor = CachedMovement->GetTrajectoryPredictor();
		}
	}

#if WITH_EDITOR
	const auto* World{GetWorld()};

	if (IsValid(World) && !World->IsGameWorld())
	{
		// Use default objects for editor preview.
		if (!CachedCharacter.IsValid())
		{
			CachedCharacter = GetMutableDefault<AGASPCharacter>();
		}

		if (!CachedMovement.IsValid())
		{
			CachedMovement = GetMutableDefault<UGASPMoverComponent>();
		}

		if (!Predictor.IsValid() && CachedMovement.IsValid())
		{
			Predictor = GetMutableDefault<UMoverTrajectoryPredictor>();
			Predictor->Setup(CachedMovement.Get());
		}
	}
#endif

	CachedCharacter->OverlayContainerChanged.AddUniqueDynamic(this, &ThisClass::OnOverlayModeChanged);
	CachedCharacter->PoseModeChanged.AddUniqueDynamic(this, &ThisClass::OnPoseModeChanged);
}

void UGASPAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	CachedCharacter = Cast<AGASPCharacter>(TryGetPawnOwner());
	if (CachedCharacter.IsValid())
	{
		CachedMovement = CachedCharacter->GetMoverComponent();
		if (CachedMovement.IsValid())
		{
			Predictor = CachedMovement->GetTrajectoryPredictor();
		}
	}

#if WITH_EDITOR
	const auto* World{GetWorld()};

	if (IsValid(World) && !World->IsGameWorld())
	{
		// Use default objects for editor preview.
		if (!CachedCharacter.IsValid())
		{
			CachedCharacter = GetMutableDefault<AGASPCharacter>();
		}

		if (!CachedMovement.IsValid())
		{
			CachedMovement = GetMutableDefault<UGASPMoverComponent>();
		}

		if (!Predictor.IsValid() && CachedMovement.IsValid())
		{
			Predictor = NewObject<UMoverTrajectoryPredictor>(CachedMovement.Get());
			Predictor->Setup(CachedMovement.Get());
		}
	}
#endif

	LocomotionSetup = AnimVars::LocomotionSetup;
	OffsetRootBoneEnabled = AnimVars::bOffsetRootBoneEnabled;
	MMDatabaseLOD = AnimVars::MMDatabaseLOD;
	FootPlacementEnabled = AnimVars::bFootPlacementEnabled;

	// AddWeakLambda, not AddLambda: console variables live for the whole process, so a plain lambda
	// capturing this would outlive the anim instance and fire on freed memory after a level change.
	AnimVars::CVarLocomotionStyleStruct->OnChangedDelegate().AddWeakLambda(
		this, [this](const IConsoleVariable* ICVar)
		{
			LocomotionSetup = ICVar ? ICVar->GetInt() : 0;
		});
	AnimVars::CVarOffsetRootBoneEnabledStruct->OnChangedDelegate().AddWeakLambda(
		this, [this](const IConsoleVariable* ICVar)
		{
			OffsetRootBoneEnabled = ICVar ? ICVar->GetBool() : false;
		});
	AnimVars::CVarMMDatabaseLODStruct->OnChangedDelegate().AddWeakLambda(
		this, [this](const IConsoleVariable* ICVar)
		{
			MMDatabaseLOD = ICVar ? ICVar->GetInt() : 0;
		});
	AnimVars::CVarFootPlacementEnabled->OnChangedDelegate().AddWeakLambda(
		this, [this](const IConsoleVariable* ICVar)
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

	RefreshTrajectory(DeltaSeconds);
	RefreshEssentialValues(DeltaSeconds);
	RefreshLayering(DeltaSeconds);
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

	const auto& InputState{CachedCharacter->GetMoverState()};
	const auto NewGait{
		CachedMovement->GetGait() == GaitTags::Sprint && IsCircling() ? GaitTags::Run : CachedMovement->GetGait()
	};

	UPDATE_STATE(Gait, NewGait, DeltaSeconds, .1f)
	UPDATE_STATE(RotationMode, CachedMovement->GetRotationMode(), DeltaSeconds, .1f)
	UPDATE_STATE(MovementState, IsMoving() ? MovementStateTags::Moving : MovementStateTags::Idle, DeltaSeconds, .1f)
	UPDATE_STATE(MovementMode, CachedCharacter->GetMovementMode(), DeltaSeconds, .1f)
	UPDATE_STATE(StanceMode, CachedCharacter->GetStanceMode(), DeltaSeconds, .1f)
	UPDATE_STATE(MovementDirection, InputState.MovementDirection, DeltaSeconds, .1f)

	RecentStateContainer.Reset();
	PreviousStateContainer.Reset();
	StateContainer.Reset();

	ADD_STATE_TAGS(RecentStateContainer, Recent)
	ADD_STATE_TAGS(PreviousStateContainer, LastFrame)
	ADD_STATE_TAGS(StateContainer, Current)

	LocomotionAction = CachedCharacter->GetLocomotionAction();

	if (MovementMode_Current == MovementModeTags::Ragdoll)
	{
		RefreshRagdollValues();
	}
}

void UGASPAnimInstance::NativePostUpdateAnimation()
{
	PreviousCharacterInfo = CharacterInfo;
	PreviousLocomotionAction = LocomotionAction;

	if (ForceFootPlacementReset)
	{
		ForceFootPlacementReset = false;
	}
}

FAnimInstanceProxy* UGASPAnimInstance::CreateAnimInstanceProxy()
{
	return &Proxy;
}

void UGASPAnimInstance::RefreshTrajectory(const float DeltaSeconds)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UGASPAnimInstance::RefreshTrajectory"),
	                            STAT_UGASPAnimInstance_RefreshTrajectory, STATGROUP_GASP)
	TRACE_CPUPROFILER_EVENT_SCOPE(__FUNCTION__);
	if (!Proxy.MoverComponent.IsValid() || !Predictor.IsValid() || !Proxy.Character.IsValid())
	{
		return;
	}

	FTransformTrajectory OutTrajectory{};
	UPoseSearchTrajectoryLibrary::PoseSearchGenerateTransformTrajectoryWithPredictor(
		Predictor.Get(), DeltaSeconds, Trajectory, BlendStack.PreviousDesiredYawRotation,
		OutTrajectory, .033f, 15, .1f, 15);

	UPoseSearchTrajectoryLibrary::GetTransformTrajectoryVelocity(Trajectory, -.3f, -.2f,
	                                                             TrajectoryInfo.PreviousVelocity,
	                                                             false);
	UPoseSearchTrajectoryLibrary::GetTransformTrajectoryVelocity(Trajectory, .1f, .2f,
	                                                             TrajectoryInfo.NearFutureVelocity,
	                                                             false);
	TrajectoryInfo.PreviousFutureVelocity = TrajectoryInfo.FutureVelocity;
	UPoseSearchTrajectoryLibrary::GetTransformTrajectoryVelocity(Trajectory, .4f, .5f, TrajectoryInfo.FutureVelocity,
	                                                             false);

	TrajectoryInfo.FutureFacing = Trajectory.GetSampleAtTime(1.5f, false).Facing.Rotator();

	TrajectoryInfo.PreviousFutureFacingDelta = TrajectoryInfo.FutureFacingDelta;
	TrajectoryInfo.FutureFacingDelta = GetTotalFacingDelta({0.f, 0.25f, 0.75f, 1.5f});

	if (FMath::Abs(TrajectoryInfo.FutureFacingDelta - TrajectoryInfo.PreviousFutureFacingDelta) > 200.f)
	{
		MovementDirection_Current = MovementDirection_Recent = EMovementDirection::B;
	}

	UPoseSearchTrajectoryLibrary::GetTransformTrajectoryAngularVelocity(
		Trajectory, -.4f, -.3f, TrajectoryInfo.PastAngularVelocity);
	UPoseSearchTrajectoryLibrary::GetTransformTrajectoryAngularVelocity(
		Trajectory, 0.f, .1f, TrajectoryInfo.CurrentAngularVelocity);

	TrajectoryInfo.CirclingTime = IsCircling() ? TrajectoryInfo.CirclingTime + DeltaSeconds : 0.f;
}

float UGASPAnimInstance::GetTotalFacingDelta(const TArray<float>& Times) const
{
	if (Times.IsEmpty())
	{
		return 0.f;
	}

	float CurrentYaw = Trajectory.GetSampleAtTime(Times[0]).Facing.Rotator().Yaw;
	float AngleSum = FMath::FindDeltaAngleDegrees(CharacterInfo.RootOffsetRotation.Yaw, CurrentYaw);

	// Num(), not Num() - 1: the final sample defines the far end of the turn horizon and was
	// previously skipped, which understated the total facing delta.
	for (int32 Index = 1; Index < Times.Num(); ++Index)
	{
		const float NextYaw = Trajectory.GetSampleAtTime(Times[Index]).Facing.Rotator().Yaw;

		AngleSum += FMath::FindDeltaAngleDegrees(CurrentYaw, NextYaw);

		CurrentYaw = NextYaw;
	}

	return AngleSum;
}

float UGASPAnimInstance::GetMatchingBlendTime() const
{
	if (MovementMode_Current == MovementModeTags::InAir)
	{
		return CharacterInfo.Velocity.Z > 100.f ? .15f : .5f;
	}

	return MovementMode_LastFrame == MovementModeTags::Grounded ? .5f : .2f;
}

FFloatInterval UGASPAnimInstance::GetMatchingPlayRate() const
{
	if (MovementMode_Current == MovementModeTags::Grounded)
	{
		return {.75f, 3.f};
	}
	return {.75f, 1.25f};
}

FVector UGASPAnimInstance::GetBlendSpaceInputs() const
{
	if (!BlendStack.DatabaseTags.Contains(AnimNames.BlendSpaceSlopeName))
	{
		return FVector::ZeroVector;
	}

	if (const float SlopeAngleY{static_cast<float>(GetSlopeAngle().Y)}; FMath::Abs(SlopeAngleY) > 15.f)
	{
		return {
			FMath::GetMappedRangeValueClamped<float, float>({-30, 30.f},
			                                                {-1.f, 1.f}, SlopeAngleY),
			0.f, 0.f
		};
	}

	return FVector::ZeroVector;
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
	const auto SlopeAngle{GetSlopeAngle()};
	return FRotator{SlopeAngle.X * -1.f, 0.f, SlopeAngle.Y * -1.f};
}

FVector2D UGASPAnimInstance::GetSlopeAngle() const
{
	const auto FloorZAxis{CharacterInfo.SmoothedGroundNormal};
	const auto FloorXAxis{FRotationMatrix(CharacterInfo.RootTransform.Rotator()).GetScaledAxis(EAxis::Y) ^ FloorZAxis};
	const auto FloorYAxis{FloorZAxis ^ FloorXAxis};
	const auto UpVector{FRotationMatrix(CharacterInfo.ActorTransform.Rotator()).GetScaledAxis(EAxis::Z)};

	const float Pitch{static_cast<float>(90.f - FMath::RadiansToDegrees(FMath::Acos(FloorXAxis.Dot(UpVector))))};
	const float Roll{static_cast<float>(90.f - FMath::RadiansToDegrees(FMath::Acos(FloorYAxis.Dot(UpVector))))};

	return {Roll, Pitch};
}

float UGASPAnimInstance::GetMatchingNotifyRecencyTimeOut() const
{
	if (Gait_Current == GaitTags::Sprint)
	{
		return .16f;
	}

	return .2f;
}


bool UGASPAnimInstance::IsStarting() const
{
	return TrajectoryInfo.FutureVelocity.Size2D() >= CharacterInfo.Velocity.Size2D() + 100.f && !BlendStack.
		DatabaseTags.Contains(AnimNames.PivotsTag) && CharacterInfo.Speed <= 100.f;
}

bool UGASPAnimInstance::IsPivoting() const
{
	if (LocomotionSetup == 0)
	{
		return FMath::Abs(GetTrajectoryTurnAngle()) >= 75.f && !IsCircling() && MovementState_Current ==
			MovementStateTags::Moving;
	}

	float MinSpeed{175.f}, MaxSpeed{600.f}, AngleThreshold{75.f};

	if (StanceMode_Current == StanceTags::Crouching)
	{
		MinSpeed = 50.f;
		MaxSpeed = 200.f;
	}
	else if (Gait_Current == GaitTags::Sprint)
	{
		MinSpeed = 200.f;
		MaxSpeed = 700.f;
		AngleThreshold = 60.f;
	}
	else if (Gait_Current == GaitTags::Walk)
	{
		MinSpeed = 50.f;
	}

	if (Gait_Current != GaitTags::Sprint && (MovementMode_Recent == MovementModeTags::InAir || MovementMode_Recent ==
		MovementModeTags::Slide))
	{
		AngleThreshold = 100.f;
	}

	return FMath::Abs(GetTrajectoryTurnAngle()) >= AngleThreshold && FMath::IsWithinInclusive(
		CharacterInfo.Speed, MinSpeed, MaxSpeed) && MovementState_Current == MovementStateTags::Moving;
}

bool UGASPAnimInstance::IsMoving() const
{
	FVector OutVelocity;
	UPoseSearchTrajectoryLibrary::GetTransformTrajectoryVelocity(Trajectory, .9f, 1.f, OutVelocity, true);

	return !OutVelocity.IsNearlyZero(10.f);
}

bool UGASPAnimInstance::ShouldTurnInPlace() const
{
	if (!Proxy.Character.IsValid())
	{
		return false;
	}

	const auto* Settings{Proxy.Character->GetSettings()};
	return Settings && CharacterInfo.Speed < 50.f && FMath::Abs(TrajectoryInfo.FutureFacingDelta) >= Settings->
		TurnInPlaceThreshold && MovementState_Current == MovementStateTags::Idle;
}

bool UGASPAnimInstance::ShouldSpinTransition() const
{
	return FMath::Abs(TrajectoryInfo.FutureFacingDelta) >= AnimConfiguration.SpinTransitionAngle && CharacterInfo.Speed
		>= 150.f && !BlendStack.DatabaseTags.Contains(AnimNames.PivotsTag);
}

bool UGASPAnimInstance::JustLanded_Light() const
{
	return FMath::Abs(PreviousCharacterInfo.Velocity.Z) < FMath::Abs(HeavyLandSpeedThreshold) && MovementMode_Current
		== MovementModeTags::Grounded && MovementMode_LastFrame == MovementModeTags::InAir;
}

bool UGASPAnimInstance::JustLanded_Heavy() const
{
	return FMath::Abs(PreviousCharacterInfo.Velocity.Z) >= FMath::Abs(HeavyLandSpeedThreshold) && MovementMode_Current
		== MovementModeTags::Grounded && MovementMode_LastFrame == MovementModeTags::InAir;
}

bool UGASPAnimInstance::JustTraversed() const
{
	return !IsSlotActive(FAnimSlotGroup::DefaultSlotName) && GetCurveValue(AnimNames.MovingTraversalCurveName) > 0.f &&
		FMath::Abs(GetTrajectoryTurnAngle()) <= 100.f;
}

bool UGASPAnimInstance::PlayLand() const
{
	return MovementMode_Current == MovementModeTags::Grounded && MovementMode_LastFrame ==
		MovementModeTags::InAir;
}

bool UGASPAnimInstance::PlayMovingLand() const
{
	return MovementMode_Current == MovementModeTags::Grounded && MovementMode_LastFrame ==
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
	float LateralAccelerationAmount = FMath::Clamp(
		CharacterInfo.Velocity.ToOrientationRotator().UnrotateVector(CharacterInfo.VelocityAcceleration).Y /
		FMath::GetMappedRangeValueClamped<float, float>({200.f, 320.f}, {500.f, 800.f}, CharacterInfo.Speed), -1.f,
		1.f);

	switch (MovementDirection_Current)
	{
	case EMovementDirection::B:
		return {LateralAccelerationAmount * -1.f, 0.f};
	case EMovementDirection::LL:
	case EMovementDirection::FL:
		return {0.f, LateralAccelerationAmount};
	case EMovementDirection::FR:
	case EMovementDirection::RR:
		return {0.f, LateralAccelerationAmount * -1.f};
	default:
		return {LateralAccelerationAmount, 0.f};
	}
}

void UGASPAnimInstance::OnUpdate_MotionMatching(const FAnimUpdateContext& Context,
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
	Databases.Reserve(BlendStack.Databases.Num());
	for (UObject* Object : BlendStack.Databases)
	{
		// The chooser was asked for UPoseSearchDatabase, but a misconfigured table can still
		// yield another type or a null entry, so each result is checked rather than assumed.
		if (auto* Database = Cast<UPoseSearchDatabase>(Object))
		{
			Databases.Add(Database);
		}
	}

	if (Databases.IsEmpty())
	{
		return;
	}

	UMotionMatchingAnimNodeLibrary::SetDatabasesToSearch(Reference, Databases, GetMatchingInterruptMode());
}

void UGASPAnimInstance::OnMotionMatchingUpdateState(const FAnimUpdateContext& Context,
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

void UGASPAnimInstance::OnUpdate_OffsetRoot(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	{
		bool Result{};
		FOffsetRootBoneAnimNodeReference OffsetRootBoneNode;
		UAnimationWarpingLibrary::ConvertToOffsetRootBoneNodePure(Node, OffsetRootBoneNode, Result);
		if (!Result)
		{
			return;
		}

		BasedMovementDelta = CachedCharacter->GetBasedMovementDelta();

		// UAnimationWarpingLibrary::ApplyDeltaToOffsetRootBone() is not exported from AnimationWarpingRuntime
		// (missing ANIMATIONWARPINGRUNTIME_API on a MinimalAPI class), so apply the delta to the node directly.
		if (auto* OffsetRootNode{OffsetRootBoneNode.GetAnimNodePtr<FAnimNode_OffsetRootBone>()})
		{
			OffsetRootNode->SetOffsetRootTranslation(
				OffsetRootNode->GetOffsetRootTranslation() + BasedMovementDelta.GetTranslation());
			OffsetRootNode->SetOffsetRootRotation(
				BasedMovementDelta.GetRotation() * OffsetRootNode->GetOffsetRootRotation());
		}
	}

	const auto TargetTransform{UAnimationWarpingLibrary::GetOffsetRootTransform(Node)};
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

void UGASPAnimInstance::OnUpdate_BlendStack(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UGASPAnimInstance::RefreshBlendStack"),
	                            STAT_UGASPAnimInstance_RefreshBlendStack, STATGROUP_GASP)
	TRACE_CPUPROFILER_EVENT_SCOPE(__FUNCTION__);

	BlendStack.AnimTime = UBlendStackAnimNodeLibrary::GetCurrentBlendStackAnimAssetTime(Node);
	BlendStack.AnimAsset = UBlendStackAnimNodeLibrary::GetCurrentBlendStackAnimAsset(Node);
	BlendStack.PlayRate = GetDynamicPlayRate(Node);
	BlendStack.bActive = UBlendStackAnimNodeLibrary::GetCurrentBlendStackAnimIsActive(Node);

	const auto* NewAnimSequence{Cast<UAnimSequence>(BlendStack.AnimAsset.Get())};

	float ProceduralTargetTime;
	UAnimationWarpingLibrary::GetCurveValueFromAnimation(NewAnimSequence, AnimNames.SteeringTargetTime,
	                                                     BlendStack.AnimTime, ProceduralTargetTime);

	BlendStack.DesiredFacingTime = FMath::GetMappedRangeValueClamped<float, float>(
		{0.f, 1.f}, {.1f, 1.5f}, ProceduralTargetTime);
	BlendStack.ProceduralTargetTime = FMath::GetMappedRangeValueClamped<float, float>(
		{0.f, 1.f}, {.1f, .3f}, ProceduralTargetTime);

	if (NewAnimSequence)
	{
		UAnimationWarpingLibrary::GetCurveValueFromAnimation(NewAnimSequence, AnimNames.EnableWarpingCurveName,
		                                                     BlendStack.AnimTime, BlendStack.WarpAlpha);
	}
	else
	{
		BlendStack.WarpAlpha = GetCurveValue(AnimNames.EnableWarpingCurveName);
	}

	UAnimationWarpingLibrary::GetCurveValueFromAnimation(NewAnimSequence, AnimNames.EnableTurnInPlaceSteering,
	                                                     BlendStack.AnimTime, BlendStack.TurnInPlaceAlpha);
}

void UGASPAnimInstance::OnBecomeRelevant_FootPlacement(const FAnimUpdateContext& Context,
                                                       const FAnimNodeReference& Node)
{
	ForceFootPlacementReset = true;
}

void UGASPAnimInstance::RefreshEssentialValues(const float DeltaSeconds)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UGASPAnimInstance::RefreshEssentialValues"),
	                            STAT_UGASPAnimInstance_RefreshEssentialValues, STATGROUP_GASP)
	TRACE_CPUPROFILER_EVENT_SCOPE(__FUNCTION__);
	if (!Proxy.Character.IsValid() || !Proxy.MoverComponent.IsValid())
	{
		return;
	}

	CharacterInfo.ActorTransform = Proxy.Character->GetActorTransform();

	if (!OffsetRootBoneEnabled)
	{
		CharacterInfo.RootTransform = CharacterInfo.ActorTransform;
	}

	const auto InputState{Proxy.Character->GetMoverState()};
	CharacterInfo.Acceleration = InputState.GetMoveInput();
	CharacterInfo.FloorLocation = InputState.FloorLocation;
	CharacterInfo.FloorNormal = InputState.FloorNormal;

	CharacterInfo.Velocity = Proxy.MoverComponent->GetVelocity();
	CharacterInfo.Speed = CharacterInfo.Velocity.Size2D();

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

bool UGASPAnimInstance::IsEnabledAO() const
{
	return FMath::Abs(GetAOValue().X) <= 115.f && RotationMode_Current != RotationTags::OrientToMovement &&
		GetSlotMontageLocalWeight(FName{TEXT("DefaultSlot")}) < .5f;
}

FVector2D UGASPAnimInstance::GetAOValue() const
{
	if (!Proxy.Character.IsValid())
	{
		return FVector2D::ZeroVector;
	}

	const auto ControlRot{Proxy.Character->GetMoverState().ControlRotation};
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
	return StanceMode_Current == StanceTags::Standing && MovementState_Current == MovementStateTags::Idle;
}

void UGASPAnimInstance::RefreshLayering(float DeltaTime)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UGASPAnimInstance::RefreshLayering"),
	                            STAT_UGASPAnimInstance_RefreshLayering, STATGROUP_GASP)
	TRACE_CPUPROFILER_EVENT_SCOPE(__FUNCTION__);

	LayeringState.SpineEnableRotationAmount = GetCurveValue(AnimNames.EnableSpineRotationName);
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
	                                        StanceMode_Current == StanceTags::Standing ? 1.f : 0.f, DeltaTime, 15.f);
	BlendPoses.BasePoseCLF = FMath::GetMappedRangeValueClamped<float, float>(
		{0.f, 1.f}, {1.f, 0.f}, BlendPoses.BasePoseN);
}

void UGASPAnimInstance::SetBlendStackAnimFromChooser(const FAnimNodeReference& Node, const FName& NewState)
{
	StateMachineState = NewState;
	PreviousBlendStackInputs = BlendStackInputs;

	bNoValidAnim = bNotifyTransition_ReTransition = bNotifyTransition_ToLoop = false;

	FGASPChooserOutputs ChooserOutputs;
	auto PoseHistory{
		Implements<UGASPAnimContextInterface>()
			? IGASPAnimContextInterface::Execute_GetPoseHistory(this)
			: FPoseHistoryReference{}
	};

	auto* Object{
		FGASPChooserUtils::EvaluateSingle<UAnimationAsset>(StateMachineTable, this, PoseHistory, ChooserOutputs)
	};
	if (!IsValid(Object))
	{
		bNoValidAnim = true;
		return;
	}

	BlendStackInputs.AnimationAsset = Object;
	UPoseSearchLibrary::IsAnimationAssetLooping(Object, BlendStackInputs.bLoop);
	BlendStackInputs.StartTime = ChooserOutputs.StartTime;
	BlendStackInputs.BlendTime = ChooserOutputs.BlendTime;
	BlendStackInputs.BlendCurve = ChooserOutputs.BlendCurve;
	BlendStackInputs.BlendProfile = GetBlendProfileByName(ChooserOutputs.BlendProfile);
	BlendStack.DatabaseTags = ChooserOutputs.Tags;

	bForceBlendNextUpdate = !BlendStackInputs.bLoop;
	// if (!BlendStackInputs.bLoop)
	// {
	// 	const auto* AnimClass = IAnimClassInterface::GetFromClass(GetClass());
	// 	const auto* TagSubsystem = AnimClass ? AnimClass->FindSubsystem<FAnimSubsystem_Tag>() : nullptr;
	// 	const int32 BlendStackNodeIndex = TagSubsystem
	// 		                                  ? TagSubsystem->FindNodeIndexByTag(StateMachineBlendStackTag)
	// 		                                  : INDEX_NONE;
	//
	// 	if (BlendStackNodeIndex != INDEX_NONE)
	// 	{
	// 		EAnimNodeReferenceConversionResult Result{};
	// 		const auto BlendRef = UBlendStackAnimNodeLibrary::ConvertToBlendStackNode(
	// 			FAnimNodeReference(this, BlendStackNodeIndex), Result);
	// 		if (Result != EAnimNodeReferenceConversionResult::Succeeded)
	// 		{
	// 			return;
	// 		}
	//
	// 		UBlendStackAnimNodeLibrary::ForceBlendNextUpdate(BlendRef);
	// 	}
	// }
}

bool UGASPAnimInstance::IsAnimationAlmostComplete()
{
	const auto* AnimAsset{BlendStack.AnimAsset.Get()};
	if (!IsValid(AnimAsset))
	{
		return false;
	}

	bool bLoop{false};
	UPoseSearchLibrary::IsAnimationAssetLooping(AnimAsset, bLoop);

	const float PredictedCurrentTime = BlendStack.AnimTime + (GetDeltaSeconds() * BlendStack.PlayRate);
	const float AssetTimeRemaining{FMath::Max(AnimAsset->GetPlayLength() - PredictedCurrentTime, 0.f)};
	return !bLoop && AssetTimeRemaining <= 0.75f;
}

float UGASPAnimInstance::GetDynamicPlayRate(const FAnimNodeReference& Node) const
{
	static const FName EnablePlayRateWarpingCurveName = TEXT("Enable_PlayRateWarping");
	static const FName MoveDataSpeedCurveName = TEXT("MoveData_Speed");
	static const FName MaxDynamicPlayRateCurveName = TEXT("MaxDynamicPlayRate");
	static const FName MinDynamicPlayRateCurveName = TEXT("MinDynamicPlayRate");

	const auto AnimSequence{
		Cast<UAnimSequence>(UBlendStackAnimNodeLibrary::GetCurrentBlendStackAnimAsset(Node))
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

void UGASPAnimInstance::OnStateEntry_IdleLoop(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	SetBlendStackAnimFromChooser(Node, StateMachineStateNames::IdleLoop);
}

void UGASPAnimInstance::OnStateEntry_IdleTransition(const FAnimUpdateContext& Context,
                                                    const FAnimNodeReference& Node)
{
	SetBlendStackAnimFromChooser(Node, StateMachineStateNames::IdleTransition);
}

void UGASPAnimInstance::OnStateEntry_LocomotionLoop(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	SetBlendStackAnimFromChooser(Node, StateMachineStateNames::LocomotionLoop);
}

void UGASPAnimInstance::OnStateEntry_LocomotionTransition(const FAnimUpdateContext& Context,
                                                          const FAnimNodeReference& Node)
{
	TrajectoryInfo.FutureFacingOnTransitionStart = TrajectoryInfo.FutureFacing;
	SetBlendStackAnimFromChooser(Node, StateMachineStateNames::LocomotionTransition);
}

void UGASPAnimInstance::OnUpdate_LocomotionTransition(const FAnimUpdateContext& Context,
                                                      const FAnimNodeReference& Node)
{
	TrajectoryInfo.FutureFacingOnTransitionStart = FMath::RInterpTo(TrajectoryInfo.FutureFacingOnTransitionStart,
	                                                                TrajectoryInfo.FutureFacing, GetDeltaSeconds(),
	                                                                4.f);
}

void UGASPAnimInstance::OnStateEntry_InAirLoop(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	SetBlendStackAnimFromChooser(Node, StateMachineStateNames::InAirLoop);
}

void UGASPAnimInstance::OnStateEntry_InAirTransition(const FAnimUpdateContext& Context,
                                                     const FAnimNodeReference& Node)
{
	SetBlendStackAnimFromChooser(Node, StateMachineStateNames::InAirTransition);
}

void UGASPAnimInstance::OnStateEntry_IdleBreak(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	SetBlendStackAnimFromChooser(Node, StateMachineStateNames::IdleBreak);
}

void UGASPAnimInstance::OnStateEntry_SlideTransition(const FAnimUpdateContext& Context,
                                                     const FAnimNodeReference& Node)
{
	SetBlendStackAnimFromChooser(Node, StateMachineStateNames::SlideTransition);
}

void UGASPAnimInstance::OnStateEntry_SlideLoop(const FAnimUpdateContext& Context, const FAnimNodeReference& Node)
{
	SetBlendStackAnimFromChooser(Node, StateMachineStateNames::SlideLoop);
}

void UGASPAnimInstance::OnUpdate_StateMachineBlendStack(const FAnimUpdateContext& Context,
                                                        const FAnimNodeReference& Node)
{
	if (bForceBlendNextUpdate)
	{
		EAnimNodeReferenceConversionResult Result{};
		const auto BlendRef = UBlendStackAnimNodeLibrary::ConvertToBlendStackNode(Node, Result);
		if (Result != EAnimNodeReferenceConversionResult::Succeeded)
		{
			return;
		}

		UBlendStackAnimNodeLibrary::ForceBlendNextUpdate(BlendRef);
		bForceBlendNextUpdate = false;
	}
}

bool UGASPAnimInstance::IsLayerOverridden(TSubclassOf<UAnimLayerInterface> InterfaceClass) const
{
	if (!InterfaceClass)
	{
		return false;
	}

	if (const auto* AnimBlueprintClass = IAnimClassInterface::GetFromClass(GetClass()))
	{
		for (const auto* LayerNodeProperty : AnimBlueprintClass->GetLinkedAnimLayerNodeProperties())
		{
			const auto* Layer = LayerNodeProperty->ContainerPtrToValuePtr<FAnimNode_LinkedAnimLayer>(this);

			if (Layer->Interface == InterfaceClass)
			{
				const UAnimInstance* Target = Layer->GetTargetInstance<UAnimInstance>();
				return Target && Target != this;
			}
		}
	}

	return false;
}

void UGASPAnimInstance::RefreshRagdollValues()
{
	if (!CachedCharacter.IsValid())

	{
		return;
	}

	const auto* CharRagdollingState = CachedCharacter->TaskStates.FindDataByType<FRagdollingState>();
	if (!CharRagdollingState)
	{
		return;
	}

	RagdollingState.Speed = CharRagdollingState->Speed;
	RagdollingState.ImpactDirection = CharRagdollingState->ImpactDirection;
	RagdollingState.RollForceAmount = CharRagdollingState->RollForce.Size();
	RagdollingState.TimeToImpact = CharRagdollingState->TimeToImpact;
	RagdollingState.InjuryState = CharRagdollingState->InjuryState;
}

FPoseSnapshot& UGASPAnimInstance::SnapshotFinalRagdollPose()
{
	check(IsInGameThread())

	// Save a snapshot of the current ragdoll pose for use in animation graph to blend out of the ragdoll.
	SnapshotPose(RagdollingState.FinalRagdollPose);

	return RagdollingState.FinalRagdollPose;
}
