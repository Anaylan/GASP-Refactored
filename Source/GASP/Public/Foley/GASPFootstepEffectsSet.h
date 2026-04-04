#pragma once

#include "Engine/DataAsset.h"
#include "GASPFootstepEffectsSet.generated.h"

class USoundBase;
class UMaterialInterface;
class UNiagaraSystem;

USTRUCT(BlueprintType)
struct GASP_API FGASPFootstepSoundSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GASP")
	TSoftObjectPtr<USoundBase> Sound;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GASP")
	TObjectPtr<USoundAttenuation> SoundAttenuation;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GASP")
	TObjectPtr<USoundConcurrency> SoundConcurrency;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GASP", Meta = (ClampMin = 0, ForceUnits = "x"))
	float VolumeMultiplier{1.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GASP", Meta = (ClampMin = 0, ForceUnits = "x"))
	float PitchMultiplier{1.f};
	
};

USTRUCT(BlueprintType)
struct GASP_API FGASPFootstepDecalSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GASP")
	TSoftObjectPtr<UMaterialInterface> DecalMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GASP", Meta = (ClampMin = 0, ForceUnits = "s"))
	float Duration{4.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GASP", Meta = (ClampMin = 0, ForceUnits = "s"))
	float FadeOutDuration{2.f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GASP", Meta = (AllowPreserveRatio))
	FVector3f Size{10.f, 20.f, 20.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GASP", Meta = (AllowPreserveRatio))
	FVector3f LocationOffset{0.f, -10.f, -1.75f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GASP")
	FRotator3f FootLeftRotationOffset{90.0f, 0.0f, -90.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GASP")
	FRotator3f FootRightRotationOffset{-90.0f, 0.0f, 90.0f};

#if WITH_EDITOR
	void PostEditChangeProperty(const FPropertyChangedEvent& ChangedEvent);
#endif
};

USTRUCT(BlueprintType)
struct GASP_API FGASPFootstepParticleSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GASP")
	TSoftObjectPtr<UNiagaraSystem> ParticleSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GASP")
	FVector3f LocationOffset{ForceInit};
};

USTRUCT(BlueprintType)
struct GASP_API FGASPFootstepEffectsSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GASP")
	FGASPFootstepSoundSettings SoundSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GASP")
	FGASPFootstepDecalSettings DecalSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GASP")
	FGASPFootstepParticleSettings ParticleSettings;
};

/**
 * 
 */
UCLASS()
class GASP_API UGASPFootstepEffectsSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GASP")
	TMap<TEnumAsByte<EPhysicalSurface>, FGASPFootstepEffectsSettings> FootstepSettings;

public:
	FGASPFootstepEffectsSettings* GetFootstepSettingsFromSurface(EPhysicalSurface Surface);
};
