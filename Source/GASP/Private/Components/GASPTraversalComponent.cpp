#include "Components/GASPTraversalComponent.h"
#include "Actors/GASPCharacter.h"
#include "AnimationWarpingLibrary.h"
#include "Animation/GASPAnimInstance.h"
#include "ChooserFunctionLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Interfaces/GASPInteractionTransformInterface.h"
#include "IObjectChooser.h"
#include "MotionWarpingComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/AssetManager.h"
#include "MovementSet/GASPMoverComponent.h"
#include "PoseSearch/PoseSearchLibrary.h"
#include "PoseSearch/PoseSearchResult.h"
#include "MoveLibrary/PlayMoverMontageCallbackProxy.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPTraversalComponent)

#if WITH_EDITOR
namespace TraversalVar
{
	int32 DrawDebugLevel{0};
	FAutoConsoleVariableRef DrawDebugLevelStruct(
		TEXT("gasp.traversal.DrawDebugLevel"), DrawDebugLevel,
		TEXT("debug level for traversal"), ECVF_Default);

	float DrawDebugDuration{0.f};
	FAutoConsoleVariableRef DrawDebugDurationStruct(
		TEXT("gasp.traversal.DrawDebugDuration"), DrawDebugDuration,
		TEXT("debug duration for traversal"), ECVF_Default);
}


#endif

namespace
{
	const FName NAME_FrontLedge{TEXT("FrontLedge")};
	const FName NAME_BackLedge{TEXT("BackLedge")};
	const FName NAME_BackFloor{TEXT("BackFloor")};
	const FName NAME_DistanceFromLedge{TEXT("Distance_From_Ledge")};
	const FName NAME_PoseHistory{TEXT("PoseHistory")};
}

// Sets default values for this component's properties
UGASPTraversalComponent::UGASPTraversalComponent()
{
	SetIsReplicatedByDefault(true);
}

void UGASPTraversalComponent::BeginPlay()
{
	Super::BeginPlay();

	CharacterOwner = Cast<AGASPCharacter>(GetOwner());
	if (!CharacterOwner.IsValid())
	{
		return;
	}

	MoverComponent = CharacterOwner->GetMoverComponent();
	MotionWarpingComponent = CharacterOwner->FindComponentByClass<UMotionWarpingComponent>();
	CapsuleComponent = CharacterOwner->GetCapsuleComponent();
	MeshComponent = CharacterOwner->GetMesh();
	if (MeshComponent.IsValid())
	{
		AnimInstance = Cast<UGASPAnimInstance>(MeshComponent->GetAnimInstance());
	}
}

void UGASPTraversalComponent::InitializeComponent()
{
	Super::InitializeComponent();

	StreamableHandle = StreamableManager.RequestAsyncLoad(TraversalAnimationsChooserTable.ToSoftObjectPath(),
	                                                      FStreamableDelegate::CreateLambda([this]()
	                                                      {
		                                                      StreamableHandle.Reset();
	                                                      }),
	                                                      FStreamableManager::DefaultAsyncLoadPriority, false);
}

void UGASPTraversalComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params;
	Params.bIsPushBased = true;
	Params.Condition = COND_SimulatedOnly;
	Params.RepNotifyCondition = REPNOTIFY_OnChanged;

	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, TraversalCheckResult, Params);
}

void UGASPTraversalComponent::ExtractWarpTargetCurveValue(const FName CurveName, const FName WarpTarget,
                                                          float& Value) const
{
	TArray<FMotionWarpingWindowData> Montages;
	UMotionWarpingUtilities::GetMotionWarpingWindowsForWarpTargetFromAnimation(
		TraversalCheckResult.ChosenMontage, WarpTarget, Montages);

	if (!Montages.IsEmpty())
	{
		UAnimationWarpingLibrary::GetCurveValueFromAnimation(TraversalCheckResult.ChosenMontage, CurveName,
		                                                     Montages[0].EndTime, Value);
		return;
	}
	MotionWarpingComponent->RemoveWarpTarget(WarpTarget);
}

