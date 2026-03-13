#pragma once

#include "Engine/DataAsset.h"
#include "GASPCharacterSettings.generated.h"

class AGASPCharacter;
class UChooserTable;
struct FStreamableHandle;

/**
 *
 */
UCLASS(Blueprintable, BlueprintType)
class GASP_API UGASPCharacterSettings : public UPrimaryDataAsset
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
	TSoftObjectPtr<UChooserTable> OverlayTable{nullptr};
	UPROPERTY(EditAnywhere, Category="Choosers", BlueprintReadOnly)
	TSoftObjectPtr<UChooserTable> PosesTable{nullptr};
	UPROPERTY(EditAnywhere, Category="Choosers", BlueprintReadOnly)
	TSoftObjectPtr<UChooserTable> RotationCurveTable{nullptr};
	UPROPERTY(EditAnywhere, Category="Choosers", BlueprintReadOnly)
	TSoftObjectPtr<UChooserTable> TraversalTable{nullptr};
	
	virtual void PostLoad() override;

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void PreloadTables();
	
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(FPrimaryAssetType{TEXT("CharacterSettings")}, GetFName());
	}

private:
	TSharedPtr<FStreamableHandle> TablesLoadHandle;
};
