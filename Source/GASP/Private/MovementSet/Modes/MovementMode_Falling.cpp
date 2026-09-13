#include "MovementSet/Modes/MovementMode_Falling.h"
#include "Engine/World.h"
#include "MoveLibrary/MovementUtils.h"
#include "Types/MovementTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MovementMode_Falling)

UMovementMode_Falling::UMovementMode_Falling(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UMovementMode_Falling::GenerateMove_Implementation(const FMoverSimContext& SimContext,
                                                        const FMoverTickStartData& StartState,
                                                        const FMoverTimeStep& TimeStep,
                                                        FProposedMove& OutProposedMove) const
{
	const auto* CharacterInputs = StartState.InputCmd.InputCollection.FindDataByType<FGASPMoverInputs>();
	const auto* StartingSyncState = StartState.SyncState.SyncStateCollection.FindDataByType<FMoverDefaultSyncState>();

	Super::GenerateMove_Implementation(SimContext, StartState, TimeStep, OutProposedMove);

	// ensure rather than check: check() compiles out in Shipping and the dereference below would
	// still run. A missing default sync state means the base move is the whole result.
	if (!ensure(StartingSyncState))
	{
		return;
	}

	if (CharacterInputs)
	{
		// The simulation step, not the render frame delta: under the Independent ticking policy the
		// two differ, and a resimulated tick must reproduce the same angular velocity it did the
		// first time round.
		const float DeltaSeconds = TimeStep.StepMs * 0.001f;

		auto OrientationRotation{CharacterInputs->OrientationIntent.ToOrientationRotator()};
		OrientationRotation.Yaw += CharacterInputs->RotationOffset;
		OutProposedMove.AngularVelocityDegrees = UMovementUtils::ComputeAngularVelocityDegrees(
			StartingSyncState->GetOrientation_BaseSpace(), OrientationRotation, DeltaSeconds, 300.f);
	}
}
