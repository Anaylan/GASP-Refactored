#include "MovementSet/Modes/MovementMode_Ragdolling.h"

#include "DrawDebugHelpers.h"
#include "MoveLibrary/FloorQueryUtils.h"
#include "MoveLibrary/GroundMovementUtils.h"
#include "MoveLibrary/MovementUtils.h"
#include "MovementSet/GASPMoverComponent.h"
#include "MoverComponent.h"
#include "DefaultMovementSet/Settings/CommonLegacyMovementSettings.h"
#include "MovementSet/Settings/RagdollSettings.h"
#include "Types/MovementTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MovementMode_Ragdolling)

namespace MovementModeNames
{
	const FName Ragdolling{TEXT("Ragdolling")};
}

namespace
{
	// Largest distance capsule is considered resting on floor in ground-based movement (matches UE::FloorQueryUtility::MAX_FLOOR_DIST)
	constexpr float MAX_FLOOR_DIST = 2.4f;
}

#if ENABLE_DRAW_DEBUG
namespace RagdollDebugVar
{
	bool bDrawDebugRagdoll{false};
	FAutoConsoleVariableRef CVarDrawDebugRagdoll(
		TEXT("gasp.ragdoll.drawdebug"), bDrawDebugRagdoll,
		TEXT("Draw the ragdoll pelvis target coordinate system"), ECVF_Default);

	float DrawDebugDuration{0.f};
	FAutoConsoleVariableRef CVarDrawDebugDuration(
		TEXT("gasp.ragdoll.drawdebugduration"), DrawDebugDuration,
		TEXT("Lifetime in seconds of ragdoll debug shapes; 0 draws for one frame"), ECVF_Default);
}
#endif

UMovementMode_Ragdolling::UMovementMode_Ragdolling(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GameplayTags.AddTag(MovementModeTags::Ragdoll);

	SharedSettingsClasses.Add(UCommonLegacyMovementSettings::StaticClass());
	SharedSettingsClasses.Add(URagdollSettings::StaticClass());
}

void UMovementMode_Ragdolling::GenerateMove_Implementation(const FMoverSimContext& SimContext,
                                                           const FMoverTickStartData& StartState,
                                                           const FMoverTimeStep& TimeStep,
                                                           FProposedMove& OutProposedMove) const
{
	const auto* MoverComp{GetMoverComponent()};
	const auto* StartingSyncState{StartState.SyncState.SyncStateCollection.FindDataByType<FMoverDefaultSyncState>()};

	// ensure rather than check: check() compiles out in Shipping, leaving the dereferences below
	// unguarded. Without a mover or a sync state the proposed move stays at its zeroed default.
	if (!MoverComp || !ensure(StartingSyncState))
	{
		return;
	}

	const auto* CharacterInputs{StartState.InputCmd.InputCollection.FindDataByType<FGASPMoverInputs>()};

	FTransform TargetTransform{
		StartingSyncState->GetOrientation_WorldSpace(),
		StartingSyncState->GetLocation_WorldSpace()
	};

	if (CharacterInputs && !CharacterInputs->RagdollTransform.GetLocation().IsNearlyZero())
	{
		TargetTransform = CharacterInputs->RagdollTransform;
	}

	const auto TargetLocation{TargetTransform.GetLocation()};

#if ENABLE_DRAW_DEBUG
	if (RagdollDebugVar::bDrawDebugRagdoll)
	{
		DrawDebugCoordinateSystem(GetWorld(), TargetLocation, TargetTransform.Rotator(), 150.0f,
		                          /*bPersistentLines*/ false, RagdollDebugVar::DrawDebugDuration);
	}
#endif

	const float DeltaSeconds = TimeStep.StepMs * 0.001f;

	const auto CurrentLocation{StartingSyncState->GetLocation_WorldSpace()};
	const auto CurrentRotation{StartingSyncState->GetOrientation_WorldSpace()};

	FVector Velocity{ForceInit};
	const auto Diff{TargetLocation - CurrentLocation};

	FFloorCheckResult LastFloorResult;
	auto* SimBlackboard{MoverComp->GetSimBlackboard_Mutable()};
	const bool bIsTouchingSurface = SimBlackboard &&
		SimBlackboard->TryGet(CommonBlackboard::LastFloorResult, LastFloorResult) &&
		(LastFloorResult.HitResult.bStartPenetrating ||
			(LastFloorResult.bBlockingHit && LastFloorResult.GetDistanceToFloor() <= MAX_FLOOR_DIST));

	if (bIsTouchingSurface)
	{
		if (const auto Len = Diff.Size2D(); DeltaSeconds > 0 && Len > 0.1f)
		{
			const auto Dir{Diff.GetSafeNormal2D()};
			Velocity = (Dir * (Len / DeltaSeconds)).GetClampedToMaxSize(MaxSpeed);
		}
	}
	else
	{
		if (DeltaSeconds > 0 && Diff.Size() > 0.1f)
		{
			Velocity = (Diff / DeltaSeconds).GetClampedToMaxSize(MaxSpeed);
		}
	}

	const auto TargetDirection{TargetTransform.GetRotation().RotateVector(FVector::RightVector)};

	const auto RDiff{(TargetDirection.GetSafeNormal2D().Rotation() - CurrentRotation).GetNormalized()};
	FVector AngularVelocityDegrees{ForceInit};
	if (DeltaSeconds > 0 && !RDiff.IsNearlyZero(1.0))
	{
		AngularVelocityDegrees.Y = FMath::Clamp((RDiff * (1.0f / DeltaSeconds)).Yaw, -360.0f * 2.0f, 360.0f * 2.0f);
	}

	OutProposedMove.LinearVelocity = Velocity;
	OutProposedMove.AngularVelocityDegrees = AngularVelocityDegrees;
}

