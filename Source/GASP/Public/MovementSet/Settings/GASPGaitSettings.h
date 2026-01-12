#pragma once

#include "Engine/DataAsset.h"
#include "Types/TagTypes.h"
#include "GASPGaitSettings.generated.h"

class UCurveVector;

/**
 * 
 */
UCLASS(MinimalAPI, BlueprintType)
class UGASPGaitSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	UGASPGaitSettings();

	UFUNCTION(BlueprintPure, Category="GASP|Gait")
	UCurveVector* GetMovementCurve() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly,
		meta = (Description = "X = Forward Speed, Y = Backwards Speed"))
	TMap<FGameplayTag, float> SpeedMap{
		{GaitTags::Walk, 165.f},
		{GaitTags::Run, 375.f},
		{GaitTags::Sprint, 575.f}
	};

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UCurveVector> MovementCurve{};
};
