#pragma once

#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "GASPMoverComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMover_OnStateChanged, FGameplayTag, OldGameplayTag, FGameplayTag,
                                             NewGameplayTag);


UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class GASP_API UGASPMoverComponent : public UCharacterMoverComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UGASPMoverComponent();

	virtual void InitCollisionParams(FCollisionQueryParams& OutParams,
	                                 FCollisionResponseParams& OutResponseParam) const;
	virtual void OnMoverPreSimulationTick(const FMoverTimeStep& TimeStep, const FMoverInputCmdContext& InputCmd) override;
	
	UPROPERTY(BlueprintAssignable)
	FMover_OnStateChanged GaitChanged;
	UPROPERTY(BlueprintAssignable)
	FMover_OnStateChanged RotationModeChanged;

protected:
};
