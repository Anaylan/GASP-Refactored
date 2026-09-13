#include "MovementSet/Transitions/MovementModeTransition_ToSlide.h"
#include "MoverComponent.h"
#include "MoverSimulationTypes.h"
#include "MovementSet/GASPMoverComponent.h"
#include "MovementSet/Modes/MovementMode_Sliding.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MovementModeTransition_ToSlide)

FTransitionEvalResult UMovementModeTransition_ToSlide::Evaluate_Implementation(
	const FSimulationTickParams& Params) const
{
	const auto* MoverComponent{Cast<UGASPMoverComponent>(Params.MovingComps.MoverComponent.Get())};

	// IsCrouching() is the same source MovementModeTransition_FromSlide reads. Using the gameplay
	// tag here instead would let entry and exit disagree for a frame around the stance change.
	if (MoverComponent && Params.ProposedMove.LinearVelocity.Size2D() > 380.f && MoverComponent->IsCrouching())
	{
		return FTransitionEvalResult{MovementModeNames::Sliding};
	}

	return Super::Evaluate_Implementation(Params);
}
