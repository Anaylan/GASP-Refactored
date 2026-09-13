#include "Actors/GASPCharacter.h"

#include "Animation/AnimInstance.h"
#include "ChooserFunctionLibrary.h"
#include "MotionWarpingComponent.h"
#include "GameplayTagContainer.h"
#include "GameplayTasksComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/GASPTraversalComponent.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Utils/GASPLinkedAnimInstanceSet.h"
#include "DefaultMovementSet/NavMoverComponent.h"
#include "MovementSet/GASPMoverComponent.h"
#include "MovementSet/Modes/MovementMode_Sliding.h"
#include "GASP.h"
#include "Settings/GASPCharacterSettings.h"
#include "PhysicsControlComponent.h"
#include "Components/GASPCharacterInteractionComponent.h"
#include "Components/GASPOverrideModeManager.h"
#include "MovementSet/Modes/MovementMode_Ragdolling.h"
#include "Tasks/CharacterTask_Ragdoll.h"
#include "UniversalObjectLocators/AnimInstanceLocatorFragment.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPCharacter)

#define SYNC_SINGLE_TAG(OldTag, NewTag) \
if ((OldTag).IsValid()) \
{ \
GameplayTags.RemoveTag(OldTag); \
} \
if ((NewTag).IsValid()) \
{ \
GameplayTags.AddLeafTag(NewTag); \
}

namespace GeneralVars
{
	int32 AimStyle{0};
	FAutoConsoleVariableRef CVarAimStyleStruct(
		TEXT("gasp.movement.style.aim"), AimStyle, TEXT("set style for aim rotation mode"), ECVF_Default);

	int32 StrafeStyle{1};
	FAutoConsoleVariableRef CVarStrafeStyleStruct(
		TEXT("gasp.movement.style.strafe"), StrafeStyle, TEXT("set style for strafe rotation mode"), ECVF_Default);

	int32 ControlStyle{0};
	FAutoConsoleVariableRef CVarControlStyleStruct(
		TEXT("gasp.control.style"), ControlStyle, TEXT("set style for strafe rotation mode"), ECVF_Default);

	int32 AnalogInputStyle{0};
	FAutoConsoleVariableRef CVarAnalogInputStyleStruct(
		TEXT("gasp.analoginput"), AnalogInputStyle, TEXT(""), ECVF_Default);

	int32 PhysicsProfileIndex{0};
	FAutoConsoleVariableRef CVarPhysicsProfileIndexStruct(
		TEXT("gasp.physics.profile"), AnalogInputStyle, TEXT(""), ECVF_Default);
}


FName AGASPCharacter::MeshComponentName(TEXT("CharacterMesh0"));
FName AGASPCharacter::CapsuleComponentName(TEXT("CollisionCylinder"));
FName AGASPCharacter::MotionWarpingComponentName(TEXT("MotionWarping"));
FName AGASPCharacter::CharacterMotionComponentName(TEXT("MoverComponent"));
FName AGASPCharacter::NavMoverComponentName(TEXT("NavMoverComponent"));
FName AGASPCharacter::PhysicsControlComponentName(TEXT("PhysicsControl"));

