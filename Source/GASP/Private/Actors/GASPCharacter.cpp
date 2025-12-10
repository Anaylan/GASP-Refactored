#include "Actors/GASPCharacter.h"
#include "AIController.h"
#include "ChooserFunctionLibrary.h"
#include "MotionWarpingComponent.h"
#include "GameplayTagContainer.h"
#include "Components/CapsuleComponent.h"
#include "Components/GASPTraversalComponent.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Utils/GASPLinkedAnimInstanceSet.h"
#include "DefaultMovementSet/NavMoverComponent.h"
#include "MovementSet/GASPMoverComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPCharacter)

namespace GeneralVars
{
	int32 AimStyle{0};
	FAutoConsoleVariableRef AimStyleStruct(
		TEXT("gasp.movement.style.aim"), AimStyle, TEXT("set style for aim rotation mode"), ECVF_Default);

	int32 StrafeStyle{0};
	FAutoConsoleVariableRef StrafeStyleStruct(
		TEXT("gasp.movement.style.strafe"), StrafeStyle, TEXT("set style for strafe rotation mode"), ECVF_Default);
}


FName AGASPCharacter::MeshComponentName(TEXT("CharacterMesh0"));
FName AGASPCharacter::CapsuleComponentName(TEXT("CollisionCylinder"));

// Sets default values
AGASPCharacter::AGASPCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	  , NavMoverComponent(nullptr)
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// SetReplicates(true);
	SetReplicatingMovement(false);

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(CapsuleComponentName);
	CapsuleComponent->InitCapsuleSize(34.0f, 88.0f);
	CapsuleComponent->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);

	CapsuleComponent->CanCharacterStepUpOn = ECB_No;
	CapsuleComponent->SetShouldUpdatePhysicsVolume(true);
	CapsuleComponent->SetCanEverAffectNavigation(false);
	CapsuleComponent->bDynamicObstacle = true;
	RootComponent = CapsuleComponent;

	Mesh = CreateOptionalDefaultSubobject<USkeletalMeshComponent>(MeshComponentName);
	if (Mesh)
	{
		Mesh->AlwaysLoadOnClient = true;
		Mesh->AlwaysLoadOnServer = true;
		Mesh->bOwnerNoSee = false;
		Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		Mesh->bCastDynamicShadow = true;
		Mesh->bAffectDynamicIndirectLighting = true;
		Mesh->PrimaryComponentTick.TickGroup = TG_PrePhysics;
		Mesh->SetupAttachment(CapsuleComponent);
		static FName MeshCollisionProfileName(TEXT("CharacterMesh"));
		Mesh->SetCollisionProfileName(MeshCollisionProfileName);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetCanEverAffectNavigation(false);

		Mesh->SetRelativeRotation_Direct({0.f, -90.f, 0.f});
		Mesh->SetRelativeLocation_Direct({0.f, 0.f, -90.f});
	}

	CharacterMotionComponent = CreateDefaultSubobject<UGASPMoverComponent>(TEXT("MoverComponent"));
	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarping"));
	TraversalComponent = CreateDefaultSubobject<UGASPTraversalComponent>(TEXT("TraversalComponent"));

	auto IsImplementedInBlueprint = [](const UFunction* Func) -> bool
	{
		return Func && ensure(Func->GetOuter())
			&& Func->GetOuter()->IsA(UBlueprintGeneratedClass::StaticClass());
	};

	static FName ProduceInputBPFuncName = FName(TEXT("OnProduceInputInBlueprint"));
	UFunction* ProduceInputFunction = GetClass()->FindFunctionByName(ProduceInputBPFuncName);
	bHasProduceInputBpFunc = IsImplementedInBlueprint(ProduceInputFunction);
}

