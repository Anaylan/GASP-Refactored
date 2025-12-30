#pragma once

#include "GameplayTagContainer.h"
#include "GASPMovementInterface.generated.h"

/**
 * 
 */
UINTERFACE()
class UGASPMovementInterface : public UInterface
{
	GENERATED_BODY()
};

class GASP_API IGASPMovementInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	FGameplayTag GetAssociatedTag();
};
