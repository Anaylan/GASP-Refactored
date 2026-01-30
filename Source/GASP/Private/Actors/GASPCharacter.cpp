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
#include "MovementSet/Modes/MovementMode_Sliding.h"
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
		Mesh->bOwnerNoSee = false;
		Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		Mesh->bCastDynamicShadow = true;
		Mesh->bAffectDynamicIndirectLighting = true;
		Mesh->PrimaryComponentTick.TickGroup = TG_PrePhysics;
		Mesh->SetupAttachment(CapsuleComponent);
		static FName MeshCollisionProfileName(TEXT("NoCollision"));
		Mesh->SetCollisionProfileName(MeshCollisionProfileName);
		Mesh->SetCollisionEnabled(ECollisionEnabled::ProbeOnly);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetCanEverAffectNavigation(false);

		Mesh->SetRelativeRotation_Direct({0.f, -90.f, 0.f});
		Mesh->SetRelativeLocation_Direct({0.f, 0.f, -90.f});
	}

	CharacterMotionComponent = CreateDefaultSubobject<UGASPMoverComponent>(TEXT("MoverComponent"));
	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarping"));
	TraversalComponent = CreateDefaultSubobject<UGASPTraversalComponent>(TEXT("TraversalComponent"));

	NavMoverComponent = CreateDefaultSubobject<UNavMoverComponent>(TEXT("NavMoverComponent"));
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

	OverlayModeChanged.AddDynamic(this, &ThisClass::OnOverlayModeChanged);
	PoseModeChanged.AddDynamic(this, &ThisClass::OnPoseModeChanged);

	SetOverlayMode(InitialOverlayMode, true);
	SetPoseMode(InitialPoseMode, true);
	SetLocomotionAction(FGameplayTag::EmptyTag, true);

	GetMesh()->AddTickPrerequisiteActor(this);

	ensureAlwaysMsgf(RotationCurveTable, TEXT("RotationCurveTable must be configured in character blueprint"));
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
		StateContainer.AddTag(NewMovementMode);
		
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
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, OverlayMode, Parameters);
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
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, PoseMode, this);
		if (GetLocalRole() == ROLE_AutonomousProxy)
		{
			Server_SetOverlayMode(NewPoseMode);
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

void AGASPCharacter::ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult)
{
	auto& CharacterInputs = InputCmdResult.InputCollection.FindOrAddMutableDataByType<FGASPMoverInputs>();
	if (!GetController())
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
	CharacterInputs.RotationMode = GetAllowedRotationMode();
	CharacterInputs.Gait = GetAllowedGait();
	CharacterInputs.Stance = PlayerInputState.DesiredStance;
	CharacterInputs.ControlRotation = GetAimingRotation();
	CharacterInputs.bIsJumpJustPressed = bJustPressedJump;
	CharacterInputs.OrientationIntent = GetOrientationIntent();
	CharacterInputs.ControlRotationRate = ControlRotationRate;

	GetMovementDirectionAddOffset(CharacterInputs.MovementDirection, CharacterInputs.RotationOffset);

	// CharacterInputs = MoverInputs_PostSim;
}

void AGASPCharacter::GetMovementDirectionAddOffset(EMovementDirection& MovementDirection, float& RotationOffset)
{
	if (MoverInputs_PostSim.RotationMode == RotationTags::OrientToMovement)
	{
		DebugAngle = RotationOffset = 0.f;
		MovementDirection = EMovementDirection::F;
		return;
	}

	auto DirectionOfMovement{FVector::ZeroVector};
	if (AllowedMovementMode == MovementModeTags::Grounded)
	{
		DirectionOfMovement = MoverInputs_PostSim.GetMoveInput();
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

	if (MoverInputs_PostSim.MovementDirection == EMovementDirection::F)
	{
		float CurrentOffset = MoverInputs_PostSim.RotationOffset;

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
			                    UGASPMath::GetDirectionThresholds(MoverInputs_PostSim.MovementDirection,
			                                                      MoverInputs_PostSim.RotationMode == RotationTags::Aim
				                                                      ? GeneralVars::AimStyle
				                                                      : GeneralVars::StrafeStyle),
			                    MovementAngle)
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
	const auto AimVector{FRotator{0.f, GetAimingRotation().Yaw, 0.f}.Vector()};
	const bool bOrientToMove{MoverInputs_PostSim.RotationMode == RotationTags::OrientToMovement};

	if (AllowedMovementMode == MovementModeTags::Slide)
	{
		return bOrientToMove ? GetMoverComponent()->GetVelocity().GetSafeNormal() : AimVector;
	}

	const auto MoveInput{MoverInputs_PostSim.GetMoveInput()};
	if (AllowedMovementMode == MovementModeTags::Grounded)
	{
		if (!MoveInput.IsZero())
		{
			return bOrientToMove ? MoveInput : AimVector;
		}

		const float YawDiff{
			static_cast<float>(FMath::Abs((GetActorRotation() - GetAimingRotation()).GetNormalized().Yaw))
		};
		const bool bShouldTurnInPlace{MoverInputs_PostSim.RotationMode == RotationTags::Aim && YawDiff > 60.f};

		return bShouldTurnInPlace ? AimVector : MoverInputs_PostSim.OrientationIntent;
	}

	if (AllowedMovementMode == MovementModeTags::InAir)
	{
		return bOrientToMove ? MoverInputs_PostSim.OrientationIntent : AimVector;
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
			if (PlayerInputState.DesiredRotationMode == RotationTags::Aim)
			{
				return RotationTags::Aim;
			}
			return RotationTags::Strafe;
		}
		return RotationTags::OrientToMovement;
	}
	return PlayerInputState.DesiredRotationMode;
}

FGameplayTag AGASPCharacter::GetAllowedGait()
{
	const auto& DesiredGait{PlayerInputState.DesiredGait};

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
		const auto Inputs{GetMoverState()};
		return {
			!Inputs.GetMoveInput().IsZero() ? Inputs.GetMoveInput().GetSafeNormal() : GetActorForwardVector(),
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

	LinkAnimInstance(PosesTable);
}

void AGASPCharacter::OnOverlayModeChanged_Implementation(const FGameplayTag OldOverlayMode,
                                                         const FGameplayTag NewOverlayMode)
{
	StateContainer.RemoveTag(OldOverlayMode);
	StateContainer.AddTag(NewOverlayMode);

	LinkAnimInstance(OverlayTable);
}

void AGASPCharacter::OnRep_OverlayMode(const FGameplayTag& OldOverlayMode)
{
	OverlayModeChanged.Broadcast(OldOverlayMode, OverlayMode);
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

	if (PreviousMovementModeName == MovementModeNames::Sliding && PlayerInputState.DesiredGait == GaitTags::Sprint)
	{
		PlayerInputState.DesiredStance = StanceTags::Standing;
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
		const auto OldStanceMode{AllowedStanceMode};
		AllowedStanceMode = NewStanceMode;
		StanceModeChanged.Broadcast(OldStanceMode, AllowedStanceMode);
	}
}
