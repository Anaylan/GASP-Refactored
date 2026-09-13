#include "PhysicsControlComponent.h"
#include "Actors/GASPCharacter.h"
#include "Animation/GASPAnimInstance.h"
#include "Components/GASPOverrideModeManager.h"
#include "Interfaces/GASPAnimContextInterface.h"
#include "MoveLibrary/PlayMoverMontageCallbackProxy.h"
#include "MovementSet/GASPMovementInterface.h"
#include "MovementSet/GASPMoverComponent.h"
#include "MovementSet/Modes/MovementMode_Ragdolling.h"
#include "Net/Core/PushModel/PushModel.h"
#include "PoseSearch/PoseSearchLibrary.h"
#include "Settings/GASPCharacterSettings.h"
#include "Tasks/CharacterTask_Ragdoll.h"
#include "Utils/GASPChooserLibrary.h"
#include "Utils/GASPPhysicsControlLibrary.h"
#include "Utils/GASPRagdollLibrary.h"


void AGASPCharacter::StartRagdolling(const bool bStopActiveMontages, const FMontageBlendSettings& BlendSettings,
                                     const FGameplayTag& InjuryState)
{
	if (GetLocalRole() <= ROLE_SimulatedProxy || IsRagdolling())
	{
		return;
	}

	if (GetLocalRole() >= ROLE_Authority)
	{
		MulticastStartRagdolling(bStopActiveMontages, BlendSettings, InjuryState);
	}
	else
	{
		ServerStartRagdolling(bStopActiveMontages, BlendSettings, InjuryState);
		StartRagdollingImplementation(bStopActiveMontages, BlendSettings, InjuryState);
	}
}

void AGASPCharacter::ServerStartRagdolling_Implementation(const bool bStopActiveMontages,
                                                          const FMontageBlendSettings& BlendSettings,
                                                          const FGameplayTag& InjuryState)
{
	MulticastStartRagdolling(bStopActiveMontages, BlendSettings, InjuryState);
	ForceNetUpdate();
}

void AGASPCharacter::MulticastStartRagdolling_Implementation(const bool bStopActiveMontages,
                                                             const FMontageBlendSettings& BlendSettings,
                                                             const FGameplayTag& InjuryState)
{
	StartRagdollingImplementation(bStopActiveMontages, BlendSettings, InjuryState);
}

void AGASPCharacter::StartRagdollingImplementation(const bool bStopActiveMontages,
                                                   const FMontageBlendSettings& BlendSettings,
                                                   const FGameplayTag& InjuryState)
{
	if (IsRagdolling() || (RagdollTask && RagdollTask->IsActive()))
	{
		return;
	}

	CharacterMotionComponent->QueueNextMode(MovementModeNames::Ragdolling);

	RagdollTask = UCharacterTask_Ragdoll::CreateRagdollTask(this, bStopActiveMontages, BlendSettings, InjuryState,
	                                                        GetSettingsChecked()->GetUpTable.LoadSynchronous());
	RagdollTask->OnRagdollStarted.AddUniqueDynamic(this, &ThisClass::OnStartRagdolling);
	RagdollTask->OnRagdollEnded.AddUniqueDynamic(this, &ThisClass::OnStopRagdolling);
	RagdollTask->ReadyForActivation();

	if (!RagdollTask->IsActive())
	{
		UE_LOG(LogTemp, Error,
		       TEXT("%s: RagdollTask failed to activate (missing GameplayTasksComponent?)"),
		       *GetName());
	}
}


