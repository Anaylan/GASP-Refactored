#pragma once

#include "GASPHeldObjectInterface.generated.h"

class UMeshComponent;

UINTERFACE()
class UGASPHeldObjectInterface : public UInterface
{
	GENERATED_BODY()
};

class GASP_API IGASPHeldObjectInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "HeldObject")
	UMeshComponent* GetHeldObject();
};