void UGASPTraversalComponent::UpdateWarpTargets()
{
	if (!MotionWarpingComponent.IsValid())
	{
		return;
	}

	// Add front ledge target
	MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(
		NAME_FrontLedge, TraversalCheckResult.FrontLedgeLocation + FVector(0.f, 0.f, .5f),
		FRotationMatrix::MakeFromX(-TraversalCheckResult.FrontLedgeNormal).Rotator()
	);

	// Process hurdle or vault
	float DistanceFromFrontLedgeToBackLedge{0.f};
	if (TraversalCheckResult.ActionType == LocomotionActionTags::Hurdle ||
		TraversalCheckResult.ActionType == LocomotionActionTags::Vault)
	{
		ExtractWarpTargetCurveValue(NAME_DistanceFromLedge, NAME_BackLedge, DistanceFromFrontLedgeToBackLedge);
		MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(
			NAME_BackLedge, TraversalCheckResult.BackLedgeLocation, FRotator::ZeroRotator
		);
	}
	else
	{
		MotionWarpingComponent->RemoveWarpTarget(NAME_BackLedge);
	}

	// Process hurdle-specific targets
	if (TraversalCheckResult.ActionType == LocomotionActionTags::Hurdle)
	{
		float DistanceFromFrontLedgeToBackFloor{0.f};
		ExtractWarpTargetCurveValue(NAME_DistanceFromLedge, NAME_BackFloor, DistanceFromFrontLedgeToBackFloor);

		const FVector NormalVector = TraversalCheckResult.BackLedgeNormal *
			FMath::Abs(DistanceFromFrontLedgeToBackLedge - DistanceFromFrontLedgeToBackFloor);

		FVector Result = TraversalCheckResult.BackLedgeLocation + NormalVector;
		Result.Z = TraversalCheckResult.BackFloorLocation.Z;

		MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(
			NAME_BackFloor, Result, FRotator::ZeroRotator);
	}
	else
	{
		MotionWarpingComponent->RemoveWarpTarget(NAME_BackFloor);
	}
}

