#include "Utils/GASPPhysicsControlLibrary.h"

#include "PBDRigidsSolver.h"
#include "PhysicsControlHelpers.h"
#include "PhysicsControlLog.h"
#include "PhysicsProxy/SingleParticlePhysicsProxy.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPPhysicsControlLibrary)

//======================================================================================================================
// True when the actor handle wraps a rigid (dynamic or kinematic) Chaos particle. Static
// particles are not supported by FIgnoreCollisionManager and pair-ignore is a silent no-op for
// them.
static bool IsRigidParticle(const FPhysicsActorHandle Handle)
{
	return Handle->GetParticle_LowLevel() != nullptr
		&& Handle->GetParticle_LowLevel()->CastToRigidParticle() != nullptr;
}

//======================================================================================================================
// Resolves both (component, bone) pairs to FPhysicsActorHandles, validating that each is
// non-null, dynamic-or-kinematic, and that both live in the same solver. Logs a warning and
// returns false on any failure.
static bool ResolveCollisionPair(
	UPrimitiveComponent* FirstComponent,  const FName FirstBoneName,
	UPrimitiveComponent* SecondComponent, const FName SecondBoneName,
	FPhysicsActorHandle& OutFirstHandle,  FPhysicsActorHandle& OutSecondHandle)
{
	FBodyInstance* const FirstBody  = UE::PhysicsControl::GetBodyInstance(FirstComponent,  FirstBoneName);
	FBodyInstance* const SecondBody = UE::PhysicsControl::GetBodyInstance(SecondComponent, SecondBoneName);
	if (!FirstBody || !SecondBody)
	{
		UE_LOGF(LogPhysicsControl, Warning,
			"CollisionBetweenBodies: could not resolve a body instance for one or both of (%ls / %ls)",
			*FirstBoneName.ToString(), *SecondBoneName.ToString());
		return false;
	}

	OutFirstHandle  = FirstBody->GetPhysicsActor();
	OutSecondHandle = SecondBody->GetPhysicsActor();
	if (!OutFirstHandle || !OutSecondHandle)
	{
		UE_LOGF(LogPhysicsControl, Warning,
			"CollisionBetweenBodies: body instance(s) not initialised in the physics scene (%ls / %ls)",
			*FirstBoneName.ToString(), *SecondBoneName.ToString());
		return false;
	}

	Chaos::FPhysicsSolver* const SolverA = OutFirstHandle->GetSolver<Chaos::FPhysicsSolver>();
	Chaos::FPhysicsSolver* const SolverB = OutSecondHandle->GetSolver<Chaos::FPhysicsSolver>();
	if (!SolverA || SolverA != SolverB)
	{
		UE_LOGF(LogPhysicsControl, Warning,
			"CollisionBetweenBodies: bodies live in invalid/different physics solvers (%ls / %ls)",
			*FirstBoneName.ToString(), *SecondBoneName.ToString());
		return false;
	}

	// FIgnoreCollisionManager silently drops non-rigid (static) particles. Surface as a warning
	// rather than a silent no-op so callers can tell why it didn't take effect.
	if (!IsRigidParticle(OutFirstHandle) || !IsRigidParticle(OutSecondHandle))
	{
		UE_LOGF(LogPhysicsControl, Warning,
			"CollisionBetweenBodies: one or both bodies are static; pair-ignore not supported (%ls / %ls)",
			*FirstBoneName.ToString(), *SecondBoneName.ToString());
		return false;
	}

	return true;
}

