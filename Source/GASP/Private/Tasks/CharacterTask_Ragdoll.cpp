#include "Tasks/CharacterTask_Ragdoll.h"
#include "PhysicsControlComponent.h"
#include "PhysicsControlData.h"
#include "Actors/GASPCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Engine/CollisionProfile.h"
#include "Kismet/GameplayStatics.h"
#include "Utils/GASPChooserLibrary.h"
#include "Utils/GASPRagdollLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CharacterTask_Ragdoll)

DEFINE_LOG_CATEGORY_STATIC(LogRagdollTask, Log, All);

const FName UCharacterTask_Ragdoll::NAME_Ragdoll(TEXT("Ragdoll"));
const FName UCharacterTask_Ragdoll::NAME_All(TEXT("All"));
const FName UCharacterTask_Ragdoll::NAME_CharacterMesh(TEXT("CharacterMesh"));
const FName UCharacterTask_Ragdoll::NAME_spine_05(TEXT("spine_05"));
const FName UCharacterTask_Ragdoll::NAME_RagdollTrace(TEXT("RagdollTrace"));
const FName UCharacterTask_Ragdoll::NAME_ParentSpace(TEXT("ParentSpace"));
const FName UCharacterTask_Ragdoll::NAME_ParentSpace_Legs(TEXT("ParentSpace_Legs"));
const FName UCharacterTask_Ragdoll::NAME_ParentSpace_Arms(TEXT("ParentSpace_Arms"));
const FName UCharacterTask_Ragdoll::NAME_ParentSpace_Torso(TEXT("ParentSpace_Torso"));
const FName UCharacterTask_Ragdoll::NAME_ParentSpace_Head(TEXT("ParentSpace_Head"));
const FName UCharacterTask_Ragdoll::NAME_Ragdoll_Strength_Legs(TEXT("Ragdoll_Strength_Legs"));
const FName UCharacterTask_Ragdoll::NAME_Ragdoll_Strength_Arms(TEXT("Ragdoll_Strength_Arms"));
const FName UCharacterTask_Ragdoll::NAME_Ragdoll_Strength_Torso(TEXT("Ragdoll_Strength_Torso"));
const FName UCharacterTask_Ragdoll::NAME_Ragdoll_Strength_Head(TEXT("Ragdoll_Strength_Head"));

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

UCharacterTask_Ragdoll::UCharacterTask_Ragdoll()
{
	bTickingTask = true;
}

UCharacterTask_Ragdoll* UCharacterTask_Ragdoll::CreateRagdollTask(
	TScriptInterface<IGameplayTaskOwnerInterface> TaskOwner, const bool bStopActiveMontages,
	const FMontageBlendSettings& BlendSettings, const FGameplayTag InjuryState, UChooserTable* GetUpTable,
	const uint8 Priority)
{
	auto* MyTask = NewTaskUninitialized<UCharacterTask_Ragdoll>();
	if (MyTask && TaskOwner.GetInterface() != nullptr)
	{
		MyTask->InitTask(*TaskOwner, Priority);
		MyTask->bStopActiveMontages = bStopActiveMontages;
		MyTask->BlendSettings = BlendSettings;
		MyTask->InjuryState = InjuryState;
		MyTask->GetUpTable = GetUpTable;
	}

	return MyTask;
}

void UCharacterTask_Ragdoll::Activate()
{
	Super::Activate();

	Mesh = Character.IsValid() ? Character->GetMesh() : nullptr;
	CapsuleComponent = Character.IsValid() ? Character->GetCapsuleComponent() : nullptr;
	PhysicsControlComponent = Character.IsValid() ? Character->GetPhysicsControlComponent() : nullptr;

	if (!Character.IsValid() || !AreComponentsValid())
	{
		UE_LOG(LogRagdollTask, Warning, TEXT("Activate: missing components on %s"),
		       Character.IsValid() ? *Character->GetName() : TEXT("None"));
		EndTask();
		return;
	}

	RagdollingState.InjuryState = InjuryState;

	if (bStopActiveMontages)
	{
		if (auto* AnimInstance = Mesh->GetAnimInstance())
		{
			AnimInstance->Montage_StopWithBlendSettings(BlendSettings);
		}
	}

	Mesh->SetSimulatePhysics(true);
	Mesh->SetCollisionProfileName(NAME_Ragdoll);

	Character->SetPhysicsProfile(NAME_Ragdoll);

	CapsuleComponent->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	CapsuleComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	CapsuleComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	CapsuleComponent->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);

	OnRagdollStarted.Broadcast();
}