AGASPCharacter::AGASPCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	SetReplicates(true);
	SetReplicatingMovement(false);

	// Default limbs for the UE mannequin skeleton. Data, not logic: a character with a different
	// skeleton or a different number of legs overrides this in its Blueprint.
	RagdollSelfCollisionGroups = {
		FGASPBodyGroup{{TEXT("thigh_l"), TEXT("calf_l"), TEXT("foot_l")}},
		FGASPBodyGroup{{TEXT("thigh_r"), TEXT("calf_r"), TEXT("foot_r")}}
	};

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(CapsuleComponentName);
	CapsuleComponent->InitCapsuleSize(34.0f, 88.0f);
	CapsuleComponent->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	CapsuleComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

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
		Mesh->PhysicsTransformUpdateMode = EPhysicsTransformUpdateMode::ComponentTransformIsKinematic;
		Mesh->SetupAttachment(CapsuleComponent);

		Mesh->SetCollisionProfileName(TEXT("Ragdoll"));
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetCanEverAffectNavigation(false);

		Mesh->SetRelativeRotation_Direct({0.f, -90.f, 0.f});
		Mesh->SetRelativeLocation_Direct({0.f, 0.f, -90.f});
	}

	CharacterMotionComponent = CreateDefaultSubobject<UGASPMoverComponent>(CharacterMotionComponentName);
	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(MotionWarpingComponentName);
	TraversalComponent = CreateDefaultSubobject<UGASPTraversalComponent>(TEXT("TraversalComponent"));
	OverrideModeManager = CreateDefaultSubobject<UGASPOverrideModeManager>(TEXT("OverrideModeManager"));
	NavMoverComponent = CreateDefaultSubobject<UNavMoverComponent>(NavMoverComponentName);
	InteractionComponent = CreateDefaultSubobject<UGASPCharacterInteractionComponent>(
		TEXT("CharacterInteractionComponent"));

	PhysicsControlComponent = CreateDefaultSubobject<UPhysicsControlComponent>(PhysicsControlComponentName);
	PhysicsControlComponent->SetupAttachment(GetMesh());
}

void AGASPCharacter::OnBasedMovementApplied(const FTransform& TransformDelta, const FMoverTimeStep& TimeStep)
{
	BasedMovementDelta = TransformDelta;
}

void AGASPCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Only the authority owns this container. Clients receive it through replication, and the
	// relative order of BeginPlay and OnRep is not guaranteed, so resetting here could discard
	// state that has already arrived.
	if (HasAuthority())
	{
		GameplayTags.Reset();
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, GameplayTags, this);
	}

	NavMoverComponent = FindComponentByClass<UNavMoverComponent>();

	// Mesh is created with CreateOptionalDefaultSubobject, so a subclass may legitimately opt out
	// of it. Everything below that drives animation needs it, so resolve it once up front.
	auto* MeshComponent{GetMesh()};

	if (const auto MoverComp = GetMoverComponent())
	{
		MoverComp->OnMovementModeChanged.AddDynamic(this, &ThisClass::OnMovementModeChanged);
		MoverComp->OnBasedMovementApplied.AddDynamic(this, &ThisClass::OnBasedMovementApplied);

		MoverComp->StanceModeChanged.AddDynamic(this, &ThisClass::OnMovementStateChanged);
		MoverComp->LocomotionModeChanged.AddDynamic(this, &ThisClass::OnMovementStateChanged);
		MoverComp->RotationModeChanged.AddDynamic(this, &ThisClass::OnMovementStateChanged);
		MoverComp->GaitChanged.AddDynamic(this, &ThisClass::OnMovementStateChanged);

		if (MeshComponent)
		{
			MeshComponent->AddTickPrerequisiteComponent(MoverComp);
		}
	}

	if (MeshComponent)
	{
		MeshRelativeTransformCache = MeshComponent->GetRelativeTransform();
		MeshComponent->AddTickPrerequisiteActor(this);
	}

	OverlayContainerChanged.AddDynamic(this, &ThisClass::OnOverlayModeChanged);
	PoseModeChanged.AddDynamic(this, &ThisClass::OnPoseModeChanged);

	SetPoseMode(PoseMode, true);
	SetLocomotionAction(FGameplayTag::EmptyTag, true);

	MoverInputs_PreSim.OrientationIntent = GetActorForwardVector();

	if (PhysicsControlComponent && MeshComponent)
	{
		PhysicsControlComponent->CreateControlsAndBodyModifiersFromPhysicsControlAsset(
			MeshComponent, nullptr, NAME_None);
	}

	ensureAlwaysMsgf(Settings, TEXT("Settings must be configured in character blueprint"));
}

void AGASPCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Mirror of the bindings made in BeginPlay. The mover component outlives this call during a
	// level teardown, so leaving them bound would keep dispatching into a destroyed actor.
	if (auto* MoverComp = GetMoverComponent())
	{
		MoverComp->OnMovementModeChanged.RemoveDynamic(this, &ThisClass::OnMovementModeChanged);
		MoverComp->OnBasedMovementApplied.RemoveDynamic(this, &ThisClass::OnBasedMovementApplied);

		MoverComp->StanceModeChanged.RemoveDynamic(this, &ThisClass::OnMovementStateChanged);
		MoverComp->LocomotionModeChanged.RemoveDynamic(this, &ThisClass::OnMovementStateChanged);
		MoverComp->RotationModeChanged.RemoveDynamic(this, &ThisClass::OnMovementStateChanged);
		MoverComp->GaitChanged.RemoveDynamic(this, &ThisClass::OnMovementStateChanged);
	}

	OverlayContainerChanged.RemoveDynamic(this, &ThisClass::OnOverlayModeChanged);
	PoseModeChanged.RemoveDynamic(this, &ThisClass::OnPoseModeChanged);

	// GetWorld() can already be null while the world is being torn down.
	if (auto* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}

	Super::EndPlay(EndPlayReason);
}

void AGASPCharacter::Tick(float DeltaTime)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("AGASPCharacter::Tick"),
	                            STAT_AGASPCharacter_Tick, STATGROUP_GASP)
	TRACE_CPUPROFILER_EVENT_SCOPE(__FUNCTION__);

	RefreshMoverState();
	RefreshFloorValues();
	RefreshControlRotationRate(DeltaTime);
	RefreshTwinStickMode();

	Super::Tick(DeltaTime);
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

	TwinStickMode = GeneralVars::ControlStyle >= 1;
	GeneralVars::CVarControlStyleStruct->OnChangedDelegate().AddWeakLambda(this, [this](const IConsoleVariable* CVar)
	{
		TwinStickMode = CVar ? CVar->GetInt() >= 1 : false;
	});

	{
		auto Profile{PhysicsProfiles[GeneralVars::PhysicsProfileIndex]};
		PhysicsProfileName = Profile;
		SetPhysicsProfile(Profile);
	}
	GeneralVars::CVarPhysicsProfileIndexStruct->OnChangedDelegate().AddWeakLambda(
		this, [this](const IConsoleVariable* CVar)
		{
			auto Profile{PhysicsProfiles[CVar ? CVar->GetInt() : 0]};
			PhysicsProfileName = Profile;
			SetPhysicsProfile(Profile);
		});

	if (Settings)
	{
		Settings->PreloadTables();
	}
}

void AGASPCharacter::RefreshMoverState()
{
	auto [InputCollection] = GetMoverComponent()->GetLastInputCmd();
	MoverInputs_PostSim = InputCollection.FindOrAddDataByType<FGASPMoverInputs>();
}

FGameplayTagContainer AGASPCharacter::BP_GetOwnedGameplayTags() const
{
	return IGameplayTagAssetInterface::BP_GetOwnedGameplayTags();
}

void AGASPCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Parameters;
	Parameters.bIsPushBased = true;

	// Replicate to everyone except owner.
	// TraversalComponent and PhysicsControlComponent are deliberately absent: both are default
	// subobjects created in the constructor, so every machine already has them and replicating the
	// pointers only costs bandwidth.
	Parameters.Condition = COND_SkipOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, LocomotionAction, Parameters);

	// Replicate to everyone.
	// TaskStates is here rather than above because COND_SkipOwner withheld it from the one
	// connection that also runs the tasks writing it: the owning client received no task state at
	// all, so nothing on it could read a state the authority had changed.
	Parameters.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, TaskStates, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, OverlayTagContainer, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PoseMode, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, GameplayTags, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, RagdollTask, Parameters);
}