// Called when the game starts or when spawned
void AGASPCharacter::BeginPlay()
{
	Super::BeginPlay();

	NavMoverComponent = FindComponentByClass<UNavMoverComponent>();

	if (auto MoverComp = GetMoverComponent())
	{
		MoverComp->OnMovementModeChanged.AddDynamic(this, &ThisClass::OnMovementModeChanged);
		MoverComp->OnPreSimulationTick.AddDynamic(this, &ThisClass::OnPreSimulateTick);
		MoverComp->OnStanceChanged.AddDynamic(this, &ThisClass::OnStanceChanged);

		GetMesh()->AddTickPrerequisiteComponent(MoverComp);
	}

	MoverCustomInputs_PreSim.OrientationIntent = GetActorForwardVector();

	OverlayModeChanged.AddDynamic(this, &ThisClass::OnOverlayModeChanged);
	PoseModeChanged.AddDynamic(this, &ThisClass::OnPoseModeChanged);

	SetOverlayMode(OverlayMode, true);
	SetPoseMode(PoseMode, true);
	SetLocomotionAction(FGameplayTag::EmptyTag, true);

	GetMesh()->AddTickPrerequisiteActor(this);
	//TODO: Init variables?
}

void AGASPCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
}

// Called every frame
void AGASPCharacter::Tick(float DeltaTime)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("AGASPCharacter::Tick"),
	                            STAT_AGASPCharacter_Tick, STATGROUP_GASP)
	TRACE_CPUPROFILER_EVENT_SCOPE(__FUNCTION__);

	Super::Tick(DeltaTime);

	RefreshInputMover();
	RefreshFloorValues();
	RefreshControlRotationRate(DeltaTime);
	RefreshTwinStickMode();

	const bool IsMoving = !GetMoverComponent()->GetVelocity().IsNearlyZero(.1f);
	SetMovementState(IsMoving ? MovementStateTags::Moving : MovementStateTags::Idle);

	if (LocomotionAction == LocomotionActionTags::Ragdoll)
	{
		RefreshRagdolling(DeltaTime);
	}

	if (MovementMode == MovementModeTags::Grounded)
	{
		RefreshRotationMode();
		RefreshGait();
	}
	else if (MovementMode == MovementModeTags::Slide)
	{
		RefreshSlidingAudio();
	}
}

void AGASPCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	CharacterMotionComponent = FindComponentByClass<UGASPMoverComponent>();

	if (CharacterMotionComponent)
	{
		if (auto* UpdatedComponent = CharacterMotionComponent->GetUpdatedComponent())
		{
			UpdatedComponent->SetCanEverAffectNavigation(bCanAffectNavigationGeneration);
		}
	}
}

void AGASPCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Parameters;
	Parameters.bIsPushBased = true;

	// Replicate to everyone except owner
	Parameters.Condition = COND_SkipOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, RotationMode, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MovementMode, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MovementState, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, StanceMode, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, LocomotionAction, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, TraversalComponent, Parameters);

	// Replicate to everyone
	Parameters.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, OverlayMode, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PoseMode, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, RagdollTargetLocation, Parameters);
}

void AGASPCharacter::SetGait(const FGameplayTag NewGait, const bool bForce)
{
	if (!ensure(GetMoverComponent()))
	{
		return;
	}

	if (NewGait != Gait || bForce)
	{
		const auto OldGait{Gait};
		Gait = NewGait;
		// MovementComponent->SetGait(NewGait);

		GaitChanged.Broadcast(OldGait, Gait);
	}
}

void AGASPCharacter::SetRotationMode(const FGameplayTag NewRotationMode, const bool bForce)
{
	if (NewRotationMode != RotationMode || bForce)
	{
		const auto OldRotationMode{RotationMode};
		RotationMode = NewRotationMode;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, RotationMode, this);

		// MovementComponent->SetRotationMode(NewRotationMode);

		if (GetLocalRole() == ROLE_AutonomousProxy && IsValid(GetNetConnection()))
		{
			Server_SetRotationMode(NewRotationMode);
		}

		RotationModeChanged.Broadcast(OldRotationMode, RotationMode);
	}
}

