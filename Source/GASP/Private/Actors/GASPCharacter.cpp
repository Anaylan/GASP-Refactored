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
#include "Utils/GASPBlueprintLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPCharacter)

namespace GeneralVars
{
	int32 AimStyle{0};
	FAutoConsoleVariableRef CVarAimStyleStruct(
		TEXT("gasp.movement.style.aim"), AimStyle, TEXT("set style for aim rotation mode"), ECVF_Default);

	int32 StrafeStyle{0};
	FAutoConsoleVariableRef CVarStrafeStyleStruct(
		TEXT("gasp.movement.style.strafe"), StrafeStyle, TEXT("set style for strafe rotation mode"), ECVF_Default);

	int32 ControlStyle{0};
	FAutoConsoleVariableRef CVarControlStyleStruct(
		TEXT("gasp.control.style"), StrafeStyle, TEXT("set style for strafe rotation mode"), ECVF_Default);
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

	SetReplicates(true);
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

	RefreshFloorValues();
	RefreshControlRotationRate(DeltaTime);
	RefreshTwinStickMode();

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

	TwinStickMode = GeneralVars::ControlStyle;
	GeneralVars::CVarControlStyleStruct->OnChangedDelegate().AddWeakLambda(this, [this](const IConsoleVariable* CVar)
	{
		TwinStickMode = CVar ? CVar->GetInt() == 1 : false;
	});
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
	if (NewGait != Gait || bForce)
	{
		const auto OldGait{Gait};
		Gait = NewGait;

		if (GetLocalRole() == ROLE_AutonomousProxy && IsValid(GetNetConnection()))
		{
			Server_SetGait(NewGait);
		}
		GaitChanged.Broadcast(OldGait, Gait);
	}
}

void AGASPCharacter::Server_SetGait_Implementation(const FGameplayTag NewGait)
{
	SetGait(NewGait);
}

void AGASPCharacter::SetRotationMode(const FGameplayTag NewRotationMode, const bool bForce)
{
	if (NewRotationMode != RotationMode || bForce)
	{
		const auto OldRotationMode{RotationMode};
		RotationMode = NewRotationMode;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, RotationMode, this);

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

		StateContainer.RemoveTag(OldMovementMode);
		StateContainer.AddTagFast(MovementMode);

		MovementModeChanged.Broadcast(OldMovementMode, MovementMode);
	}
}

void AGASPCharacter::SetStanceMode(const FGameplayTag NewStanceMode, const bool bForce)
{
	if (StanceMode != NewStanceMode || bForce)
	{
		const auto OldStanceMode{StanceMode};
		StanceMode = NewStanceMode;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, StanceMode, this);

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
	bJustPressedJump = true;
}

