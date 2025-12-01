#include "Actors/GASPCharacter.h"
#include "Animation/GASPAnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/GASPCharacterMovementComponent.h"
#include "Net/Core/PushModel/PushModel.h"

static const FName NAME_pelvis(TEXT("pelvis"));
static const FName NAME_spine_03(TEXT("spine_03"));

UAnimMontage* AGASPCharacter::SelectGetUpMontage(bool bRagdollFacingUpward)
{
	return bRagdollFacingUpward ? GetUpMontageBack : GetUpMontageFront;
}

bool AGASPCharacter::IsRagdollingAllowedToStart() const
{
	return LocomotionAction != LocomotionActionTags::Ragdoll && GetMesh()->GetBodyInstance(NAME_pelvis) != nullptr &&
		GetMesh()->GetBodyInstance(NAME_spine_03) != nullptr && *NAME_pelvis.ToString() && *NAME_spine_03.ToString();
}

void AGASPCharacter::StartRagdolling()
{
	if (GetLocalRole() <= ROLE_SimulatedProxy || !IsRagdollingAllowedToStart())
	{
		return;
	}

	if (GetLocalRole() >= ROLE_Authority)
	{
		MulticastStartRagdolling();
	}
	else
	{
		MovementComponent->FlushServerMoves();

		ServerStartRagdolling();
	}
}

void AGASPCharacter::ServerStartRagdolling_Implementation()
{
	if (IsRagdollingAllowedToStart())
	{
		MulticastStartRagdolling();
		ForceNetUpdate();
	}
}

void AGASPCharacter::MulticastStartRagdolling_Implementation()
{
	StartRagdollingImplementation();
}

void AGASPCharacter::StartRagdollingImplementation()
{
	if (!IsRagdollingAllowedToStart())
	{
		return;
	}

	GetMesh()->bUpdateJointsFromAnimation = true; // Required for the flail animation to work properly.

	if (!GetMesh()->IsRunningParallelEvaluation() && GetMesh()->GetBoneSpaceTransforms().Num() > 0)
	{
		GetMesh()->UpdateRBJointMotors();
	}

	// Stop any active montages.
	static constexpr auto BlendOutDuration{0.2f};

	if (IsValid(GetMesh()->GetAnimInstance()))
	{
		GetMesh()->GetAnimInstance()->Montage_Stop(BlendOutDuration);
	}

	// Disable movement corrections and reset network smoothing.
	MovementComponent->NetworkSmoothingMode = ENetworkSmoothingMode::Disabled;
	MovementComponent->bIgnoreClientMovementErrorChecksAndCorrection = true;

	// Detach the mesh so that character transformation changes will not affect it in any way.
	GetMesh()->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

	// Disable capsule collision and enable mesh physics simulation.
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	GetMesh()->SetCollisionObjectType(ECC_PhysicsBody);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetSimulatePhysics(true);

	const auto* PelvisBody{GetMesh()->GetBodyInstance(NAME_pelvis)};
	FVector PelvisLocation;

	FPhysicsCommand::ExecuteRead(PelvisBody->ActorHandle,
	                             [this, &PelvisLocation](const FPhysicsActorHandle& ActorHandle)
	                             {
		                             PelvisLocation = FPhysicsInterface::GetTransform_AssumesLocked(ActorHandle, true).
			                             GetLocation();
		                             RagdollingState.Velocity = FPhysicsInterface::GetLinearVelocity_AssumesLocked(
			                             ActorHandle);
	                             });

	RagdollingState.PullForce = 0.0f;

	if (bLimitInitialRagdollSpeed)
	{
		// Limit the ragdoll's speed for a few frames, because for some unclear reason,
		// it can get a much higher initial speed than the character's last speed.
		static constexpr auto MinSpeedLimit{200.0f};

		RagdollingState.SpeedLimitFrameTimeRemaining = 8;
		RagdollingState.SpeedLimit = FMath::Max(MinSpeedLimit, UE_REAL_TO_FLOAT(GetVelocity().Size()));

		ConstraintRagdollSpeed();
	}

	if (GetLocalRole() >= ROLE_Authority)
	{
		SetRagdollTargetLocation(FVector::ZeroVector);
	}

	if (IsLocallyControlled() || (GetLocalRole() >= ROLE_Authority && !IsValid(GetController())))
	{
		SetRagdollTargetLocation(PelvisLocation);
	}

	// Clear the character movement mode and set the locomotion action to ragdolling.
	MovementComponent->SetMovementMode(MOVE_None);

	SetLocomotionAction(LocomotionActionTags::Ragdoll);
	OnStartRagdolling();
}

