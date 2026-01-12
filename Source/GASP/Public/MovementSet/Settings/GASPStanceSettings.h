#pragma once

#include "MovementMode.h"
#include "Types/TagTypes.h"
#include "GASPStanceSettings.generated.h"

class UGASPGaitSettings;
/**
 * 
 */
UCLASS(MinimalAPI, BlueprintType)
class UGASPStanceSettings : public UObject, public IMovementSettingsInterface
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stance")
	TMap<FGameplayTag, TObjectPtr<UGASPGaitSettings>> StanceSettings{
		{StanceTags::Standing, nullptr},
		{StanceTags::Crouching, nullptr}
	};

	virtual FString GetDisplayName() const override { return GetName(); }
};
