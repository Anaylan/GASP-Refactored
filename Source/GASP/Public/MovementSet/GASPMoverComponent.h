#pragma once

#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "GASPMoverComponent.generated.h"

UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class GASP_API UGASPMoverComponent : public UCharacterMoverComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGASPMoverComponent();

protected:
	// Called when the game starts
	virtual FVector GetFeetLocation();
};