void AGASPCharacter::SetRagdollTargetLocation(const FVector& NewTargetLocation)
{
	if (RagdollTargetLocation != NewTargetLocation)
	{
		RagdollTargetLocation = NewTargetLocation;

		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, RagdollTargetLocation, this)

		if (GetLocalRole() == ROLE_AutonomousProxy)
		{
			ServerSetRagdollTargetLocation(RagdollTargetLocation);
		}
	}
}

void AGASPCharacter::ServerSetRagdollTargetLocation_Implementation(const FVector_NetQuantize& NewTargetLocation)
{
	SetRagdollTargetLocation(NewTargetLocation);
}

void AGASPCharacter::RefreshRagdolling(const float DeltaTime)
{
	// Since we are dealing with physics here, we should not use functions such as USkinnedMeshComponent::GetSocketTransform() as
	// they may return an incorrect result in situations like when the animation blueprint is not ticking or when URO is enabled.
	const auto* PelvisBody{GetMesh()->GetBodyInstance(NAME_pelvis)};
	FVector PelvisLocation;

	FPhysicsCommand::ExecuteRead(PelvisBody->ActorHandle,
	                             [this, &PelvisLocation](const FPhysicsActorHandle& ActorHandle)
	                             {
		                             PelvisLocation = FPhysicsInterface::GetTransform_AssumesLocked(ActorHandle, true).
			                             GetLocation();
		                             RagdollingState.Velocity = FPhysicsInterface::GetLinearVelocity_AssumesLocked(
			                             ActorHandle);
	                             });

	const auto bLocallyControlled{
		IsLocallyControlled() || (GetLocalRole() >= ROLE_Authority && !IsValid(GetController()))
	};

	if (bLocallyControlled)
	{
		SetRagdollTargetLocation(PelvisLocation);
	}

	// Prevent the capsule from going through the ground when the ragdoll is lying on the ground.

	// While we could get rid of the line trace here and just use RagdollTargetLocation
	// as the character's location, we don't do that because the camera depends on the
	// capsule's bottom location, so its removal will cause the camera to behave erratically.
	bool bGrounded;
	SetActorLocation(RagdollTraceGround(bGrounded), false);

	// Zero target location means that it hasn't been replicated yet, so we can't apply the logic below.
	if (!bLocallyControlled && !RagdollTargetLocation.IsZero())
	{
		// Apply ragdoll location corrections.
		static constexpr auto PullForce{750.0f};
		static constexpr auto InterpolationSpeed{0.6f};

		RagdollingState.PullForce = FMath::FInterpTo(RagdollingState.PullForce, PullForce, DeltaTime,
		                                             InterpolationSpeed);

		const auto HorizontalSpeedSquared{RagdollingState.Velocity.SizeSquared2D()};

		const auto PullForceBoneName{
			HorizontalSpeedSquared > FMath::Square(300.0f) ? NAME_spine_03 : NAME_pelvis
		};

		auto* PullForceBody{GetMesh()->GetBodyInstance(PullForceBoneName)};

		FPhysicsCommand::ExecuteWrite(PullForceBody->ActorHandle, [this](const FPhysicsActorHandle& ActorHandle)
		{
			if (!FPhysicsInterface::IsRigidBody(ActorHandle))
			{
				return;
			}

			const auto PullForceVector{
				RagdollTargetLocation - FPhysicsInterface::GetTransform_AssumesLocked(ActorHandle, true).GetLocation()
			};

			static constexpr auto MinPullForceDistance{5.0f};
			static constexpr auto MaxPullForceDistance{50.0f};

			if (PullForceVector.SizeSquared() > FMath::Square(MinPullForceDistance))
			{
				FPhysicsInterface::AddForce_AssumesLocked(
					ActorHandle, PullForceVector.GetClampedToMaxSize(MaxPullForceDistance) * RagdollingState.PullForce,
					true, true);
			}
		});
	}

	// Use the speed to scale ragdoll joint strength for physical animation.
	static constexpr auto ReferenceSpeed{1000.0f};
	static constexpr auto Stiffness{25000.0f};

	const auto SpeedAmount{FMath::Clamp(UE_REAL_TO_FLOAT(RagdollingState.Velocity.Size() / ReferenceSpeed), 0.f, 1.f)};

	GetMesh()->SetAllMotorsAngularDriveParams(SpeedAmount * Stiffness, 0.0f, 0.0f);

	// Limit the speed of ragdoll bodies.
	if (RagdollingState.SpeedLimitFrameTimeRemaining > 0)
	{
		RagdollingState.SpeedLimitFrameTimeRemaining -= 1;

		ConstraintRagdollSpeed();
	}
}

