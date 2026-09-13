#include "Tasks/RagdollTask.h"

#include "ChooserFunctionLibrary.h"
#include "Net/Core/PushModel/PushModel.h"
#include "PhysicsControlComponent.h"
#include "PhysicsControlData.h"
#include "Actors/GASPCharacter.h"
#include "Animation/GASPAnimInstance.h"
#include "Animation/Linked/RagdollLinkedAnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Engine/CollisionProfile.h"
#include "Interfaces/GASPInteractionInterface.h"
#include "Kismet/GameplayStatics.h"
#include "MoveLibrary/PlayMoverMontageCallbackProxy.h"
#include "MovementSet/GASPMoverComponent.h"
#include "MovementSet/Modes/MovementMode_Ragdolling.h"
#include "Settings/GASPCharacterSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RagdollTask)

DEFINE_LOG_CATEGORY_STATIC(LogRagdollTask, Log, All);

const FName URagdollTask::NAME_Ragdoll(TEXT("Ragdoll"));
const FName URagdollTask::NAME_CharacterMesh(TEXT("CharacterMesh"));
const FName URagdollTask::NAME_spine_05(TEXT("spine_05"));
const FName URagdollTask::NAME_RagdollTrace(TEXT("RagdollTrace"));
const FName URagdollTask::NAME_ParentSpace(TEXT("ParentSpace"));
const FName URagdollTask::NAME_ParentSpace_Legs(TEXT("ParentSpace_Legs"));
const FName URagdollTask::NAME_ParentSpace_Arms(TEXT("ParentSpace_Arms"));
const FName URagdollTask::NAME_ParentSpace_Torso(TEXT("ParentSpace_Torso"));
const FName URagdollTask::NAME_ParentSpace_Head(TEXT("ParentSpace_Head"));
const FName URagdollTask::NAME_Ragdoll_Strength_Legs(TEXT("Ragdoll_Strength_Legs"));
const FName URagdollTask::NAME_Ragdoll_Strength_Arms(TEXT("Ragdoll_Strength_Arms"));
const FName URagdollTask::NAME_Ragdoll_Strength_Torso(TEXT("Ragdoll_Strength_Torso"));
const FName URagdollTask::NAME_Ragdoll_Strength_Head(TEXT("Ragdoll_Strength_Head"));

#if ENABLE_DRAW_DEBUG
namespace RagdollTaskDebugVar
{
	// Distinct from gasp.ragdoll.drawdebug, which MovementMode_Ragdolling already owns.
	bool bDrawDebugImpactTrace{false};
	FAutoConsoleVariableRef CVarDrawDebugImpactTrace(
		TEXT("gasp.ragdoll.task.drawdebug"), bDrawDebugImpactTrace,
		TEXT("Draw the ragdoll impact prediction trace"), ECVF_Default);
}
#endif

URagdollTask::URagdollTask()
{
	bTickingTask = true;
}