void UCharacterTask_Ragdoll::OnDestroy(bool bInOwnerFinished)
{
	if (CapsuleComponent.IsValid())
	{
		CapsuleComponent->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
		CapsuleComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		CapsuleComponent->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
	}

	if (Mesh.IsValid())
	{
		Mesh->SetSimulatePhysics(false);
		auto OriginalCDO{Cast<AGASPCharacter>(Character->GetClass()->GetDefaultObject())};
		Mesh->SetCollisionProfileName(OriginalCDO->GetMesh()->GetCollisionProfileName());
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
		Character->TaskStates.RemoveDataByType(FRagdollingState::StaticStruct());
	}

	Mesh.Reset();
	CapsuleComponent.Reset();
	PhysicsControlComponent.Reset();

	OnRagdollEnded.Broadcast();

	Super::OnDestroy(bInOwnerFinished);
}

void UCharacterTask_Ragdoll::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	if (!AreComponentsValid())
	{
		return;
	}

	FGASPRagdollLibrary::IntegrateBodyState(RagdollingState, Mesh->GetSkeletalCenterOfMass(),
	                                        Mesh->GetPhysicsLinearVelocity(NAME_spine_05), DeltaTime);

	RefreshRagdollImpactDirection();
	RefreshRagdollRollBehavior(DeltaTime);

	PhysicsControlComponent->SetBodyModifierGravityMultiplier(
		NAME_All, FGASPRagdollLibrary::ComputeGravityMultiplier(RagdollingState));

	RefreshRagdollPhysicsStrengthsFromCurves();

	Character->TaskStates.AddOrOverwriteData(FInstancedStruct::Make(RagdollingState));
}

void UCharacterTask_Ragdoll::RefreshRagdollImpactDirection()
{
	const auto TraceOrigin{Mesh->GetSocketTransform(NAME_RagdollTrace)};
	const auto StartLoc{TraceOrigin.GetLocation()};

	FPredictProjectilePathParams Params(20.f, StartLoc, RagdollingState.SpineVelocity, 1.f, ECC_PhysicsBody,
	                                    Character.Get());
	Params.SimFrequency = 10.0f;
#if ENABLE_DRAW_DEBUG
	Params.DrawDebugType = RagdollTaskDebugVar::bDrawDebugImpactTrace
		                       ? EDrawDebugTrace::Type::ForOneFrame
		                       : EDrawDebugTrace::Type::None;
#endif

	FPredictProjectilePathResult Result;
	UGameplayStatics::PredictProjectilePath(GetWorld(), Params, Result);

	RagdollingState.TimeToImpact = Result.LastTraceDestination.Time;
	RagdollingState.ImpactDirection = FGASPRagdollLibrary::ComputeImpactDirection(
		TraceOrigin, Result.LastTraceDestination.Location);
}

void UCharacterTask_Ragdoll::RefreshRagdollRollBehavior(const float DeltaTime)
{
	FGASPRagdollLibrary::AdvanceRollForces(RagdollingState, Character->GetMoverState().FloorNormal, DeltaTime);

	if (Mesh->IsAnySimulatingPhysics())
	{
		Mesh->AddTorqueInDegrees(FGASPRagdollLibrary::ComputeAutoRollTorque(RagdollingState), NAME_spine_05, true);
	}
}

void UCharacterTask_Ragdoll::RefreshRagdollPhysicsStrengthsFromCurves() const
{
	if (RagdollingState.AppliedProfileName != NAME_Ragdoll)
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

bool UCharacterTask_Ragdoll::AreComponentsValid() const
{
	return Mesh.IsValid() && CapsuleComponent.IsValid() && PhysicsControlComponent.IsValid();
}
