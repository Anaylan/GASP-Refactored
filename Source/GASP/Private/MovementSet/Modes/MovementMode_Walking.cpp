#include "MovementSet/Modes/MovementMode_Walking.h"
#include "MoverComponent.h"
#include "DefaultMovementSet/Settings/StanceSettings.h"
#include "MovementSet/Transitions/MovementModeTransition_ToSlide.h"
#include "Types/MovementTypes.h"


UMovementMode_Walking::UMovementMode_Walking(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SharedSettingsClasses.Add(UStanceSettings::StaticClass());
	Transitions.Add(CreateDefaultSubobject<UMovementModeTransition_ToSlide>(FName{TEXT("ToSlide")}));
	
	Acceleration = 1000.f;
	TurningStrength = 8.f;
	FacingSmoothingTime = .5f;
	bSmoothFacingWithDoubleSpring = false;
}

void UMovementMode_Walking::GenerateWalkMove_Implementation(FMoverTickStartData& StartState, float DeltaSeconds,
                                                            const FVector& DesiredVelocity, const FQuat& DesiredFacing,
                                                            const FQuat& CurrentFacing,
                                                            FVector& InOutAngularVelocityDegrees,
                                                            FVector& InOutVelocity)
{
	const auto* CharacterInputs = StartState.InputCmd.InputCollection.FindDataByType<FGASPMoverInputs>();

	float CurrentOffset{static_cast<float>((CurrentFacing.Rotator() - DesiredFacing.Rotator()).GetNormalized().Yaw)};

	float RotRad{
		FMath::DegreesToRadians(FMath::Clamp(CharacterInputs->RotationOffset, CurrentOffset - 179.f,
		                                     CurrentOffset + 179.f))
	};
	auto OverridenDesiredFacing{DesiredFacing * FQuat{FVector::UpVector, RotRad}};

	if (CharacterInputs->Stance == StanceTags::Crouching)
	{
		MaxSpeedOverride = CrouchSpeed;
	}
	else
	{
		if (CharacterInputs->Gait == GaitTags::Walk)
		{
			MaxSpeedOverride = WalkSpeed;
		}
		else if (CharacterInputs->Gait == GaitTags::Sprint)
		{
			MaxSpeedOverride = SprintSpeed;
		}
		else
		{
			MaxSpeedOverride = RunSpeed;
		}
	}

	if (CharacterInputs->Gait == GaitTags::Walk)
	{
		Acceleration = WalkAcceleration;
	}
	else if (CharacterInputs->Gait == GaitTags::Sprint)
	{
		Acceleration = InOutVelocity.Size2D() > RunSpeed ? SprintAcceleration : RunAcceleration;
	}
	else
	{
		Acceleration = RunAcceleration;
	}

	Deceleration = CharacterInputs->GetMoveInput().IsZero()
		               ? bJustLanded
			                 ? StoppingDeceleration
			                 : 20000.f
		               : GaitChangeDeceleration;

	TurningStrength = FMath::GetMappedRangeValueClamped<float, float>({RunSpeed, SprintSpeed},
	                                                                  {WalkRunTurnStrength, SprintTurnStrength},
	                                                                  InOutVelocity.Size2D());

	const float YawDeg{static_cast<float>((DesiredFacing.Rotator() - CurrentFacing.Rotator()).GetNormalized().Yaw)};
	float VelocityMapped{
		FMath::GetMappedRangeValueClamped<float, float>({RunSpeed, SprintSpeed}, {WalkRunFacingTime, SprintFacingTime},
		                                                InOutVelocity.Size2D())
	};
	FacingSmoothingTime = CharacterInputs->GetMoveInput().IsZero()
		                      ? IdleFacingTime
		                      : FMath::GetMappedRangeValueClamped<float, float>(
			                      {90.f, 135.f}, {VelocityMapped, .2f}, FMath::Abs(YawDeg));

	Super::GenerateWalkMove_Implementation(StartState, DeltaSeconds, DesiredVelocity, OverridenDesiredFacing,
	                                       CurrentFacing, InOutAngularVelocityDegrees, InOutVelocity);

	if (CurrentOffset <= -135.f)
	{
		InOutAngularVelocityDegrees.Z = FMath::Clamp(InOutAngularVelocityDegrees.Z,
		                                             CharacterInputs->ControlRotationRate, 1000.f);
	}
	else if (CurrentOffset >= 135.f)
	{
		InOutAngularVelocityDegrees.Z = FMath::Clamp(InOutAngularVelocityDegrees.Z,
		                                             -1000.f, CharacterInputs->ControlRotationRate);
	}
}

void UMovementMode_Walking::Activate()
{
	if (GetMoverComponent()->GetMovementModeName() == DefaultModeNames::Falling)
	{
		bJustLanded = true;
		GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([this]
		{
			bJustLanded = false;
		}));
	}
	Super::Activate();
}