void AGASPCharacter::SetMovementMode(const FGameplayTag NewMovementMode, const bool bForce)
{
	if (NewMovementMode != MovementMode || bForce)
	{
		auto OldMovementMode{MovementMode};
		MovementMode = NewMovementMode;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MovementMode, this);
		if (GetLocalRole() == ROLE_AutonomousProxy && IsValid(GetNetConnection()))
		{
			Server_SetMovementMode(NewMovementMode);
		}

		StateContainer.RemoveTag(OldMovementMode);
		StateContainer.AddTagFast(MovementMode);

		MovementModeChanged.Broadcast(OldMovementMode, MovementMode);
	}
}

void AGASPCharacter::Server_SetMovementMode_Implementation(const FGameplayTag NewMovementMode)
{
	SetMovementMode(NewMovementMode);
}

void AGASPCharacter::Server_SetRotationMode_Implementation(const FGameplayTag NewRotationMode)
{
	SetRotationMode(NewRotationMode);
}

void AGASPCharacter::SetMovementState(const FGameplayTag NewMovementState, const bool bForce)
{
	if (NewMovementState != MovementState || bForce)
	{
		const auto OldMovementState{MovementState};
		MovementState = NewMovementState;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MovementState, this);

		if (GetLocalRole() == ROLE_AutonomousProxy && IsValid(GetNetConnection()))
		{
			Server_SetMovementState(NewMovementState);
		}
		MovementStateChanged.Broadcast(OldMovementState, MovementState);
	}
}

void AGASPCharacter::SetStanceMode(const FGameplayTag NewStanceMode, const bool bForce)
{
	if (StanceMode != NewStanceMode || bForce)
	{
		const auto OldStanceMode{StanceMode};
		StanceMode = NewStanceMode;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, StanceMode, this);

		// MovementComponent->SetStance(NewStanceMode);

		if (GetLocalRole() == ROLE_AutonomousProxy && IsValid(GetNetConnection()))
		{
			Server_SetStanceMode(NewStanceMode);
		}
		StanceModeChanged.Broadcast(OldStanceMode, StanceMode);
	}
}

void AGASPCharacter::Server_SetStanceMode_Implementation(const FGameplayTag NewStanceMode)
{
	SetStanceMode(NewStanceMode);
}

void AGASPCharacter::Server_SetMovementState_Implementation(const FGameplayTag NewMovementState)
{
	SetMovementState(NewMovementState);
}

bool AGASPCharacter::CanSprint()
{
	if (RotationMode == RotationTags::OrientToMovement)
	{
		return true;
	}
	if (RotationMode == RotationTags::Aim)
	{
		return false;
	}

	const float Dot = FVector::DotProduct(MoverCustomInputs_PreSim.GetMoveInput().GetSafeNormal2D(),
	                                      GetActorForwardVector().GetSafeNormal2D());

	return Dot > FMath::Cos(FMath::DegreesToRadians(50.f));
}

void AGASPCharacter::Jump()
{
	bPressedJump = true;
}

void AGASPCharacter::StopJumping()
{
	bPressedJump = false;
}

void AGASPCharacter::SetOverlayMode(const FGameplayTag NewOverlayMode, const bool bForce)
{
	if (NewOverlayMode != OverlayMode || bForce)
	{
		const auto OldOverlayMode{OverlayMode};
		OverlayMode = NewOverlayMode;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, OverlayMode, this);
		if (GetLocalRole() == ROLE_AutonomousProxy)
		{
			Server_SetOverlayMode(NewOverlayMode);
		}
		OverlayModeChanged.Broadcast(OldOverlayMode, OverlayMode);
	}
}

void AGASPCharacter::SetPoseMode(const FGameplayTag NewPoseMode, const bool bForce)
{
	if (NewPoseMode != PoseMode || bForce)
	{
		const auto OldPoseMode{PoseMode};
		PoseMode = NewPoseMode;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, OverlayMode, this);
		if (GetLocalRole() == ROLE_AutonomousProxy)
		{
			Server_SetOverlayMode(NewPoseMode);
		}
		PoseModeChanged.Broadcast(OldPoseMode, PoseMode);
	}
}

