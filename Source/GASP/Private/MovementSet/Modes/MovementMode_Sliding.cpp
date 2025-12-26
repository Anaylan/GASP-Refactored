#include "MovementSet/Modes/MovementMode_Sliding.h"
#include "MoverComponent.h"
#include "DefaultMovementSet/Settings/StanceSettings.h"
#include "MovementSet/Transitions/MovementModeTransition_FromSlide.h"
#include "Types/MovementTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MovementMode_Sliding)

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

	const FVector FwdCurrent = CurrentFacing.GetForwardVector();
	const FVector FwdDesired = DesiredFacing.GetForwardVector();

	const float CurrentOffset = FMath::RadiansToDegrees(FMath::Atan2(
		FwdDesired.X * FwdCurrent.Y - FwdDesired.Y * FwdCurrent.X,
		FwdDesired.X * FwdCurrent.X + FwdDesired.Y * FwdCurrent.Y
	));

	const float RotRad{
		FMath::DegreesToRadians(FMath::Clamp(CharacterInputs->RotationOffset, CurrentOffset - 179.f,
		                                     CurrentOffset + 179.f))
	};
	const auto OverridenDesiredFacing{DesiredFacing * FQuat{FVector::UpVector, RotRad}};

	FHitResult FloorHit{};
	GetMoverComponent()->TryGetFloorCheckHitResult(FloorHit);

	float SlopeAngle{0.0f};
	if (const float VSizeSq = DesiredVelocity.SizeSquared(); VSizeSq > KINDA_SMALL_NUMBER)
	{
		const float SlopeSin = FVector::DotProduct(FloorHit.Normal, DesiredVelocity * FMath::InvSqrt(VSizeSq));
		SlopeAngle = -FMath::RadiansToDegrees(FMath::FastAsin(SlopeSin));
	}

	MaxSpeedOverride = InitialBoost
		                   ? InitialBoostSpeed
		                   : SlopeAngle > -ShallowSlopeAngle
		                   ? FlatGroundSpeed
		                   : FMath::GetMappedRangeValueClamped<float, float>(
			                   {-ShallowSlopeAngle, -SteepSlopeAngle}, {ShallowSlopeSpeed, SteepSlopeSpeed},
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
