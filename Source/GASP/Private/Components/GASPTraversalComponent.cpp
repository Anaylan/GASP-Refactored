#include "Components/GASPTraversalComponent.h"
#include "Animation/AnimInstance.h"
#include "AnimationWarpingLibrary.h"
#include "ChooserFunctionLibrary.h"
#include "Components/CapsuleComponent.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "DefaultMovementSet/LayeredMoves/AnimRootMotionLayeredMove.h"
#include "IObjectChooser.h"
#include "Interfaces/GASPInteractionInterface.h"
#include "Net/UnrealNetwork.h"
#include "MotionWarpingComponent.h"
#include "Actors/GASPCharacter.h"
#include "MovementSet/GASPMoverComponent.h"
#include "Settings/GASPCharacterSettings.h"
#include "Types/TagTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPTraversalComponent)

#if WITH_EDITOR
namespace TraversalVar
{
	int32 DrawDebugLevel{0};
	FAutoConsoleVariableRef
	DrawDebugLevelStruct(TEXT("gasp.traversal.DrawDebugLevel"), DrawDebugLevel,
	                     TEXT("debug level for traversal"), ECVF_Default);

	float DrawDebugDuration{0.f};
	FAutoConsoleVariableRef
	DrawDebugDurationStruct(TEXT("gasp.traversal.DrawDebugDuration"),
	                        DrawDebugDuration,
	                        TEXT("debug duration for traversal"), ECVF_Default);
}
#endif

namespace
{
	const FName NAME_FrontLedge{TEXT("FrontLedge")};
	const FName NAME_BackLedge{TEXT("BackLedge")};
	const FName NAME_BackFloor{TEXT("BackFloor")};
	const FName NAME_DistanceFromLedge{TEXT("Distance_From_Ledge")};
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
		AnimInstance = MeshComponent->GetAnimInstance();
	}
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
		FRotationMatrix::MakeFromX(-TraversalCheckResult.FrontLedgeNormal).Rotator());

	// Process hurdle or vault
	float DistanceFromFrontLedgeToBackLedge{0.f};
	if (TraversalCheckResult.ActionType == LocomotionActionTags::Hurdle ||
		TraversalCheckResult.ActionType == LocomotionActionTags::Vault)
	{
		ExtractWarpTargetCurveValue(NAME_DistanceFromLedge, NAME_BackLedge,
		                            DistanceFromFrontLedgeToBackLedge);
		MotionWarpingComponent->AddOrUpdateWarpTargetFromLocationAndRotation(
			NAME_BackLedge, TraversalCheckResult.BackLedgeLocation,
			FRotator::ZeroRotator);
	}
	else
	{
		MotionWarpingComponent->RemoveWarpTarget(NAME_BackLedge);
	}

	// Process hurdle-specific targets
	if (TraversalCheckResult.ActionType == LocomotionActionTags::Hurdle)
	{
		float DistanceFromFrontLedgeToBackFloor{0.f};
		ExtractWarpTargetCurveValue(NAME_DistanceFromLedge, NAME_BackFloor,
		                            DistanceFromFrontLedgeToBackFloor);

		const FVector NormalVector = TraversalCheckResult.BackLedgeNormal * FMath::Abs(
			DistanceFromFrontLedgeToBackLedge - DistanceFromFrontLedgeToBackFloor);

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

	const double StartTime = FPlatformTime::Seconds();
	const FVector& ActorLocation = CharacterOwner->GetActorLocation();
	const float CapsuleRadius = CapsuleComponent.IsValid()
		                            ? CapsuleComponent->GetScaledCapsuleRadius()
		                            : 30.f;
	const float CapsuleHalfHeight = CapsuleComponent.IsValid()
		                                ? CapsuleComponent->GetScaledCapsuleHalfHeight()
		                                : 60.f;

	FTraversalCheckResult NewTraversalCheckResult;

	// Step 1: Perform a trace to find a Traversable Level Block
	FVector StartLocation = ActorLocation + CheckInputs.TraceOriginOffset;
	FHitResult Hit;
	if (!DetectObstacle(CheckInputs, StartLocation, Hit))
	{
		return {true, false};
	}

	NewTraversalCheckResult.HitComponent = Hit.GetComponent();

	TArray<FName> TagsToCompare;
	if (IsValid(Hit.GetActor()))
	{
		TagsToCompare.Append(Hit.GetActor()->Tags);
	}
	if (IsValid(NewTraversalCheckResult.HitComponent))
	{
		TagsToCompare.Append(NewTraversalCheckResult.HitComponent->ComponentTags);
		TryAndCalculateLedges(Hit, NewTraversalCheckResult);
	}

	if (TagsToCompare.Contains(BannedTag) ||
		!NewTraversalCheckResult.bHasFrontLedge)
	{
		return {true, false};
	}

#if WITH_EDITOR
	DrawDebugLedges(NewTraversalCheckResult);
#endif

	// Step 2: Verify clearance room to the front ledge
	FVector HasRoomCheckFrontLedgeLocation;
	if (!VerifyFrontLedgeClearance(ActorLocation, NewTraversalCheckResult,
	                               CapsuleRadius, CapsuleHalfHeight,
	                               HasRoomCheckFrontLedgeLocation))
	{
		return {true, false};
	}

	// Step 3: Analyze top and back obstacle dimensions
	AnalyzeObstacleDimensions(NewTraversalCheckResult,
	                          HasRoomCheckFrontLedgeLocation, CapsuleRadius,
	                          CapsuleHalfHeight);

	// Step 4: Evaluate chooser and select montage
	if (!SelectTraversalMontage(NewTraversalCheckResult))
	{
		return {true, false};
	}

	TraversalCheckResult = NewTraversalCheckResult;
	PerformTraversalAction();
	Server_Traversal(TraversalCheckResult);

#if WITH_EDITOR
	DrawDebugPerformance(NewTraversalCheckResult, StartTime);
#endif

	return {};
}

