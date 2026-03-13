#include "Actors/GASPCharacter.h"
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
#include "MovementSet/Modes/MovementMode_Sliding.h"
#include "Settings/GASPCharacterSettings.h"
#include "Utils/GASPBlueprintLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPCharacter)

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
}


FName AGASPCharacter::MeshComponentName(TEXT("CharacterMesh0"));
FName AGASPCharacter::CapsuleComponentName(TEXT("CollisionCylinder"));
FName AGASPCharacter::MotionWarpingComponentName(TEXT("MotionWarping"));
FName AGASPCharacter::CharacterMotionComponentName(TEXT("MoverComponent"));
FName AGASPCharacter::NavMoverComponentName(TEXT("NavMoverComponent"));

// Sets default values
AGASPCharacter::AGASPCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
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
		Mesh->bOwnerNoSee = false;
		Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		Mesh->bCastDynamicShadow = true;
		Mesh->bAffectDynamicIndirectLighting = true;
		Mesh->PrimaryComponentTick.TickGroup = TG_PrePhysics;
		Mesh->SetupAttachment(CapsuleComponent);
		
		static FName MeshCollisionProfileName(TEXT("NoCollision"));
		Mesh->SetCollisionProfileName(MeshCollisionProfileName);
		Mesh->SetCollisionEnabled(ECollisionEnabled::ProbeOnly);
		Mesh->SetCollisionObjectType(ECC_Pawn);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetCanEverAffectNavigation(false);

		Mesh->SetRelativeRotation_Direct({0.f, -90.f, 0.f});
		Mesh->SetRelativeLocation_Direct({0.f, 0.f, -90.f});
	}

	CharacterMotionComponent = CreateDefaultSubobject<UGASPMoverComponent>(CharacterMotionComponentName);
	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(MotionWarpingComponentName);
	TraversalComponent = CreateDefaultSubobject<UGASPTraversalComponent>(TEXT("TraversalComponent"));

	NavMoverComponent = CreateDefaultSubobject<UNavMoverComponent>(NavMoverComponentName);
}

// Called when the game starts or when spawned
void AGASPCharacter::BeginPlay()
{
	Super::BeginPlay();

	StateContainer.Reset();
	NavMoverComponent = FindComponentByClass<UNavMoverComponent>();

	if (const auto MoverComp = GetMoverComponent())
	{
		MoverComp->OnMovementModeChanged.AddDynamic(this, &ThisClass::OnMovementModeChanged);
		MoverComp->OnStanceChanged.AddDynamic(this, &ThisClass::OnStanceChanged);

		GetMesh()->AddTickPrerequisiteComponent(MoverComp);
	}

	if (GetMesh())
	{
		MeshRelativeTransformCache = GetMesh()->GetRelativeTransform();
	}

	OverlayContainerChanged.AddDynamic(this, &ThisClass::OnOverlayModeChanged);
	PoseModeChanged.AddDynamic(this, &ThisClass::OnPoseModeChanged);

	SetPoseMode(PoseMode, true);
	SetLocomotionAction(FGameplayTag::EmptyTag, true);

	GetMesh()->AddTickPrerequisiteActor(this);

	MoverInputs_PreSim.OrientationIntent = GetActorForwardVector();

	ensureAlwaysMsgf(Settings->RotationCurveTable.IsValid(),
	                 TEXT("RotationCurveTable must be configured in character blueprint"));
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

	RefreshMoverState();
	RefreshFloorValues();
	RefreshControlRotationRate(DeltaTime);
	RefreshTwinStickMode();

	Super::Tick(DeltaTime);

	if (LocomotionAction == LocomotionActionTags::Ragdoll)
	{
		RefreshRagdolling(DeltaTime);
	}
}

