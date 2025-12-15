#pragma once

#include "GameplayTagContainer.h"
#include "Types/EnumTypes.h"
#include "MoverSimulationTypes.h"
#include "Types/MovementTypes.h"
#include "Types/TagTypes.h"
#include "Types/StructTypes.h"
#include "GASPCharacter.generated.h"

class UNavMoverComponent;
class UGASPMoverComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStateChanged, FGameplayTag, OldGameplayTag, FGameplayTag,
                                             NewGameplayTag);

UCLASS()
class GASP_API AGASPCharacter : public APawn, public IMoverInputProducerInterface
{
	GENERATED_BODY()

	UFUNCTION(BlueprintSetter)
	void SetMovementMode(const FGameplayTag NewMovementMode, bool bForce = false);
	UFUNCTION(Server, Reliable)
	void Server_SetMovementMode(const FGameplayTag NewMovementMode);

	/** The main skeletal mesh associated with this Character (optional sub-object). */
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> Mesh;
	/** The CapsuleComponent being used for movement collision (by CharacterMovement). Always treated as being vertically aligned in simple collision check functions. */
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<class UCapsuleComponent> CapsuleComponent;

protected:
	UPROPERTY(EditAnywhere, Category="PoseSearchData|Choosers", BlueprintReadOnly)
	TObjectPtr<class UChooserTable> OverlayTable{nullptr};
	UPROPERTY(EditAnywhere, Category="PoseSearchData|Choosers", BlueprintReadOnly)
	TObjectPtr<UChooserTable> PosesTable{nullptr};
	UPROPERTY(EditAnywhere, Category="PoseSearchData|Choosers", BlueprintReadOnly)
	TObjectPtr<UChooserTable> RotationCurveTable{nullptr};

	UPROPERTY(BlueprintReadOnly)
	float DebugAngle{0.f};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UMotionWarpingComponent> MotionWarpingComponent{};
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", Replicated)
	TObjectPtr<class UGASPTraversalComponent> TraversalComponent{};

	UPROPERTY(BlueprintGetter=GetMoverInputs_PreSim, Replicated)
	FGASPMoverInputs MoverCustomInputs_PreSim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	FGASPInputState PlayerInputState;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void PostInitializeComponents() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	virtual void OnMovementModeChanged(const FName& PreviousMovementModeName, const FName& NewMovementModeName);
	UFUNCTION()
	virtual void OnPreSimulateTick(const FMoverTimeStep& TimeStep, const FMoverInputCmdContext& InputCmd);
	UFUNCTION()
	virtual void OnStanceChanged(EStanceMode OldStance, EStanceMode NewStance);

	UPROPERTY(BlueprintReadOnly, Transient)
	FGameplayTag Gait{GaitTags::Run};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_RotationMode, Transient)
	FGameplayTag RotationMode{RotationTags::Strafe};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_MovementMode, Transient)
	FGameplayTag MovementMode{MovementModeTags::Grounded};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_StanceMode, Transient)
	FGameplayTag StanceMode{StanceTags::Standing};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_OverlayMode, Transient)
	FGameplayTag OverlayMode{OverlayModeTags::Default};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_PoseMode, Transient)
	FGameplayTag PoseMode{PoseModeTags::Default};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_LocomotionAction, Transient)
	FGameplayTag LocomotionAction{FGameplayTag::EmptyTag};

	UPROPERTY(BlueprintReadOnly, Transient)
	FGameplayTag PreviousMovementMode{MovementModeTags::Grounded};

	UPROPERTY(BlueprintReadOnly, Replicated, Transient)
	FVector_NetQuantize RagdollTargetLocation{ForceInit};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Character", Transient)
	FRagdollingState RagdollingState;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State|Character")
	TObjectPtr<UAnimMontage> GetUpMontageFront{};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State|Character")
	TObjectPtr<UAnimMontage> GetUpMontageBack{};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State|Character")
	uint8 bLimitInitialRagdollSpeed : 1{false};

	UFUNCTION(BlueprintPure)
	UAnimMontage* SelectGetUpMontage(bool bRagdollFacingUpward);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Input")
	FRotator LastControlRotation{FRotator::ZeroRotator};

	/** Please add a function description */
	UFUNCTION(BlueprintPure, Category = "Traversal")
	struct FTraversalCheckInputs GetTraversalCheckInputs() const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta=(ClampMin="0.0", ClampMax="1.0"))
	float AnalogMovementThreshold{.7f};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	EAnalogStickBehaviorMode MovementStickMode{EAnalogStickBehaviorMode::FixedSingleGait};

	UFUNCTION(BlueprintPure, Category = "Input")
	bool HasFullMovementInput() const;

	UFUNCTION(BlueprintPure, Category = "Input")
	FVector2D GetMovementInputScaleValue(const FVector2D InVector) const;
	void GetMovementDirectionAddOffset(EMovementDirection& MovementDirection, float& RotationOffset);

	// Entry point for input production.
	virtual void ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult) override;

	UPROPERTY(BlueprintReadOnly)
	float ControlRotationRate{0.f};
	UPROPERTY(BlueprintReadOnly)
	uint8 TwinStickMode : 1{false};
	UPROPERTY(BlueprintReadOnly)
	FRotator TwinStickAimRotation{FRotator::ZeroRotator};

	virtual void RefreshFloorValues();
	virtual void RefreshControlRotationRate(const float DeltaTime);
	virtual void RefreshTwinStickMode();
	virtual void RefreshRotationMode();
	virtual void RefreshSlidingAudio();

