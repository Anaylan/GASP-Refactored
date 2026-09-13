#include "MovementSet/GASPMoverComponent.h"

#include "MotionWarpingComponent.h"
#include "MotionWarpingMoverAdapter.h"
#include "MoverPoseSearchTrajectoryPredictor.h"
#include "GASP.h"
#include "Actors/GASPCharacter.h"
#include "Components/PrimitiveComponent.h"
#include "MovementSet/Modes/MovementMode_Falling.h"
#include "DefaultMovementSet/Modes/FlyingMode.h"
#include "MovementSet/Modes/MovementMode_Sliding.h"
#include "MovementSet/Modes/MovementMode_Ragdolling.h"
#include "MovementSet/Modes/MovementMode_Walking.h"
#include "MovementSet/Modifiers/MovementModifier_Ragdoll.h"
#include "Types/MovementTypes.h"
#include "Types/TagTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPMoverComponent)

UGASPMoverComponent::UGASPMoverComponent()
{
	SetIsReplicatedByDefault(true);
	bSyncInputsForSimProxy = true;

	PersistentSyncStateDataTypes.Add(FMoverDataPersistence(FGASPMoverSyncState::StaticStruct(), true));

	MovementModes.Reset();
	MovementModes.Add(DefaultModeNames::Walking,
	                  CreateDefaultSubobject<UMovementMode_Walking>(DefaultModeNames::Walking));
	MovementModes.Add(MovementModeNames::Sliding,
	                  CreateDefaultSubobject<UMovementMode_Sliding>(MovementModeNames::Sliding));
	MovementModes.Add(DefaultModeNames::Falling,
	                  CreateDefaultSubobject<UMovementMode_Falling>(DefaultModeNames::Falling));
	MovementModes.Add(DefaultModeNames::Flying, CreateDefaultSubobject<UFlyingMode>(DefaultModeNames::Flying));
	MovementModes.Add(MovementModeNames::Ragdolling,
	                  CreateDefaultSubobject<UMovementMode_Ragdolling>(MovementModeNames::Ragdolling));
	MovementModes.Add(UNullMovementMode::NullModeName,
	                  CreateDefaultSubobject<UNullMovementMode>(UNullMovementMode::NullModeName));
}

void UGASPMoverComponent::InitializeComponent()
{
	Super::InitializeComponent();

	TrajectoryPredictor = NewObject<UMoverTrajectoryPredictor>(this);
	TrajectoryPredictor->Setup(this);

	// The component is BlueprintSpawnableComponent, so it can be placed on any actor. The motion
	// warping adapter is the only part that needs an AGASPCharacter; trajectory prediction above
	// works regardless, so a foreign owner leaves the component usable rather than crashing.
	const auto* Character{Cast<AGASPCharacter>(GetOwner())};
	if (!Character)
	{
		UE_LOG(LogGASP, Error, TEXT("%s: owner %s is not an AGASPCharacter; motion warping is disabled"),
		       *GetName(), *GetNameSafe(GetOwner()));
		return;
	}

	auto* WarpingComponent{Character->GetMotionWarpingComponent()};
	if (!WarpingComponent)
	{
		UE_LOG(LogGASP, Error, TEXT("%s: %s has no MotionWarpingComponent; motion warping is disabled"),
		       *GetName(), *Character->GetName());
		return;
	}

	MotionWarpingMoverAdapter = WarpingComponent->CreateOwnerAdapter<UMotionWarpingMoverAdapter>();
	MotionWarpingMoverAdapter->SetMoverComp(this);
}

void UGASPMoverComponent::BeginPlay()
{
	Super::BeginPlay();

	OnMovementModeChanged.AddUniqueDynamic(this, &ThisClass::HandleMovementModeChanged);
}

void UGASPMoverComponent::InitCollisionParams(FCollisionQueryParams& OutParams,
                                              FCollisionResponseParams& OutResponseParam) const
{
	if (const auto* PrimitiveComponent = GetMovementBase())
	{
		PrimitiveComponent->InitSweepCollisionParams(OutParams, OutResponseParam);
	}
}

FGameplayTag UGASPMoverComponent::GetRotationMode() const
{
	return RotationMode;
}

FGameplayTag UGASPMoverComponent::GetStanceMode() const
{
	return Stance;
}

FGameplayTag UGASPMoverComponent::GetGait() const
{
	return Gait;
}

FGameplayTag UGASPMoverComponent::GetLocomotionMode() const
{
	const auto MovementMode = GetActiveMode_Mutable();
	return IGASPMovementInterface::GetAssociatedTagSafe(MovementMode);
}

bool UGASPMoverComponent::IsRagdolling() const
{
	return HasGameplayTag(MovementModeTags::Ragdoll, true);
}

void UGASPMoverComponent::OnPostSimulate(const FMoverTimeStep& TimeStep, const FMoverTickStartData& StartingData,
                                         OUT FMoverTickEndData& EndingData)
{
	Super::OnPostSimulate(TimeStep, StartingData, EndingData);

	if (auto* CharacterSyncState = EndingData.SyncState.SyncStateCollection.FindMutableDataByType<
		FGASPMoverSyncState>())
	{
		CharacterSyncState->Stance = GetStanceMode();
		CharacterSyncState->Gait = GetGait();
		CharacterSyncState->RotationMode = GetRotationMode();
	}
}

