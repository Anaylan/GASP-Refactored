#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "PoseSearch/PoseSearchHistory.h"
#include "Types/StructTypes.h"
#include "GASPTraversalComponent.generated.h"

class AGASPCharacter;
class UGASPMoverComponent;
class UAnimInstance;
class UCapsuleComponent;
class USplineComponent;
class UMotionWarpingComponent;
class AActor;
class UChooserTable;

UENUM(BlueprintType)
enum class ETraversalEventType : uint8
{
	Triggered, Done
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTraversalSimpleDelegate);

/**
 * Input structure for the traversal chooser system that determines which
 * traversal animations to play
 */
USTRUCT(BlueprintType)
struct GASP_API FTraversalChooserInput
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traversal")
	FGameplayTag ActionType{FGameplayTag::EmptyTag};
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traversal")
	FGameplayTagContainer StateContainer{};
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traversal")
	float Speed{0.0f};
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traversal")
	float ObstacleHeight{0.0f};
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Traversal")
	float ObstacleDepth{0.0f};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	float BackLedgeHeight{0.f};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	float DistanceToLedge{0.f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	uint8 bHasFrontLedge : 1 {false};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	uint8 bHasBackLedge : 1 {false};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	uint8 bHasBackFloor : 1 {false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	FPoseHistoryReference PoseHistory{};
};

/**
 * Output structure returned by the traversal chooser system
 */
USTRUCT(BlueprintType)
struct GASP_API FTraversalChooserOutput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag ActionType{FGameplayTag::EmptyTag};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MontageStartTime{0.f};
};

/**
 * Input parameters for performing a traversal check
 */
USTRUCT(BlueprintType)
struct GASP_API FTraversalCheckInputs
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	FVector TraceForwardDirection{FVector::ZeroVector};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	float TraceForwardDistance{0.f};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	FVector TraceOriginOffset{FVector::ZeroVector};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	FVector TraceEndOffset{FVector::ZeroVector};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	float TraceRadius{0.f};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	float TraceHalfHeight{0.f};
};

/**
 * Result structure for traversal action attempts
 */
USTRUCT(BlueprintType)
struct GASP_API FTraversalResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	bool bTraversalCheckFailed{false};
	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	bool bMontageSelectionFailed{false};
};

/**
 * Data structure containing information about traced corners during traversal
 */
USTRUCT(BlueprintType)
struct GASP_API FTraceCorners
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	FVector OfsettedCornerPoint{FVector::ZeroVector};
	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	bool bCloseToCorner{false};
	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	float DistanceToCorner{0.f};
};

/**
 * Data structure for ledge detection results
 */
USTRUCT(BlueprintType)
struct GASP_API FComputeLedgeData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	bool bFoundFrontLedge{false};
	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	bool bFoundBackLedge{false};
	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	FVector StartLedgeLocation{FVector::ZeroVector};
	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	FVector StartLedgeNormal{FVector::ZeroVector};
	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	FVector EndLedgeLocation{FVector::ZeroVector};
	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	FVector EndLedgeNormal{FVector::ZeroVector};

	/**
	 * Converts the ledge data to a readable string format
	 * @return String representation of the ledge data
	 */
	FORCEINLINE FString ToString() const
	{
		return FString::Printf(
			TEXT("Found Front Ledge: %hhd\nFound Back Ledge: %hhd\nStart Ledge "
				"Location:%s\nStart Ledge Normal: "
				"%s\nEnd Ledge Location: %s\nEnd Ledge Normal: %s"),
			bFoundFrontLedge, bFoundBackLedge, *StartLedgeLocation.ToString(),
			*StartLedgeNormal.ToString(), *EndLedgeLocation.ToString(),
			*EndLedgeNormal.ToString());
	}
};

