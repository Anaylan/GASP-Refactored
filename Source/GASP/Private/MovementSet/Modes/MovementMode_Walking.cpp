#include "MovementSet/Modes/MovementMode_Walking.h"

#include "Actors/GASPCharacter.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "DefaultMovementSet/Settings/StanceSettings.h"
#include "MovementSet/GASPMoverComponent.h"
#include "Types/MovementTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MovementMode_Walking)

UMovementMode_Walking::UMovementMode_Walking()
{
	SharedSettingsClasses.Add(UStanceSettings::StaticClass());
}

void UMovementMode_Walking::GenerateWalkMove_Implementation(FMoverTickStartData& StartState, float DeltaSeconds,
                                                            const FVector& DesiredVelocity, const FQuat& DesiredFacing,
                                                            const FQuat& CurrentFacing,
                                                            FVector& InOutAngularVelocityDegrees,
                                                            FVector& InOutVelocity)
{
	const auto* CharacterInputs = StartState.InputCmd.InputCollection.FindDataByType<FGASPMoverInputs>();
	const auto* StartingSyncState = StartState.SyncState.SyncStateCollection.FindDataByType<FMoverDefaultSyncState>();
	check(StartingSyncState);

	float CurrentOffset{static_cast<float>((CurrentFacing.Rotator() - DesiredFacing.Rotator()).Yaw)};

	const float ClampedYawDegrees = FMath::Clamp(CharacterInputs->RotationOffset, CurrentOffset - 179.f,
	                                             CurrentOffset + 179.f);
	const float YawRadians = FMath::DegreesToRadians(ClampedYawDegrees);
	const FQuat OverridenDesiredFacing{DesiredFacing * FQuat{FVector::UpVector, YawRadians}};

	FVector Velocity = GetMoverComponent()->GetVelocity();
	float RunSpeed = GaitSettings.GetSpeed(GaitTags::Run, Velocity, GetMoverComponent()->GetTargetOrientation());
	float SprintSpeed = GaitSettings.GetSpeed(GaitTags::Sprint, Velocity, GetMoverComponent()->GetTargetOrientation());

	MaxSpeedOverride = GaitSettings.GetSpeed(CharacterInputs->Gait, Velocity,
	                                         StartingSyncState->GetOrientation_BaseSpace());
	Acceleration = GaitSettings.GetMovementCurve()
		               ? GaitSettings.GetMovementCurve()->GetVectorValue(GetMappedSpeed()).X
		               : 1000.f;
	Deceleration = GaitSettings.GetMovementCurve()
		               ? GaitSettings.GetMovementCurve()->GetVectorValue(GetMappedSpeed()).Y
		               : 1000.f;

	//TODO: add settings?
	TurningStrength = FMath::GetMappedRangeValueClamped<float, float>({RunSpeed, SprintSpeed},
	                                                                  {WalkRunTurnStrength, SprintTurnStrength},
	                                                                  InOutVelocity.Size2D());

	FRotator DeltaFacing = DesiredFacing.Rotator() - CurrentFacing.Rotator();
	float FacingStrength = FMath::GetMappedRangeValueClamped<float, float>({RunSpeed, SprintSpeed},
	                                                                       {WalkRunFacingTime, SprintFacingTime},
	                                                                       InOutVelocity.Size2D());
	FacingSmoothingTime = CharacterInputs->GetMoveInput().IsZero()
		                      ? IdleFacingTime
		                      : FMath::GetMappedRangeValueClamped<float, float>(
			                      {90.f, 135.f}, {FacingStrength, .2f}, FMath::Abs(DeltaFacing.Yaw));

	Super::GenerateWalkMove_Implementation(StartState, DeltaSeconds, DesiredVelocity, OverridenDesiredFacing,
	                                       CurrentFacing, InOutAngularVelocityDegrees, InOutVelocity);

	if (CurrentOffset >= 135.f)
	{
		InOutAngularVelocityDegrees.Z = FMath::Clamp(InOutAngularVelocityDegrees.Z, -1000.f,
		                                             CharacterInputs->ControlRotationRate);
	}
	else if (CurrentOffset <= -135.f)
	{
		InOutAngularVelocityDegrees.Z = FMath::Clamp(InOutAngularVelocityDegrees.Z,
		                                             CharacterInputs->ControlRotationRate, 1000.f);
	}
}

void UMovementMode_Walking::OnStanceChanged(const FGameplayTag OldGameplayTag, const FGameplayTag NewGameplayTag)
{
	GaitSettings = MovementSettings.FindRef(NewGameplayTag);
}

void UMovementMode_Walking::Activate()
{
	if (GetMoverComponent()->GetMovementModeName() == DefaultModeNames::Falling)
	{
		bJustLanded = true;
		GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			bJustLanded = false;
		}));
	}

	auto* Character = Cast<AGASPCharacter>(GetMoverComponent()->GetOwner());
	Character->StanceModeChanged.AddDynamic(this, &ThisClass::OnStanceChanged);

	Super::Activate();
}

void UMovementMode_Walking::Deactivate()
{
	Super::Deactivate();

	auto* Character = Cast<AGASPCharacter>(GetMoverComponent()->GetOwner());
	Character->StanceModeChanged.RemoveDynamic(this, &ThisClass::OnStanceChanged);
}

float UMovementMode_Walking::GetMappedSpeed() const
{
	auto MoverComp = GetMoverComponent();
	FVector Velocity = MoverComp->GetVelocity();
	float WalkSpeed = GaitSettings.GetSpeed(GaitTags::Walk, Velocity, MoverComp->GetTargetOrientation());
	float RunSpeed = GaitSettings.GetSpeed(GaitTags::Run, Velocity, MoverComp->GetTargetOrientation());
	float SprintSpeed = GaitSettings.GetSpeed(GaitTags::Sprint, Velocity, MoverComp->GetTargetOrientation());

	const auto Speed{UE_REAL_TO_FLOAT(Velocity.Size2D())};

	if (Speed > RunSpeed)
	{
		return FMath::GetMappedRangeValueClamped<float, float>({RunSpeed, SprintSpeed}, {2.0f, 3.0f}, Speed);
	}
	if (Speed > WalkSpeed)
	{
		return FMath::GetMappedRangeValueClamped<float, float>({WalkSpeed, RunSpeed}, {1.0f, 2.0f}, Speed);
	}

	return FMath::GetMappedRangeValueClamped<float, float>({0.0f, WalkSpeed}, {0.0f, 1.0f}, Speed);
}
