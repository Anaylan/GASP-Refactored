#pragma once

#include "GameplayTagContainer.h"
#include "Interfaces/GASPTargetedActor.h"
#include "MoverSimulationTypes.h"
#include "Tasks/TaskDataTypes.h"
#include "Types/EnumTypes.h"
#include "Types/MovementTypes.h"
#include "Types/StructTypes.h"
#include "Types/TagTypes.h"
#include "GASPCharacter.generated.h"

class UCharacterTask_Ragdoll;
struct FMontageBlendSettings;
enum class EStanceMode : uint8;
class UNavMoverComponent;
class UGASPMoverComponent;
class UGASPCharacterSettings;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStateChanged, FGameplayTag, OldGameplayTag, FGameplayTag,
                                             NewGameplayTag);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGameplayTagContainerChanged, FGameplayTagContainer,
                                             OldGameplayTagContainer, FGameplayTagContainer, NewGameplayTagContainer);

UCLASS()
class GASP_API AGASPCharacter : public APawn, public IMoverInputProducerInterface, public IGASPTargetedActor,
                                public IGameplayTagAssetInterface, public IGameplayTaskOwnerInterface
{
	GENERATED_BODY()

	/** The main skeletal mesh associated with this Character (optional sub-object). */
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> Mesh;
	/** The CapsuleComponent being used for movement collision (by CharacterMovement). Always treated as being vertically aligned in simple collision check functions. */
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<class UCapsuleComponent> CapsuleComponent;
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<class UPhysicsControlComponent> PhysicsControlComponent;
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<class UGASPOverrideModeManager> OverrideModeManager;
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<class UGASPCharacterInteractionComponent> InteractionComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintGetter=GetPoseMode, ReplicatedUsing=OnRep_PoseMode, Transient)
	FGameplayTag PoseMode{PoseModeTags::Default};
	UPROPERTY(BlueprintGetter=GetLocomotionAction, ReplicatedUsing=OnRep_LocomotionAction, Transient)
	FGameplayTag LocomotionAction{FGameplayTag::EmptyTag};

	UPROPERTY(BlueprintGetter=GetOverlayMode, BlueprintSetter=SetOverlayMode,
		ReplicatedUsing=OnRep_OverlayMode, Transient)
	FGameplayTagContainer OverlayTagContainer{};

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TObjectPtr<UGASPCharacterSettings> Settings;

	UPROPERTY(BlueprintReadOnly)
	float DebugAngle{0.f};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UMotionWarpingComponent> MotionWarpingComponent{};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UGASPTraversalComponent> TraversalComponent{};

	UPROPERTY(BlueprintReadOnly)
	FGASPMoverInputs MoverInputs_PostSim{};

	UFUNCTION()
	void OnBasedMovementApplied(const FTransform& TransformDelta, const FMoverTimeStep& TimeStep);

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void PostInitializeComponents() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult) override;


	FName PhysicsProfileName{TEXT("")};

	/**
	 * Limbs that must not interpenetrate while the Ragdoll physics profile is active.
	 *
	 * Collision is enabled between every pair of distinct groups when that profile is applied and
	 * disabled again for any other, so a biped lists two groups and a six-legged character lists
	 * six. Bones inside a single group are left to whatever the physics asset says.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ragdolling")
	TArray<FGASPBodyGroup> RagdollSelfCollisionGroups;

	FName GetPhysicsProfile(int32 Index);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> PhysicsProfiles;

	UFUNCTION()
	virtual void OnMovementModeChanged(const FName& PreviousMovementModeName, const FName& NewMovementModeName);

	// Cached to restore the mesh transform after ragdoll without assuming default values.
	FTransform MeshRelativeTransformCache{FTransform::Identity};
	FTransform BasedMovementDelta{FTransform::Identity};
	UPROPERTY(BlueprintReadOnly, Category = "Input")
	FRotator LastControlRotation{FRotator::ZeroRotator};

	UFUNCTION(BlueprintPure, Category = "Traversal")
	struct FTraversalCheckInputs GetTraversalCheckInputs() const;

	UFUNCTION(BlueprintPure, Category = "Input")
	bool HasFullMovementInput() const;

	void GetMovementDirectionAndOffset(EMovementDirection& MovementDirection, float& RotationOffset);

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

	virtual FGameplayTagContainer BP_GetOwnedGameplayTags() const override;

public:
	FTransform GetMeshRelativeTransformCache() const { return MeshRelativeTransformCache; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input",
		meta = (BaseStruct = "/Script/GASP.GASPInputState"))
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

	inline class UCapsuleComponent* GetCapsuleComponent() const { return CapsuleComponent; }
	static FName CapsuleComponentName;

	UFUNCTION(BlueprintPure, Category = Mover)
	UGASPMoverComponent* GetMoverComponent() const { return CharacterMotionComponent; }

	UFUNCTION(BlueprintPure, Category = Mover)
	UNavMoverComponent* GetNavMoverComponent() const { return NavMoverComponent; }

	UFUNCTION(BlueprintPure, Category = Warping)
	UMotionWarpingComponent* GetMotionWarpingComponent() const { return MotionWarpingComponent; }

	UFUNCTION(BlueprintPure, Category = Physics)
	UPhysicsControlComponent* GetPhysicsControlComponent() const { return PhysicsControlComponent; }

	static FName MotionWarpingComponentName;
	static FName CharacterMotionComponentName;
	static FName NavMoverComponentName;
	static FName PhysicsControlComponentName;

	//~ Begin INavAgentInterface Interface
	virtual FVector GetNavAgentLocation() const override;
	//~ End INavAgentInterface Interface

	virtual void UpdateNavigationRelevance() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D TwinStickAimDirection{FVector2D::ZeroVector};

	UFUNCTION(BlueprintPure)
	FORCEINLINE UGASPCharacterSettings* GetSettings() const
	{
		return Settings;
	}

	/**
	 * Settings accessor that is safe to dereference. Settings is an unset EditAnywhere property
	 * by default, and PostInitializeComponents runs before the BeginPlay ensure that guards it,
	 * so every internal read goes through here and falls back to the class defaults.
	 */
	const UGASPCharacterSettings* GetSettingsChecked() const;

	UFUNCTION(BlueprintPure)
	FTransform GetBasedMovementDelta() const { return BasedMovementDelta; };

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;
	virtual bool HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;
	virtual bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;
	virtual bool HasMatchingGameplayTag(FGameplayTag TagToCheck) const override;

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

	UFUNCTION()
	virtual void OnOverlayModeChanged(const FGameplayTagContainer OldOverlayMode,
	                                  const FGameplayTagContainer NewOverlayMode);
	UFUNCTION()
	virtual void OnPoseModeChanged(const FGameplayTag OldPoseMode, const FGameplayTag NewPoseMode);
	UFUNCTION()
	virtual void OnMovementStateChanged(const FGameplayTag OldMovementState, const FGameplayTag NewMovementState);

	TSubclassOf<UAnimInstance> GetLinkedAnimLayer(const class UChooserTable* DataTable) const;

	explicit AGASPCharacter(const FObjectInitializer& ObjectInitializer);
	AGASPCharacter() = default;

	virtual void Tick(float DeltaTime) override;

	/****************************
	 *		Movement States		*
	 ****************************/

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

	UFUNCTION(BlueprintCallable, Category=Character)
	virtual void Jump();

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
	FORCEINLINE FGameplayTag GetMovementMode() const;

	UFUNCTION(BlueprintPure)
	FORCEINLINE FGameplayTag GetStanceMode() const;

	UFUNCTION(BlueprintPure)
	FORCEINLINE UGASPTraversalComponent* GetTraversalComponent() const
	{
		return TraversalComponent;
	}

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Replicated)
	FGameplayTagContainer GameplayTags;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_TaskStates)
	FInstancedStructCollection TaskStates{};

