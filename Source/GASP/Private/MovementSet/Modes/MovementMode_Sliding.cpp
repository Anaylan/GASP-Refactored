#include "MovementSet/Modes/MovementMode_Sliding.h"
#include "MoverComponent.h"
#include "DefaultMovementSet/Settings/StanceSettings.h"
#include "MovementSet/Transitions/MovementModeTransition_FromSlide.h"
#include "Types/MovementTypes.h"

UMovementMode_Sliding::UMovementMode_Sliding(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SharedSettingsClasses.Add(UStanceSettings::StaticClass());
	Transitions.Add(CreateDefaultSubobject<UMovementModeTransition_FromSlide>(FName{TEXT("FromSlide")}));

	Acceleration = 2000.f;
	Deceleration = 200.f;
	TurningStrength = 1.f;
	AccelerationSmoothingTime = 0.f;
	DecelerationSmoothingTime = 0.f;
	FacingSmoothingTime = .15f;

	bSmoothFacingWithDoubleSpring = false;
}

void UMovementMode_Sliding::GenerateWalkMove_Implementation(FMoverTickStartData& StartState, float DeltaSeconds,
                                                            const FVector& DesiredVelocity, const FQuat& DesiredFacing,
                                                            const FQuat& CurrentFacing,
                                                            FVector& InOutAngularVelocityDegrees,
                                                            FVector& InOutVelocity)
{
	const auto* CharacterInputs = StartState.InputCmd.InputCollection.FindDataByType<FGASPMoverInputs>();

	const float CurrentOffset{
		static_cast<float>((CurrentFacing.Rotator() - DesiredFacing.Rotator()).GetNormalized().Yaw)
	};

	const float RotRad{
		FMath::DegreesToRadians(FMath::Clamp(CharacterInputs->RotationOffset, CurrentOffset - 179.f,
		                                     CurrentOffset + 179.f))
	};
	const auto OverridenDesiredFacing{DesiredFacing * FQuat{FVector::UpVector, RotRad}};

	FHitResult FloorHit{};
	GetMoverComponent()->TryGetFloorCheckHitResult(FloorHit);

	const float SlopeAngle{
		static_cast<float>(180.0 / UE_DOUBLE_PI * FMath::Acos(
			FVector::DotProduct(FloorHit.Normal, DesiredVelocity.GetSafeNormal())) - 90.f)
	};

	MaxSpeedOverride = InitialBoost
		                   ? InitialBoostSpeed
		                   : SlopeAngle > ShallowSlopeAngle * -1.f
		                   ? FlatGroundSpeed
		                   : FMath::GetMappedRangeValueClamped<float, float>(
			                   {ShallowSlopeAngle * -1.f, SteepSlopeAngle * -1.f}, {ShallowSlopeSpeed, SteepSlopeSpeed},
			                   SlopeAngle);

	Acceleration = InitialBoost ? AfterBoostAcceleration : InitialBoostAcceleration;
	Deceleration = FMath::GetMappedRangeValueClamped<float, float>({ShallowSlopeAngle, SteepSlopeAngle},
	                                                               {FlatGroundDeceleration, SteepSlopeDeceleration},
	                                                               SlopeAngle);

	Super::GenerateWalkMove_Implementation(StartState, DeltaSeconds, DesiredVelocity, OverridenDesiredFacing,
	                                       CurrentFacing, InOutAngularVelocityDegrees, InOutVelocity);
}

void UMovementMode_Sliding::Activate()
{
	InitialBoost = true;

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateLambda([this]
	{
		InitialBoost = false;
	}), InitialBoostTime, false);
	Super::Activate();
}
