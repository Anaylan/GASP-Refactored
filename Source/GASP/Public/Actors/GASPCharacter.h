#pragma once

#include "GameplayTagContainer.h"
#include "Types/EnumTypes.h"
#include "MoverSimulationTypes.h"
#include "StructUtils/InstancedStruct.h"
#include "Types/MovementTypes.h"
#include "Types/TagTypes.h"
#include "Types/StructTypes.h"
#include "GASPCharacter.generated.h"

enum class EStanceMode : uint8;
class UNavMoverComponent;
class UGASPMoverComponent;
class UGASPCharacterSettings;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStateChanged, FGameplayTag, OldGameplayTag, FGameplayTag,
                                             NewGameplayTag);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGameplayTagContainerChanged, FGameplayTagContainer,
                                             OldGameplayTagContainer, FGameplayTagContainer, NewGameplayTagContainer);

UCLASS()
class GASP_API AGASPCharacter : public APawn, public IMoverInputProducerInterface
{
	GENERATED_BODY()

	/** The main skeletal mesh associated with this Character (optional sub-object). */
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> Mesh;
	/** The CapsuleComponent being used for movement collision (by CharacterMovement). Always treated as being vertically aligned in simple collision check functions. */
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<class UCapsuleComponent> CapsuleComponent;

	UPROPERTY(BlueprintGetter=GetMovementMode, ReplicatedUsing=OnRep_AllowedMovementMode, Transient)
	FGameplayTag AllowedMovementMode{MovementModeTags::Grounded};
	UPROPERTY(BlueprintGetter=GetStanceMode, Transient)
	FGameplayTag AllowedStanceMode{StanceTags::Standing};
	UPROPERTY(BlueprintGetter=GetPoseMode, ReplicatedUsing=OnRep_PoseMode, Transient)
	FGameplayTag PoseMode{PoseModeTags::Default};
	UPROPERTY(BlueprintGetter=GetLocomotionAction, ReplicatedUsing=OnRep_LocomotionAction, Transient)
	FGameplayTag LocomotionAction{FGameplayTag::EmptyTag};

	UPROPERTY(BlueprintGetter=GetOverlayMode, BlueprintSetter=SetOverlayMode, ReplicatedUsing=OnRep_OverlayMode,
		Transient)
	FGameplayTagContainer OverlayTagContainer{OverlayModeTags::Default};

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TObjectPtr<UGASPCharacterSettings> Settings;