void AGASPCharacter::Server_SetPoseMode_Implementation(const FGameplayTag NewPoseMode)
{
	SetPoseMode(NewPoseMode);
}

void AGASPCharacter::SetLocomotionAction(const FGameplayTag NewLocomotionAction, const bool bForce)
{
	if (NewLocomotionAction != LocomotionAction || bForce)
	{
		const auto OldLocomotionAction{LocomotionAction};
		LocomotionAction = NewLocomotionAction;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, LocomotionAction, this);

		if (GetLocalRole() == ROLE_AutonomousProxy)
		{
			Server_SetLocomotionAction(NewLocomotionAction);
		}

		LocomotionActionChanged.Broadcast(OldLocomotionAction, LocomotionAction);
	}
}

void AGASPCharacter::Server_SetLocomotionAction_Implementation(const FGameplayTag NewLocomotionAction)
{
	SetLocomotionAction(NewLocomotionAction);
}

void AGASPCharacter::Server_SetOverlayMode_Implementation(const FGameplayTag NewOverlayMode)
{
	SetOverlayMode(NewOverlayMode);
}

bool AGASPCharacter::HasFullMovementInput() const
{
	if (MovementStickMode == EAnalogStickBehaviorMode::FixedWalkRun || MovementStickMode ==
		EAnalogStickBehaviorMode::VariableWalkRun)
	{
		return GetPendingMovementInputVector().Size2D() >= AnalogMovementThreshold;
	}

	return true;
}

FVector2D AGASPCharacter::GetMovementInputScaleValue(const FVector2D InVector) const
{
	return MovementStickMode > EAnalogStickBehaviorMode::FixedWalkRun ? InVector : InVector.GetSafeNormal();
}

void AGASPCharacter::ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult)
{
	OnProduceInput(static_cast<float>(SimTimeMs), InputCmdResult);

	if (bHasProduceInputBpFunc)
	{
		InputCmdResult = OnProduceInputInBlueprint(static_cast<float>(SimTimeMs), InputCmdResult);
	}
}