bool UGASPTraversalComponent::DetectObstacle(const FTraversalCheckInputs& CheckInputs, const FVector& StartLocation,
                                             FHitResult& OutHit)
{
	auto* World = CharacterOwner->GetWorld();
	FVector EndLocation = StartLocation + CheckInputs.TraceForwardDirection * CheckInputs.TraceForwardDistance +
		CheckInputs.TraceEndOffset;

	bool bHit = SweepTrace(World, OutHit, StartLocation, EndLocation,
	                       CheckInputs.TraceRadius, CheckInputs.TraceHalfHeight,
	                       ECC_Visibility);

#if WITH_EDITOR && ALLOW_CONSOLE
	if (TraversalVar::DrawDebugLevel >= 2)
	{
		const float LifeTime = TraversalVar::DrawDebugDuration > 0.f
			                       ? TraversalVar::DrawDebugDuration
			                       : 2.f;
		FColor DebugColor = bHit ? FColor::Green : FColor::Red;

		DrawDebugCapsule(World, StartLocation, CheckInputs.TraceHalfHeight,
		                 CheckInputs.TraceRadius, FQuat::Identity, DebugColor,
		                 false, LifeTime);
		if (bHit)
		{
			DrawDebugCapsule(World, OutHit.Location, CheckInputs.TraceHalfHeight,
			                 CheckInputs.TraceRadius, FQuat::Identity, DebugColor,
			                 false, LifeTime);
		}
		DrawDebugLine(World, StartLocation, bHit ? OutHit.Location : EndLocation,
		              DebugColor, false, LifeTime);
	}
#endif

	return bHit;
}

bool UGASPTraversalComponent::VerifyFrontLedgeClearance(
	const FVector& ActorLocation, FTraversalCheckResult& TraversalData,
	float CapsuleRadius, float CapsuleHalfHeight, FVector& OutCheckLocation)
{
	OutCheckLocation = TraversalData.FrontLedgeLocation +
		TraversalData.FrontLedgeNormal * (CapsuleRadius + 2.0f) +
		FVector::ZAxisVector * (CapsuleHalfHeight + 2.0f);

	auto* World = CharacterOwner->GetWorld();
	FHitResult Hit;
	bool bHit = SweepTrace(World, Hit, ActorLocation, OutCheckLocation,
	                       CapsuleRadius, CapsuleHalfHeight, ECC_Visibility);

#if WITH_EDITOR && ALLOW_CONSOLE
	if (TraversalVar::DrawDebugLevel >= 1)
	{
		const float LifeTime = TraversalVar::DrawDebugDuration > 0.f
			                       ? TraversalVar::DrawDebugDuration
			                       : 2.f;
		FColor DebugColor =
			(bHit || Hit.bStartPenetrating) ? FColor::Red : FColor::Green;

		DrawDebugCapsule(World, ActorLocation, CapsuleHalfHeight, CapsuleRadius,
		                 FQuat::Identity, DebugColor, false, LifeTime);
		DrawDebugLine(World, ActorLocation, OutCheckLocation, DebugColor, false,
		              LifeTime);
	}
#endif

	if (bHit || Hit.bStartPenetrating)
	{
		TraversalData.bHasFrontLedge = false;
		return false;
	}

	TraversalData.ObstacleHeight = FMath::Abs(
		(ActorLocation - FVector::ZAxisVector * CapsuleHalfHeight - TraversalData.FrontLedgeLocation).Z);
	return true;
}