FVector AGASPCharacter::RagdollTraceGround(bool& bGrounded) const
{
	auto RagdollLocation{!RagdollTargetLocation.IsZero() ? FVector{RagdollTargetLocation} : GetActorLocation()};

	// We use a sphere sweep instead of a simple line trace to keep capsule
	// movement consistent between ragdolling and regular character movement.
	const auto CapsuleRadius{GetCapsuleComponent()->GetScaledCapsuleRadius()};
	const auto CapsuleHalfHeight{GetCapsuleComponent()->GetScaledCapsuleHalfHeight()};

	const FVector TraceStart{RagdollLocation.X, RagdollLocation.Y, RagdollLocation.Z + 2.0f * CapsuleRadius};
	const FVector TraceEnd{RagdollLocation.X, RagdollLocation.Y, RagdollLocation.Z - CapsuleHalfHeight + CapsuleRadius};

	const auto CollisionChannel{MovementComponent->UpdatedComponent->GetCollisionObjectType()};

	FCollisionQueryParams QueryParameters{__FUNCTION__, false, this};
	FCollisionResponseParams CollisionResponses;
	MovementComponent->InitCollisionParams(QueryParameters, CollisionResponses);

	FHitResult Hit;
	bGrounded = GetWorld()->SweepSingleByChannel(Hit, TraceStart, TraceEnd, FQuat::Identity,
	                                             CollisionChannel, FCollisionShape::MakeSphere(CapsuleRadius),
	                                             QueryParameters, CollisionResponses);

	return FVector{
		RagdollLocation.X, RagdollLocation.Y,
		bGrounded
			? Hit.Location.Z + CapsuleHalfHeight - CapsuleRadius + UCharacterMovementComponent::MIN_FLOOR_DIST
			: RagdollLocation.Z
	};
}

void AGASPCharacter::ConstraintRagdollSpeed() const
{
	GetMesh()->ForEachBodyBelow(NAME_None, true, false, [this](FBodyInstance* Body)
	{
		FPhysicsCommand::ExecuteWrite(Body->ActorHandle, [this](const FPhysicsActorHandle& ActorHandle)
		{
			if (!FPhysicsInterface::IsRigidBody(ActorHandle))
			{
				return;
			}

			auto Velocity{FPhysicsInterface::GetLinearVelocity_AssumesLocked(ActorHandle)};
			if (Velocity.SizeSquared() <= FMath::Square(RagdollingState.SpeedLimit))
			{
				return;
			}

			Velocity.Normalize();
			Velocity *= RagdollingState.SpeedLimit;

			FPhysicsInterface::SetLinearVelocity_AssumesLocked(ActorHandle, Velocity);
		});
	});
}

bool AGASPCharacter::IsRagdollingAllowedToStop() const
{
	return LocomotionAction == LocomotionActionTags::Ragdoll && GetMesh()->GetBodyInstance(NAME_pelvis) != nullptr &&
		GetMesh()->GetBodyInstance(NAME_spine_03) != nullptr && *NAME_pelvis.ToString() && *NAME_spine_03.ToString();
}

bool AGASPCharacter::StopRagdolling()
{
	if (GetLocalRole() <= ROLE_SimulatedProxy || !IsRagdollingAllowedToStop())
	{
		return false;
	}

	if (GetLocalRole() >= ROLE_Authority)
	{
		MulticastStopRagdolling();
	}
	else
	{
		ServerStopRagdolling();
	}

	return true;
}

