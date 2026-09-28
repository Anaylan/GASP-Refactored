#include "Components/GASPCharacterInteractionComponent.h"

#include "Chooser.h"
#include "ChooserFunctionLibrary.h"
#include "MoverComponent.h"
#include "Actors/GASPCharacter.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/Skeleton.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Interfaces/GASPInteractionInterface.h"
#include "MoveLibrary/PlayMoverMontageCallbackProxy.h"
#include "MovementSet/GASPMoverComponent.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Net/UnrealNetwork.h"
#include "PoseSearch/PoseSearchDatabase.h"
#include "PoseSearch/PoseSearchInteractionAsset.h"
#include "PoseSearch/PoseSearchInteractionLibrary.h"
#include "Settings/GASPCharacterSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPCharacterInteractionComponent)

namespace
{
	/** Pose history node name expected in participant anim graphs. */
	static const FName PoseHistoryName{TEXTVIEW("PoseHistory")};

	bool IsInteractionSlotBusy(const UAnimInstance* AnimInstance)
	{
		return AnimInstance && AnimInstance->IsSlotActive(FAnimSlotGroup::DefaultSlotName);
	}
}

UGASPCharacterInteractionComponent::UGASPCharacterInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// Required for subobject RPC routing and Iris replication support.
	SetIsReplicatedByDefault(true);
}

void UGASPCharacterInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	CharacterOwner = Cast<AGASPCharacter>(GetOwner());
}

void UGASPCharacterInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FinishInteraction();

	Super::EndPlay(EndPlayReason);
}

void UGASPCharacterInteractionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	// Replicate to all connections since interaction execution is authority-driven.
	Params.Condition = COND_None;
	Params.RepNotifyCondition = REPNOTIFY_OnChanged;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, InteractionReps, Params);
}

void UGASPCharacterInteractionComponent::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	FinishInteraction();
}

void UGASPCharacterInteractionComponent::FinishInteraction()
{
	auto* Mesh{CharacterOwner.IsValid() ? CharacterOwner->GetMesh() : nullptr};
	auto* Capsule{CharacterOwner.IsValid() ? CharacterOwner->GetCapsuleComponent() : nullptr};

	for (auto It = Participants.CreateIterator(); It; ++It)
	{
		if (auto Participant{*It})
		{
			Participant->TaskStates.RemoveDataByType(FPoseSearchBlueprintResult::StaticStruct());

			if (Mesh)
			{
				Mesh->RemoveTickPrerequisiteComponent(Participant->GetMesh());
			}
			if (Capsule)
			{
				Capsule->ClearMoveIgnoreComponents();
				Participant->GetCapsuleComponent()->ClearMoveIgnoreComponents();
			}
		}
		It.RemoveCurrent();
	}
}

void UGASPCharacterInteractionComponent::Server_Interact_Implementation(const FName InteractionType,
                                                                        const TArray<AActor*>& Candidates)
{
	TArray<FGASPInteractionRep> Reps;
	if (!RunInteractionSearch(InteractionType, Candidates, Reps))
	{
		return;
	}

	Interact_ServerImplementation(Reps);
}

void UGASPCharacterInteractionComponent::Interact_ServerImplementation(const TArray<FGASPInteractionRep>& Reps)
{
	InteractionReps = Reps;

	// Strip non-replicated AnimContexts before sending; reconstructed locally in PerformInteraction.
	for (auto& [Actor, Result] : InteractionReps)
	{
		Result.AnimContexts.Empty();
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, InteractionReps, this);

	PerformInteraction();
}

void UGASPCharacterInteractionComponent::OnRep_InteractionReps()
{
	PerformInteraction();
}

TArray<TObjectPtr<const UObject>> UGASPCharacterInteractionComponent::MakeAnimContexts(
	TArrayView<const FGASPInteractionRep> Reps)
{
	// Indexed by role for UPoseSearchLibrary::GetActor lookup.
	int32 NumRoles{0};
	for (const auto& [Actor, Result] : Reps)
	{
		NumRoles = FMath::Max(NumRoles, Result.RoleIndex + 1);
	}

	TArray<TObjectPtr<const UObject>> AnimContexts;
	if (NumRoles <= 0)
	{
		return AnimContexts;
	}

	AnimContexts.SetNum(NumRoles);
	for (const auto& Rep : Reps)
	{
		if (AnimContexts.IsValidIndex(Rep.Result.RoleIndex))
		{
			AnimContexts[Rep.Result.RoleIndex] = GetAnimContext(Rep.Actor.Get());
		}
	}

	return AnimContexts;
}

