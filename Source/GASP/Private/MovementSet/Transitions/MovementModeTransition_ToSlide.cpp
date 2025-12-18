#include "MovementSet/Transitions/MovementModeTransition_ToSlide.h"
#include "MoverComponent.h"
#include "MoverSimulationTypes.h"
#include "Types/MovementTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MovementModeTransition_ToSlide)

FTransitionEvalResult UMovementModeTransition_ToSlide::Evaluate_Implementation(
	const FSimulationTickParams& Params) const
{
	if (Params.ProposedMove.LinearVelocity.Size2D() > 380.f && Params.MovingComps.MoverComponent->HasGameplayTag(
		Mover_IsCrouching, false))
	{
		return FTransitionEvalResult{MovementModeNames::Sliding};
	}

	return Super::Evaluate_Implementation(Params);
}
