#pragma once

#include "MovementMode_Smoothing.h"
#include "MovementMode_Walking.generated.h"

DECLARE_STATS_GROUP(TEXT("MovementWalkStats"), STATGROUP_Movement_Walk, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("GenerateWalkMove Logic"), STAT_GenerateWalkMove, STATGROUP_Movement_Walk);
/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class GASP_API UMovementMode_Walking : public UMovementMode_Smoothing
{
	GENERATED_BODY()

public:
	UMovementMode_Walking(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GenerateWalkMove_Implementation(FMoverTickStartData& StartState, float DeltaSeconds,
	                                             const FVector& DesiredVelocity, const FQuat& DesiredFacing,
	                                             const FQuat& CurrentFacing, FVector& InOutAngularVelocityDegrees,
	                                             FVector& InOutVelocity) override;
	virtual void Activate() override;

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Blueprint Overrides (Change These)",
		meta=(ForceUnits="CentimetersPerSecond"))
	float WalkSpeed{165.f};
	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Blueprint Overrides (Change These)")
	float WalkAcceleration{500.f};
	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Blueprint Overrides (Change These)",
		meta=(ForceUnits="CentimetersPerSecond"))
	float RunSpeed{375.f};
	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Blueprint Overrides (Change These)")
	float RunAcceleration{800.f};

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Blueprint Overrides (Change These)",
		meta=(ForceUnits="CentimetersPerSecond"))
	float SprintSpeed{585.f};

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Blueprint Overrides (Change These)")
	float SprintAcceleration{300.f};

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Blueprint Overrides (Change These)",
		meta=(ForceUnits="CentimetersPerSecond"))
	float CrouchSpeed{200.f};

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Blueprint Overrides (Change These)")
	float WalkRunTurnStrength{8.f};

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Blueprint Overrides (Change These)")
	float SprintTurnStrength{4.f};

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Blueprint Overrides (Change These)")
	float GaitChangeDeceleration{300.f};

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Blueprint Overrides (Change These)")
	float StoppingDeceleration{1000.f};

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Blueprint Overrides (Change These)")
	float WalkRunFacingTime{.4f};

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Blueprint Overrides (Change These)")
	float SprintFacingTime{.8f};

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Blueprint Overrides (Change These)")
	float IdleFacingTime{.2f};

private:
	uint8 bJustLanded : 1{false};
};
