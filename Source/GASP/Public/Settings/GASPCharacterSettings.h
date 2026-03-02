#pragma once

#include "Engine/DataAsset.h"
#include "GASPCharacterSettings.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType, MinimalAPI)
class UGASPCharacterSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	UGASPCharacterSettings();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rotation")
	float TurnInPlaceThreshold{60.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdolling")
	uint8 bStartRagdollingOnLand : 1 {false};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdolling",
		Meta = (ClampMin = 0, EditCondition = "bStartRagdollingOnLand", ForceUnits = "cm/s"))
	float RagdollingOnLandSpeedThreshold{1000.0f};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ragdolling")
	TObjectPtr<UAnimMontage> GetUpMontageFront{};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ragdolling")
	TObjectPtr<UAnimMontage> GetUpMontageBack{};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ragdolling")
	uint8 bLimitInitialRagdollSpeed : 1{false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta=(ClampMin="0.0", ClampMax="1.0"))
	float AnalogMovementThreshold{.7f};

	UPROPERTY(EditAnywhere, Category="Choosers", BlueprintReadOnly)
	TObjectPtr<class UChooserTable> OverlayTable{nullptr};
	UPROPERTY(EditAnywhere, Category="Choosers", BlueprintReadOnly)
	TObjectPtr<UChooserTable> PosesTable{nullptr};
	UPROPERTY(EditAnywhere, Category="Choosers", BlueprintReadOnly)
	TObjectPtr<UChooserTable> RotationCurveTable{nullptr};
};
