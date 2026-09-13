#pragma once

#include "MovementMode.h"
#include "UObject/Object.h"
#include "RagdollSettings.generated.h"

/**
 * 
 */
UCLASS()
class GASP_API URagdollSettings : public UObject, public IMovementSettingsInterface
{
	GENERATED_BODY()

public:
	URagdollSettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	virtual FString GetDisplayName() const override { return GetName(); }
};