	UPROPERTY(BlueprintReadOnly)
	float DebugAngle{0.f};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UMotionWarpingComponent> MotionWarpingComponent{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", Replicated)
	TObjectPtr<class UGASPTraversalComponent> TraversalComponent{};

	UPROPERTY(BlueprintReadOnly)
	FGASPMoverInputs MoverInputs_PostSim{};

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void PostInitializeComponents() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	virtual void OnMovementModeChanged(const FName& PreviousMovementModeName, const FName& NewMovementModeName);
	UFUNCTION()
	virtual void OnStanceChanged(EStanceMode OldStance, EStanceMode NewStance);

	UPROPERTY(BlueprintReadOnly, Replicated, Transient)
	FVector_NetQuantize RagdollTargetLocation{ForceInit};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State|Character", Transient)
	FRagdollingState RagdollingState;

	UFUNCTION(BlueprintPure)
	UAnimMontage* SelectGetUpMontage(bool bRagdollFacingUpward);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Input")
	FRotator LastControlRotation{FRotator::ZeroRotator};

	/** Please add a function description */
	UFUNCTION(BlueprintPure, Category = "Traversal")
	struct FTraversalCheckInputs GetTraversalCheckInputs() const;

	UFUNCTION(BlueprintPure, Category = "Input")
	bool HasFullMovementInput() const;

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
	virtual void RefreshMoverState();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input", meta = (BaseStruct = "/Script/GASP.GASPInputState"))
	FInstancedStruct PlayerInputState{FGASPInputState::StaticStruct()};

	UPROPERTY(BlueprintReadOnly, Category=Character)
	uint8 bJustPressedJump : 1;

	UFUNCTION(BlueprintPure)
	const FGASPMoverInputs& GetMoverState() const;

	UFUNCTION(BlueprintPure)
	virtual FVector GetMovementInputVector();

	UFUNCTION(BlueprintPure)
	virtual FVector GetOrientationIntent();

	UFUNCTION(BlueprintPure)
	virtual FRotator GetAimingRotation();

	UFUNCTION(BlueprintPure)
	virtual FGameplayTag GetAllowedRotationMode();
	UFUNCTION(BlueprintPure)
	virtual FGameplayTag GetAllowedGait();

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

	/** Name of the MotionWarpingComponent. */
	static FName MotionWarpingComponentName;

	/** Name of the CharacterMotionComponent. */
	static FName CharacterMotionComponentName;

	/** Name of the NavMoverComponent. */
	static FName NavMoverComponentName;

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
	UPROPERTY(BlueprintReadOnly)
	FGASPMoverInputs MoverInputs_PreSim{};

	UFUNCTION(BlueprintCallable, Category="Traversal")
	FTraversalResult TryTraversalAction() const;
	UFUNCTION(BlueprintPure, Category="Traversal")
	bool IsDoingTraversal() const;

	UPROPERTY(BlueprintAssignable)
	FOnGameplayTagContainerChanged OverlayContainerChanged;
	UPROPERTY(BlueprintAssignable)
	FOnStateChanged PoseModeChanged;
	UPROPERTY(BlueprintAssignable)
	FOnStateChanged StanceModeChanged;
	UPROPERTY(BlueprintAssignable)
	FOnStateChanged LocomotionActionChanged;
	UPROPERTY(BlueprintAssignable)
	FOnStateChanged MovementModeChanged;

	UFUNCTION(BlueprintNativeEvent)
	void OnOverlayModeChanged(const FGameplayTagContainer OldOverlayMode, const FGameplayTagContainer NewOverlayMode);
	UFUNCTION(BlueprintNativeEvent)
	void OnPoseModeChanged(const FGameplayTag OldPoseMode, const FGameplayTag NewPoseMode);

	void LinkAnimInstance(const class UChooserTable* DataTable) const;

	// Sets default values for this character's properties
	explicit AGASPCharacter(const FObjectInitializer& ObjectInitializer);
	AGASPCharacter() = default;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/****************************
	 *		Movement States		*
	 ****************************/
	UFUNCTION(BlueprintCallable)
	void SetMovementMode(const FGameplayTag NewMovementMode, const bool bForce = false);
	UFUNCTION(BlueprintCallable)
	void SetStanceMode(const FGameplayTag NewStanceMode, const bool bForce = false);

	UFUNCTION(BlueprintCallable)
	void SetOverlayMode(const FGameplayTagContainer NewOverlayMode);
	UFUNCTION(Server, Reliable)
	void Server_SetOverlayMode(const FGameplayTagContainer NewOverlayMode);

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
	 */
	UFUNCTION(BlueprintCallable, Category=Character)
	virtual void Jump();

	/** 
	 */
	UFUNCTION(BlueprintCallable, Category=Character)
	virtual void StopJumping();

	UFUNCTION(BlueprintPure)
	FORCEINLINE FGameplayTagContainer GetOverlayMode() const
	{
		return OverlayTagContainer;
	}

	UFUNCTION(BlueprintPure)
	FORCEINLINE FGameplayTag GetPoseMode() const
	{
		return PoseMode;
	}

	UFUNCTION(BlueprintPure)
	FORCEINLINE FGameplayTag GetLocomotionAction() const
	{
		return LocomotionAction;
	}

	UFUNCTION(BlueprintPure)
	FORCEINLINE FGameplayTag GetMovementMode() const
	{
		return AllowedMovementMode;
	}

	UFUNCTION(BlueprintPure)
	FORCEINLINE FGameplayTag GetStanceMode() const
	{
		return AllowedStanceMode;
	}

	UPROPERTY(BlueprintReadOnly, Replicated)
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
	virtual void OnRep_OverlayMode(const FGameplayTagContainer& OldOverlayMode);
	UFUNCTION()
	virtual void OnRep_PoseMode(const FGameplayTag& OldPoseMode);
	UFUNCTION()
	virtual void OnRep_AllowedMovementMode(const FGameplayTag& OldMovementMode);
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

	// TODO: maybe we should move this method to the mover component
	void RefreshRagdolling(float DeltaTime);

	FVector RagdollTraceGround(bool& bGrounded) const;

	void ConstraintRagdollSpeed() const;
};