void AGASPCharacter::StopJumping()
{
	bJustPressedJump = false;
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
	auto& CharacterInputs = InputCmdResult.InputCollection.FindOrAddMutableDataByType<FGASPMoverInputs>();
	if (GetController() == nullptr)
	{
		if (GetLocalRole() == ENetRole::ROLE_Authority && GetRemoteRole() == ENetRole::ROLE_SimulatedProxy)
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
	CharacterInputs.bIsJumpJustPressed = bJustPressedJump;
	CharacterInputs.OrientationIntent = GetOrientationIntent();
	CharacterInputs.RotationMode = RotationMode;
	CharacterInputs.Gait = Gait;
	CharacterInputs.ControlRotationRate = ControlRotationRate;
	CharacterInputs.Stance = PlayerInputState.DesiredStance;
	GetMovementDirectionAddOffset(CharacterInputs.MovementDirection, CharacterInputs.RotationOffset);

	MoverCustomInputs_PreSim = CharacterInputs;
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

	const FVector OrientationDir = MoverCustomInputs_PreSim.OrientationIntent.GetSafeNormal2D();
	float Dot = FVector::DotProduct(DirectionOfMovement.GetSafeNormal2D(), OrientationDir);
	const float CrossZ = FVector::CrossProduct(OrientationDir, DirectionOfMovement.GetSafeNormal2D()).Z;

	// Result is strictly between -180 and 180
	float MovementAngle = DebugAngle = FMath::RadiansToDegrees(FMath::Atan2(CrossZ, Dot));

	if (MoverCustomInputs_PreSim.MovementDirection == EMovementDirection::F)
	{
		float CurrentOffset = MoverCustomInputs_PreSim.RotationOffset;

		if (FMath::IsNearlyZero(CurrentOffset))
		{
			const FVector ActorForward = GetActorForwardVector().GetSafeNormal2D();
			const float ActorDot = FVector::DotProduct(ActorForward, OrientationDir);
			const float ActorCrossZ = FVector::CrossProduct(OrientationDir, ActorForward).Z;
			CurrentOffset = FMath::RadiansToDegrees(FMath::Atan2(ActorCrossZ, ActorDot));
		}

		if (CurrentOffset > 0.f && FMath::IsWithinInclusive(MovementAngle, -180.f, -170.f))
		{
			MovementAngle = DebugAngle = 179.f;
		}
		else if (CurrentOffset <= 0.f && FMath::IsWithinInclusive(MovementAngle, 170.f, 180.f))
		{
			MovementAngle = DebugAngle = -179.f;
		}
	}

	MovementDirection = MoverCustomInputs_PreSim.Gait != GaitTags::Sprint
		                    ? FGASPMath::GetMovementDirectionFromThreshold(
			                    UGASPBlueprintLibrary::GetDirectionThresholds(
				                    MoverCustomInputs_PreSim.MovementDirection, RotationMode == RotationTags::Aim
						                    ? GeneralVars::AimStyle
						                    : GeneralVars::StrafeStyle), MovementAngle)
		                    : EMovementDirection::F;

	if (RotationCurveTable)
	{
		if (const auto* RotationCurve = static_cast<UCurveFloat*>(UChooserFunctionLibrary::EvaluateChooser(
			this, RotationCurveTable, UCurveFloat::StaticClass())))
		{
			RotationOffset = RotationCurve->GetFloatValue(MovementAngle);
		}
	}
}

void AGASPCharacter::RefreshFloorValues()
{
	if (FHitResult HitResult; GetMoverComponent()->TryGetFloorCheckHitResult(HitResult))
	{
		MoverCustomInputs_PreSim.FloorLocation = HitResult.ImpactPoint;
		MoverCustomInputs_PreSim.FloorNormal = HitResult.ImpactNormal;
	}
	else
	{
		MoverCustomInputs_PreSim.FloorLocation = GetMesh()->GetComponentLocation();
		MoverCustomInputs_PreSim.FloorNormal = FVector::ZeroVector;
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

	if (GetController())
	{
		GetController()->SetControlRotation(FRotator::ZeroRotator);

		if (TwinStickAimDirection.IsNearlyZero(.1f))
		{
			TwinStickAimRotation = FRotator{
				FQuat{GetControlRotation()} * FQuat{
					FRotator{
						0.f,
						(180.0) / UE_DOUBLE_PI * FMath::Atan2(TwinStickAimDirection.Y, TwinStickAimDirection.X * -1),
						0.f
					}
				}
			};

			const float StickYaw = FMath::RadiansToDegrees(
				FMath::Atan2(TwinStickAimDirection.Y, -TwinStickAimDirection.X));
			TwinStickAimRotation = FRotator(0.f, GetControlRotation().Yaw + StickYaw, 0.f);
		}
		else
		{
			TwinStickAimRotation = GetActorRotation();
		}
	}
}

void AGASPCharacter::RefreshRotationMode()
{
	// TODO: targeted actor?
	if (TwinStickMode)
	{
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

const FGASPMoverInputs& AGASPCharacter::GetMoverState() const
{
	auto InputCmdContext = GetMoverComponent()->GetLastInputCmd();
	auto Input = InputCmdContext.InputCollection.FindDataByType<FGASPMoverInputs>();

	static const FGASPMoverInputs DoNothingInput;
	return Input ? *Input : DoNothingInput;
}

FVector AGASPCharacter::GetMovementInputVector()
{
	if (Cast<AAIController>(GetController()))
	{
		FVector OutInputIntent, OutInputVelocity;
		NavMoverComponent->ConsumeNavMovementData(OutInputIntent, OutInputVelocity);
		return OutInputVelocity.GetSafeNormal();
	}

	const FRotator YawRotation{0.f, GetControlRotation().Yaw, 0.f};
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

		const float YawDiff = FMath::Abs((GetActorRotation() - GetAimingRotation()).GetNormalized().Yaw);
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
		auto Inputs{GetMoverState()};
		return {
			!Inputs.GetMoveInput().IsZero() ? Inputs.GetMoveInput().GetSafeNormal() : ForwardVector,
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

void AGASPCharacter::OnOverlayModeChanged_Implementation(const FGameplayTag OldOverlayMode,
                                                         const FGameplayTag NewOverlayMode)
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
