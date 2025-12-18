#pragma once

#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "Engine/StreamableManager.h"
#include "PoseSearch/PoseSearchHistory.h"
#include "Types/StructTypes.h"
#include "GASPTraversalComponent.generated.h"

class UGASPMoverComponent;
class UGASPAnimInstance;
class UCapsuleComponent;
class USplineComponent;
class UMotionWarpingComponent;
class AGASPCharacter;

/**
 * Input structure for the traversal chooser system that determines which traversal animations to play
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
	uint8 bHasFrontLedge : 1{false};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	uint8 bHasBackLedge : 1{false};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Traversal")
	uint8 bHasBackFloor : 1{false};

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
			TEXT(
				"Found Front Ledge: %hhd\nFound Back Ledge: %hhd\nStart Ledge Location:%s\nStart Ledge Normal: "
				"%s\nEnd Ledge Location: %s\nEnd Ledge Normal: %s"), bFoundFrontLedge, bFoundBackLedge,
			*StartLedgeLocation.ToString(), *StartLedgeNormal.ToString(),
			*EndLedgeLocation.ToString(), *EndLedgeNormal.ToString());
	}
};

/**
 * Component that handles character traversal over and around obstacles such as
 * vaulting, climbing, jumping, etc. Uses a combination of collision detection,
 * motion warping, and animation selection to achieve realistic movement.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GASP_API UGASPTraversalComponent : public UActorComponent
{
	GENERATED_BODY()

	/** Timer handle for ending traversal actions */
	FTimerHandle TraversalEndHandle;

	/** Manager for asynchronous loading of assets */
	FStreamableManager StreamableManager;

	/** Handle for the current streaming request */
	TSharedPtr<FStreamableHandle> StreamableHandle;

public:
	// Sets default values for this component's properties
	UGASPTraversalComponent();