//======================================================================================================================
bool UGASPPhysicsControlLibrary::DisableCollisionBetweenBodyArrays(
	UPrimitiveComponent* FirstComponent,  const TArray<FName>& FirstBoneNames,
	UPrimitiveComponent* SecondComponent, const TArray<FName>& SecondBoneNames)
{
	// An empty bone array (or a single entry with name None) means "use the component's single
	// body" - e.g. for a static-mesh component, which has no per-bone bodies. Substitute a single
	// NAME_None so the loop can still work.
	const FName UnnamedFallback = NAME_None;
	const TArrayView<const FName> FirstNames  =
		FirstBoneNames.IsEmpty()  ? MakeArrayView(&UnnamedFallback, 1) : MakeArrayView(FirstBoneNames);
	const TArrayView<const FName> SecondNames =
		SecondBoneNames.IsEmpty() ? MakeArrayView(&UnnamedFallback, 1) : MakeArrayView(SecondBoneNames);

	bool bAllOk = true;
	TArray<TPair<FPhysicsActorHandle, FPhysicsActorHandle>> Pairs;
	Pairs.Reserve(FirstNames.Num() * SecondNames.Num());
	for (const FName& FirstBoneName : FirstNames)
	{
		for (const FName& SecondBoneName : SecondNames)
		{
			FPhysicsActorHandle HandleA, HandleB;
			if (!ResolveCollisionPair(FirstComponent, FirstBoneName, SecondComponent, SecondBoneName, HandleA, HandleB))
			{
				bAllOk = false;
				continue;
			}
			// Same body resolves on both sides - ignoring a body against itself is meaningless.
			if (HandleA == HandleB)
			{
				continue;
			}
			Pairs.Emplace(HandleA, HandleB);
		}
	}
	if (Pairs.IsEmpty())
	{
		return bAllOk;
	}

	// One EnqueueCommandImmediate for the whole batch instead of multiple physics-thread hops.
	// AddIgnoreCollisions is symmetric, so each pair only needs a single A->B call. See
	// DisableCollisionBetweenBodies for why we bypass FChaosEngineInterface:: AddDisabledCollisionsFor_AssumesLocked.
	Chaos::FPhysicsSolver* const Solver = Pairs[0].Key->GetSolver<Chaos::FPhysicsSolver>();
	// ResolveCollisionPair confirmed a valid, matching solver for every pair we kept.
	Solver->EnqueueCommandImmediate([Pairs = MoveTemp(Pairs)]()
	{
		for (const TPair<FPhysicsActorHandle, FPhysicsActorHandle>& Pair : Pairs)
		{
			Chaos::FGeometryParticleHandle* const HA = Pair.Key->GetHandle_LowLevel();
			Chaos::FGeometryParticleHandle* const HB = Pair.Value->GetHandle_LowLevel();
			if (!HA || !HB)
			{
				continue;
			}
			if (Chaos::FPhysicsSolver* const PhysicsThreadSolver = Pair.Key->GetSolver<Chaos::FPhysicsSolver>())
			{
				PhysicsThreadSolver->GetEvolution()->GetBroadPhase().GetIgnoreCollisionManager().
					AddIgnoreCollisions(HA, HB);
			}
		}
	});
	return bAllOk;
}

//======================================================================================================================
bool UGASPPhysicsControlLibrary::EnableCollisionBetweenBodyArrays(
	UPrimitiveComponent* FirstComponent,  const TArray<FName>& FirstBoneNames,
	UPrimitiveComponent* SecondComponent, const TArray<FName>& SecondBoneNames)
{
	// An empty bone array (or a single entry with name None) means "use the component's single
	// body" - e.g. for a static-mesh component, which has no per-bone bodies. Substitute a single
	// NAME_None if empty so the loop can still work.
	const FName UnnamedFallback = NAME_None;
	const TArrayView<const FName> FirstNames  = 
		FirstBoneNames.IsEmpty()  ? MakeArrayView(&UnnamedFallback, 1) : MakeArrayView(FirstBoneNames);
	const TArrayView<const FName> SecondNames = 
		SecondBoneNames.IsEmpty() ? MakeArrayView(&UnnamedFallback, 1) : MakeArrayView(SecondBoneNames);

	bool bAllOk = true;
	TArray<TPair<FPhysicsActorHandle, FPhysicsActorHandle>> Pairs;
	Pairs.Reserve(FirstNames.Num() * SecondNames.Num());
	for (const FName& FirstBoneName : FirstNames)
	{
		for (const FName& SecondBoneName : SecondNames)
		{
			FPhysicsActorHandle HandleA, HandleB;
			if (!ResolveCollisionPair(FirstComponent, FirstBoneName, SecondComponent, SecondBoneName, HandleA, HandleB))
			{
				bAllOk = false;
				continue;
			}
			if (HandleA == HandleB)
			{
				continue;
			}
			Pairs.Emplace(HandleA, HandleB);
		}
	}
	if (Pairs.IsEmpty())
	{
		return bAllOk;
	}

	// One EnqueueCommandImmediate for the whole batch instead of multiple physics-thread hops.
	Chaos::FPhysicsSolver* const Solver = Pairs[0].Key->GetSolver<Chaos::FPhysicsSolver>();
	// ResolveCollisionPair confirmed a valid, matching solver for every pair we kept.
	Solver->EnqueueCommandImmediate([Pairs = MoveTemp(Pairs)]()
	{
		for (const TPair<FPhysicsActorHandle, FPhysicsActorHandle>& Pair : Pairs)
		{
			Chaos::FGeometryParticleHandle* const HA = Pair.Key->GetHandle_LowLevel();
			Chaos::FGeometryParticleHandle* const HB = Pair.Value->GetHandle_LowLevel();
			if (!HA || !HB)
			{
				continue;
			}
			if (Chaos::FPhysicsSolver* const PhysicsThreadSolver = Pair.Key->GetSolver<Chaos::FPhysicsSolver>())
			{
				PhysicsThreadSolver->GetEvolution()->GetBroadPhase().GetIgnoreCollisionManager().
					RemoveIgnoreCollisions(HA, HB);
			}
		}
	});
	return bAllOk;
}