void AGASPCharacter::ServerStopRagdolling_Implementation()
{
	if (IsRagdollingAllowedToStop())
	{
		MulticastStopRagdolling();
		ForceNetUpdate();
	}
}

void AGASPCharacter::MulticastStopRagdolling_Implementation()
{
	StopRagdollingImplementation();
}

void AGASPCharacter::StopRagdollingImplementation()
{
	if (!IsRagdollingAllowedToStop())
	{
		return;
	}

	UGASPAnimInstance* AnimationInstance = {Cast<UGASPAnimInstance>(GetMesh()->GetAnimInstance())};
	auto& FinalRagdollPose{AnimationInstance->SnapshotFinalRagdollPose()};

	const auto PelvisTransform{GetMesh()->GetSocketTransform(NAME_pelvis)};
	const auto PelvisRotation{PelvisTransform.Rotator()};

	// Disable mesh physics simulation and enable capsule collision.
	GetMesh()->bUpdateJointsFromAnimation = false;

	GetMesh()->SetSimulatePhysics(false);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GetMesh()->SetCollisionObjectType(ECC_Pawn);

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	MovementComponent->NetworkSmoothingMode = ENetworkSmoothingMode::Exponential;
	MovementComponent->bIgnoreClientMovementErrorChecksAndCorrection = false;

	bool bGrounded;
	const auto NewActorLocation{RagdollTraceGround(bGrounded)};

	// Determine whether the ragdoll is facing upward or downward and set the actor rotation accordingly.

	const auto bRagdollFacingUpward{FMath::UnwindDegrees(PelvisRotation.Roll) <= 0.0f};

	auto NewActorRotation{GetActorRotation()};
	NewActorRotation.Yaw = bRagdollFacingUpward ? PelvisRotation.Yaw - 180.0f : PelvisRotation.Yaw;

	SetActorLocationAndRotation(NewActorLocation, NewActorRotation, false, nullptr, ETeleportType::TeleportPhysics);

	// Attach the mesh back and restore its default relative location.
	const auto& ActorTransform{GetActorTransform()};

	GetMesh()->SetWorldLocationAndRotationNoPhysics(ActorTransform.TransformPositionNoScale(GetBaseTranslationOffset()),
	                                                ActorTransform.TransformRotation(
		                                                GetBaseRotationOffset()).Rotator());

	GetMesh()->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepWorldTransform);

	if (GetMesh()->ShouldUseUpdateRateOptimizations())
	{
		// Disable URO for one frame to force the animation blueprint to update and get rid of the incorrect mesh pose.
		GetMesh()->bEnableUpdateRateOptimizations = false;

		GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]
		{
			GetMesh()->bEnableUpdateRateOptimizations = true;
		}));
	}

	// Restore the pelvis transform to the state it was in before we changed
	// the character and mesh transforms to keep its world transform unchanged.

	const auto& ReferenceSkeleton{GetMesh()->GetSkinnedAsset()->GetRefSkeleton()};

	const auto PelvisBoneIndex{ReferenceSkeleton.FindBoneIndex(NAME_pelvis)};
	if (PelvisBoneIndex >= 0)
	{
		// We expect the pelvis bone to be the root bone or attached to it, so we can safely use the mesh transform here.
		FinalRagdollPose.LocalTransforms[PelvisBoneIndex] = PelvisTransform.GetRelativeTransform(
			GetMesh()->GetComponentTransform());
	}

	// If the ragdoll is on the ground, set the movement mode to walking and play a get up montage. If not, set
	// the movement mode to falling and update the character movement velocity to match the last ragdoll velocity.

	if (bGrounded)
	{
		MovementComponent->SetMovementMode(MOVE_Walking);
		AnimationInstance->Montage_Play(SelectGetUpMontage(bRagdollFacingUpward));
	}
	else
	{
		MovementComponent->SetMovementMode(MOVE_Falling);
		MovementComponent->Velocity = RagdollingState.Velocity;
	}

	SetLocomotionAction(FGameplayTag::EmptyTag);
	OnStopRagdolling();
}
