#include "MovementSet/Modes/MovementMode_Slide.h"
#include "MoverComponent.h"
#include "Types/MovementTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MovementMode_Slide)

UMovementMode_Slide::UMovementMode_Slide(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UMovementMode_Slide::GenerateWalkMove_Implementation(FMoverTickStartData& StartState, float DeltaSeconds,
                                                          const FVector& DesiredVelocity, const FQuat& DesiredFacing,
                                                          const FQuat& CurrentFacing,
                                                          FVector& InOutAngularVelocityDegrees, FVector& InOutVelocity)
{
	const auto* CharacterInputs = StartState.InputCmd.InputCollection.FindDataByType<FGASPMoverInputs>();
	const auto* StartingSyncState = StartState.SyncState.SyncStateCollection.FindDataByType<FMoverDefaultSyncState>();
	check(StartingSyncState);

	const double CurrentOffset{(CurrentFacing.Rotator() - DesiredFacing.Rotator()).Yaw};

	const float ClampedYawDegrees = FMath::Clamp(CharacterInputs->RotationOffset, CurrentOffset - 179.f,
	                                             CurrentOffset + 179.f);
	const float YawRadians = FMath::DegreesToRadians(ClampedYawDegrees);
	const FQuat OverridenDesiredFacing{DesiredFacing * FQuat{FVector::UpVector, YawRadians}};

	FHitResult HitFloor;
	GetMoverComponent()->TryGetFloorCheckHitResult(HitFloor);

	float SlopeAngle = FMath::RadiansToDegrees(
		FMath::Acos(FVector::DotProduct(HitFloor.ImpactNormal, DesiredVelocity.GetSafeNormal()))) - 90.f;

	if (bInitialBoost)
	{
		MaxSpeedOverride = InitialBoostTime;
	}
	else
	{
		MaxSpeedOverride = SlopeAngle > ShallowSlopeAngle * -1.f
			                   ? FlatGroundSpeed
			                   : FMath::GetMappedRangeValueClamped<float, float>(
				                   {ShallowSlopeAngle * -1.f, SteepSlopeAngle * -1.f},
				                   {ShallowSlopeAngle, SteepSlopeAngle}, SlopeAngle);
	}

	Acceleration = bInitialBoost ? AfterBoostAcceleration : InitialBoostAcceleration;
	Deceleration = FMath::GetMappedRangeValueClamped<float, float>({ShallowSlopeAngle, SteepSlopeAngle},
	                                                               {FlatGroundDeceleration, SteepSlopeDeceleration},
	                                                               SlopeAngle);

	Super::GenerateWalkMove_Implementation(StartState, DeltaSeconds, DesiredVelocity, OverridenDesiredFacing,
	                                       CurrentFacing, InOutAngularVelocityDegrees, InOutVelocity);
}

void UMovementMode_Slide::Activate()
{
	Super::Activate();

	bInitialBoost = true;

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		bInitialBoost = false;
	}), InitialBoostTime, false);
}
