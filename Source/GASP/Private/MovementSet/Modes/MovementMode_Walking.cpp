#include "MovementSet/Modes/MovementMode_Walking.h"
#include "MoverComponent.h"
#include "DefaultMovementSet/Settings/StanceSettings.h"
#include "MovementSet/Settings/GASPGaitSettings.h"
#include "MovementSet/Settings/GASPStanceSettings.h"
#include "MovementSet/Transitions/MovementModeTransition_ToSlide.h"
#include "Types/MovementTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MovementMode_Walking)

UMovementMode_Walking::UMovementMode_Walking(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SharedSettingsClasses.Add(UGASPStanceSettings::StaticClass());
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
	SCOPE_CYCLE_COUNTER(STAT_GenerateWalkMove);

	const auto* CharacterInputs = StartState.InputCmd.InputCollection.FindDataByType<FGASPMoverInputs>();

	if (const auto SharedSettings = GetMoverComponent()->FindSharedSettings<UGASPStanceSettings>())
	{
		StanceSettings = SharedSettings->StanceSettings.FindRef(CharacterInputs->Stance);
	}

	const auto* RunSettings{StanceSettings->SettingsMap.Find(GaitTags::Run)};
	const auto* SprintSettings{StanceSettings->SettingsMap.Find(GaitTags::Sprint)};
	const auto* CurrentSettings{StanceSettings->SettingsMap.Find(CharacterInputs->Gait)};

	const float RunSpeed{RunSettings->MaxSpeed};
	const float SprintSpeed{SprintSettings->MaxSpeed};

	MaxSpeedOverride = CurrentSettings->MaxSpeed;
	Acceleration = CharacterInputs->Gait == GaitTags::Sprint && InOutVelocity.Size2D() <= RunSpeed
		               ? RunSettings->Acceleration
		               : CurrentSettings->Acceleration;

	Deceleration = CharacterInputs->GetMoveInput().IsZero()
		               ? bJustLanded
			                 ? 20000.f
			                 : StoppingDeceleration
		               : GaitChangeDeceleration;

	const auto FwdCurrent{CurrentFacing.GetForwardVector()};
	const auto FwdDesired{DesiredFacing.GetForwardVector()};

	const float CurrentOffset{
		static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(
			FwdDesired.X * FwdCurrent.Y - FwdDesired.Y * FwdCurrent.X,
			FwdDesired.X * FwdCurrent.X + FwdDesired.Y * FwdCurrent.Y
		)))
	};

	const float RotRad{
		FMath::DegreesToRadians(FMath::Clamp(CharacterInputs->RotationOffset, CurrentOffset - 179.f,
		                                     CurrentOffset + 179.f))
	};
	const auto OverridenDesiredFacing{DesiredFacing * FQuat{FVector::UpVector, RotRad}};
	const FVector2f SpeedRange{RunSpeed, SprintSpeed};

	TurningStrength = FMath::GetMappedRangeValueClamped<float, float>(SpeedRange,
	                                                                  {
		                                                                  RunSettings->TurnStrength,
		                                                                  SprintSettings->TurnStrength
	                                                                  },
	                                                                  InOutVelocity.Size2D());
	float VelocityMapped{
		FMath::GetMappedRangeValueClamped<float, float>(SpeedRange,
		                                                {RunSettings->FacingTime, SprintSettings->FacingTime},
		                                                InOutVelocity.Size2D())
	};

	const float YawDeg{static_cast<float>((DesiredFacing.Rotator() - CurrentFacing.Rotator()).GetNormalized().Yaw)};
	FacingSmoothingTime = CharacterInputs->GetMoveInput().IsZero()
		                      ? IdleFacingTime
		                      : FMath::GetMappedRangeValueClamped<float, float>(
			                      {90.f, 135.f}, {VelocityMapped, .2f}, FMath::Abs(YawDeg));

	Super::GenerateWalkMove_Implementation(StartState, DeltaSeconds, DesiredVelocity, OverridenDesiredFacing,
	                                       CurrentFacing, InOutAngularVelocityDegrees, InOutVelocity);

	if (FMath::Abs(CurrentOffset) >= 135.f)
	{
		const float& Rate{CharacterInputs->ControlRotationRate};
		InOutAngularVelocityDegrees.Z = (CurrentOffset <= -135.f)
			                                ? FMath::Clamp(InOutAngularVelocityDegrees.Z, Rate, 1000.f)
			                                : FMath::Clamp(InOutAngularVelocityDegrees.Z, -1000.f, Rate);
	}
}

void UMovementMode_Walking::Activate()
{
	Super::Activate();

	if (GetMoverComponent()->GetMovementModeName() == DefaultModeNames::Falling)
	{
		bJustLanded = true;
		GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([this]
		{
			bJustLanded = false;
		}));
	}
}
