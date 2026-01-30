#include "MovementSet/GASPMoverComponent.h"
#include "Actors/GASPCharacter.h"
#include "MovementSet/Modes/MovementMode_Falling.h"
#include "DefaultMovementSet/Modes/FlyingMode.h"
#include "MovementSet/Modes/MovementMode_Sliding.h"
#include "MovementSet/Modes/MovementMode_Walking.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPMoverComponent)


// Sets default values for this component's properties
UGASPMoverComponent::UGASPMoverComponent()
{
	MovementModes.Reset();
	MovementModes.Add(DefaultModeNames::Walking,
	                  CreateDefaultSubobject<UMovementMode_Walking>(DefaultModeNames::Walking));
	MovementModes.Add(MovementModeNames::Sliding,
	                  CreateDefaultSubobject<UMovementMode_Sliding>(MovementModeNames::Sliding));
	MovementModes.Add(DefaultModeNames::Falling,
	                  CreateDefaultSubobject<UMovementMode_Falling>(DefaultModeNames::Falling));
	MovementModes.Add(DefaultModeNames::Flying, CreateDefaultSubobject<UFlyingMode>(DefaultModeNames::Flying));

	bSyncInputsForSimProxy = true;
}

void UGASPMoverComponent::InitCollisionParams(FCollisionQueryParams& OutParams,
                                              FCollisionResponseParams& OutResponseParam) const
{
	if (const auto* PrimitiveComponent = GetMovementBase())
	{
		PrimitiveComponent->InitSweepCollisionParams(OutParams, OutResponseParam);
	}
}

void UGASPMoverComponent::OnMoverPreSimulationTick(const FMoverTimeStep& TimeStep,
                                                   const FMoverInputCmdContext& InputCmd)
{
	const auto* LastInput{GetLastInputCmd().InputCollection.FindDataByType<FGASPMoverInputs>()};

	Super::OnMoverPreSimulationTick(TimeStep, InputCmd);

	const auto* CharacterInputs{InputCmd.InputCollection.FindDataByType<FGASPMoverInputs>()};

	if (CharacterInputs && CharacterInputs->Stance == StanceTags::Crouching)
	{
		Crouch();
	}
	else
	{
		UnCrouch();
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
	}
}
