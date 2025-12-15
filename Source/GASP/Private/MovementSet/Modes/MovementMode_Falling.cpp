#include "MovementSet/Modes/MovementMode_Falling.h"
#include "MoveLibrary/MovementUtils.h"
#include "Types/MovementTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MovementMode_Falling)

void UMovementMode_Falling::GenerateMove_Implementation(const FMoverTickStartData& StartState,
                                                        const FMoverTimeStep& TimeStep,
                                                        FProposedMove& OutProposedMove) const
{
	const auto* CharacterInputs = StartState.InputCmd.InputCollection.FindDataByType<FGASPMoverInputs>();
	const auto* StartingSyncState = StartState.SyncState.SyncStateCollection.FindDataByType<FMoverDefaultSyncState>();
	check(StartingSyncState);

	Super::GenerateMove_Implementation(StartState, TimeStep, OutProposedMove);

	if (CharacterInputs)
	{
		FRotator OrientationRotation{CharacterInputs->OrientationIntent.ToOrientationRotator()};
		OrientationRotation.Yaw += CharacterInputs->RotationOffset;
		OutProposedMove.AngularVelocityDegrees = UMovementUtils::ComputeAngularVelocityDegrees(
			StartingSyncState->GetOrientation_BaseSpace(), OrientationRotation, GetWorld()->GetDeltaSeconds(), 300.f);
	}
}
