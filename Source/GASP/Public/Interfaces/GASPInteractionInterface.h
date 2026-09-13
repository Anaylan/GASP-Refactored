#pragma once

#include "PoseSearch/PoseSearchResult.h"
#include "UObject/Interface.h"
#include "GASPInteractionInterface.generated.h"

class UAnimInstance;

UINTERFACE(BlueprintType, MinimalAPI)
class UGASPInteractionInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Lets any actor take part in a synchronized interaction, not just AGASPCharacter.
 */
class GASP_API IGASPInteractionInterface
{
	GENERATED_BODY()

public:
	// Null for static interactables that do not participate in motion matching.
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
	UAnimInstance* GetInteractionAnimContext() const;

	/** Pose search result of the running interaction; read by the anim graph and the warping notify. */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
	FPoseSearchBlueprintResult GetInteractionResult() const;
};