void URagdollTask::Activate()
{
	Super::Activate();

	Mesh = Character->GetMesh();
	MoverComponent = Character->GetMoverComponent();
	CapsuleComponent = Character->GetCapsuleComponent();
	PhysicsControlComponent = Character->GetPhysicsControlComponent();

	RagdollingState = FTaskStateUtils::FindOrAdd<FRagdollingState>(Character->TaskStates);

	if (!AreComponentsValid() || !RagdollingState)
	{
		EndTask();
		return;
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(AGASPCharacter, TaskStates, Character.Get());

	if (bStopActiveMontages)
	{
		if (auto* AnimInstance = Mesh->GetAnimInstance())
		{
			AnimInstance->Montage_StopWithBlendSettings(BlendSettings);
		}
	}

	Mesh->SetSimulatePhysics(true);

	if (Character.IsValid())
	{
		Character->SetPhysicsProfile(NAME_Ragdoll);
	}

	if (MoverComponent.IsValid())
	{
		MoverComponent->QueueNextMode(MovementModeNames::Ragdolling);
	}

	if (CapsuleComponent.IsValid())
	{
		CapsuleComponent->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
		CapsuleComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		CapsuleComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}
}

void URagdollTask::OnDestroy(bool bInOwnerFinished)
{
	if (!bInOwnerFinished && Mesh.IsValid() && Character.IsValid() && RagdollingState)
	{
		if (auto* AnimInstance{Cast<UGASPAnimInstance>(Mesh->GetAnimInstance())})
		{
			if (auto* RagdollInstance{
				Cast<URagdollLinkedAnimInstance>(
					AnimInstance->GetLinkedAnimLayerInstanceByClass(URagdollLinkedAnimInstance::StaticClass(), true))
			})
			{
				RagdollInstance->SnapshotFinalRagdollPose();
			}

			if (MoverComponent.IsValid())
			{
				UPoseSearchLibrary::OverridePoseHistoryFromOwningMesh(AnimInstance, TEXT("PoseHistory"));

				if (IsValid(GetUpTable))
				{
					FGASPGetUpInput Input;
					Input.bRollingGetup = RagdollingState->CenterOfMass_Velocity.SizeSquared2D() > 22500.0f;

					auto* MovementMode{
						MoverComponent->FindMovementModeByName(MoverComponent->GetNextMovementModeName())
					};
					Input.StateContainer =
						IGASPMovementInterface::GetAssociatedTagSafe(MovementMode).GetSingleTagContainer();

					FGASPGetUpOutput Output;
					auto PoseHistory = IGASPInteractionInterface::Execute_GetPoseHistory(AnimInstance);

					auto Context = UChooserFunctionLibrary::MakeChooserEvaluationContext();
					Context.AddStructParam(Input);
					Context.AddStructParam(PoseHistory);
					Context.AddStructParam(Output);

					auto* Montage = Cast<UAnimMontage>(UChooserFunctionLibrary::EvaluateObjectChooserBase(
						Context,
						UChooserFunctionLibrary::MakeEvaluateChooser(GetUpTable),
						UAnimMontage::StaticClass()));

					if (Montage)
					{
						UPlayMoverMontageCallbackProxy::CreateProxyObjectForPlayMoverMontage(
							MoverComponent.Get(), Montage, 1.0f, Output.MontageStartTime);
					}
				}
			}
		}
	}

	if (CapsuleComponent.IsValid())
	{
		CapsuleComponent->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
		CapsuleComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	}

	if (Mesh.IsValid())
	{
		Mesh->SetSimulatePhysics(false);
		Mesh->UnlinkAnimClassLayers(AnimClass);
	}

	if (PhysicsControlComponent.IsValid())
	{
		static const auto ZeroVelocityControlData = []()
		{
			FPhysicsControlData Data;
			Data.LinearTargetVelocityMultiplier = 0.0f;
			Data.AngularTargetVelocityMultiplier = 0.0f;
			return Data;
		}();

		PhysicsControlComponent->SetControlDatasInSet(NAME_ParentSpace, ZeroVelocityControlData);
	}

	if (Character.IsValid())
	{
		Character->SetPhysicsProfile(Character->PhysicsProfileName);
		FTaskStateUtils::Remove(Character->TaskStates, FRagdollingState::StaticStruct());
		MARK_PROPERTY_DIRTY_FROM_NAME(AGASPCharacter, TaskStates, Character.Get());
	}

	RagdollingState = nullptr;

	Super::OnDestroy(bInOwnerFinished);
}

URagdollTask* URagdollTask::CreateRagdollTask(TScriptInterface<IGameplayTaskOwnerInterface> TaskOwner,
                                              const bool bStopActiveMontages, const FMontageBlendSettings BlendSettings,
                                              const FGameplayTag InjuryState, UChooserTable* GetUpTable,
                                              const uint8 Priority)
{
	auto* MyTask = NewTaskUninitialized<URagdollTask>();
	if (MyTask && TaskOwner.GetInterface() != nullptr)
	{
		MyTask->InitTask(*TaskOwner, Priority);
		MyTask->bStopActiveMontages = bStopActiveMontages;
		MyTask->BlendSettings = BlendSettings;
		MyTask->InjuryState = InjuryState;
		MyTask->GetUpTable = GetUpTable;
	}
	else
	{
		UE_LOG(LogGASPTaskState, Error, TEXT("CreateRagdollTask: invalid TaskOwner, task will not activate"));
	}

	return MyTask;
}

void URagdollTask::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	if (!AreComponentsValid())
	{
		return;
	}

	RagdollingState->CenterOfMass = Mesh->GetSkeletalCenterOfMass();
	RagdollingState->CenterOfMass_Velocity = (RagdollingState->CenterOfMass - RagdollingState->CenterOfMass_LastFrame) /
		FMath::Max(DeltaTime, UE_KINDA_SMALL_NUMBER);
	RagdollingState->CenterOfMass_LastFrame = RagdollingState->CenterOfMass;

	RagdollingState->SpineVelocity = Mesh->GetPhysicsLinearVelocity(NAME_spine_05);
	RagdollingState->Speed = UE_REAL_TO_FLOAT(RagdollingState->SpineVelocity.Size());

	RefreshRagdollImpactDirection();
	RefreshRagdollRollBehavior(DeltaTime);
	PhysicsControlComponent->SetBodyModifierGravityMultiplier(
		TEXT("All"), RagdollingState->CenterOfMass_Velocity.Z < -2500.f ? 0.f : /* gravity multiplier */ 1.f);

	RefreshRagdollPhysicsStrengthsFromCurves();

	MARK_PROPERTY_DIRTY_FROM_NAME(AGASPCharacter, TaskStates, Character.Get());
}