void UGASPMoverComponent::OnMoverPreSimulationTick(const FMoverTimeStep& TimeStep,
                                                   const FMoverInputCmdContext& InputCmd)
{
	const auto* LastInput{GetLastInputCmd().InputCollection.FindDataByType<FGASPMoverInputs>()};
	const auto* CharacterInputs{InputCmd.InputCollection.FindDataByType<FGASPMoverInputs>()};

	const bool bWantsRagdoll{GetNextMovementModeName() == MovementModeNames::Ragdolling};
	const bool bHasRagdollModifier{FindMovementModifierByType<FMovementModifier_Ragdoll>() != nullptr};

	// Ragdolling owns the capsule through FMovementModifier_Ragdoll, so stance intent is frozen for the whole
	// episode - neither Crouch() nor UnCrouch(). bWantsToCrouch is a plain component field belonging to neither
	// the input cmd nor the sync state, while FStanceModifier belongs to the replicated sync state; letting the
	// two disagree mid-episode costs a modifier group reconcile, which FMovementModifierGroup::ShouldReconcile
	// compares by index with no tolerance, and a TeleportPhysics through the ragdolling mesh.
	if (!bWantsRagdoll && !bHasRagdollModifier && CharacterInputs)
	{
		if (CharacterInputs->Stance == StanceTags::Crouching)
		{
			Crouch();
		}
		else
		{
			UnCrouch();
		}
	}

	// Queues the stance modifier. Keeping it ahead of the ragdoll reconcile fixes the order the two are appended
	// to the modifier group, which FMovementModifierGroup::ShouldReconcile compares by index.
	Super::OnMoverPreSimulationTick(TimeStep, InputCmd);

	const auto* RagdollModifier{FindMovementModifier(RagdollModifierHandle)};
	// The handle is local and does not survive a rollback, so fall back to a type lookup before deciding.
	if (!RagdollModifier)
	{
		RagdollModifier = FindMovementModifierByType<FMovementModifier_Ragdoll>();
	}

	if (bWantsRagdoll)
	{
		// Sole owner of the capsule for the episode. An FStanceModifier that was already running keeps its own
		// bookkeeping untouched - freezing stance intent above guarantees it neither starts nor ends here - and
		// FMovementModifier_Ragdoll::OnEnd hands the capsule back in the shape that surviving stance implies.
		if (!RagdollModifier)
		{
			RagdollModifierHandle = QueueMovementModifier(MakeShared<FMovementModifier_Ragdoll>());
		}
	}
	else if (RagdollModifier)
	{
		// Cancel through the instance's own handle rather than the cached one, so an orphaned or rolled back
		// handle still resolves.
		CancelModifierFromHandle(RagdollModifier->GetHandle());
		RagdollModifierHandle.Invalidate();
	}

	if (CharacterInputs)
	{
		if (CharacterInputs->RotationMode.IsValid() && RotationMode != CharacterInputs->RotationMode)
		{
			RotationMode = CharacterInputs->RotationMode;
		}

		if (CharacterInputs->Stance.IsValid() && Stance != CharacterInputs->Stance)
		{
			Stance = CharacterInputs->Stance;
		}

		if (CharacterInputs->Gait.IsValid() && Gait != CharacterInputs->Gait)
		{
			Gait = CharacterInputs->Gait;
		}

		if (LastInput)
		{
			if (LastInput->Gait != CharacterInputs->Gait)
			{
				GaitChanged.Broadcast(LastInput->Gait, CharacterInputs->Gait);
			}
			if (LastInput->RotationMode != CharacterInputs->RotationMode)
			{
				RotationModeChanged.Broadcast(LastInput->RotationMode, CharacterInputs->RotationMode);
			}
			if (LastInput->Stance != CharacterInputs->Stance)
			{
				StanceModeChanged.Broadcast(LastInput->Stance, CharacterInputs->Stance);
			}
		}
	}
}

void UGASPMoverComponent::OnMoverPostFinalize(const FMoverSyncState& SyncState, const FMoverAuxStateContext& AuxState)
{
	if (const auto* CharacterSyncState = SyncState.SyncStateCollection.FindDataByType<FGASPMoverSyncState>())
	{
		Stance = CharacterSyncState->Stance;
		Gait = CharacterSyncState->Gait;
		RotationMode = CharacterSyncState->RotationMode;
	}

	Super::OnMoverPostFinalize(SyncState, AuxState);
}

void UGASPMoverComponent::HandleMovementModeChanged(const FName& PreviousMovementModeName,
                                                    const FName& NewMovementModeName)
{
	if (PreviousMovementModeName != NewMovementModeName)
	{
		const auto PrevLocomotionModeTag{
			IGASPMovementInterface::GetAssociatedTagSafe(FindMovementModeByName(PreviousMovementModeName))
		};
		const auto NextLocomotionModeTag{
			IGASPMovementInterface::GetAssociatedTagSafe(FindMovementModeByName(NewMovementModeName))
		};

		LocomotionModeChanged.Broadcast(PrevLocomotionModeTag, NextLocomotionModeTag);
	}
}
