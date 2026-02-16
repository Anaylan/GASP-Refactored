#pragma once

#include "Engine/DataAsset.h"
#include "Types/TagTypes.h"
#include "GASPGaitSettings.generated.h"

class UCurveVector;

USTRUCT(BlueprintType)
struct FGaitSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gait")
	float MaxSpeed{.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gait")
	float Acceleration{.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gait")
	float FacingTime{.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Gait")
	float TurnStrength{.0f};
};

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<FGameplayTag, FGaitSettings> SettingsMap{
		{GaitTags::Walk, {165.f, 500.f, .4f, 8.f}},
		{GaitTags::Run, {375.f,800.f, .4f, 8.f}},
		{GaitTags::Sprint, {575.f,300.f, .8f, 4.f}}
	};

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UCurveVector> MovementCurve{};
};
