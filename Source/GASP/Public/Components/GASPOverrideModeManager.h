#pragma once

#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "GASPOverrideModeManager.generated.h"


class UCharacterTask_Override;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnOverrideLayerChanged,
                                              FGameplayTag, OverrideTag, bool, bIsActive);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GASP_API UGASPOverrideModeManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UGASPOverrideModeManager();

	UFUNCTION(BlueprintCallable, Category = "GASP|Override")
	void AddOverrideLayer(FGameplayTag OverrideTag, TSubclassOf<UAnimInstance> OverrideAnimClass);

	UFUNCTION(BlueprintCallable, Category = "GASP|Override")
	void RemoveOverrideLayer(FGameplayTag OverrideTag);

	UPROPERTY(BlueprintAssignable, Category = "GASP|Override")
	FOnOverrideLayerChanged OnOverrideLayerChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	TMap<FGameplayTag, TObjectPtr<UCharacterTask_Override>> InstancedOverrideTasks;
};