void AGASPCharacter::SetMovementMode(const FGameplayTag NewMovementMode, const bool bForce)
{
	if (NewMovementMode != AllowedMovementMode || bForce)
	{
		StateContainer.RemoveTag(AllowedMovementMode);
		StateContainer.AddLeafTag(NewMovementMode);

		const auto OldMovementMode{AllowedMovementMode};
		AllowedMovementMode = NewMovementMode;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, AllowedMovementMode, this);

		MovementModeChanged.Broadcast(OldMovementMode, AllowedMovementMode);
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

void AGASPCharacter::RefreshMoverState()
{
	auto [InputCollection] = GetMoverComponent()->GetLastInputCmd();
	MoverInputs_PostSim = InputCollection.FindOrAddDataByType<FGASPMoverInputs>();
}

void AGASPCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Parameters;
	Parameters.bIsPushBased = true;

	// Replicate to everyone except owner
	Parameters.Condition = COND_SkipOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, AllowedMovementMode, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, LocomotionAction, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, TraversalComponent, Parameters);


	// Replicate to everyone
	Parameters.Condition = COND_None;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, OverlayTagContainer, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PoseMode, Parameters);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, RagdollTargetLocation, Parameters);
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

void AGASPCharacter::Server_SetPoseMode_Implementation(const FGameplayTag NewOverlayMode)
{
	SetPoseMode(NewOverlayMode);
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

void AGASPCharacter::Server_SetOverlayMode_Implementation(const FGameplayTagContainer NewOverlayMode)
{
	SetOverlayMode(NewOverlayMode);
}

bool AGASPCharacter::HasFullMovementInput() const
{
	if (GeneralVars::AnalogInputStyle > 1)
	{
		return GetPendingMovementInputVector().Size2D() >= Settings->AnalogMovementThreshold;
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
	GetMovementDirectionAddOffset(MoverInputs_PreSim.MovementDirection, MoverInputs_PreSim.RotationOffset);

	InputCmdResult.InputCollection.AddDataByCopy(&MoverInputs_PreSim);
}

void AGASPCharacter::GetMovementDirectionAddOffset(EMovementDirection& MovementDirection, float& RotationOffset)
{
	if (MoverInputs_PreSim.RotationMode == RotationTags::OrientToMovement)
	{
		DebugAngle = RotationOffset = 0.f;
		MovementDirection = EMovementDirection::F;
		return;
	}

	auto DirectionOfMovement{FVector::ZeroVector};
	if (AllowedMovementMode == MovementModeTags::Grounded)
	{
		DirectionOfMovement = MoverInputs_PreSim.GetMoveInput();
	}
	else if (AllowedMovementMode == MovementModeTags::InAir || AllowedMovementMode == MovementModeTags::Slide)
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

	if (const auto RotationCurveTable{Settings->RotationCurveTable.LoadSynchronous()})
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
		MoverInputs_PostSim.FloorLocation = HitResult.ImpactPoint;
		MoverInputs_PostSim.FloorNormal = HitResult.ImpactNormal;
	}
	else
	{
		MoverInputs_PostSim.FloorLocation = GetMesh()->GetComponentLocation();
		MoverInputs_PostSim.FloorNormal = FVector::ZeroVector;
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

	if (TwinStickAimDirection.IsNearlyZero(.1f))
	{
		const float StickYaw = FMath::RadiansToDegrees(FMath::Atan2(TwinStickAimDirection.Y, -TwinStickAimDirection.X));
		TwinStickAimRotation = FRotator(0.f, GetControlRotation().Yaw + StickYaw, 0.f);
	}
	else
	{
		TwinStickAimRotation = GetActorRotation();
	}
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
	const auto AimVector{FRotator{0.f, GetAimingRotation().Yaw, 0.f}.Vector()};
	const bool bOrientToMove{MoverInputs_PreSim.RotationMode == RotationTags::OrientToMovement};

	if (AllowedMovementMode == MovementModeTags::Slide)
	{
		return bOrientToMove ? GetMoverComponent()->GetVelocity().GetSafeNormal() : AimVector;
	}

	const auto MoveInput{MoverInputs_PreSim.GetMoveInput()};
	if (AllowedMovementMode == MovementModeTags::Grounded)
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
		const bool bShouldTurnInPlace{YawDiff > Settings->TurnInPlaceThreshold};

		return bShouldTurnInPlace ? AimVector : MoverInputs_PreSim.OrientationIntent;
	}

	if (AllowedMovementMode == MovementModeTags::InAir)
	{
		return bOrientToMove ? MoverInputs_PreSim.OrientationIntent : AimVector;
	}

	return GetActorRotation().Vector();
}

FRotator AGASPCharacter::GetAimingRotation()
{
	return TwinStickMode ? TwinStickAimRotation : GetControlRotation();
}

FGameplayTag AGASPCharacter::GetAllowedRotationMode()
{
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
	if (AllowedMovementMode == MovementModeTags::InAir)
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

void AGASPCharacter::LinkAnimInstance(const UChooserTable* DataTable) const
{
	if (!DataTable || !GetMesh())
	{
		return;
	}

	auto* MeshComponent = GetMesh();
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
	StateContainer.RemoveTag(OldPoseMode);
	StateContainer.AddTag(NewPoseMode);

	LinkAnimInstance(Settings->PosesTable.LoadSynchronous());
}

void AGASPCharacter::OnOverlayModeChanged_Implementation(const FGameplayTagContainer OldOverlayMode,
                                                         const FGameplayTagContainer NewOverlayMode)
{
	LinkAnimInstance(Settings->OverlayTable.LoadSynchronous());
}

void AGASPCharacter::OnRep_OverlayMode(const FGameplayTagContainer& OldOverlayMode)
{
	OverlayContainerChanged.Broadcast(OldOverlayMode, OverlayTagContainer);
}

void AGASPCharacter::OnRep_PoseMode(const FGameplayTag& OldPoseMode)
{
	PoseModeChanged.Broadcast(OldPoseMode, PoseMode);
}

void AGASPCharacter::OnRep_AllowedMovementMode(const FGameplayTag& OldMovementMode)
{
	MovementModeChanged.Broadcast(OldMovementMode, AllowedMovementMode);
}

void AGASPCharacter::OnRep_LocomotionAction(const FGameplayTag& OldLocomotionAction)
{
	LocomotionActionChanged.Broadcast(OldLocomotionAction, LocomotionAction);
}

void AGASPCharacter::OnMovementModeChanged(const FName& PreviousMovementModeName, const FName& NewMovementModeName)
{
	const auto MovementMode = GetMoverComponent()->FindMovementModeByName(NewMovementModeName);
	SetMovementMode(MovementMode->Implements<UGASPMovementInterface>()
		                ? IGASPMovementInterface::Execute_GetAssociatedTag(MovementMode)
		                : MovementModeTags::Traverse);

	if (PreviousMovementModeName == MovementModeNames::Sliding && PlayerInputState.Get<FGASPInputState>().DesiredGait ==
		GaitTags::Sprint)
	{
		PlayerInputState.GetMutablePtr<FGASPInputState>()->DesiredStance = StanceTags::Standing;
	}

	if (PreviousMovementModeName == DefaultModeNames::Falling && NewMovementModeName == DefaultModeNames::Walking)
	{
		if (Settings->bStartRagdollingOnLand && GetMoverComponent()->GetVelocity().Z <= -Settings->
			RagdollingOnLandSpeedThreshold)
		{
			StartRagdolling();
		}
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

void AGASPCharacter::SetStanceMode(const FGameplayTag NewStanceMode, const bool bForce)
{
	if (NewStanceMode != AllowedStanceMode || bForce)
	{
		StateContainer.RemoveTag(AllowedStanceMode);
		StateContainer.AddLeafTag(NewStanceMode);

		const auto OldStanceMode{AllowedStanceMode};
		AllowedStanceMode = NewStanceMode;
		StanceModeChanged.Broadcast(OldStanceMode, AllowedStanceMode);
	}
}

void AGASPCharacter::OnRep_Settings()
{
}