void UGASPTraversalComponent::AnalyzeObstacleDimensions(FTraversalCheckResult& TraversalData,
                                                        const FVector& HasRoomCheckFrontLedgeLocation,
                                                        float CapsuleRadius, float CapsuleHalfHeight)
{
	auto* World = CharacterOwner->GetWorld();
	const FVector HasRoomCheckBackLedgeLocation = TraversalData.BackLedgeLocation + TraversalData.BackLedgeNormal * (
		CapsuleRadius + 2.0f) + FVector::ZAxisVector * (CapsuleHalfHeight + 2.0f);

	FHitResult Hit;
	bool bHit = SweepTrace(World, Hit, HasRoomCheckFrontLedgeLocation,
	                       HasRoomCheckBackLedgeLocation, CapsuleRadius,
	                       CapsuleHalfHeight, ECC_Visibility);

#if WITH_EDITOR && ALLOW_CONSOLE
	const float LifeTime = TraversalVar::DrawDebugDuration > 0.f
		                       ? TraversalVar::DrawDebugDuration
		                       : 2.f;
	if (TraversalVar::DrawDebugLevel >= 1)
	{
		const auto DebugColor = bHit ? FColor::Red : FColor::Green;
		DrawDebugCapsule(World, HasRoomCheckFrontLedgeLocation, CapsuleHalfHeight,
		                 CapsuleRadius, FQuat::Identity, DebugColor, false,
		                 LifeTime);
		DrawDebugLine(World, HasRoomCheckFrontLedgeLocation,
		              HasRoomCheckBackLedgeLocation, DebugColor, false, LifeTime);
	}
#endif

	if (bHit)
	{
		TraversalData.ObstacleDepth = (Hit.ImpactPoint - TraversalData.FrontLedgeLocation).Size2D();
		TraversalData.bHasBackLedge = false;
	}
	else
	{
		TraversalData.ObstacleDepth = (TraversalData.FrontLedgeLocation - TraversalData.BackLedgeLocation).Size2D();

		const FVector EndTraceLocation = TraversalData.BackLedgeLocation + TraversalData.BackLedgeNormal * (
			CapsuleRadius + 2.0f) - FVector::ZAxisVector * (TraversalData.ObstacleHeight - CapsuleHalfHeight + 50.0f);

		SweepTrace(World, Hit, HasRoomCheckBackLedgeLocation, EndTraceLocation,
		           CapsuleRadius, CapsuleHalfHeight, ECC_Visibility);

#if WITH_EDITOR && ALLOW_CONSOLE
		if (TraversalVar::DrawDebugLevel >= 1)
		{
			const auto FloorDebugColor = Hit.bBlockingHit ? FColor::Green : FColor::Red;
			DrawDebugCapsule(World, HasRoomCheckBackLedgeLocation, CapsuleHalfHeight,
			                 CapsuleRadius, FQuat::Identity, FloorDebugColor, false,
			                 LifeTime);
			DrawDebugLine(World, HasRoomCheckBackLedgeLocation, EndTraceLocation,
			              FloorDebugColor, false, LifeTime);
			if (Hit.bBlockingHit)
			{
				DrawDebugSphere(World, Hit.ImpactPoint, 15.0f, 12, FColor::Green, false,
				                LifeTime, SDPG_World, 2.0f);
			}
		}
#endif

		if (Hit.bBlockingHit)
		{
			TraversalData.bHasBackFloor = true;
			TraversalData.BackFloorLocation = Hit.ImpactPoint;
			TraversalData.BackLedgeHeight = FMath::Abs((Hit.ImpactPoint - TraversalData.BackLedgeLocation).Z);
		}
		else
		{
			TraversalData.bHasBackFloor = false;
		}
	}
}

