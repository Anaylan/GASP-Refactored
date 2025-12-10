#pragma once

#include "GASPWalkingMode_Base.h"
#include "DefaultMovementSet/Modes/SmoothWalkingMode.h"
#include "MovementMode_Slide.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class GASP_API UMovementMode_Slide : public UGASPWalkingMode_Base
{
	GENERATED_BODY()
	
public:	
	UMovementMode_Slide(const FObjectInitializer& ObjectInitializer); 
	
	virtual void GenerateWalkMove_Implementation(FMoverTickStartData& StartState, float DeltaSeconds,
												 const FVector& DesiredVelocity, const FQuat& DesiredFacing,
												 const FQuat& CurrentFacing, FVector& InOutAngularVelocityDegrees,
												 FVector& InOutVelocity) override;

	virtual void Activate() override;
	
private:
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess))
	uint8 bInitialBoost : 1{false};
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess))
	float InitialBoostTime{.2f};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess))
	float InitialBoostSpeed{800.f};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess))
	float FlatGroundSpeed{100.0};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess))
	float ShallowSlopeAngle{10.f};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess))
	float SteepSlopeAngle{40.0};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess))
	float AfterBoostAcceleration{300.0};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess))
	float InitialBoostAcceleration{2000.0};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess))
	float FlatGroundDeceleration{500.0};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess))
	float SteepSlopeDeceleration{2000.0};
};
