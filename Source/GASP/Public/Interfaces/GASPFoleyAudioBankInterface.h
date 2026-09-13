#pragma once

#include "GameplayTagContainer.h"
#include "GASPFoleyAudioBankInterface.generated.h"

UINTERFACE()
class UGASPFoleyAudioBankInterface : public UInterface
{
	GENERATED_BODY()
};

class GASP_API IGASPFoleyAudioBankInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Foley|Audio")
	UGASPFootstepEffectsSet* GetFootstepEffects(FGameplayTag GameplayTag);
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Foley|Audio")
	bool CanPlayFootstepEffects();
};