bool UGASPTraversalComponent::SelectTraversalMontage(FTraversalCheckResult& TraversalData)
{
	if (!AnimInstance.IsValid() || !AnimInstance->Implements<UGASPInteractionInterface>())
	{
		return false;
	}

	const auto InteractionTransform = FTransform(
		FRotationMatrix::MakeFromZ(TraversalData.FrontLedgeNormal).ToQuat(),
		TraversalData.FrontLedgeLocation, FVector::OneVector);
	IGASPInteractionInterface::Execute_SetInteractionTransform(AnimInstance.Get(), InteractionTransform);

	FTraversalChooserInput ChooserParameters;
	ChooserParameters.ActionType = TraversalData.ActionType;
	ChooserParameters.Speed = MoverComponent->GetVelocity().Size2D();
	ChooserParameters.StateContainer = CharacterOwner->StateContainer;
	ChooserParameters.bHasBackFloor = TraversalData.bHasBackFloor;
	ChooserParameters.bHasBackLedge = TraversalData.bHasBackLedge;
	ChooserParameters.bHasFrontLedge = TraversalData.bHasFrontLedge;
	ChooserParameters.ObstacleHeight = TraversalData.ObstacleHeight;
	ChooserParameters.ObstacleDepth = TraversalData.ObstacleDepth;
	ChooserParameters.BackLedgeHeight = TraversalData.BackLedgeHeight;
	ChooserParameters.PoseHistory = IGASPInteractionInterface::Execute_GetPoseHistory(AnimInstance.Get());
	ChooserParameters.DistanceToLedge = FVector::Dist(TraversalData.FrontLedgeLocation,
	                                                  MeshComponent->GetComponentLocation());

	FTraversalChooserOutput ChooserOutput;
	auto Context = UChooserFunctionLibrary::MakeChooserEvaluationContext();

	Context.AddStructParam(ChooserParameters);
	Context.AddStructParam(ChooserOutput);

	auto ChooserTable = CharacterOwner->GetSettings()->TraversalTable.LoadSynchronous();
	auto AnimationMontage{
		UChooserFunctionLibrary::EvaluateObjectChooserBase(
			Context, UChooserFunctionLibrary::MakeEvaluateChooser(ChooserTable),
			UAnimMontage::StaticClass())
	};

	TraversalData.ActionType = ChooserOutput.ActionType;
	TraversalData.StartTime = ChooserOutput.MontageStartTime;
	TraversalData.ChosenMontage = static_cast<UAnimMontage*>(AnimationMontage);
	TraversalData.PlayRate = 1.f;

	return TraversalData.ActionType != FGameplayTag::EmptyTag;
}

#if WITH_EDITOR
void UGASPTraversalComponent::DrawDebugLedges(const FTraversalCheckResult& TraversalData) const
{
#if ALLOW_CONSOLE
	if (TraversalVar::DrawDebugLevel >= 1)
	{
		auto* World = CharacterOwner->GetWorld();
		const float LifeTime = TraversalVar::DrawDebugDuration > 0.f
			                       ? TraversalVar::DrawDebugDuration
			                       : 2.f;

		if (TraversalData.bHasFrontLedge)
		{
			DrawDebugSphere(World, TraversalData.FrontLedgeLocation, 12.0f, 16,
			                FColor::Green, false, LifeTime, SDPG_World, 2.0f);
			DrawDebugDirectionalArrow(World, TraversalData.FrontLedgeLocation,
			                          TraversalData.FrontLedgeLocation +
			                          TraversalData.FrontLedgeNormal * 50.f,
			                          20.f, FColor::Green, false, LifeTime,
			                          SDPG_World, 2.0f);
		}

		if (TraversalData.bHasBackLedge)
		{
			DrawDebugSphere(World, TraversalData.BackLedgeLocation, 12.0f, 16,
			                FColor::Blue, false, LifeTime, SDPG_World, 2.0f);
			DrawDebugDirectionalArrow(World, TraversalData.BackLedgeLocation,
			                          TraversalData.BackLedgeLocation +
			                          TraversalData.BackLedgeNormal * 50.f,
			                          20.f, FColor::Blue, false, LifeTime, SDPG_World,
			                          2.0f);
		}
	}
#endif
}