bool AGASPCharacter::CanSprint()
{
	if (MoverInputs_PostSim.RotationMode == RotationTags::OrientToMovement)
	{
		return true;
	}

	const float Dot = FVector::DotProduct(MoverInputs_PostSim.GetMoveInput().GetSafeNormal2D(),
	                                      MoverInputs_PostSim.OrientationIntent.GetSafeNormal2D());

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

const UGASPCharacterSettings* AGASPCharacter::GetSettingsChecked() const
{
	if (Settings)
	{
		return Settings;
	}

	UE_LOG(LogGASP, Error, TEXT("%s: Settings asset is not assigned; falling back to class defaults"), *GetName());
	return GetDefault<UGASPCharacterSettings>();
}

FGameplayTag AGASPCharacter::GetMovementMode() const
{
	return GetMoverComponent() ? GetMoverComponent()->GetLocomotionMode() : FGameplayTag::EmptyTag;
}

FGameplayTag AGASPCharacter::GetStanceMode() const
{
	return GetMoverComponent() ? GetMoverComponent()->GetStanceMode() : FGameplayTag::EmptyTag;
}

void AGASPCharacter::SetOverlayMode(const FGameplayTagContainer NewOverlayMode)
{
	if (NewOverlayMode != OverlayTagContainer)
	{
		const auto OldOverlayTagContainer{OverlayTagContainer};

		OverlayTagContainer = NewOverlayMode;

		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, OverlayTagContainer, this);
		if (GetLocalRole() == ROLE_AutonomousProxy)
		{
			Server_SetOverlayMode(NewOverlayMode);
		}

		OverlayContainerChanged.Broadcast(OldOverlayTagContainer, OverlayTagContainer);
	}
}

