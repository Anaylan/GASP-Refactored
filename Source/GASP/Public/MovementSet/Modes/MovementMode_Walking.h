#pragma once

#include "GASPWalkingMode_Base.h"
#include "DefaultMovementSet/MovementModifiers/StanceModifier.h"
#include "Types/MovementTypes.h"
#include "MovementMode_Walking.generated.h"

class USmoothWalkingMode;
class UStanceSettings;
/**
 * 
 */
UCLASS(BlueprintType)
class GASP_API UMovementMode_Walking : public UGASPWalkingMode_Base
{
	GENERATED_BODY()

public:
	UMovementMode_Walking();

	virtual void GenerateWalkMove_Implementation(FMoverTickStartData& StartState, float DeltaSeconds,
	                                             const FVector& DesiredVelocity, const FQuat& DesiredFacing,
	                                             const FQuat& CurrentFacing, FVector& InOutAngularVelocityDegrees,
	                                             FVector& InOutVelocity) override;


	virtual void Activate() override;
	virtual void Deactivate() override;
	float GetMappedSpeed() const;

private:
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess))
	uint8 bJustLanded : 1{false};
	

	UFUNCTION()
	void OnStanceChanged(const FGameplayTag OldGameplayTag, const FGameplayTag NewGameplayTag);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<FGameplayTag, FGaitSettings> MovementSettings;

	UPROPERTY(BlueprintReadOnly)
	FGaitSettings GaitSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float IdleFacingTime{.2f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float WalkRunTurnStrength{8.f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float SprintTurnStrength{4.f};
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float WalkRunFacingTime{.4f};
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float SprintFacingTime{.8f};
};