void AGASPCharacter::GetMovementDirectionAddOffset(EMovementDirection& MovementDirection, float& RotationOffset)
{
	if (MoverCustomInputs_PreSim.RotationMode == RotationTags::OrientToMovement)
	{
		DebugAngle = RotationOffset = 0.f;
		MovementDirection = EMovementDirection::F;
		return;
	}

	FVector DirectionOfMovement{FVector::ZeroVector};
	if (MovementMode == MovementModeTags::Grounded)
	{
		DirectionOfMovement = MoverCustomInputs_PreSim.GetMoveInput();
	}
	else if (MovementMode == MovementModeTags::InAir || MovementMode == MovementModeTags::Slide)
	{
		DirectionOfMovement = GetMoverComponent()->GetVelocity().GetSafeNormal();
	}

	if (DirectionOfMovement.IsZero())
	{
		DebugAngle = RotationOffset = 0.f;
		MovementDirection = EMovementDirection::F;

		return;
	}

	float MovementDirectionAngle = DebugAngle = (DirectionOfMovement.ToOrientationRotator() - MoverCustomInputs_PreSim.
		OrientationIntent.ToOrientationRotator()).Yaw;

	if (MoverCustomInputs_PreSim.MovementDirection == EMovementDirection::F)
	{
		const float Offset = MoverCustomInputs_PreSim.RotationOffset == 0
			                     ? (GetActorRotation() - MoverCustomInputs_PreSim.OrientationIntent.
				                     ToOrientationRotator()).Yaw
			                     : MoverCustomInputs_PreSim.RotationOffset;

		if (Offset > 0.f)
		{
			MovementDirectionAngle = DebugAngle = MovementDirectionAngle >= -180.f && MovementDirectionAngle <= -170.f
				                                      ? 179.f
				                                      : MovementDirectionAngle;
		}
		else
		{
			MovementDirectionAngle = DebugAngle = MovementDirectionAngle >= 170.f && MovementDirectionAngle <= 180.f
				                                      ? -179.f
				                                      : MovementDirectionAngle;
		}
	}

	auto GetDirectionThresholds = [this](const int32 Style)
	{
		switch (Style)
		{
		case 1:
			if (MoverCustomInputs_PreSim.MovementDirection == EMovementDirection::B)
			{
				return FVector4f{-120.f, 120.f, -120.f, 120.f};
			}
			return FVector4f{-140.f, 140.f, -140.f, 140.f};
		case 2:
			return FVector4f{-180.f, 180.f, -180.f, 180.f};
		default:
			if (MoverCustomInputs_PreSim.MovementDirection == EMovementDirection::B || MoverCustomInputs_PreSim.
				MovementDirection == EMovementDirection::F)
			{
				return FVector4f{-60.f, 60.f, -120.f, 120.f};
			}
			return FVector4f{-40.f, 40.f, -140.f, 140.f};
		}
	};

	MovementDirection = MoverCustomInputs_PreSim.Gait != GaitTags::Sprint
		                    ? FGASPMath::GetMovementDirectionFromThreshold(
			                    GetDirectionThresholds(RotationMode == RotationTags::Aim
				                                           ? GeneralVars::AimStyle
				                                           : GeneralVars::StrafeStyle), MovementDirectionAngle)
		                    : EMovementDirection::F;
	if (RotationCurveTable)
	{
		if (const auto* RotationCurve = static_cast<UCurveFloat*>(UChooserFunctionLibrary::EvaluateChooser(
			this, RotationCurveTable, UCurveFloat::StaticClass())))
		{
			RotationOffset = RotationCurve->GetFloatValue(MovementDirectionAngle);
		}
	}
}

void AGASPCharacter::OnProduceInput(float DeltaMs, FMoverInputCmdContext& InputCmdResult)
{
	auto& CharacterInputs = InputCmdResult.InputCollection.FindOrAddMutableDataByType<FGASPMoverInputs>();

	if (GetController() == nullptr)
	{
		if (GetLocalRole() == ROLE_Authority && GetRemoteRole() == ROLE_SimulatedProxy)
		{
			static const FGASPMoverInputs DoNothingInput;
			// If we get here, that means this pawn is not currently possessed and we're choosing to provide default do-nothing input
			CharacterInputs = DoNothingInput;
		}

		// We don't have a local controller so we can't run the code below. This is ok. Simulated proxies will just use previous input when extrapolating
		return;
	}

	CharacterInputs.SetMoveInput(EMoveInputType::DirectionalIntent, GetMovementInputVector());
	CharacterInputs.ControlRotation = GetAimingRotation();
	CharacterInputs.bIsJumpJustPressed = bPressedJump;
	CharacterInputs.OrientationIntent = GetOrientationIntent();
	CharacterInputs.RotationMode = RotationMode;
	CharacterInputs.Gait = Gait;
	CharacterInputs.ControlRotationRate = ControlRotationRate;
	CharacterInputs.Stance = PlayerInputState.DesiredStance;
	GetMovementDirectionAddOffset(CharacterInputs.MovementDirection, CharacterInputs.RotationOffset);
	MoverCustomInputs_PreSim = CharacterInputs;
}

void AGASPCharacter::RefreshInputMover()
{
	auto LastInputCmd = GetMoverComponent()->GetLastInputCmd();

	if (auto Collection = LastInputCmd.InputCollection.FindMutableDataByType<FGASPMoverInputs>())
	{
		MoverCustomInputs_PostSim = *Collection;
	}
}