protected:
	/**
	 * Called when the game starts or when spawned
	 */
	virtual void BeginPlay() override;

	virtual void InitializeComponent() override;

	/**
	 * Configures network replication for this component
	 * @param OutLifetimeProps Array of lifetime replicated properties
	 */
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	/** 
	 * Processes motion warping for a traversal movement by extracting curve values
	 * @param CurveName Name of the curve to retrieve values from
	 * @param WarpTarget The motion warping target name
	 * @param Value Reference to store the retrieved value from the animation curve
	 */
	void ExtractWarpTargetCurveValue(const FName CurveName, const FName WarpTarget, float& Value) const;

	/** 
	 * Updates motion warping targets based on current traversal results
	 * Sets up front ledge, back ledge, and back floor targets for motion warping
	 */
	UFUNCTION(BlueprintCallable, Category="Traversal")
	void UpdateWarpTargets();

	/** 
	 * Implements traversal action processing on the server
	 * @param TraversalRep The replicated traversal result data
	 */
	UFUNCTION(BlueprintCallable, Category="Traversal")
	void Traversal_ServerImplementation(const FTraversalCheckResult TraversalRep);

	/** 
	 * Replication callback for TraversalCheckResult
	 * Triggers traversal action execution when replicated from server
	 */
	UFUNCTION(BlueprintCallable, Category = "Traversal")
	void OnRep_TraversalResult();

	/** 
	 * Handles the completion of a traversal action 
	 */
	UFUNCTION()
	void OnCompleteTraversal(FName NotifyName);

	/** Cached traversal check results from the most recent traversal attempt */
	UPROPERTY(BlueprintReadOnly, Category="Traversal", ReplicatedUsing=OnRep_TraversalResult, Transient)
	FTraversalCheckResult TraversalCheckResult{};

	/** Whether the character is currently performing a traversal action */
	UPROPERTY(BlueprintReadOnly, Category="Traversal", Transient)
	uint8 bDoingTraversalAction : 1{false};

	/** Tags that prevent specific traversal actions from being selected */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Traversal")
	FName BannedTag{TEXT("Banned")};

	/** Minimum required width of a ledge for traversal in units */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="Traversal")
	float MinLedgeWidth{30.f};

	/** Minimum required depth of the front ledge for traversal in units */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="Traversal")
	float MinFrontLedgeDepth{37.522631f};

	/** Reference to the data table used for choosing traversal animations */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Traversal")
	TSoftObjectPtr<class UChooserTable> TraversalAnimationsChooserTable;

	/**
	 * Identifies front and back ledges of an obstacle from a hit result
	 * @param HitResult The hit result from the initial obstacle detection
	 * @return Structure containing computed ledge data
	 */
	UFUNCTION(BlueprintCallable, Category="Traversal")
	FComputeLedgeData ComputeLedgeData(FHitResult& HitResult) const;

	/**
	 * Populates traversal data with ledge information
	 * @param HitResult The hit result from the initial obstacle detection
	 * @param TraversalData Output structure to populate with ledge information
	 */
	UFUNCTION(BlueprintCallable, Category="Traversal")
	void TryAndCalculateLedges(FHitResult& HitResult, FTraversalCheckResult& TraversalData);

	/**
	 * Traces corners of an obstacle to detect edges
	 * @param HitResult The hit result from the initial obstacle detection
	 * @param TraceDirection Direction to trace along the obstacle
	 * @param TraceLength Length of the trace
	 * @return Structure containing detected corner information
	 */
	UFUNCTION(BlueprintCallable, Category="Traversal")
	FTraceCorners TraceCorners(FHitResult HitResult, const FVector TraceDirection, const float TraceLength) const;

	/**
	 * Traces along the plane of a hit to find ledges
	 * @param HitResult The hit result to trace from
	 * @param TraceDirection Direction to trace along the hit plane
	 * @param TraceLength Length of the trace
	 * @param OutHit Output hit result containing detected ledge
	 * @return True if a ledge was detected, false otherwise
	 */
	UFUNCTION(BlueprintCallable, Category="Traversal")
	bool TraceAlongHitPlane(const FHitResult& HitResult, const FVector TraceDirection, const float TraceLength,
	                        FHitResult& OutHit) const;

	/**
	 * Checks if an obstacle has sufficient width for traversal
	 * @param HitResult The hit result from the initial obstacle detection
	 * @param Direction Direction to check width
	 * @return True if the obstacle has sufficient width, false otherwise
	 */
	UFUNCTION(BlueprintCallable, Category="Traversal")
	bool TraceWidth(FHitResult HitResult, const FVector Direction) const;


	/**
	 * Performs a capsule sweep trace in the world
	 * @param World The world context
	 * @param HitResult Output hit result
	 * @param Start Start location of the trace
	 * @param End End location of the trace
	 * @param CapsuleRadius Radius of the capsule trace
	 * @param TraceHalfHeight Half-height of the capsule trace
	 * @param CollisionChannel Collision channel to trace against
	 * @return True if the trace hit something, false otherwise
	 */
	UFUNCTION(BlueprintCallable, Category="Traversal")
	bool SweepTrace(const UWorld* World, FHitResult& HitResult, const FVector& Start, const FVector& End,
	                const float CapsuleRadius, const float TraceHalfHeight, ECollisionChannel CollisionChannel);

public:
	/**
	 * Returns pre-configured collision query parameters for traversal traces
	 * Sets up character and child actor ignores for accurate traversal detection
	 * @return Configured collision query parameters
	 */
	FCollisionQueryParams GetQueryParams() const;

	/**
	 * Attempts to perform a traversal action based on input parameters
	 * Performs environment detection, animation selection, and initiates traversal
	 * @param CheckInputs The inputs required to perform the traversal check
	 * @return Result structure indicating success or failure
	 */
	UFUNCTION(BlueprintCallable, Category="Traversal")
	FTraversalResult TryTraversalAction(FTraversalCheckInputs CheckInputs);

	/**
	 * Executes the traversal action using the selected animation
	 * Sets up motion warping and plays the appropriate montage
	 * Can be overridden in Blueprints for custom traversal behavior
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Traversal")
	void PerformTraversalAction();

	/**
	 * Replicates traversal actions from client to server
	 * @param TraversalRep The traversal result data to be replicated
	 */
	UFUNCTION(Reliable, Server, Category="Traversal")
	void Server_Traversal(FTraversalCheckResult TraversalRep);
	UFUNCTION(Reliable, NetMulticast, Category="Traversal")
	void Multicast_Traversal(FTraversalCheckResult TraversalRep);

	/**
	 * Checks whether the character is currently performing a traversal action
	 * @return True if the character is in a traversal action, false otherwise
	 */
	UFUNCTION(BlueprintPure, Category="Traversal")
	bool IsDoingTraversal() const;

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
	TWeakObjectPtr<UGASPAnimInstance> AnimInstance{};
};
