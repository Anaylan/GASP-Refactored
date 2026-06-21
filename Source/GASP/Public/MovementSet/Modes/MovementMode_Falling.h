#pragma once

#include "DefaultMovementSet/Modes/FallingMode.h"
#include "MovementSet/GASPMovementInterface.h"
#include "Types/TagTypes.h"
#include "MovementMode_Falling.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class GASP_API UMovementMode_Falling : public UFallingMode, public IGASPMovementInterface
{
	GENERATED_BODY()

public:
	virtual void GenerateMove_Implementation(const FMoverSimContext& SimContext, const FMoverTickStartData& StartState,
	                                         const FMoverTimeStep& TimeStep,
	                                         FProposedMove& OutProposedMove) const override;
	virtual FGameplayTag GetAssociatedTag_Implementation() override { return MovementModeTags::InAir; }
};
