#pragma once

#include "MovementMode_Smoothing.h"
#include "MovementSet/GASPMovementInterface.h"
#include "Types/TagTypes.h"
#include "MovementMode_Walking.generated.h"

DECLARE_STATS_GROUP(TEXT("MovementWalkStats"), STATGROUP_Movement_Walk, STATCAT_Advanced);

DECLARE_CYCLE_STAT(TEXT("GenerateWalkMove Logic"), STAT_GenerateWalkMove, STATGROUP_Movement_Walk);

class UGASPGaitSettings;
/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class GASP_API UMovementMode_Walking : public UMovementMode_Smoothing, public IGASPMovementInterface
{
	GENERATED_BODY()

public:
	UMovementMode_Walking(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual FGameplayTag GetAssociatedTag_Implementation() override { return MovementModeTags::Grounded; }
	virtual void GenerateWalkMove_Implementation(FMoverTickStartData& StartState, float DeltaSeconds,
	                                             const FVector& DesiredVelocity, const FQuat& DesiredFacing,
	                                             const FQuat& CurrentFacing, FVector& InOutAngularVelocityDegrees,
	                                             FVector& InOutVelocity) override;
	virtual void Activate() override;
	
	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Blueprint Overrides (Change These)")
	float GaitChangeDeceleration{300.f};

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Blueprint Overrides (Change These)")
	float StoppingDeceleration{1000.f};
	
	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Blueprint Overrides (Change These)")
	float IdleFacingTime{.2f};

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Blueprint Overrides (Change These)",
		meta=(ForceUnits="CentimetersPerSecond"))
	TObjectPtr<UGASPGaitSettings> StanceSettings{};

private:
	uint8 bJustLanded : 1{false};
};