FTraversalResult UGASPTraversalComponent::TryTraversalAction(FTraversalCheckInputs CheckInputs)
{
	if (!CharacterOwner.IsValid())
	{
		return {true, false};
	}

	// Step 1: Cache some important values for use later in the function.
	const double StartTime = FPlatformTime::Seconds();
	const FVector& ActorLocation = CharacterOwner->GetActorLocation();
	const float& CapsuleRadius = CapsuleComponent.IsValid() ? CapsuleComponent->GetScaledCapsuleRadius() : 30.f;
	const float& CapsuleHalfHeight = CapsuleComponent.IsValid() ? CapsuleComponent->GetScaledCapsuleHalfHeight() : 60.f;

	FTraversalCheckResult NewTraversalCheckResult;

	// Step 2.1: Perform a trace in the actor's forward direction to find a Traversable Level Block. If found, set the Hit Component, if not, exit the function.
	auto* World = CharacterOwner->GetWorld();
	FHitResult Hit;
	FVector StartLocation = ActorLocation + CheckInputs.TraceOriginOffset;
	FVector EndLocation = StartLocation + CheckInputs.TraceForwardDirection * CheckInputs.TraceForwardDistance +
		CheckInputs.TraceEndOffset;

	SweepTrace(World, Hit, StartLocation, EndLocation, CheckInputs.TraceRadius, CheckInputs.TraceHalfHeight,
	           ECC_Visibility);

#if WITH_EDITOR && ALLOW_CONSOLE
	const float& LifeTime = TraversalVar::DrawDebugDuration;
	if (TraversalVar::DrawDebugLevel >= 2)
	{
		DrawDebugCapsule(World, StartLocation, CheckInputs.TraceHalfHeight, CheckInputs.TraceRadius, FQuat::Identity,
		                 FColor::Black, false, LifeTime);
		if (Hit.bBlockingHit)
		{
			DrawDebugCapsule(World, Hit.Location, CheckInputs.TraceHalfHeight, CheckInputs.TraceRadius, FQuat::Identity,
			                 FColor::Black, false, LifeTime);
		}

		DrawDebugLine(World, StartLocation, Hit.Location, FColor::Black, false, LifeTime);
	}
#endif

	if (!Hit.bBlockingHit)
	{
		return {true, false};
	}

	NewTraversalCheckResult.HitComponent = Hit.GetComponent();

	TArray<FName> TagsToCompare;
	if (IsValid(Hit.GetActor()))
	{
		Algo::Transform(Hit.GetActor()->Tags, TagsToCompare, [&](const FName& Tag)
		{
			return Tag;
		});
	}
	if (IsValid(NewTraversalCheckResult.HitComponent))
	{
		TagsToCompare.Append(Hit.GetComponent()->ComponentTags);
		TryAndCalculateLedges(Hit, NewTraversalCheckResult);
	}

	if (TagsToCompare.Contains(BannedTag))
	{
		return {true, false};
	}

#if WITH_EDITOR && ALLOW_CONSOLE
	// DEBUG: Draw Debug shapes at ledge locations.
	if (TraversalVar::DrawDebugLevel >= 1)
	{
		if (NewTraversalCheckResult.bHasFrontLedge)
		{
			DrawDebugSphere(World, NewTraversalCheckResult.FrontLedgeLocation, 10.0f, 12,
			                FLinearColor::Green.ToFColor(true), false, TraversalVar::DrawDebugDuration,
			                SDPG_World, 1.0f);
		}

		if (NewTraversalCheckResult.bHasBackLedge)
		{
			DrawDebugSphere(World, NewTraversalCheckResult.BackLedgeLocation, 10.0f, 12,
			                FLinearColor::Blue.ToFColor(true), false, TraversalVar::DrawDebugDuration,
			                SDPG_World, 1.0f);
		}
	}
#endif

	// Step 3.1 If the traversable level block has a valid front ledge, continue the function. If not, exit early.
	if (!NewTraversalCheckResult.bHasFrontLedge)
	{
		return {true, false};
	}

	/** Step 3.2: Perform a trace from the actors location up to the front ledge location to determine if there is
	 * room for the actor to move up to it. If so, continue the function. If not, exit early. */
	const FVector HasRoomCheckFrontLedgeLocation = NewTraversalCheckResult.FrontLedgeLocation +
		NewTraversalCheckResult.FrontLedgeNormal * (CapsuleRadius + 2.0f) +
		FVector::ZAxisVector * (CapsuleHalfHeight + 2.0f);

	SweepTrace(World, Hit, ActorLocation, HasRoomCheckFrontLedgeLocation, CapsuleRadius, CapsuleHalfHeight,
	           ECC_Visibility);

#if WITH_EDITOR && ALLOW_CONSOLE
	if (TraversalVar::DrawDebugLevel >= 1)
	{
		DrawDebugCapsule(World, ActorLocation, CapsuleHalfHeight, CapsuleRadius, FQuat::Identity, FColor::Red,
		                 false,
		                 LifeTime);
	}
#endif

	if (Hit.bBlockingHit || Hit.bStartPenetrating)
	{
		NewTraversalCheckResult.bHasFrontLedge = false;
		return {true, false};
	}

	// Step 3.3: save the height of the obstacle using the delta between the actor and front ledge transform.
	NewTraversalCheckResult.ObstacleHeight = FMath::Abs(
		(ActorLocation - FVector::ZAxisVector * CapsuleHalfHeight - NewTraversalCheckResult.FrontLedgeLocation).Z);

	/** Step 3.4: Perform a trace across the top of the obstacle from the front ledge to the back ledge to see if
	 * there's room for the actor to move across it.*/
	const FVector HasRoomCheckBackLedgeLocation = NewTraversalCheckResult.BackLedgeLocation + NewTraversalCheckResult.
		BackLedgeNormal * (CapsuleRadius + 2.0f) + FVector::ZAxisVector * (CapsuleHalfHeight + 2.0f);

	bool bHit = SweepTrace(World, Hit, HasRoomCheckFrontLedgeLocation, HasRoomCheckBackLedgeLocation, CapsuleRadius,
	                       CapsuleHalfHeight, ECC_Visibility);
#if WITH_EDITOR && ALLOW_CONSOLE
	if (TraversalVar::DrawDebugLevel >= 1)
	{
		DrawDebugCapsule(World, HasRoomCheckFrontLedgeLocation, CapsuleHalfHeight, CapsuleRadius, FQuat::Identity,
		                 FColor::Red, false,
		                 LifeTime);
	}
#endif


	if (bHit)
	{
		/** Step 3.5: If there is not room, save the obstacle depth using the difference between the front ledge and the
		 * trace impact point, and invalidate the back ledge. */
		NewTraversalCheckResult.ObstacleDepth = (Hit.ImpactPoint - NewTraversalCheckResult.FrontLedgeLocation).Size2D();
		NewTraversalCheckResult.bHasBackLedge = false;
	}
	else
	{
		// Step 3.5: If there is room, save the obstacle depth using the difference between the front and back ledge locations.
		NewTraversalCheckResult.ObstacleDepth =
			(NewTraversalCheckResult.FrontLedgeLocation - NewTraversalCheckResult.BackLedgeLocation).Size2D();

		/** Step 3.6: Trace downward from the back ledge location (using the height of the obstacle for the distance)
		 * to find the floor. If there is a floor, save its location and the back ledges height (using the distance
		 * between the back ledge and the floor). If no floor was found, invalidate the back floor.*/
		const FVector EndTraceLocation = NewTraversalCheckResult.BackLedgeLocation +
			NewTraversalCheckResult.BackLedgeNormal * (CapsuleRadius + 2.0f) - FVector::ZAxisVector * (
				NewTraversalCheckResult.ObstacleHeight - CapsuleHalfHeight + 50.0f);

		SweepTrace(World, Hit, HasRoomCheckBackLedgeLocation, EndTraceLocation, CapsuleRadius,
		           CapsuleHalfHeight, ECC_Visibility);

#if WITH_EDITOR && ALLOW_CONSOLE
		if (TraversalVar::DrawDebugLevel >= 1)
		{
			DrawDebugCapsule(World, HasRoomCheckBackLedgeLocation, CapsuleHalfHeight, CapsuleRadius, FQuat::Identity,
			                 FColor::Red, false, LifeTime);
		}
#endif

		if (Hit.bBlockingHit)
		{
			NewTraversalCheckResult.bHasBackFloor = true;
			NewTraversalCheckResult.BackFloorLocation = Hit.ImpactPoint;
			NewTraversalCheckResult.BackLedgeHeight = FMath::Abs(
				(Hit.ImpactPoint - NewTraversalCheckResult.BackLedgeLocation).Z);
		}
		else
		{
			NewTraversalCheckResult.bHasBackFloor = false;
		}
	}

	// Step 5.3: Evaluate a chooser to select all montages that match the conditions of the traversal check.
	auto* ChooserTable{TraversalAnimationsChooserTable.LoadSynchronous()};
	FTraversalChooserInput ChooserParameters;
	ChooserParameters.ActionType = NewTraversalCheckResult.ActionType;
	ChooserParameters.Speed = CharacterOwner->GetVelocity().Size2D();
	ChooserParameters.StateContainer = CharacterOwner->StateContainer;
	ChooserParameters.bHasBackFloor = NewTraversalCheckResult.bHasBackFloor;
	ChooserParameters.bHasBackLedge = NewTraversalCheckResult.bHasBackLedge;
	ChooserParameters.bHasFrontLedge = NewTraversalCheckResult.bHasFrontLedge;
	ChooserParameters.ObstacleHeight = NewTraversalCheckResult.ObstacleHeight;
	ChooserParameters.ObstacleDepth = NewTraversalCheckResult.ObstacleDepth;
	ChooserParameters.BackLedgeHeight = NewTraversalCheckResult.BackLedgeHeight;

	FTraversalChooserOutput ChooserOutput;
	auto Context = UChooserFunctionLibrary::MakeChooserEvaluationContext();

	Context.AddStructParam(ChooserParameters);
	Context.AddStructParam(ChooserOutput);
	auto AnimationMontages{
		UChooserFunctionLibrary::EvaluateObjectChooserBaseMulti(
			Context, UChooserFunctionLibrary::MakeEvaluateChooser(ChooserTable), UAnimMontage::StaticClass())
	};

	NewTraversalCheckResult.ActionType = ChooserOutput.ActionType;

	/* Step 5.1: Continue if there is a valid action type. If none of the conditions were met, no action can be
	 * performed, therefore exit the function. */
	if (NewTraversalCheckResult.ActionType == FGameplayTag::EmptyTag)
	{
		UE_LOG(LogTemp, Warning, TEXT("Traversal: 366"));
		return {true, false};
	}

	/** Step 5.2: Send the front ledge location to the Anim BP using an interface. This transform will be used for a
	 * custom channel within the following Motion Matching search. */
	if (!AnimInstance.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Traversal: 373"));
		return {true, false};
	}
	auto* InteractableObject = Cast<IGASPInteractionTransformInterface>(AnimInstance);
	if (InteractableObject == nullptr && !AnimInstance->Implements<UGASPInteractionTransformInterface>())
	{
		return {true, false};
	}

	const auto InteractionTransform = FTransform(
		FRotationMatrix::MakeFromZ(NewTraversalCheckResult.FrontLedgeNormal).ToQuat(),
		NewTraversalCheckResult.FrontLedgeLocation, FVector::OneVector);
	if (InteractableObject != nullptr)
	{
		InteractableObject->Execute_SetInteractionTransform(AnimInstance.Get(), InteractionTransform);
	}
	else
	{
		IGASPInteractionTransformInterface::Execute_SetInteractionTransform(AnimInstance.Get(), InteractionTransform);
	}

	/** Step 5.4: Perform a Motion Match on all the montages that were chosen by the chooser to find the best result.
	 * This match will elect the best montage AND the best entry frame (start time) based on the distance to the ledge,
	 * and the current characters pose. If for some reason no montage was found (motion matching failed, perhaps due to
	 * an invalid database or issue with the schema), print a warning and exit the function. */
	FPoseSearchBlueprintResult Result;
	UPoseSearchLibrary::MotionMatch(AnimInstance.Get(), AnimationMontages, NAME_PoseHistory,
	                                FPoseSearchContinuingProperties(), FPoseSearchFutureProperties(), Result);
	const auto* AnimationMontage = Cast<UAnimMontage>(Result.SelectedAnim);
	if (!IsValid(AnimationMontage))
	{
#if WITH_EDITOR && ALLOW_CONSOLE
		GEngine->AddOnScreenDebugMessage(-1, TraversalVar::DrawDebugDuration, FColor::Red,
		                                 FString::Printf(TEXT("Failed To Find Montage!")));
#endif
		return {true, false};
	}
	NewTraversalCheckResult.ChosenMontage = AnimationMontage;
	NewTraversalCheckResult.StartTime = Result.SelectedTime;
	NewTraversalCheckResult.PlayRate = Result.WantedPlayRate;

	TraversalCheckResult = NewTraversalCheckResult;
	PerformTraversalAction();
	Server_Traversal(TraversalCheckResult);

#if WITH_EDITOR
	if (TraversalVar::DrawDebugLevel >= 2)
	{
		GEngine->AddOnScreenDebugMessage(-1, TraversalVar::DrawDebugDuration,
		                                 FLinearColor(0.0f, 0.66f, 1.0f).ToFColor(true),
		                                 FString::Printf(TEXT("%s"), *NewTraversalCheckResult.ToString()));
		GEngine->AddOnScreenDebugMessage(-1, TraversalVar::DrawDebugDuration,
		                                 FLinearColor(1.0f, 0.0f, 0.824021f).ToFColor(true),
		                                 FString::Printf(
			                                 TEXT("%s"), *NewTraversalCheckResult.ActionType.ToString()));

		const auto PerfString = FString::Printf(TEXT("Execution Time: %f seconds"),
		                                        FPlatformTime::Seconds() - StartTime);
		GEngine->AddOnScreenDebugMessage(-1, TraversalVar::DrawDebugDuration,
		                                 FLinearColor(1.0f, 0.5f, 0.15f).ToFColor(true),
		                                 FString::Printf(TEXT("%s"), *PerfString));
	}
#endif
	return {};
}

