#include "MovementSet/Transitions/MovementModeTransition_FromSlide.h"
#include "MoverSimulationTypes.h"
#include "MovementSet/GASPMoverComponent.h"
#include "Types/MovementTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MovementModeTransition_FromSlide)

FTransitionEvalResult UMovementModeTransition_FromSlide::Evaluate_Implementation(
	const FSimulationTickParams& Params) const
{
	if (Params.ProposedMove.LinearVelocity.Size2D() <= 200.f || !static_cast<UGASPMoverComponent*>(Params.
		MovingComps.MoverComponent.Get())->IsCrouching())
	{
		return FTransitionEvalResult{DefaultModeNames::Walking};
	}

	return Super::Evaluate_Implementation(Params);
}
