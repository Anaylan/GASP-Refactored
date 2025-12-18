#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "PoseSearch/PoseSearchHistory.h"
#include "GASPInteractionInterface.generated.h"

UINTERFACE()
class UGASPInteractionInterface : public UInterface
{
	GENERATED_BODY()
};

class GASP_API IGASPInteractionInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Traversal")
	void SetInteractionTransform(const FTransform& InteractionTransform);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Traversal")
	void GetInteractionTransform(FTransform& OutInteractionTransform) const;
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Traversal")
	FPoseHistoryReference GetPoseHistory() const;
};

UCLASS(meta = (BlueprintThreadSafe))
class GASP_API UInteractionTransformBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Traversal")
	static UPARAM(DisplayName = "Success") bool SetInteractionTransform(UObject* InteractionTransformObject,
	                                                                    const FTransform& InteractionTransform);
	UFUNCTION(BlueprintCallable, Category="Traversal")
	static UPARAM(DisplayName = "Success") bool GetInteractionTransform(UObject* InteractionTransformObject,
	                                                                    UPARAM(DisplayName = "InteractionTransform")
	                                                                    FTransform& OutInteractionTransform);
};