void AGASPCharacter::RefreshFloorValues()
{
	if (FHitResult HitResult; GetMoverComponent()->TryGetFloorCheckHitResult(HitResult))
	{
		MoverCustomInputs_PostSim.FloorLocation = HitResult.ImpactPoint;
		MoverCustomInputs_PostSim.FloorNormal = HitResult.ImpactNormal;
	}
	else
	{
		MoverCustomInputs_PostSim.FloorLocation = GetMesh()->GetComponentLocation();
		MoverCustomInputs_PostSim.FloorNormal = FVector::ZeroVector;
	}
}

void AGASPCharacter::RefreshControlRotationRate(const float DeltaTime)
{
	ControlRotationRate = (GetControlRotation() - LastControlRotation).Yaw / DeltaTime;
	LastControlRotation = GetControlRotation();
}

void AGASPCharacter::RefreshTwinStickMode()
{
	if (!TwinStickMode || !GetController())
	{
		return;
	}

	GetController()->SetControlRotation(FRotator::ZeroRotator);

	//TODO: Add TwinStickAimDirection to global?
	auto TwinStickAimDirection{FVector2D::ZeroVector};
	if (TwinStickAimDirection.IsNearlyZero(.1f))
	{
		// TwinStickAimRotation = FRotator{
		// 	FQuat{GetControlRotation()} * FQuat{
		// 		FRotator{0.f, FMath::Atan2(TwinStickAimDirection.Y, TwinStickAimDirection.X * -1), 0.f}
		// 	}
		// };

		const float StickYaw = FMath::RadiansToDegrees(FMath::Atan2(TwinStickAimDirection.Y, -TwinStickAimDirection.X));
		TwinStickAimRotation = FRotator(0.f, GetControlRotation().Yaw + StickYaw, 0.f);
	}
	else
	{
		TwinStickAimRotation = GetActorRotation();
	}
}

void AGASPCharacter::RefreshRotationMode()
{
	// TODO: targeted actor?
	if (TwinStickMode)
	{
		FVector2D TwinStickAimDirection;
		if (!TwinStickAimDirection.IsZero())
		{
			if (PlayerInputState.DesiredRotationMode == RotationTags::Aim)
			{
				SetRotationMode(RotationTags::Aim);
				return;
			}
			SetRotationMode(RotationTags::Strafe);
			return;
		}
		SetRotationMode(RotationTags::OrientToMovement);
		return;
	}
	SetRotationMode(PlayerInputState.DesiredRotationMode);
}

void AGASPCharacter::RefreshSlidingAudio()
{
}

FVector AGASPCharacter::GetMovementInputVector()
{
	if (Cast<AAIController>(GetController()))
	{
		FVector OutInputIntent, OutInputVelocity;
		NavMoverComponent->ConsumeNavMovementData(OutInputIntent, OutInputVelocity);
		return OutInputVelocity.GetSafeNormal();
	}

	const auto YawRotation = FRotator(0.f, GetControlRotation().Yaw, 0.f);
	return YawRotation.RotateVector(ConsumeMovementInputVector().GetClampedToSize(0.f, 1.f)).
	                   GetSafeNormal();
}

FVector AGASPCharacter::GetOrientationIntent()
{
	const FVector AimVector = FRotator(0.f, GetAimingRotation().Yaw, 0.f).Vector();
	const bool bOrientToMove = RotationMode == RotationTags::OrientToMovement;

	if (MovementMode == MovementModeTags::Slide)
	{
		return bOrientToMove ? GetMoverComponent()->GetVelocity().GetSafeNormal() : AimVector;
	}

	const FVector MoveInput = MoverCustomInputs_PreSim.GetMoveInput();
	if (MovementMode == MovementModeTags::Grounded)
	{
		if (!MoveInput.IsZero())
		{
			return bOrientToMove ? MoveInput : AimVector;
		}
		const float YawDiff = FMath::Abs((GetActorRotation() - GetAimingRotation()).Yaw);
		const bool bShouldTurnInPlace = RotationMode == RotationTags::Aim && YawDiff > 60.f;

		return bShouldTurnInPlace ? AimVector : MoverCustomInputs_PreSim.OrientationIntent;
	}

	if (MovementMode == MovementModeTags::InAir)
	{
		return bOrientToMove ? MoverCustomInputs_PreSim.OrientationIntent : AimVector;
	}

	return GetActorRotation().Vector();
}

