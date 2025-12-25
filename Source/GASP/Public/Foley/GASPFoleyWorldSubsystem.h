#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "GASPFoleyWorldSubsystem.generated.h"

class UNiagaraComponent;

USTRUCT(BlueprintType)
struct FGASPFoleyOut
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UAudioComponent> AudioComponent{};

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UDecalComponent> DecalComponent{};

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UNiagaraComponent> NiagaraComponent{};
};

/**
 * 
 */
UCLASS()
class GASP_API UGASPFoleyWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "Foley")
	FGASPFoleyOut PlayFoleyEvent(class UGASPFootstepEffectsSet* AudioBank, const USkinnedMeshComponent* Mesh,
	                             const FName SocketName, const bool bSpawnSound = true, const bool bSpawnDecal = true,
	                             const bool bSpawnParticleSystem = true, const bool bSpawnInAir = false,
	                             const float TraceLength = 25.f, const float VolumeMultiplier = 1.f,
	                             const float PitchMultiplier = 1.f);

	UFUNCTION(BlueprintCallable, Category = "Foley")
	UAudioComponent* SpawnSound(const USkinnedMeshComponent* Mesh,
	                            const struct FGASPFootstepSoundSettings& SoundSettings,
	                            const FVector& FootstepLocation, const float VolumeMultiplier = 1.f,
	                            const float PitchMultiplier = 1.f) const;

	UFUNCTION(BlueprintCallable, Category = "Foley")
	UDecalComponent* SpawnDecal(const USkinnedMeshComponent* Mesh,
	                            const struct FGASPFootstepDecalSettings& DecalSettings,
	                            const FVector& FootstepLocation, const FRotator& FootstepRotation,
	                            const FHitResult& FootstepHit,
	                            const FName& SocketName) const;

	UFUNCTION(BlueprintCallable, Category = "Foley")
	UNiagaraComponent* SpawnParticleSystem(const USkinnedMeshComponent* Mesh,
	                                       const struct FGASPFootstepParticleSettings& ParticleSystemSettings,
	                                       const FVector& FootstepLocation, const FRotator& FootstepRotation) const;

#if WITH_EDITOR && ALLOW_CONSOLE
	UFUNCTION(BlueprintCallable, Category = "Foley")
	void DebugLog(const FTransform& Transform, const FLinearColor& VisLogDebugColor, const FString& VisLogDebugText);
#endif

private:
	UPROPERTY()
	TObjectPtr<UWorld> World;
};
