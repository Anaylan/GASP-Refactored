#pragma once

#include "MoverSimulationTypes.h"
#include "GameFramework/PlayerController.h"
#include "GASPPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class GASP_API AGASPPlayerController : public APlayerController, public IMoverInputProducerInterface
{
	GENERATED_BODY()
	
public:
	
	// Override this function in native class to author input for the next simulation frame. Consider also calling Super method.
	virtual void ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext &InputCmdResult) override;
};
