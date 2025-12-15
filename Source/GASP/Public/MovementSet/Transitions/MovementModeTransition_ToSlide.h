#pragma once

#include "MovementModeTransition.h"
#include "MovementModeTransition_ToSlide.generated.h"

/**
 * 
 */
UCLASS()
class GASP_API UMovementModeTransition_ToSlide : public UBaseMovementModeTransition
{
	GENERATED_BODY()

public:
	virtual FTransitionEvalResult Evaluate_Implementation(const FSimulationTickParams& Params) const override;
};
