#pragma once

#include "Types/StructTypes.h"

/**
 * Stateless ragdoll math. Every function is a pure transform over FRagdollingState plus
 * explicit inputs - no UObject, no world, no component access - so it can be exercised
 * headlessly and reused by any driver (task, component, movement mode).
 *
 * The driver keeps ownership of everything that touches the engine: traces, torque
 * application, physics-control writes and replication dirtying.
 */
struct GASP_API FGASPRagdollLibrary
{
	/** Squared 2D center-of-mass speed above which the get-up should roll instead of stand. */
	static constexpr float RollingGetUpSpeedSqThreshold{22500.0f};

	/** Downward center-of-mass speed below which gravity is cancelled to soften the impact. */
	static constexpr float FreeFallGravityCutoffZ{-2500.0f};

	/** Torque magnitude applied along the roll axis, in degrees-based units. */
	static constexpr float AutoRollTorqueScale{100000.0f};

	/** Floor normal range, as slope angles, over which auto-roll fades in. */
	static constexpr float FloorAngleMinDegrees{20.0f};
	static constexpr float FloorAngleMaxDegrees{30.0f};

	/** Constant interp speeds for releasing versus building auto-roll force. */
	static constexpr float AutoRollReleaseSpeed{0.5f};
	static constexpr float AutoRollBuildSpeed{2.0f};

	/** Integrates center-of-mass position/velocity and the spine-derived speed. */
	static void IntegrateBodyState(FRagdollingState& State, const FVector& CurrentCenterOfMass,
	                               const FVector& SpineVelocity, float DeltaTime);

	/** Converts a world-space predicted impact point into a local (yaw, pitch) pair in degrees. */
	static FVector2D ComputeImpactDirection(const FTransform& TraceOrigin, const FVector& ImpactLocation);

	/** Advances AutoRollForce/RollForce toward the target implied by FloorNormal. */
	static void AdvanceRollForces(FRagdollingState& State, const FVector& FloorNormal, float DeltaTime);

	/** Torque vector for the current RollForce - analytical cross product with world up. */
	static FVector ComputeAutoRollTorque(const FRagdollingState& State);

	/** 0 while the body is in fast free-fall, 1 otherwise. */
	static float ComputeGravityMultiplier(const FRagdollingState& State);

	/** True when the get-up chooser should be asked for a rolling variant. */
	static bool ShouldRollingGetUp(const FRagdollingState& State);
};