void UGASPTraversalComponent::DrawDebugPerformance(const FTraversalCheckResult& TraversalData, double StartTime) const
{
#if ALLOW_CONSOLE
	if (TraversalVar::DrawDebugLevel >= 2)
	{
		const float LifeTime = TraversalVar::DrawDebugDuration > 0.f
			                       ? TraversalVar::DrawDebugDuration
			                       : 2.f;
		GEngine->AddOnScreenDebugMessage(-1, LifeTime, FColor::Cyan, FString::Printf(TEXT("Traversal Action: %s"),
			                                 *TraversalData.ActionType.ToString()));
		GEngine->AddOnScreenDebugMessage(-1, LifeTime, FColor::Emerald,
		                                 FString::Printf(TEXT("Result:\n%s"), *TraversalData.ToString()));
		GEngine->AddOnScreenDebugMessage(-1, LifeTime, FColor::Orange, FString::Printf(TEXT("Execution Time: %.4f ms"),
			                                 (FPlatformTime::Seconds() - StartTime) * 1000.0));
	}
#endif
}
#endif

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
	CapsuleComponent->IgnoreComponentWhenMoving(TraversalCheckResult.HitComponent,
	                                            false);

	const auto MovementMode{
		TraversalCheckResult.ActionType ==
		LocomotionActionTags::Vault
			? DefaultModeNames::Falling
			: DefaultModeNames::Walking
	};
	MoverComponent->QueueNextMode(MovementMode);

	OnTraversalEvent.Broadcast(ETraversalEventType::Done);
}

void UGASPTraversalComponent::PerformTraversalAction_Implementation()
{
	UpdateWarpTargets();

	auto* MontageToPlay{const_cast<UAnimMontage*>(TraversalCheckResult.ChosenMontage.Get())};
	float MontageDuration{
		AnimInstance->Montage_Play(MontageToPlay, TraversalCheckResult.PlayRate, EMontagePlayReturnType::MontageLength,
		                           TraversalCheckResult.StartTime)
	};

	FOnMontageBlendingOutStarted BlendedOutEndedDelegate;
	BlendedOutEndedDelegate.BindWeakLambda(this, [this](UAnimMontage* Montage, bool bInterrupted)
	{
		if (bInterrupted)
		{
			OnCompleteTraversal(NAME_None);
		}
	});
	AnimInstance->Montage_SetBlendingOutDelegate(BlendedOutEndedDelegate,
	                                             MontageToPlay);

	FOnMontageEnded EndedDelegate;
	EndedDelegate.BindWeakLambda(this, [this](UAnimMontage* Montage, bool bInterrupted)
	{
		if (!bInterrupted)
		{
			OnCompleteTraversal(NAME_None);
		}
	});
	AnimInstance->Montage_SetEndDelegate(EndedDelegate, MontageToPlay);

	if (auto* MontageInstance = AnimInstance->GetActiveInstanceForMontage(MontageToPlay); MontageDuration > 0.f)
	{
		// Disable the actual animation-driven root motion, in favor of our own
		// layered move
		MontageInstance->PushDisableRootMotion();

		const float StartingMontagePosition = MontageInstance->GetPosition();
		// position in seconds, disregarding PlayRate

		// Queue a layered move to perform the same anim root motion over the same
		// time span
		auto AnimRootMotionMove = MakeShared<FLayeredMove_AnimRootMotion>();
		AnimRootMotionMove->MontageState.Montage = MontageToPlay;
		AnimRootMotionMove->MontageState.PlayRate = TraversalCheckResult.PlayRate;
		AnimRootMotionMove->MontageState.StartingMontagePosition = StartingMontagePosition;
		AnimRootMotionMove->MontageState.CurrentPosition = StartingMontagePosition;

		float RemainingUnscaledMontageSeconds{StartingMontagePosition};
		if (TraversalCheckResult.PlayRate > 0.f)
		{
			// playing forwards, so working towards the end of the montage
			RemainingUnscaledMontageSeconds = MontageDuration - StartingMontagePosition;
		}

		AnimRootMotionMove->DurationMs = (RemainingUnscaledMontageSeconds / FMath::Abs(TraversalCheckResult.PlayRate)) *
			1000.f;

		MoverComponent->QueueLayeredMove(AnimRootMotionMove);
	}

	bDoingTraversalAction = true;
	CapsuleComponent->IgnoreComponentWhenMoving(TraversalCheckResult.HitComponent,
	                                            true);

	MoverComponent->QueueNextMode(DefaultModeNames::Flying);

	OnTraversalEvent.Broadcast(ETraversalEventType::Triggered);
}

