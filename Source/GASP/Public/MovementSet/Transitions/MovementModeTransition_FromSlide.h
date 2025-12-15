#pragma once

#include "MovementModeTransition.h"
#include "MovementModeTransition_FromSlide.generated.h"

/**
 * 
 */
UCLASS()
class GASP_API UMovementModeTransition_FromSlide : public UBaseMovementModeTransition
{
	GENERATED_BODY()
	
public:
	virtual FTransitionEvalResult Evaluate_Implementation(const FSimulationTickParams& Params) const override;
};