void UMovementMode_Ragdolling::SimulationTick_Implementation(const FSimulationTickParams& Params,
                                                             FMoverTickEndData& OutputState)
{
	auto* MoverComp{GetMoverComponent()};
	const auto& StartState{Params.StartState};
	auto* UpdatedComponent{Params.MovingComps.UpdatedComponent.Get()};
	auto ProposedMove{Params.ProposedMove};

	if (!UpdatedComponent || !MoverComp)
	{
		return;
	}

	const auto* StartingSyncState{StartState.SyncState.SyncStateCollection.FindDataByType<FMoverDefaultSyncState>()};

	// ensure rather than check, for the same reason as in GenerateMove: the dereferences below
	// would otherwise run unguarded in Shipping. Skipping the tick leaves the actor where it is.
	if (!ensure(StartingSyncState))
	{
		return;
	}

	auto& OutputSyncState{
		OutputState.SyncState.SyncStateCollection.FindOrAddMutableDataByType<FMoverDefaultSyncState>()
	};

	const float DeltaSeconds{Params.TimeStep.StepMs * 0.001f};

	FMovementRecord MoveRecord;
	MoveRecord.SetDeltaSeconds(DeltaSeconds);

	auto* SimBlackboard{MoverComp->GetSimBlackboard_Mutable()};
	if (SimBlackboard)
	{
		SimBlackboard->Invalidate(CommonBlackboard::LastFoundDynamicMovementBase);
	}

	OutputSyncState.MoveDirectionIntent = ProposedMove.bHasDirIntent
		                                      ? ProposedMove.DirectionIntent
		                                      : FVector::ZeroVector;

	// Use the orientation intent directly. If no intent is provided, use last frame's orientation. Note that we are assuming rotation changes can't fail. 
	const auto StartingOrient{StartingSyncState->GetOrientation_WorldSpace()};

	const auto TargetOrient{
		UMovementUtils::ApplyAngularVelocityToRotator(StartingOrient, ProposedMove.AngularVelocityDegrees, DeltaSeconds)
	};
	const bool bIsOrientationChanging{!StartingOrient.Equals(TargetOrient)};

	auto MoveDelta{ProposedMove.LinearVelocity * DeltaSeconds};

	auto TargetOrientQuat{TargetOrient.Quaternion()};
	if (CommonLegacySettings->bShouldRemainVertical)
	{
		TargetOrientQuat = FRotationMatrix::MakeFromZX(MoverComp->GetUpDirection(), TargetOrientQuat.GetForwardVector())
			.ToQuat();
	}

	FHitResult Hit(1.f);

	if (!MoveDelta.IsNearlyZero() || bIsOrientationChanging)
	{
		UMovementUtils::TrySafeMoveUpdatedComponent(Params.MovingComps, MoveDelta, TargetOrientQuat, true, Hit,
		                                            ETeleportType::None, MoveRecord);
	}

	if (Hit.IsValidBlockingHit())
	{
		FMoverOnImpactParams ImpactParams(DefaultModeNames::Flying, Hit, MoveDelta);
		MoverComp->HandleImpact(ImpactParams);

		// Try to slide the remaining distance along the surface.
		UMovementUtils::TryMoveToSlideAlongSurface(Params.MovingComps, MoveDelta, 1.f - Hit.Time,
		                                           TargetOrientQuat, Hit.Normal, Hit, true, MoveRecord);
	}

	// If we are very close to a walkable surface, make sure we maintain a small gap over it
	FFloorCheckResult FloorUnderActor;

	FFloorCheckSettings FloorCheckSettings{
		CommonLegacySettings->FloorSweepDistance, CommonLegacySettings->MaxWalkSlopeCosine,
		CommonLegacySettings->bUseFlatBaseForFloorChecks
	};
	UFloorQueryUtils::FindFloor(Params.MovingComps, FloorCheckSettings, UpdatedComponent->GetComponentLocation(),
	                            OUT FloorUnderActor);

	if (FloorUnderActor.IsWalkableFloor())
	{
		UGroundMovementUtils::TryMoveToAdjustHeightAboveFloor(MoverComp, FloorUnderActor,
		                                                      CommonLegacySettings->MaxWalkSlopeCosine,
		                                                      MoveRecord);
	}

	if (SimBlackboard)
	{
		SimBlackboard->Set(CommonBlackboard::LastFloorResult, FloorUnderActor);
	}

	CaptureFinalState(UpdatedComponent, MoveRecord, *StartingSyncState, ProposedMove.AngularVelocityDegrees,
	                  OutputSyncState, OutputState, DeltaSeconds);
}