bool UGASPTraversalComponent::IsDoingTraversal() const
{
	return bDoingTraversalAction;
}

void UGASPTraversalComponent::Traversal_ServerImplementation(const FTraversalCheckResult TraversalRep)
{
	TraversalCheckResult = TraversalRep;
	PerformTraversalAction();
	Multicast_Traversal(TraversalCheckResult);
}

void UGASPTraversalComponent::OnRep_TraversalResult()
{
	PerformTraversalAction();
}

void UGASPTraversalComponent::OnCompleteTraversal(FName NotifyName)
{
	bDoingTraversalAction = false;
	CapsuleComponent->IgnoreComponentWhenMoving(TraversalCheckResult.HitComponent, false);

	const auto MovementMode{
		TraversalCheckResult.ActionType == LocomotionActionTags::Vault
			? DefaultModeNames::Falling
			: DefaultModeNames::Walking
	};
	MoverComponent->QueueNextMode(MovementMode);
}

void UGASPTraversalComponent::PerformTraversalAction_Implementation()
{
	UpdateWarpTargets();

	auto* MontageToPlay{const_cast<UAnimMontage*>(TraversalCheckResult.ChosenMontage.Get())};

	const auto MontageProxy = UPlayMoverMontageCallbackProxy::CreateProxyObjectForPlayMoverMontage(
		MoverComponent.Get(), MontageToPlay, TraversalCheckResult.PlayRate, TraversalCheckResult.StartTime);

	MontageProxy->OnCompleted.AddDynamic(this, &ThisClass::OnCompleteTraversal);
	MontageProxy->OnBlendOut.AddDynamic(this, &ThisClass::OnCompleteTraversal);
	MontageProxy->OnInterrupted.AddDynamic(this, &ThisClass::OnCompleteTraversal);

	bDoingTraversalAction = true;
	CapsuleComponent->IgnoreComponentWhenMoving(TraversalCheckResult.HitComponent, true);

	MoverComponent->QueueNextMode(DefaultModeNames::Flying);
}

