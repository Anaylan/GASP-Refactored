#pragma once

#include "PoseSearch/PoseSearchHistory.h"
#include "UObject/Interface.h"
#include "GASPAnimContextInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UGASPAnimContextInterface : public UInterface
{
	GENERATED_BODY()
};

class GASP_API IGASPAnimContextInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Traversal")
	FPoseHistoryReference GetPoseHistory() const;
};