public:
	UPROPERTY(BlueprintReadOnly, Category=Character)
	uint8 bJustPressedJump : 1;

	UFUNCTION(BlueprintPure)
	const FGASPMoverInputs& GetMoverState() const;

	UFUNCTION(BlueprintPure)
	const FGASPMoverInputs& GetMoverInputs_PreSim()
	{
		return MoverCustomInputs_PreSim;
	}

	UFUNCTION(BlueprintPure)
	virtual FVector GetMovementInputVector();

	UFUNCTION(BlueprintPure)
	virtual FVector GetOrientationIntent();

	UFUNCTION(BlueprintPure)
	virtual FRotator GetAimingRotation();

	/** Returns Mesh subobject **/
	inline class USkeletalMeshComponent* GetMesh() const { return Mesh; }

	/** Name of the MeshComponent. Use this name if you want to prevent creation of the component (with ObjectInitializer.DoNotCreateDefaultSubobject). */
	static FName MeshComponentName;

	/** Returns CapsuleComponent subobject **/
	inline class UCapsuleComponent* GetCapsuleComponent() const { return CapsuleComponent; }

	/** Name of the CapsuleComponent. */
	static FName CapsuleComponentName;

	// Accessor for the actor's movement component
	UFUNCTION(BlueprintPure, Category = Mover)
	UGASPMoverComponent* GetMoverComponent() const { return CharacterMotionComponent; }

	//~ Begin INavAgentInterface Interface
	virtual FVector GetNavAgentLocation() const override;
	//~ End INavAgentInterface Interface

	virtual void UpdateNavigationRelevance() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D TwinStickAimDirection{FVector2D::ZeroVector};

