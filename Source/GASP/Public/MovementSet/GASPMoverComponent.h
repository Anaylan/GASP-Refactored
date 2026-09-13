#pragma once

#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "Types/TagTypes.h"
#include "GASPMoverComponent.generated.h"

class UMoverTrajectoryPredictor;
class UMotionWarpingMoverAdapter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMover_OnTagChanged, FGameplayTag, OldGameplayTag, FGameplayTag,
                                             NewGameplayTag);

UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class GASP_API UGASPMoverComponent : public UCharacterMoverComponent
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	TObjectPtr<UMoverTrajectoryPredictor> TrajectoryPredictor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State", Transient)
	TObjectPtr<UMotionWarpingMoverAdapter> MotionWarpingMoverAdapter;

public:
	UGASPMoverComponent();

	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
	virtual void InitCollisionParams(FCollisionQueryParams& OutParams,
	                                 FCollisionResponseParams& OutResponseParam) const;

	UPROPERTY(BlueprintAssignable)
	FMover_OnTagChanged GaitChanged;
	UPROPERTY(BlueprintAssignable)
	FMover_OnTagChanged RotationModeChanged;
	UPROPERTY(BlueprintAssignable)
	FMover_OnTagChanged StanceModeChanged;
	UPROPERTY(BlueprintAssignable)
	FMover_OnTagChanged LocomotionModeChanged;

	UFUNCTION(BlueprintPure)
	FGameplayTag GetRotationMode() const;
	UFUNCTION(BlueprintPure)
	FGameplayTag GetStanceMode() const;
	UFUNCTION(BlueprintPure)
	FGameplayTag GetGait() const;
	UFUNCTION(BlueprintPure)
	FGameplayTag GetLocomotionMode() const;

	UMoverTrajectoryPredictor* GetTrajectoryPredictor() const { return TrajectoryPredictor; }

	UFUNCTION(BlueprintPure, Category = Mover)
	virtual bool IsRagdolling() const;

	virtual void OnPostSimulate(const FMoverTimeStep& TimeStep, const FMoverTickStartData& StartingData,
	                            OUT FMoverTickEndData& EndingData) override;

protected:
	virtual void OnMoverPreSimulationTick(const FMoverTimeStep& TimeStep,
	                                      const FMoverInputCmdContext& InputCmd) override;
	virtual void OnMoverPostFinalize(const FMoverSyncState& SyncState, const FMoverAuxStateContext& AuxState) override;

	UFUNCTION()
	void HandleMovementModeChanged(const FName& PreviousMovementModeName, const FName& NewMovementModeName);
private:
	FMovementModifierHandle RagdollModifierHandle;

	FGameplayTag Stance{StanceTags::Standing};
	FGameplayTag Gait{GaitTags::Run};
	FGameplayTag RotationMode{RotationTags::OrientToMovement};
};
