#pragma once

#include "MovementMode_Smoothing.h"
#include "MovementMode_Sliding.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class GASP_API UMovementMode_Sliding : public UMovementMode_Smoothing
{
	GENERATED_BODY()

public:
	UMovementMode_Sliding(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GenerateWalkMove_Implementation(FMoverTickStartData& StartState, float DeltaSeconds,
	                                             const FVector& DesiredVelocity, const FQuat& DesiredFacing,
	                                             const FQuat& CurrentFacing, FVector& InOutAngularVelocityDegrees,
	                                             FVector& InOutVelocity) override;
virtual void Activate() override;
private:
	/** Please add a variable description */
	UPROPERTY(EditDefaultsOnly, Category="Default")
	uint8 InitialBoost:1{false};

public:
	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Blueprint Overrides (Change These)")
	float InitialBoostTime{.2f};

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Blueprint Overrides (Change These)")
	float InitialBoostSpeed{800.f};

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Blueprint Overrides (Change These)")
	float InitialBoostAcceleration{2000.f};

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Blueprint Overrides (Change These)")
	float AfterBoostAcceleration{300.f};

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Blueprint Overrides (Change These)")
	float SteepSlopeAngle{40.f};

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Blueprint Overrides (Change These)")
	float ShallowSlopeAngle{10.f};

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Blueprint Overrides (Change These)")
	float SteepSlopeSpeed{800.f};

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Blueprint Overrides (Change These)")
	float ShallowSlopeSpeed{500.f};

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Blueprint Overrides (Change These)")
	float FlatGroundSpeed{100.f};

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Blueprint Overrides (Change These)")
	float SteepSlopeDeceleration{2000.f};

	/** Please add a variable description */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="Blueprint Overrides (Change These)")
	float FlatGroundDeceleration{500.f};
};