/**
 * Component that handles character traversal over and around obstacles such as
 * vaulting, climbing, jumping, etc. Uses a combination of collision detection,
 * motion warping, and animation selection to achieve realistic movement.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GASP_API UGASPTraversalComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGASPTraversalComponent();

protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	/** Extracts curve values for a motion warping target. */
	void ExtractWarpTargetCurveValue(const FName CurveName,
	                                 const FName WarpTarget, float& Value) const;

	/** Configures front ledge, back ledge, and back floor motion warping targets. */
	UFUNCTION(BlueprintCallable, Category = "Traversal")
	void UpdateWarpTargets();

	/** Authoritative traversal action processing. */
	UFUNCTION(BlueprintCallable, Category = "Traversal")
	void Traversal_ServerImplementation(const FTraversalCheckResult TraversalRep);

	/** Replicates traversal check result to trigger local montage playback. */
	UFUNCTION(BlueprintCallable, Category = "Traversal")
	void OnRep_TraversalResult();

	UFUNCTION()
	void OnCompleteTraversal(FName NotifyName);

	UPROPERTY(BlueprintReadOnly, Category = "Traversal",
		ReplicatedUsing = OnRep_TraversalResult, Transient)
	FTraversalCheckResult TraversalCheckResult{};

	UPROPERTY(BlueprintReadOnly, Category = "Traversal", Transient)
	uint8 bDoingTraversalAction : 1 {false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	FName BannedTag{TEXT("Banned")};

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Traversal")
	float MinLedgeWidth{30.f};

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Traversal")
	float MinFrontLedgeDepth{37.522631f};

	/** Detects front and back ledges from an obstacle hit. */
	UFUNCTION(BlueprintCallable, Category = "Traversal")
	FComputeLedgeData ComputeLedgeData(FHitResult& HitResult) const;

	/** Populates traversal data with computed ledge geometry. */
	UFUNCTION(BlueprintCallable, Category = "Traversal")
	void TryAndCalculateLedges(FHitResult& HitResult,
	                           FTraversalCheckResult& TraversalData);

	/** Traces obstacle corners to identify geometry bounds. */
	UFUNCTION(BlueprintCallable, Category = "Traversal")
	FTraceCorners TraceCorners(FHitResult HitResult, const FVector TraceDirection,
	                           const float TraceLength) const;

	/** Traces along the hit surface plane to locate ledges. */
	UFUNCTION(BlueprintCallable, Category = "Traversal")
	bool TraceAlongHitPlane(const FHitResult& HitResult,
	                        const FVector TraceDirection, const float TraceLength,
	                        FHitResult& OutHit) const;

	/** Validates that the obstacle surface meets the minimum width requirement. */
	UFUNCTION(BlueprintCallable, Category = "Traversal")
	bool TraceWidth(FHitResult HitResult, const FVector Direction) const;

	/** Helper for capsule sweep collision queries. */
	UFUNCTION(BlueprintCallable, Category = "Traversal")
	bool SweepTrace(const UWorld* World, FHitResult& HitResult,
	                const FVector& Start, const FVector& End,
	                const float CapsuleRadius, const float TraceHalfHeight,
	                ECollisionChannel CollisionChannel);

public:
	/** Returns collision query parameters ignoring character and attached actors. */
	FCollisionQueryParams GetQueryParams() const;

	/** Evaluates environment clearance, selects an animation, and starts traversal if valid. */
	UFUNCTION(BlueprintCallable, Category = "Traversal")
	FTraversalResult TryTraversalAction(FTraversalCheckInputs CheckInputs);

	/** Executes selected traversal montage and configures motion warping. */
	UFUNCTION(BlueprintNativeEvent, Category = "Traversal")
	void PerformTraversalAction();

	UFUNCTION(Reliable, Server, Category = "Traversal")
	void Server_Traversal(FTraversalCheckResult TraversalRep);
	UFUNCTION(Reliable, NetMulticast, Category = "Traversal")
	void Multicast_Traversal(FTraversalCheckResult TraversalRep);

	UFUNCTION(BlueprintPure, Category = "Traversal")
	bool IsDoingTraversal() const;

	UPROPERTY(BlueprintAssignable)
	FOnTraversalSimpleDelegate OnTraversalStarted;

	UPROPERTY(BlueprintAssignable)
	FOnTraversalSimpleDelegate OnTraversalEnded;

protected:
	/** Performs initial forward trace to find an obstacle. */
	bool DetectObstacle(const FTraversalCheckInputs& CheckInputs,
	                    const FVector& StartLocation, FHitResult& OutHit);

	/** Verifies capsule clearance on the front ledge surface. */
	bool VerifyFrontLedgeClearance(const FVector& ActorLocation,
	                               FTraversalCheckResult& TraversalData,
	                               float CapsuleRadius, float CapsuleHalfHeight,
	                               FVector& OutCheckLocation);

	/** Measures obstacle depth and tests for back floor presence. */
	void AnalyzeObstacleDimensions(FTraversalCheckResult& TraversalData,
	                               const FVector& HasRoomCheckFrontLedgeLocation,
	                               float CapsuleRadius, float CapsuleHalfHeight);

	/** Evaluates traversal Chooser Table to select matching montage. */
	bool SelectTraversalMontage(FTraversalCheckResult& TraversalData);

#if WITH_EDITOR
	void DrawDebugLedges(const FTraversalCheckResult& TraversalData) const;
	void DrawDebugPerformance(const FTraversalCheckResult& TraversalData,
	                          double StartTime) const;
#endif

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<AGASPCharacter> CharacterOwner{};

	UPROPERTY(Transient)
	TWeakObjectPtr<UGASPMoverComponent> MoverComponent{};

	UPROPERTY(Transient)
	TWeakObjectPtr<UMotionWarpingComponent> MotionWarpingComponent{};

	UPROPERTY(Transient)
	TWeakObjectPtr<UCapsuleComponent> CapsuleComponent{};

	UPROPERTY(Transient)
	TWeakObjectPtr<USkeletalMeshComponent> MeshComponent{};

	UPROPERTY(Transient)
	TWeakObjectPtr<UAnimInstance> AnimInstance{};
};