void UGASPTraversalComponent::Server_Traversal_Implementation(
	const FTraversalCheckResult TraversalRep)
{
	Traversal_ServerImplementation(TraversalRep);
}

void UGASPTraversalComponent::Multicast_Traversal_Implementation(const FTraversalCheckResult TraversalRep)
{
	TraversalCheckResult = TraversalRep;
	PerformTraversalAction();
}

FComputeLedgeData
UGASPTraversalComponent::ComputeLedgeData(FHitResult& HitResult) const
{
	HitResult.ImpactPoint -= (HitResult.ImpactPoint - FVector::PointPlaneProject(
		HitResult.GetComponent()->Bounds.Origin, HitResult.ImpactPoint, HitResult.ImpactNormal)).GetSafeNormal();

	const auto StartNormal{HitResult.ImpactNormal};
	const float TraceLength = HitResult.GetComponent()->Bounds.SphereRadius * 2;

	const auto AbsoluteObjectUpVector =
		HitResult.GetComponent()->GetUpVector() *
		FMath::Sign(FVector::DotProduct(HitResult.GetComponent()->GetUpVector(), CharacterOwner->GetActorUpVector()));

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
			if (!TraceWidth(HitResult,
			                FVector::CrossProduct(AbsoluteObjectUpVector,
			                                      HitResult.ImpactNormal)) ||
				!TraceWidth(HitResult,
				            -FVector::CrossProduct(AbsoluteObjectUpVector,
				                                   HitResult.ImpactNormal)))
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
	if (!TraceAlongHitPlane(HitResult, AbsoluteObjectUpVector, TraceLength,
	                        OutHit))
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
	auto [bFoundFrontLedge, bFoundBackLedge, StartLedgeLocation, StartLedgeNormal,
		EndLedgeLocation, EndLedgeNormal] = ComputeLedgeData(HitResult);

	TraversalData.bHasFrontLedge = bFoundFrontLedge;
	TraversalData.FrontLedgeLocation = StartLedgeLocation;
	TraversalData.FrontLedgeNormal = StartLedgeNormal;

	TraversalData.bHasBackLedge = bFoundBackLedge;
	TraversalData.BackLedgeLocation = EndLedgeLocation;
	TraversalData.BackLedgeNormal = EndLedgeNormal;
}

FTraceCorners
UGASPTraversalComponent::TraceCorners(FHitResult HitResult, const FVector TraceDirection, const float TraceLength) const
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
	const auto CrossPoint =
		FVector::CrossProduct(HitResult.ImpactNormal, TraceDirection);
	const auto NormalizedPoint =
		FVector::CrossProduct(CrossPoint, HitResult.ImpactNormal)
		.GetSafeNormal(.0001f);
	const auto DeltaPoint = HitResult.ImpactPoint - HitResult.ImpactNormal;
	const auto TraceStart = TraceLength * NormalizedPoint + DeltaPoint;

	return HitResult.GetComponent()->LineTraceComponent(
		OutHit, TraceStart, TraceStart + 1.5 * (DeltaPoint - TraceStart),
		ECC_Visibility, GetQueryParams(), FCollisionResponseParams(),
		FCollisionObjectQueryParams());
}

bool UGASPTraversalComponent::TraceWidth(FHitResult HitResult, const FVector Direction) const
{
	const auto StartLocation = Direction * (MinLedgeWidth / 2.f) + HitResult.ImpactPoint;
	const auto FrontLedgeNormalDepth = HitResult.ImpactNormal * MinFrontLedgeDepth;

	return HitResult.GetComponent()->LineTraceComponent(HitResult, StartLocation + FrontLedgeNormalDepth,
	                                                    StartLocation - FrontLedgeNormalDepth, GetQueryParams());
}

bool UGASPTraversalComponent::SweepTrace(const UWorld* World, FHitResult& HitResult, const FVector& Start,
                                         const FVector& End, const float CapsuleRadius, const float TraceHalfHeight,
                                         const ECollisionChannel CollisionChannel)
{
	return World->SweepSingleByChannel(HitResult, Start, End, FQuat::Identity, CollisionChannel,
	                                   FCollisionShape::MakeCapsule(CapsuleRadius, TraceHalfHeight),
	                                   GetQueryParams());
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