void UGASPTraversalComponent::Server_Traversal_Implementation(FTraversalCheckResult TraversalRep)
{
	Traversal_ServerImplementation(TraversalRep);
}

void UGASPTraversalComponent::Multicast_Traversal_Implementation(FTraversalCheckResult TraversalRep)
{
	TraversalCheckResult = TraversalRep;
	PerformTraversalAction();
}

FComputeLedgeData UGASPTraversalComponent::ComputeLedgeData(FHitResult& HitResult) const
{
	HitResult.ImpactPoint -= (HitResult.ImpactPoint - FVector::PointPlaneProject(
		HitResult.GetComponent()->Bounds.Origin, HitResult.ImpactPoint, HitResult.ImpactNormal)).GetSafeNormal();

	const auto StartNormal{HitResult.ImpactNormal};
	const float TraceLength = HitResult.GetComponent()->Bounds.SphereRadius * 2;

	const FVector AbsoluteObjectUpVector = HitResult.GetComponent()->GetUpVector() * FMath::Sign(
		FVector::DotProduct(HitResult.GetComponent()->GetUpVector(), CharacterOwner->GetActorUpVector()));

	auto TraceCorner = TraceCorners(HitResult, FVector::CrossProduct(HitResult.ImpactNormal, AbsoluteObjectUpVector),
	                                TraceLength);

	const float RightEdgeDistance{TraceCorner.DistanceToCorner};
	if (TraceCorner.bCloseToCorner)
	{
		HitResult.ImpactPoint = TraceCorner.OfsettedCornerPoint;
	}

	TraceCorner = TraceCorners(HitResult, FVector::CrossProduct(AbsoluteObjectUpVector, HitResult.ImpactNormal),
	                           TraceLength);

	if (TraceCorner.bCloseToCorner)
	{
		if (TraceCorner.DistanceToCorner + RightEdgeDistance < MinLedgeWidth)
		{
			if (!TraceWidth(HitResult, FVector::CrossProduct(AbsoluteObjectUpVector, HitResult.ImpactNormal)) ||
				!TraceWidth(HitResult, -FVector::CrossProduct(AbsoluteObjectUpVector, HitResult.ImpactNormal)))
			{
				return {};
			}
		}
		else
		{
			HitResult.ImpactPoint = TraceCorner.OfsettedCornerPoint;
		}
	}

	auto OutHit{HitResult};
	if (!TraceAlongHitPlane(HitResult, AbsoluteObjectUpVector, TraceLength, OutHit))
	{
		return {};
	}

	const auto StartLedge{OutHit.ImpactPoint};
	auto EndNormal{FVector::ZeroVector};
	auto EndLedge{FVector::ZeroVector};

	const bool bHasBackLedge = HitResult.GetComponent()->LineTraceComponent(
		OutHit, HitResult.ImpactPoint - HitResult.ImpactNormal * TraceLength, HitResult.ImpactPoint, GetQueryParams());

	if (bHasBackLedge)
	{
		EndNormal = OutHit.ImpactNormal;
		TraceAlongHitPlane(OutHit, AbsoluteObjectUpVector, TraceLength, OutHit);

		EndLedge = OutHit.ImpactPoint;
	}

	return {true, bHasBackLedge, StartLedge, StartNormal, EndLedge, EndNormal};
}

