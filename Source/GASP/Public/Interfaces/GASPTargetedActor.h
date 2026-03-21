#pragma once

#include "GASPTargetedActor.generated.h"

/**
 * 
 */
UINTERFACE()
class UGASPTargetedActor : public UInterface
{
	GENERATED_BODY()
};

class GASP_API IGASPTargetedActor
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	AActor* GetTargetedActor();
};