void UGASPCharacterInteractionComponent::PerformInteraction_Implementation()
{
	if (!CharacterOwner.IsValid())
	{
		return;
	}

	const auto AnimContexts{MakeAnimContexts(InteractionReps)};

	TArray<TObjectPtr<AGASPCharacter>> NewParticipants;
	for (const auto& Rep : InteractionReps)
	{
		auto* Pawn{Cast<AGASPCharacter>(Rep.Actor.Get())};
		if (!Pawn)
		{
			continue;
		}

		if (const auto* InteractionAsset{Cast<UPoseSearchInteractionAsset>(Rep.Result.SelectedAnim)})
		{
			auto MontageToPlay{Cast<UAnimMontage>(InteractionAsset->GetAnimationAsset(Rep.Result.Role))};

			UPlayMoverMontageCallbackProxy::CreateProxyObjectForPlayMoverMontage(
				Pawn->FindComponentByClass<UMoverComponent>(), MontageToPlay, 1.f, Rep.Result.SelectedTime);

			if (auto* PawnAnimInstance{GetAnimContext(Pawn)})
			{
				FOnMontageEnded EndedDelegate;
				EndedDelegate.BindUObject(this, &ThisClass::OnMontageEnded);
				PawnAnimInstance->Montage_SetEndDelegate(EndedDelegate, MontageToPlay);

				FOnMontageBlendingOutStarted BlendOutDelegate;
				BlendOutDelegate.BindUObject(this, &ThisClass::OnMontageEnded);
				
				PawnAnimInstance->Montage_SetBlendingOutDelegate(BlendOutDelegate, MontageToPlay);
			}

			// Reattach local AnimContexts for IGASPInteractionInterface consumers.
			FPoseSearchBlueprintResult Stored{Rep.Result};
			Stored.AnimContexts = AnimContexts;

			// Store locally on each machine (driven by replicated InteractionReps).
			Pawn->TaskStates.AddOrOverwriteData(FInstancedStruct::Make(Stored));
			
			NewParticipants.Emplace(Pawn);
		}

		if (CharacterOwner != Pawn)
		{
			auto PawnMesh{Pawn->GetMesh()};
			CharacterOwner->GetMesh()->AddTickPrerequisiteComponent(PawnMesh);

			CharacterOwner->GetCapsuleComponent()->IgnoreComponentWhenMoving(Pawn->GetCapsuleComponent(), true);
			Pawn->GetCapsuleComponent()->IgnoreComponentWhenMoving(CharacterOwner->GetCapsuleComponent(), true);
		}
	}

	Participants.Append(MoveTemp(NewParticipants));
}

bool UGASPCharacterInteractionComponent::RunInteractionSearch(const FName InteractionType,
                                                              const TArray<AActor*>& Candidates,
                                                              TArray<FGASPInteractionRep>& OutReps)
{
	if (!CharacterOwner.IsValid())
	{
		return false;
	}

	auto* AnimInstance{GetAnimContext(CharacterOwner.Get())};
	if (!AnimInstance || IsInteractionSlotBusy(AnimInstance))
	{
		return false;
	}

	if (Candidates.IsEmpty())
	{
		return false;
	}

	const auto InteractionTable{
		CharacterOwner->GetSettings() ? CharacterOwner->GetSettings()->InteractionTable.LoadSynchronous() : nullptr
	};
	if (!InteractionTable)
	{
		return false;
	}

	FCharacterInteractionInput InputParam;
	InputParam.InteractionType = InteractionType;
	InputParam.Speed = CharacterOwner->GetMoverComponent()->GetVelocity().Size2D();
	FCharacterInteractionOutput OutputParam;

	auto Context{UChooserFunctionLibrary::MakeChooserEvaluationContext()};
	Context.AddStructParam(InputParam);
	Context.OutputArrays.Emplace(static_cast<uint32>(Context.Params.Num()));
	Context.AddStructParam(OutputParam);

	auto Objects{
		UChooserFunctionLibrary::EvaluateObjectChooserBaseMulti(
			Context, UChooserFunctionLibrary::MakeEvaluateChooser(InteractionTable), UPoseSearchDatabase::StaticClass())
	};

	const auto& OutputValues{Context.OutputArrays[0].OutputValues};

	TArray<FPoseSearchMotionMatchMultiQuery> Queries;
	Queries.Reserve(Objects.Num());

	for (auto It = Objects.CreateConstIterator(); It; ++It)
	{
		const auto& Roles{OutputValues[It.GetIndex()].Get<FCharacterInteractionOutput>()};
		auto& Query{Queries.AddDefaulted_GetRef()};
		Query.Database = Cast<UPoseSearchDatabase>(*It);
		Query.AnimContextsRoles.Add({AnimInstance, Roles.InitiatorRoles});

		for (const auto* Candidate : Candidates)
		{
			if (!Candidate)
			{
				continue;
			}

			// Skip candidates already playing an active montage.
			auto* CandidateAnimInstance{GetAnimContext(Candidate)};
			if (IsInteractionSlotBusy(CandidateAnimInstance))
			{
				continue;
			}

			Query.AnimContextsRoles.Add({CandidateAnimInstance, Roles.TargetRoles});
		}
	}

	TArray<FPoseSearchBlueprintResult> Results;
	UPoseSearchInteractionLibrary::MotionMatchMulti(Queries, PoseHistoryName, {}, Results);

	// Associate each pose search result with its corresponding actor for replication.
	OutReps.Reserve(Results.Num());
	for (auto It = Results.CreateConstIterator(); It; ++It)
	{
		auto* Actor{const_cast<AActor*>(UPoseSearchLibrary::GetActor(*It))};
		if (!Actor)
		{
			continue;
		}

		auto& Rep{OutReps.AddDefaulted_GetRef()};
		Rep.Actor = Actor;
		Rep.Result = *It;
	}

	if (OutReps.IsEmpty())
	{
		return false;
	}

	return true;
}

void UGASPCharacterInteractionComponent::TryInteract(const FName InteractionType, const TArray<AActor*>& Candidates)
{
	const auto* Owner{GetOwner()};
	if (!Owner || !Owner->HasAuthority())
	{
		// Forward request to server when called on client.
		Server_Interact(InteractionType, Candidates);
		return;
	}

	TArray<FGASPInteractionRep> Reps;
	if (!RunInteractionSearch(InteractionType, Candidates, Reps))
	{
		return;
	}

	Interact_ServerImplementation(Reps);
}

UAnimInstance* UGASPCharacterInteractionComponent::GetAnimContext(const AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return nullptr;
	}

	if (Actor->Implements<UGASPInteractionInterface>())
	{
		return IGASPInteractionInterface::Execute_GetInteractionAnimContext(Actor);
	}

	const auto* Mesh{Actor->FindComponentByClass<USkeletalMeshComponent>()};
	return Mesh ? Mesh->GetAnimInstance() : nullptr;
}