void AGASPCharacter::SetPhysicsProfile(const FName NewPhysicsProfileName)
{
	if (!PhysicsControlComponent || !Mesh)
	{
		return;
	}

	// Update AppliedProfileName in TaskStates
	if (auto* State{TaskStates.FindMutableDataByType<FRagdollingState>()})
	{
		State->AppliedProfileName = NewPhysicsProfileName;
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, TaskStates, this);
	}

	PhysicsControlComponent->InvokeControlProfile(NewPhysicsProfileName);

	const bool bRagdollProfile{NewPhysicsProfileName == TEXT("Ragdoll")};

	// Every pair of distinct limb groups, so the number of legs is a property of the character
	// rather than of this function: two groups for a biped, six for a six-legged one.
	for (int32 First{0}; First < RagdollSelfCollisionGroups.Num(); ++First)
	{
		if (RagdollSelfCollisionGroups[First].BoneNames.IsEmpty())
		{
			continue;
		}

		for (int32 Second{First + 1}; Second < RagdollSelfCollisionGroups.Num(); ++Second)
		{
			if (RagdollSelfCollisionGroups[Second].BoneNames.IsEmpty())
			{
				continue;
			}

			const auto& FirstBones{RagdollSelfCollisionGroups[First].BoneNames};
			const auto& SecondBones{RagdollSelfCollisionGroups[Second].BoneNames};

			if (bRagdollProfile)
			{
				UGASPPhysicsControlLibrary::EnableCollisionBetweenBodyArrays(
					Mesh, FirstBones, Mesh, SecondBones
				);
			}
			else
			{
				UGASPPhysicsControlLibrary::DisableCollisionBetweenBodyArrays(
					Mesh, FirstBones, Mesh, SecondBones
				);
			}
		}
	}

	Mesh->SetConstraintProfileForAll(bRagdollProfile ? NAME_None : FName{TEXT("Free")});
}

bool AGASPCharacter::StopRagdolling()
{
	if (GetLocalRole() <= ROLE_SimulatedProxy)
	{
		return false;
	}

	if (GetLocalRole() >= ROLE_Authority)
	{
		MulticastStopRagdolling();
	}
	else
	{
		ServerStopRagdolling();
	}

	return true;
}

bool AGASPCharacter::IsRagdolling() const
{
	return RagdollTask && GetMoverComponent()->IsRagdolling();
}

void AGASPCharacter::ServerStopRagdolling_Implementation()
{
	MulticastStopRagdolling();
	ForceNetUpdate();
}

void AGASPCharacter::MulticastStopRagdolling_Implementation()
{
	StopRagdollingImplementation();
}

void AGASPCharacter::StopRagdollingImplementation()
{
	if (!RagdollTask)
	{
		return;
	}

	FHitResult Floor;
	const bool bHasFloor = GetMoverComponent()->TryGetFloorCheckHitResult(Floor) && Floor.bBlockingHit;
	const FName TargetModeName = bHasFloor ? DefaultModeNames::Walking : DefaultModeNames::Falling;
	GetMoverComponent()->QueueNextMode(TargetModeName);
}

void AGASPCharacter::TryPlayGetUpMontage()
{
	auto* AnimInstance{Mesh ? Cast<UGASPAnimInstance>(Mesh->GetAnimInstance()) : nullptr};
	if (!AnimInstance)
	{
		return;
	}

	AnimInstance->SnapshotFinalRagdollPose();

	auto GetUpTable{Settings ? Settings->GetUpTable.LoadSynchronous() : nullptr};
	if (!CharacterMotionComponent || !IsValid(GetUpTable))
	{
		return;
	}

	UPoseSearchLibrary::OverridePoseHistoryFromOwningMesh(AnimInstance, TEXT("PoseHistory"));

	FGASPGetUpInput Input;
	if (const auto* State{TaskStates.FindDataByType<FRagdollingState>()})
	{
		Input.bRollingGetup = FGASPRagdollLibrary::ShouldRollingGetUp(*State);
	}

	auto* MovementMode{
		CharacterMotionComponent->FindMovementModeByName(CharacterMotionComponent->GetNextMovementModeName())
	};
	Input.StateContainer = IGASPMovementInterface::GetAssociatedTagSafe(MovementMode).GetSingleTagContainer();

	FGASPGetUpOutput Output;
	auto PoseHistory = IGASPAnimContextInterface::Execute_GetPoseHistory(AnimInstance);

	auto* Montage{FGASPChooserUtils::EvaluateSingle<UAnimMontage>(GetUpTable, Input, PoseHistory, Output)};
	if (!Montage)
	{
		return;
	}

	UPlayMoverMontageCallbackProxy::CreateProxyObjectForPlayMoverMontage(
		CharacterMotionComponent, Montage, 1.0f, Output.MontageStartTime);
}