void URagdollTask::RefreshRagdollImpactDirection() const
{
	const auto TraceOrigin{Mesh->GetSocketTransform(NAME_RagdollTrace)};
	const auto StartLoc{TraceOrigin.GetLocation()};

	FPredictProjectilePathParams Params(20.f, StartLoc, RagdollingState->SpineVelocity, 1.f, ECC_PhysicsBody,
	                                    Character.Get());
	Params.SimFrequency = 10.0f;
#if ENABLE_DRAW_DEBUG
	Params.DrawDebugType = RagdollTaskDebugVar::bDrawDebugImpactTrace
		                       ? EDrawDebugTrace::Type::ForOneFrame
		                       : EDrawDebugTrace::Type::None;
#endif

	FPredictProjectilePathResult Result;
	UGameplayStatics::PredictProjectilePath(GetWorld(), Params, Result);

	RagdollingState->TimeToImpact = Result.LastTraceDestination.Time;

	const auto RelImpact{
		TraceOrigin.InverseTransformVectorNoScale(
			Result.LastTraceDestination.Location - StartLoc
		)
	};

	const float Size2D{static_cast<float>(RelImpact.Size2D())};
	RagdollingState->ImpactDirection = {
		FMath::RadiansToDegrees(FMath::Atan2(RelImpact.Y, RelImpact.X)),
		FMath::RadiansToDegrees(FMath::Atan2(RelImpact.Z, Size2D))
	};
}

void URagdollTask::RefreshRagdollRollBehavior(const float DeltaTime) const
{
	static const float CosFloorAngleMin = FMath::Cos(FMath::DegreesToRadians(20.0f));
	static const float CosFloorAngleMax = FMath::Cos(FMath::DegreesToRadians(30.0f));

	const auto& FloorNormal{Character->GetMoverState().FloorNormal};
	const float FloorDot{static_cast<float>(FloorNormal.Z)};

	const float Alpha{FMath::Clamp((FloorDot - CosFloorAngleMin) / (CosFloorAngleMax - CosFloorAngleMin), 0.f, 1.f)};
	const auto TargetAutoRollForce{FloorNormal * Alpha};

	const float TargetSizeSq{Alpha * Alpha};
	const float CurrentSizeSq = RagdollingState->AutoRollForce.SizeSquared();
	const float InterpSpeed = (TargetSizeSq < CurrentSizeSq) ? 0.5f : 2.f;

	RagdollingState->AutoRollForce = FMath::VInterpConstantTo(
		RagdollingState->AutoRollForce, TargetAutoRollForce, DeltaTime, InterpSpeed);

	RagdollingState->RollForce = RagdollingState->AutoRollForce.GetClampedToMaxSize(1.f);

	if (Mesh->IsAnySimulatingPhysics())
	{
		// Analytical CrossProduct with UpVector (0, 0, 1): (-Y, X, 0)
		const FVector TorqueDir{-RagdollingState->RollForce.Y, RagdollingState->RollForce.X, 0.f};
		Mesh->AddTorqueInDegrees(TorqueDir * 100000.0f, NAME_spine_05, true);
	}
}

void URagdollTask::RefreshRagdollPhysicsStrengthsFromCurves() const
{
	if (RagdollingState->AppliedProfileName != NAME_Ragdoll)
	{
		return;
	}

	FPhysicsControlMultiplier Multiplier{};

	auto SetControlStrength = [&](const FName& CurveName, const FName& ControlSetName)
	{
		if (Mesh->GetCurveValue(CurveName, 0.f, Multiplier.AngularStrengthMultiplier))
		{
			PhysicsControlComponent->SetControlMultipliersInSet(ControlSetName, Multiplier, false);
		}
	};

	SetControlStrength(NAME_Ragdoll_Strength_Legs, NAME_ParentSpace_Legs);
	SetControlStrength(NAME_Ragdoll_Strength_Arms, NAME_ParentSpace_Arms);
	SetControlStrength(NAME_Ragdoll_Strength_Torso, NAME_ParentSpace_Torso);
	SetControlStrength(NAME_Ragdoll_Strength_Head, NAME_ParentSpace_Head);
}

bool URagdollTask::AreComponentsValid() const
{
	return Mesh.IsValid() && MoverComponent.IsValid() && CapsuleComponent.IsValid() && PhysicsControlComponent.
		IsValid();
}