void UGASPTraversalComponent::TryAndCalculateLedges(FHitResult& HitResult, FTraversalCheckResult& TraversalData)
{
	auto [bFoundFrontLedge, bFoundBackLedge, StartLedgeLocation, StartLedgeNormal, EndLedgeLocation, EndLedgeNormal] =
		ComputeLedgeData(HitResult);

	TraversalData.bHasFrontLedge = bFoundFrontLedge;
	TraversalData.FrontLedgeLocation = StartLedgeLocation;
	TraversalData.FrontLedgeNormal = StartLedgeNormal;

	TraversalData.bHasBackLedge = bFoundBackLedge;
	TraversalData.BackLedgeLocation = EndLedgeLocation;
	TraversalData.BackLedgeNormal = EndLedgeNormal;
}

FTraceCorners UGASPTraversalComponent::TraceCorners(FHitResult HitResult, const FVector TraceDirection,
                                                    const float TraceLength) const
{
	if (auto OutHit = HitResult; TraceAlongHitPlane(HitResult, TraceDirection, TraceLength, OutHit))
	{
		const float DistanceToCorner = FVector::Distance(OutHit.ImpactPoint, HitResult.ImpactPoint);
		return {
			OutHit.ImpactPoint + (-TraceDirection) * (MinLedgeWidth / 2.f),
			DistanceToCorner < (MinLedgeWidth / 2.f), DistanceToCorner
		};
	}
	return {};
}

