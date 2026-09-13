#pragma once


#include "PoseSearch/PoseSearchResult.h"
#include "UObject/Interface.h"
#include "GASPInteractionInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UGASPInteractionInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class GASP_API IGASPInteractionInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Interaction")
	FPoseSearchBlueprintResult GetInteractionResult() const;
};