void UMovementMode_Ragdolling::OnRegistered(const FName ModeName, const FMoverSimContext& SimContext)
{
	Super::OnRegistered(ModeName, SimContext);

	if (const auto* MoverComp{GetMoverComponent()})
	{
		CommonLegacySettings = MoverComp->FindSharedSettings<UCommonLegacyMovementSettings>();
	}
}

void UMovementMode_Ragdolling::OnUnregistered(const FMoverSimContext& SimContext)
{
	Super::OnUnregistered(SimContext);

	CommonLegacySettings = nullptr;
}

void UMovementMode_Ragdolling::CaptureFinalState(USceneComponent* UpdatedComponent, FMovementRecord& Record,
                                                 const FMoverDefaultSyncState& StartSyncState,
                                                 const FVector& AngularVelocityDegrees,
                                                 FMoverDefaultSyncState& OutputSyncState,
                                                 FMoverTickEndData& TickEndData,
                                                 const float DeltaSeconds) const
{
	const FVector FinalLocation = UpdatedComponent->GetComponentLocation();
	const FVector FinalVelocity = Record.GetRelevantVelocity();

	OutputSyncState.SetTransforms_WorldSpace(FinalLocation,
	                                         UpdatedComponent->GetComponentRotation(),
	                                         FinalVelocity,
	                                         AngularVelocityDegrees,
	                                         nullptr); // no movement base

	UpdatedComponent->ComponentVelocity = FinalVelocity;
}