bool UGASPTraversalComponent::TraceAlongHitPlane(const FHitResult& HitResult, const FVector TraceDirection,
                                                 const float TraceLength, FHitResult& OutHit) const
{
	const FVector CrossPoint = FVector::CrossProduct(HitResult.ImpactNormal, TraceDirection);
	const FVector NormalizedPoint = FVector::CrossProduct(CrossPoint, HitResult.ImpactNormal).GetSafeNormal(.0001f);
	const FVector DeltaPoint = HitResult.ImpactPoint - HitResult.ImpactNormal;
	const FVector TraceStart = TraceLength * NormalizedPoint + DeltaPoint;

	return HitResult.GetComponent()->LineTraceComponent(OutHit, TraceStart,
	                                                    TraceStart + 1.5 * (DeltaPoint - TraceStart),
	                                                    ECC_Visibility, GetQueryParams(), FCollisionResponseParams(),
	                                                    FCollisionObjectQueryParams());
}

bool UGASPTraversalComponent::TraceWidth(FHitResult HitResult, const FVector Direction) const
{
	const FVector StartLocation = Direction * (MinLedgeWidth / 2.f) + HitResult.ImpactPoint;
	const FVector FrontLedgeNormalDepth = HitResult.ImpactNormal * MinFrontLedgeDepth;

	return HitResult.GetComponent()->LineTraceComponent(HitResult, StartLocation + FrontLedgeNormalDepth,
	                                                    StartLocation - FrontLedgeNormalDepth, GetQueryParams());
}

bool UGASPTraversalComponent::SweepTrace(const UWorld* World, FHitResult& HitResult, const FVector& Start,
                                         const FVector& End, const float CapsuleRadius, const float TraceHalfHeight,
                                         const ECollisionChannel CollisionChannel)
{
	return World->SweepSingleByChannel(HitResult, Start, End, FQuat::Identity, CollisionChannel,
	                                   FCollisionShape::MakeCapsule(CapsuleRadius, TraceHalfHeight), GetQueryParams());
}

FCollisionQueryParams UGASPTraversalComponent::GetQueryParams() const
{
	TArray<AActor*> IgnoredActors;
	CharacterOwner->GetAllChildActors(IgnoredActors);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(CharacterOwner.Get());
	QueryParams.AddIgnoredActors(IgnoredActors);
	QueryParams.bTraceComplex = true;

	return QueryParams;
}
