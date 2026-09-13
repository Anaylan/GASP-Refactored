#pragma once

#include "MovementMode.h"
#include "MovementSet/GASPMovementInterface.h"
#include "Types/TagTypes.h"
#include "MovementMode_Ragdolling.generated.h"

class UCommonLegacyMovementSettings;
struct FMoverTickEndData;

namespace MovementModeNames
{
	// extern plus a single definition in the .cpp. A namespace-scope const FName in a header has internal
	// linkage, so every translation unit including this header would get its own copy and its own static
	// initialization of the same name.
	GASP_API extern const FName Ragdolling;
}


UCLASS(MinimalAPI, Blueprintable, BlueprintType)
class UMovementMode_Ragdolling : public UBaseMovementMode, public IGASPMovementInterface
{
	GENERATED_BODY()

public:
	UMovementMode_Ragdolling(const FObjectInitializer& ObjectInitializer);

	/**
	 * Bounds of the speed budget the capsule uses to chase the ragdoll's top bone. These are not the
	 * character's minimum and maximum speeds: they clamp the top bone's own speed, and the result becomes
	 * the ceiling on the length of the chase vector.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Mover,
		meta = (ClampMin = "0", UIMin = "0", ForceUnits = "cm/s"))
	float MinSpeed{300.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Mover,
		meta = (ClampMin = "0", UIMin = "0", ForceUnits = "cm/s"))
	float MaxSpeed{5000.0f};

	virtual void GenerateMove_Implementation(const FMoverSimContext& SimContext, const FMoverTickStartData& StartState,
	                                         const FMoverTimeStep& TimeStep,
	                                         FProposedMove& OutProposedMove) const override;
	virtual void SimulationTick_Implementation(const FSimulationTickParams& Params,
	                                           FMoverTickEndData& OutputState) override;

	virtual FGameplayTag GetAssociatedTag_Implementation() const override { return MovementModeTags::Ragdoll; }

	virtual void OnRegistered(const FName ModeName, const FMoverSimContext& SimContext) override;
	virtual void OnUnregistered(const FMoverSimContext& SimContext) override;

protected:
	void CaptureFinalState(USceneComponent* UpdatedComponent, FMovementRecord& Record,
	                       const FMoverDefaultSyncState& StartSyncState, const FVector& AngularVelocityDegrees,
	                       FMoverDefaultSyncState& OutputSyncState, FMoverTickEndData& TickEndData,
	                       const float DeltaSeconds) const;

	UPROPERTY()
	TObjectPtr<const UCommonLegacyMovementSettings> CommonLegacySettings;
};