FRotator AGASPCharacter::GetAimingRotation()
{
	return TwinStickMode ? TwinStickAimRotation : GetControlRotation();
}

FVector AGASPCharacter::GetNavAgentLocation() const
{
	FVector AgentLocation = FNavigationSystem::InvalidLocation;
	const USceneComponent* UpdatedComponent = CharacterMotionComponent
		                                          ? CharacterMotionComponent->GetUpdatedComponent()
		                                          : nullptr;

	if (NavMoverComponent)
	{
		AgentLocation = NavMoverComponent->GetFeetLocation();
	}

	if (FNavigationSystem::IsValidLocation(AgentLocation) == false && UpdatedComponent != nullptr)
	{
		AgentLocation = UpdatedComponent->GetComponentLocation() - FVector(0, 0, UpdatedComponent->Bounds.BoxExtent.Z);
	}

	return AgentLocation;
}

void AGASPCharacter::UpdateNavigationRelevance()
{
	if (CharacterMotionComponent)
	{
		if (auto* UpdatedComponent = CharacterMotionComponent->GetUpdatedComponent())
		{
			UpdatedComponent->SetCanEverAffectNavigation(bCanAffectNavigationGeneration);
		}
	}
}

void AGASPCharacter::RefreshGait()
{
	auto& DesiredGait{PlayerInputState.DesiredGait};
	auto NewGait{PlayerInputState.DesiredGait};

	if (DesiredGait == GaitTags::Sprint && CanSprint())
	{
		NewGait = HasFullMovementInput() ? GaitTags::Sprint : GaitTags::Run;
	}
	else if (DesiredGait == GaitTags::Walk)
	{
		NewGait = GaitTags::Walk;
	}
	else if (DesiredGait == GaitTags::Sprint || DesiredGait == GaitTags::Run)
	{
		NewGait = HasFullMovementInput() ? GaitTags::Run : GaitTags::Walk;
	}

	SetGait(NewGait);
}

FTraversalResult AGASPCharacter::TryTraversalAction() const
{
	if (IsValid(TraversalComponent))
	{
		return TraversalComponent->TryTraversalAction(GetTraversalCheckInputs());
	}

	return {true, false};
}

bool AGASPCharacter::IsDoingTraversal() const
{
	return IsValid(TraversalComponent) && TraversalComponent->IsDoingTraversal();
}

FTraversalCheckInputs AGASPCharacter::GetTraversalCheckInputs() const
{
	const FVector ForwardVector{GetActorForwardVector()};
	if (MovementMode == MovementModeTags::InAir)
	{
		return {
			!MoverCustomInputs_PostSim.GetMoveInput().IsZero()
				? MoverCustomInputs_PostSim.GetMoveInput().GetSafeNormal()
				: ForwardVector,
			75.f, FVector::ZeroVector, {0.f, 0.f, 50.f}, 30.f, 86.f
		};
	}

	const FVector RotationVector = GetMoverComponent()->GetVelocity().Size2D() > 50.f
		                               ? GetMoverComponent()->GetVelocity().GetSafeNormal()
		                               : GetMoverComponent()->GetTargetOrientation().Vector();
	const float ClampedDistance = FMath::GetMappedRangeValueClamped<float, float>(
		{0.f, 375.f}, {75.f, 350.f}, RotationVector.X);

	return {
		ForwardVector, ClampedDistance, FVector::ZeroVector,
		FVector::ZeroVector, 30.f, 60.f
	};
}