void AGASPCharacter::SetPoseMode(const FGameplayTag NewPoseMode, const bool bForce)
{
	if (NewPoseMode != PoseMode || bForce)
	{
		SYNC_SINGLE_TAG(PoseMode, NewPoseMode);

		const auto OldPoseMode{PoseMode};
		PoseMode = NewPoseMode;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, PoseMode, this);
		if (GetLocalRole() == ROLE_AutonomousProxy)
		{
			Server_SetPoseMode(NewPoseMode);
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
		SYNC_SINGLE_TAG(LocomotionAction, NewLocomotionAction);

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

void AGASPCharacter::Server_SetOverlayMode_Implementation(const FGameplayTagContainer NewOverlayMode)
{
	SetOverlayMode(NewOverlayMode);
}

bool AGASPCharacter::HasFullMovementInput() const
{
	if (GeneralVars::AnalogInputStyle > 1)
	{
		return GetPendingMovementInputVector().Size2D() >= GetSettingsChecked()->AnalogMovementThreshold;
	}

	return true;
}

void AGASPCharacter::ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult)
{
	if (!GetController())
	{
		if (GetLocalRole() == ENetRole::ROLE_Authority && GetRemoteRole() == ENetRole::ROLE_SimulatedProxy)
		{
			static const FGASPMoverInputs DoNothingInput;
			MoverInputs_PreSim = DoNothingInput;

			InputCmdResult.InputCollection.FindOrAddMutableDataByType<FGASPMoverInputs>() = MoverInputs_PreSim;
		}
		return;
	}

	MoverInputs_PreSim.SetMoveInput(EMoveInputType::DirectionalIntent, GetMovementInputVector());
	MoverInputs_PreSim.RotationMode = GetAllowedRotationMode();
	MoverInputs_PreSim.Gait = GetAllowedGait();
	MoverInputs_PreSim.Stance = PlayerInputState.Get<FGASPInputState>().DesiredStance;
	MoverInputs_PreSim.ControlRotation = GetAimingRotation();
	MoverInputs_PreSim.bIsJumpJustPressed = bJustPressedJump;
	MoverInputs_PreSim.OrientationIntent = GetOrientationIntent();
	MoverInputs_PreSim.ControlRotationRate = ControlRotationRate;
	GetMovementDirectionAndOffset(MoverInputs_PreSim.MovementDirection, MoverInputs_PreSim.RotationOffset);

	if (Mesh && IsRagdolling())
	{
		if (const auto* TopBoneBody = Mesh->GetBodyInstance(TEXT("pelvis")))
		{
			MoverInputs_PreSim.RagdollTransform = TopBoneBody->GetUnrealWorldTransform();
		}
	}

	InputCmdResult.InputCollection.AddDataByCopy(&MoverInputs_PreSim);

	MoverInputs_PreSim.SuggestedMovementMode = NAME_None;
}

void AGASPCharacter::GetMovementDirectionAndOffset(EMovementDirection& MovementDirection, float& RotationOffset)
{
	if (MoverInputs_PreSim.RotationMode == RotationTags::OrientToMovement)
	{
		DebugAngle = RotationOffset = 0.f;
		MovementDirection = EMovementDirection::F;
		return;
	}

	const auto MovementMode{GetMovementMode()};
	auto DirectionOfMovement{FVector::ZeroVector};
	if (MovementMode == MovementModeTags::Grounded)
	{
		DirectionOfMovement = MoverInputs_PreSim.GetMoveInput();
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

	const auto OrientationDir = MoverInputs_PostSim.OrientationIntent.GetSafeNormal2D();
	const float Dot = FVector::DotProduct(DirectionOfMovement.GetSafeNormal2D(), OrientationDir);
	const float CrossZ = FVector::CrossProduct(OrientationDir, DirectionOfMovement.GetSafeNormal2D()).Z;

	// Result is strictly between -180 and 180
	float MovementAngle = DebugAngle = FMath::RadiansToDegrees(FMath::Atan2(CrossZ, Dot));

	if (MoverInputs_PreSim.MovementDirection == EMovementDirection::F)
	{
		float CurrentOffset = MoverInputs_PreSim.RotationOffset;

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

	MovementDirection = MoverInputs_PostSim.Gait != GaitTags::Sprint
		                    ? UGASPMath::GetMovementDirectionFromThreshold(
			                    UGASPMath::GetDirectionThresholds(MoverInputs_PreSim.MovementDirection,
			                                                      MoverInputs_PreSim.RotationMode == RotationTags::Aim
				                                                      ? GeneralVars::AimStyle
				                                                      : GeneralVars::StrafeStyle),
			                    MovementAngle)
		                    : EMovementDirection::F;

	if (const auto RotationCurveTable{GetSettingsChecked()->RotationCurveTable.LoadSynchronous()})
	{
		if (const auto* RotationCurve = Cast<UCurveFloat>(UChooserFunctionLibrary::EvaluateChooser(
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
		MoverInputs_PostSim.FloorLocation = HitResult.ImpactPoint;
		MoverInputs_PostSim.FloorNormal = HitResult.ImpactNormal;
	}
	else
	{
		MoverInputs_PostSim.FloorLocation = GetMesh()->GetComponentLocation();
		MoverInputs_PostSim.FloorNormal = FVector::UpVector;
	}
}

void AGASPCharacter::RefreshControlRotationRate(const float DeltaTime)
{
	const FRotator CurrentControlRotation{GetControlRotation()};

	// UnwindDegrees keeps the difference in (-180, 180]: without it, crossing the yaw wrap point
	// reports a ~360 degree jump. The clamped divisor covers a paused or first frame, where a raw
	// division would push inf into the replicated mover input.
	const float DeltaYaw{
		FMath::UnwindDegrees(UE_REAL_TO_FLOAT(CurrentControlRotation.Yaw - LastControlRotation.Yaw))
	};

	ControlRotationRate = DeltaYaw / FMath::Max(DeltaTime, UE_SMALL_NUMBER);
	LastControlRotation = CurrentControlRotation;
}

void AGASPCharacter::RefreshTwinStickMode()
{
	if (!TwinStickMode || !GetController())
	{
		return;
	}

	// A deflected stick defines the aim direction; a centred one has no direction to read, so the
	// character keeps facing where it already faces. The control rotation is deliberately left
	// alone here: zeroing it every frame made the yaw term below always zero.
	if (TwinStickAimDirection.IsNearlyZero(.1f))
	{
		TwinStickAimRotation = GetActorRotation();
		return;
	}

	// FVector2D components are doubles, so the result is narrowed explicitly.
	const float StickYaw{
		UE_REAL_TO_FLOAT(FMath::RadiansToDegrees(FMath::Atan2(TwinStickAimDirection.Y, -TwinStickAimDirection.X)))
	};
	TwinStickAimRotation = FRotator(0.f, GetControlRotation().Yaw + StickYaw, 0.f);
}

const FGASPMoverInputs& AGASPCharacter::GetMoverState() const
{
	return MoverInputs_PostSim;
}

FVector AGASPCharacter::GetMovementInputVector()
{
	const auto InputVector{ConsumeMovementInputVector()};

	auto NavIntent{FVector::ZeroVector};
	auto NavVelocity{FVector::ZeroVector};

	if (!InputVector.IsNearlyZero())
	{
		const FRotator YawRotation{0.f, GetControlRotation().Yaw, 0.f};
		return YawRotation.RotateVector(InputVector.GetClampedToSize(0.f, 1.f)).GetSafeNormal();
	}

	if (NavMoverComponent && NavMoverComponent->ConsumeNavMovementData(NavIntent, NavVelocity))
	{
		return !NavIntent.IsNearlyZero() ? NavIntent : NavVelocity.GetSafeNormal();
	}

	return FVector::ZeroVector;
}

FVector AGASPCharacter::GetOrientationIntent()
{
	if (GetMoverComponent()->HasGameplayTag(Mover_AnimRootMotion, false))
	{
		return GetActorForwardVector();
	}

	const auto AimVector{FRotator{0.f, GetAimingRotation().Yaw, 0.f}.Vector()};
	const bool bOrientToMove{MoverInputs_PreSim.RotationMode == RotationTags::OrientToMovement};

	const auto MovementMode{GetMovementMode()};
	if (MovementMode == MovementModeTags::Slide)
	{
		return bOrientToMove ? GetMoverComponent()->GetVelocity().GetSafeNormal() : AimVector;
	}

	const auto MoveInput{MoverInputs_PreSim.GetMoveInput()};
	if (MovementMode == MovementModeTags::Grounded)
	{
		if (!MoveInput.IsZero())
		{
			return bOrientToMove ? MoveInput : AimVector;
		}

		if (bOrientToMove)
		{
			return MoverInputs_PreSim.OrientationIntent;
		}
		const float YawDiff{
			static_cast<float>(FMath::Abs((GetActorRotation() - GetAimingRotation()).GetNormalized().Yaw))
		};
		const bool bShouldTurnInPlace{YawDiff > GetSettingsChecked()->TurnInPlaceThreshold};

		return bShouldTurnInPlace ? AimVector : MoverInputs_PreSim.OrientationIntent;
	}

	if (MovementMode == MovementModeTags::InAir)
	{
		return bOrientToMove ? MoverInputs_PreSim.OrientationIntent : AimVector;
	}

	return GetActorRotation().Vector();
}

FRotator AGASPCharacter::GetAimingRotation()
{
	if (auto* Target{IGASPTargetedActor::Execute_GetTargetedActor(this)})
	{
		return FVector{Target->GetActorLocation() - GetActorLocation()}.ToOrientationRotator();
	}

	return TwinStickMode ? TwinStickAimRotation : GetControlRotation();
}

FGameplayTag AGASPCharacter::GetAllowedRotationMode()
{
	if (IGASPTargetedActor::Execute_GetTargetedActor(this))
	{
		return PlayerInputState.Get<FGASPInputState>().DesiredRotationMode == RotationTags::Aim
			       ? RotationTags::Aim
			       : RotationTags::Strafe;
	}

	if (TwinStickMode)
	{
		if (!TwinStickAimDirection.IsZero())
		{
			if (PlayerInputState.Get<FGASPInputState>().DesiredRotationMode == RotationTags::Aim)
			{
				return RotationTags::Aim;
			}
			return RotationTags::Strafe;
		}
		return RotationTags::OrientToMovement;
	}
	return PlayerInputState.Get<FGASPInputState>().DesiredRotationMode;
}

FGameplayTag AGASPCharacter::GetAllowedGait()
{
	const auto& DesiredGait{PlayerInputState.Get<FGASPInputState>().DesiredGait};

	if (DesiredGait == GaitTags::Sprint && CanSprint())
	{
		return HasFullMovementInput() ? GaitTags::Sprint : GaitTags::Run;
	}
	if (DesiredGait == GaitTags::Walk)
	{
		return GaitTags::Walk;
	}
	if (DesiredGait == GaitTags::Sprint || DesiredGait == GaitTags::Run)
	{
		return HasFullMovementInput() ? GaitTags::Run : GaitTags::Walk;
	}

	return DesiredGait;
}

FVector AGASPCharacter::GetNavAgentLocation() const
{
	FVector AgentLocation = FNavigationSystem::InvalidLocation;
	const auto* UpdatedComponent = CharacterMotionComponent ? CharacterMotionComponent->GetUpdatedComponent() : nullptr;

	if (NavMoverComponent)
	{
		AgentLocation = NavMoverComponent->GetFeetLocation();
	}

	if (FNavigationSystem::IsValidLocation(AgentLocation) == false && UpdatedComponent != nullptr)
	{
		AgentLocation = UpdatedComponent->GetComponentLocation() - FVector::UpVector * UpdatedComponent->Bounds.
			BoxExtent.Z;
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

void AGASPCharacter::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	TagContainer.Reset();
	TagContainer.AppendTags(GameplayTags);
}

bool AGASPCharacter::HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	return IGameplayTagAssetInterface::HasAllMatchingGameplayTags(TagContainer);
}

bool AGASPCharacter::HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	return IGameplayTagAssetInterface::HasAnyMatchingGameplayTags(TagContainer);
}

bool AGASPCharacter::HasMatchingGameplayTag(FGameplayTag TagToCheck) const
{
	return IGameplayTagAssetInterface::HasMatchingGameplayTag(TagToCheck);
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
	if (GetMovementMode() == MovementModeTags::InAir)
	{
		return {
			!MoverInputs_PostSim.GetMoveInput().IsZero()
				? MoverInputs_PostSim.GetMoveInput().GetSafeNormal()
				: GetActorForwardVector(),
			75.f, FVector::ZeroVector, FVector::UpVector * 50.f, 30.f, 86.f
		};
	}

	const auto ForwardVector{
		GetMoverComponent()->GetVelocity().Size2D() > 50.f
			? GetMoverComponent()->GetVelocity().GetSafeNormal()
			: GetMoverComponent()->GetTargetOrientation().Vector()
	};

	const auto ActorVelocity{GetActorRotation().UnrotateVector(GetMoverComponent()->GetVelocity())};
	const float ClampedDistance = FMath::GetMappedRangeValueClamped<float, float>(
		{0.f, 375.f}, {75.f, 300.f}, ActorVelocity.X);

	return {
		ForwardVector, ClampedDistance, FVector::ZeroVector,
		FVector::ZeroVector, 30.f, 60.f
	};
}

TSubclassOf<UAnimInstance> AGASPCharacter::GetLinkedAnimLayer(const UChooserTable* DataTable) const
{
	if (!DataTable)
	{
		return nullptr;
	}

	// The chooser returns null when no row matches, which is a normal outcome rather than an error.
	const auto* DataAsset{
		Cast<UGASPLinkedAnimInstanceSet>(UChooserFunctionLibrary::EvaluateChooser(
			this, DataTable, UGASPLinkedAnimInstanceSet::StaticClass()))
	};

	return DataAsset ? DataAsset->GetAnimInstance() : nullptr;
}

void AGASPCharacter::OnPoseModeChanged(const FGameplayTag OldPoseMode, const FGameplayTag NewPoseMode)
{
	if (OldPoseMode.IsValid())
	{
		OverrideModeManager->RemoveOverrideLayer(OldPoseMode);
	}

	if (const auto LinkedAnimInstance{
		GetLinkedAnimLayer(GetSettingsChecked()->PosesTable.LoadSynchronous())
	}; NewPoseMode.IsValid())
	{
		OverrideModeManager->AddOverrideLayer(NewPoseMode, LinkedAnimInstance);
	}
}

void AGASPCharacter::OnMovementStateChanged(const FGameplayTag OldMovementState, const FGameplayTag NewMovementState)
{
	GameplayTags.RemoveTag(OldMovementState);
	GameplayTags.AddTag(NewMovementState);
}

void AGASPCharacter::OnOverlayModeChanged(const FGameplayTagContainer OldOverlayMode,
                                          const FGameplayTagContainer NewOverlayMode)
{
	for (const auto OverlayTag : OldOverlayMode)
	{
		if (OverlayTag.IsValid())
		{
			OverrideModeManager->RemoveOverrideLayer(OverlayTag);
		}
	}

	if (const auto LinkedAnimInstance{GetLinkedAnimLayer(GetSettingsChecked()->OverlayTable.LoadSynchronous())})
	{
		for (const auto OverlayTag : NewOverlayMode)
		{
			if (OverlayTag.IsValid())
			{
				OverrideModeManager->AddOverrideLayer(OverlayTag, LinkedAnimInstance);
			}
		}
	}
}

void AGASPCharacter::OnRep_OverlayMode(const FGameplayTagContainer& OldOverlayMode)
{
	OverlayContainerChanged.Broadcast(OldOverlayMode, OverlayTagContainer);
}

void AGASPCharacter::OnRep_PoseMode(const FGameplayTag& OldPoseMode)
{
	PoseModeChanged.Broadcast(OldPoseMode, PoseMode);
}

void AGASPCharacter::OnRep_LocomotionAction(const FGameplayTag& OldLocomotionAction)
{
	LocomotionActionChanged.Broadcast(OldLocomotionAction, LocomotionAction);
}

void AGASPCharacter::OnRep_TaskStates(const FInstancedStructCollection& OldTaskStates)
{
	TaskStates = OldTaskStates;
}

void AGASPCharacter::OnMovementModeChanged(const FName& PreviousMovementModeName, const FName& NewMovementModeName)
{
	if (PreviousMovementModeName == MovementModeNames::Sliding && PlayerInputState.Get<FGASPInputState>().DesiredGait ==
		GaitTags::Sprint)
	{
		PlayerInputState.GetMutablePtr<FGASPInputState>()->DesiredStance = StanceTags::Standing;
	}

	if (PreviousMovementModeName == DefaultModeNames::Falling && NewMovementModeName == DefaultModeNames::Walking)
	{
		const auto* CharacterSettings{GetSettingsChecked()};
		if (CharacterSettings->bStartRagdollingOnLand && GetMoverComponent()->GetVelocity().Z <= -CharacterSettings->
			RagdollingOnLandSpeedThreshold)
		{
			FMontageBlendSettings BlendSettings;
			BlendSettings.BlendMode = EMontageBlendMode::Inertialization;
			BlendSettings.Blend.BlendTime = .3f;

			StartRagdolling(true, BlendSettings, FGameplayTag::EmptyTag);
		}
	}

	if (NewMovementModeName == MovementModeNames::Ragdolling)
	{
		if (!RagdollTask || !RagdollTask->IsActive())
		{
			FMontageBlendSettings BlendSettings;
			BlendSettings.BlendMode = EMontageBlendMode::Inertialization;
			BlendSettings.Blend.BlendTime = .3f;

			StartRagdolling(true, BlendSettings, FGameplayTag::EmptyTag);
		}
	}

	if (PreviousMovementModeName == MovementModeNames::Ragdolling && NewMovementModeName !=
		MovementModeNames::Ragdolling)
	{
		if (RagdollTask)
		{
			TryPlayGetUpMontage();

			RagdollTask->EndTask();
			RagdollTask = nullptr;
		}

		PlayerInputState.GetMutablePtr<FGASPInputState>()->DesiredStance = StanceTags::Standing;
	}
}

UGameplayTasksComponent* AGASPCharacter::GetGameplayTasksComponent(const UGameplayTask& Task) const
{
	return FindComponentByClass<UGameplayTasksComponent>();
}

AActor* AGASPCharacter::GetGameplayTaskOwner(const UGameplayTask* Task) const
{
	return const_cast<ThisClass*>(this);
}

void AGASPCharacter::OnGameplayTaskInitialized(UGameplayTask& Task)
{
	IGameplayTaskOwnerInterface::OnGameplayTaskInitialized(Task);
}

void AGASPCharacter::OnGameplayTaskActivated(UGameplayTask& Task)
{
	IGameplayTaskOwnerInterface::OnGameplayTaskActivated(Task);
}

void AGASPCharacter::OnGameplayTaskDeactivated(UGameplayTask& Task)
{
	IGameplayTaskOwnerInterface::OnGameplayTaskDeactivated(Task);
}
