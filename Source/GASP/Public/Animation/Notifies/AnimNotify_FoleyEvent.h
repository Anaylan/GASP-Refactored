#pragma once

#include "GameplayTagContainer.h"
#include "AnimNotify_FoleyEvent.generated.h"

/**
 * 
 */
UCLASS()
class GASP_API UAnimNotify_FoleyEvent : public UAnimNotify
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AnimNotify")
	FGameplayTag Event{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AnimNotify")
	FName SocketName{NAME_None};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AnimNotify", Meta = (ClampMin = 0, ForceUnits = "x"))
	float VolumeMultiplier{1.f};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AnimNotify", Meta = (ClampMin = 0, ForceUnits = "x"))
	float PitchMultiplier{1.f};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AnimNotify", Meta = (ForceInlineRow))
	TObjectPtr<class UGASPFootstepEffectsSet> DefaultBank{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AnimNotify|Debug")
	FLinearColor VisLogDebugColor{FLinearColor::Black};
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AnimNotify|Debug")
	FString VisLogDebugText{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AnimNotify")
	FGameplayTagContainer MovementTags{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AnimNotify")
	float TraceLength{25.f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimNotify|Sound")
	uint8 bSpawnSound : 1 {true};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimNotify|Decal")
	uint8 bSpawnDecal : 1 {true};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimNotify|Particle System")
	uint8 bSpawnParticleSystem : 1 {true};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimNotify")
	uint8 bSpawnInAir : 1 {false};

	UGASPFootstepEffectsSet* GetAudioBank(AActor* Owner) const;
public:
	UAnimNotify_FoleyEvent();

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	                    const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

	UFUNCTION(Blueprintable, Category="Foley|Audio")
	bool CanPlayFootstepEffects(AActor* Owner) const;
};
