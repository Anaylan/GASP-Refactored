#include "MovementSet/Transitions/MovementModeTransition_FromSlide.h"
#include "MoverSimulationTypes.h"
#include "Types/MovementTypes.h"

FTransitionEvalResult UMovementModeTransition_FromSlide::Evaluate_Implementation(
	const FSimulationTickParams& Params) const
{
	if (const auto& InputCmd = Params.StartState.InputCmd.InputCollection.FindDataByType<FGASPMoverInputs>())
	{
		if (Params.ProposedMove.LinearVelocity.Size2D() <= 200.f || InputCmd->Stance != StanceTags::Crouching)
		{
			return FTransitionEvalResult{DefaultModeNames::Walking};
		}
	}
	return Super::Evaluate_Implementation(Params);
}
