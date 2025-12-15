#pragma once

#include "DefaultMovementSet/Modes/FallingMode.h"
#include "MovementMode_Falling.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class GASP_API UMovementMode_Falling : public UFallingMode
{
	GENERATED_BODY()

public:	
	virtual void GenerateMove_Implementation(const FMoverTickStartData& StartState, const FMoverTimeStep& TimeStep,
	                                         FProposedMove& OutProposedMove) const override;
};