protected:
	UPROPERTY(Category = Movement, VisibleAnywhere, BlueprintReadOnly, Transient, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGASPMoverComponent> CharacterMotionComponent;

	/** Holds functionality for nav movement data and functions */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Nav Movement")
	TObjectPtr<UNavMoverComponent> NavMoverComponent;

public:
	void RefreshGait();

	UFUNCTION(BlueprintCallable, Category="Traversal")
	FTraversalResult TryTraversalAction() const;
	UFUNCTION(BlueprintPure, Category="Traversal")
	bool IsDoingTraversal() const;

	UPROPERTY(BlueprintAssignable)
	FOnStateChanged OverlayModeChanged;
	UPROPERTY(BlueprintAssignable)
	FOnStateChanged PoseModeChanged;
	UPROPERTY(BlueprintAssignable)
	FOnStateChanged GaitChanged;
	UPROPERTY(BlueprintAssignable)
	FOnStateChanged RotationModeChanged;
	UPROPERTY(BlueprintAssignable)
	FOnStateChanged StanceModeChanged;
	UPROPERTY(BlueprintAssignable)
	FOnStateChanged LocomotionActionChanged;
	UPROPERTY(BlueprintAssignable)
	FOnStateChanged MovementModeChanged;

	UFUNCTION(BlueprintNativeEvent)
	void OnOverlayModeChanged(const FGameplayTag OldOverlayMode, const FGameplayTag NewOverlayMode);
	UFUNCTION(BlueprintNativeEvent)
	void OnPoseModeChanged(const FGameplayTag OldPoseMode, const FGameplayTag NewPoseMode);

	void LinkAnimInstance(const UChooserTable* DataTable, const FGameplayTag OldState, const FGameplayTag State);

	// Sets default values for this character's properties
	explicit AGASPCharacter(const FObjectInitializer& ObjectInitializer);
	AGASPCharacter() = default;

	// Called every frame
	virtual void Tick(float DeltaTime) override;


	/****************************
	 *		Movement States		*
	 ****************************/
	UFUNCTION(BlueprintCallable)
	void SetGait(const FGameplayTag NewGait, bool bForce = false);
	UFUNCTION(Server, Reliable)
	void Server_SetGait(const FGameplayTag NewGait);
	UFUNCTION(BlueprintCallable)
	void SetRotationMode(const FGameplayTag NewRotationMode, const bool bForce = false);
	UFUNCTION(Server, Reliable)
	void Server_SetRotationMode(const FGameplayTag NewRotationMode);

	UFUNCTION(BlueprintCallable)
	void SetStanceMode(const FGameplayTag NewStanceMode, const bool bForce = false);
	UFUNCTION(Server, Reliable)
	void Server_SetStanceMode(const FGameplayTag NewStanceMode);

	UFUNCTION(BlueprintCallable)
	void SetOverlayMode(const FGameplayTag NewOverlayMode, const bool bForce = false);
	UFUNCTION(Server, Reliable)
	void Server_SetOverlayMode(const FGameplayTag NewOverlayMode);

	UFUNCTION(BlueprintCallable)
	void SetPoseMode(const FGameplayTag NewPoseMode, const bool bForce = false);
	UFUNCTION(Server, Reliable)
	void Server_SetPoseMode(const FGameplayTag NewPoseMode);

	UFUNCTION(BlueprintCallable)
	void SetLocomotionAction(const FGameplayTag NewLocomotionAction, const bool bForce = false);
	UFUNCTION(Server, Reliable)
	void Server_SetLocomotionAction(const FGameplayTag NewLocomotionAction);

	UFUNCTION(BlueprintPure)
	virtual bool CanSprint();

	/** 
	 * Make the character jump on the next update.	 
	 * If you want your character to jump according to the time that the jump key is held,
	 * then you can set JumpMaxHoldTime to some non-zero value. Make sure in this case to
	 * call StopJumping() when you want the jump's z-velocity to stop being applied (such 
	 * as on a button up event), otherwise the character will carry on receiving the 
	 * velocity until JumpKeyHoldTime reaches JumpMaxHoldTime.
	 */
	UFUNCTION(BlueprintCallable, Category=Character)
	virtual void Jump();

	/** 
	 * Stop the character from jumping on the next update. 
	 * Call this from an input event (such as a button 'up' event) to cease applying
	 * jump Z-velocity. If this is not called, then jump z-velocity will be applied
	 * until JumpMaxHoldTime is reached.
	 */
	UFUNCTION(BlueprintCallable, Category=Character)
	virtual void StopJumping();

	UFUNCTION(BlueprintGetter)
	FORCEINLINE FGameplayTag GetOverlayMode() const
	{
		return OverlayMode;
	}

	UFUNCTION(BlueprintGetter)
	FORCEINLINE FGameplayTag GetLocomotionAction() const
	{
		return LocomotionAction;
	}

	UFUNCTION(BlueprintGetter)
	FORCEINLINE FGameplayTag GetGait() const
	{
		return Gait;
	}

	UFUNCTION(BlueprintGetter)
	FORCEINLINE FGameplayTag GetRotationMode() const
	{
		return RotationMode;
	}

	UFUNCTION(BlueprintGetter)
	FORCEINLINE FGameplayTag GetMovementMode() const
	{
		return MovementMode;
	}

	UFUNCTION(BlueprintGetter)
	FORCEINLINE FGameplayTag GetStanceMode() const
	{
		return StanceMode;
	}

	UPROPERTY(BlueprintReadOnly)
	FGameplayTagContainer StateContainer;

public:
	bool IsRagdollingAllowedToStart() const;

	const FRagdollingState& GetRagdollingState() const
	{
		return RagdollingState;
	}

	UFUNCTION(BlueprintCallable, Category = "GASP|Character")
	void StartRagdolling();

private:
	UFUNCTION(Server, Reliable)
	void ServerStartRagdolling();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartRagdolling();

	void StartRagdollingImplementation();

	UFUNCTION()
	virtual void OnRep_OverlayMode(const FGameplayTag& OldOverlayMode);
	UFUNCTION()
	virtual void OnRep_PoseMode(const FGameplayTag& OldPoseMode);
	UFUNCTION()
	virtual void OnRep_Gait(const FGameplayTag& OldGait);
	UFUNCTION()
	virtual void OnRep_StanceMode(const FGameplayTag& OldStanceMode);
	UFUNCTION()
	virtual void OnRep_MovementMode(const FGameplayTag& OldMovementMode);
	UFUNCTION()
	virtual void OnRep_RotationMode(const FGameplayTag& OldRotationMode);
	UFUNCTION()
	virtual void OnRep_LocomotionAction(const FGameplayTag& OldLocomotionAction);

public:
	bool IsRagdollingAllowedToStop() const;

	UFUNCTION(BlueprintCallable, Category = "GASP|Character", Meta = (ReturnDisplayName = "Success"))
	bool StopRagdolling();

	UFUNCTION(BlueprintImplementableEvent)
	void OnStartRagdolling();
	UFUNCTION(BlueprintImplementableEvent)
	void OnStopRagdolling();

private:
	UFUNCTION(Server, Reliable)
	void ServerStopRagdolling();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStopRagdolling();

	void StopRagdollingImplementation();

	void SetRagdollTargetLocation(const FVector& NewTargetLocation);

	UFUNCTION(Server, Unreliable)
	void ServerSetRagdollTargetLocation(const FVector_NetQuantize& NewTargetLocation);

	void RefreshRagdolling(float DeltaTime);

	FVector RagdollTraceGround(bool& bGrounded) const;

	void ConstraintRagdollSpeed() const;
};