private:
	UFUNCTION()
	virtual void OnRep_OverlayMode(const FGameplayTagContainer& OldOverlayMode);
	UFUNCTION()
	virtual void OnRep_PoseMode(const FGameplayTag& OldPoseMode);
	UFUNCTION()
	virtual void OnRep_LocomotionAction(const FGameplayTag& OldLocomotionAction);
	UFUNCTION()
	virtual void OnRep_TaskStates(const FInstancedStructCollection& OldTaskStates);

	// Ragdoll
public:
	UFUNCTION(BlueprintPure, Category = "GASP|Ragdoll")
	bool IsRagdolling() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Replicated)
	TObjectPtr<UCharacterTask_Ragdoll> RagdollTask;

	UFUNCTION(BlueprintCallable, Category = "GASP|Character", meta = (AutoCreateRefTerm = "InjuryState"))
	void StartRagdolling(const bool bStopActiveMontages, const FMontageBlendSettings& BlendSettings,
	                     const FGameplayTag& InjuryState);

	void SetPhysicsProfile(const FName NewPhysicsProfileName);

	UFUNCTION(BlueprintCallable, Category = "GASP|Character")
	bool StopRagdolling();

	UFUNCTION(BlueprintImplementableEvent)
	void OnStartRagdolling();
	UFUNCTION(BlueprintImplementableEvent)
	void OnStopRagdolling();

private:
	UFUNCTION(Server, Reliable)
	void ServerStartRagdolling(const bool bStopActiveMontages, const FMontageBlendSettings& BlendSettings,
	                           const FGameplayTag& InjuryState);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartRagdolling(const bool bStopActiveMontages, const FMontageBlendSettings& BlendSettings,
	                              const FGameplayTag& InjuryState);

	void StartRagdollingImplementation(const bool bStopActiveMontages, const FMontageBlendSettings& BlendSettings,
	                                   const FGameplayTag& InjuryState);

	UFUNCTION(Server, Reliable)
	void ServerStopRagdolling();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStopRagdolling();

	void StopRagdollingImplementation();
	void TryPlayGetUpMontage();

	// Gameplay tasks
public:
	/** Finds tasks component for given GameplayTask, Task.GetGameplayTasksComponent() may not be initialized at this point! */
	virtual UGameplayTasksComponent* GetGameplayTasksComponent(const UGameplayTask& Task) const override;

	/** Get owner of a task or default one when task is null */
	virtual AActor* GetGameplayTaskOwner(const UGameplayTask* Task) const override;

	/** Get "body" of task's owner / default, having location in world (e.g. Owner = AIController, Avatar = Pawn) */
	virtual AActor* GetGameplayTaskAvatar(const UGameplayTask* Task) const override
	{
		return GetGameplayTaskOwner(Task);
	}

	/** Get default priority for running a task */
	virtual uint8 GetGameplayTaskDefaultPriority() const override { return FGameplayTasks::DefaultPriority; }

	/** Notify called after GameplayTask finishes initialization (not active yet) */
	virtual void OnGameplayTaskInitialized(UGameplayTask& Task) override;

	/** Notify called after GameplayTask changes state to Active (initial activation or resuming) */
	virtual void OnGameplayTaskActivated(UGameplayTask& Task) override;

	/** Notify called after GameplayTask changes state from Active (finishing or pausing) */
	virtual void OnGameplayTaskDeactivated(UGameplayTask& Task) override;
};
