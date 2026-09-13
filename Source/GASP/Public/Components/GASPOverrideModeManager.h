#pragma once

#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "GASPOverrideModeComponent.generated.h"


class UCharacterTask_Override;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GASP_API UGASPOverrideModeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGASPOverrideModeComponent();
	
	void AddOverrideLayer(FGameplayTag OverrideTag, TSubclassOf<UAnimInstance> OverrideAnimClass);
	void RemoveOverrideLayer(FGameplayTag OverrideTag);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient, Meta = (DisplayThumbnail = false))
	TMap<FGameplayTag, TSubclassOf<UCharacterTask_Override>> OverrideClassMap;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	FGameplayTag CurrentOverrideTag;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	TWeakObjectPtr<UCharacterTask_Override> CurrentOverrideTask;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	TMap<FGameplayTag, TObjectPtr<UCharacterTask_Override>> InstancedOverrideTasks;
};