void AGASPCharacter::LinkAnimInstance(const UChooserTable* DataTable, const FGameplayTag OldState,
                                      const FGameplayTag State)
{
	if (!DataTable)
	{
		return;
	}

	StateContainer.RemoveTag(OldState);
	StateContainer.AddLeafTag(State);

	auto* MeshComponent = GetMesh();
	if (!IsValid(MeshComponent))
	{
		return;
	}
	const auto* DataAsset{
		static_cast<UGASPLinkedAnimInstanceSet*>(UChooserFunctionLibrary::EvaluateChooser(
			this, DataTable, UGASPLinkedAnimInstanceSet::StaticClass()))
	};

	if (IsValid(DataAsset))
	{
		MeshComponent->LinkAnimClassLayers(DataAsset->GetAnimInstance());
	}
}

void AGASPCharacter::OnPoseModeChanged_Implementation(const FGameplayTag OldPoseMode, const FGameplayTag NewPoseMode)
{
	LinkAnimInstance(PosesTable, OldPoseMode, NewPoseMode);
}

void AGASPCharacter::OnOverlayModeChanged_Implementation(const FGameplayTag OldOverlayMode, const FGameplayTag NewOverlayMode)
{
	LinkAnimInstance(OverlayTable, OldOverlayMode, NewOverlayMode);
}

void AGASPCharacter::OnRep_OverlayMode(const FGameplayTag& OldOverlayMode)
{
	OverlayModeChanged.Broadcast(OldOverlayMode, OverlayMode);
}

void AGASPCharacter::OnRep_PoseMode(const FGameplayTag& OldPoseMode)
{
	PoseModeChanged.Broadcast(OldPoseMode, PoseMode);
}

void AGASPCharacter::OnRep_Gait(const FGameplayTag& OldGait)
{
	GaitChanged.Broadcast(OldGait, Gait);
}

void AGASPCharacter::OnRep_StanceMode(const FGameplayTag& OldStanceMode)
{
	StanceModeChanged.Broadcast(OldStanceMode, StanceMode);
}

void AGASPCharacter::OnRep_MovementMode(const FGameplayTag& OldMovementMode)
{
	MovementModeChanged.Broadcast(OldMovementMode, MovementMode);
}

void AGASPCharacter::OnRep_RotationMode(const FGameplayTag& OldRotationMode)
{
	RotationModeChanged.Broadcast(OldRotationMode, RotationMode);
}

void AGASPCharacter::OnRep_MovementState(const FGameplayTag& OldMovementState)
{
	MovementStateChanged.Broadcast(OldMovementState, MovementState);
}

void AGASPCharacter::OnRep_LocomotionAction(const FGameplayTag& OldLocomotionAction)
{
	LocomotionActionChanged.Broadcast(OldLocomotionAction, LocomotionAction);
}

void AGASPCharacter::OnMovementModeChanged(const FName& PreviousMovementModeName, const FName& NewMovementModeName)
{
	if (NewMovementModeName == DefaultModeNames::Flying)
	{
		SetMovementMode(MovementModeTags::Traverse);
	}
	else if (NewMovementModeName == DefaultModeNames::Falling)
	{
		SetMovementMode(MovementModeTags::InAir);
	}
	else if (NewMovementModeName == MovementModeNames::Sliding)
	{
		SetMovementMode(MovementModeTags::Slide);
	}
	else
	{
		SetMovementMode(MovementModeTags::Grounded);
	}
}

void AGASPCharacter::OnPreSimulateTick(const FMoverTimeStep& TimeStep, const FMoverInputCmdContext& InputCmd)
{
	auto CharacterInputs = InputCmd.InputCollection.FindMutableDataByType<FGASPMoverInputs>();
	if (!CharacterInputs)
	{
		return;
	}

	if (CharacterInputs->Stance == StanceTags::Crouching)
	{
		GetMoverComponent()->Crouch();
	}
	else
	{
		GetMoverComponent()->UnCrouch();
	}
}

void AGASPCharacter::OnStanceChanged(EStanceMode OldStance, EStanceMode NewStance)
{
	switch (NewStance)
	{
	case EStanceMode::Crouch:
		SetStanceMode(StanceTags::Crouching);
		break;
	default:
		SetStanceMode(StanceTags::Standing);
	}
}
