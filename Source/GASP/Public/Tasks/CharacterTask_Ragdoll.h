#pragma once

#include "CharacterTask.h"
#include "Animation/AnimMontage.h"
#include "Types/StructTypes.h"
#include "CharacterTask_Ragdoll.generated.h"

class UChooserTable;
class AGASPCharacter;
class UCapsuleComponent;
class UGASPMoverComponent;
class UPhysicsControlComponent;
class USkeletalMeshComponent;

USTRUCT(BlueprintType)
struct GASP_API FGASPGetUpInput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP")
	bool bRollingGetup{false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP")
	FGameplayTagContainer StateContainer{};
};

USTRUCT(BlueprintType)
struct GASP_API FGASPGetUpOutput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP")
	float MontageStartTime{0.f};
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCharacterRagdollSimpleSignature);

/**
 * Drives character ragdoll episode: enables Chaos physics on skeletal mesh, transitions mover
 * to UMovementMode_Ragdolling, and updates FRagdollingState in AGASPCharacter::TaskStates per frame.
 */
UCLASS(Blueprintable)
class GASP_API UCharacterTask_Ragdoll : public UCharacterTask
{
	GENERATED_BODY()

public:
	UCharacterTask_Ragdoll();

	UPROPERTY(BlueprintAssignable)
	FCharacterRagdollSimpleSignature OnRagdollStarted;

	UPROPERTY(BlueprintAssignable)
	FCharacterRagdollSimpleSignature OnRagdollEnded;

	UFUNCTION(BlueprintCallable, Category="Tasks", meta = (AdvancedDisplay = "TaskOwner, Priority",
		DefaultToSelf = "TaskOwner", BlueprintInternalUseOnly = "TRUE" ))
	static UCharacterTask_Ragdoll* CreateRagdollTask(TScriptInterface<IGameplayTaskOwnerInterface> TaskOwner,
	                                                 const bool bStopActiveMontages,
	                                                 const FMontageBlendSettings& BlendSettings,
	                                                 const FGameplayTag InjuryState, UChooserTable* GetUpTable,
	                                                 const uint8 Priority = 192);

protected:
	/** Called once by the owning GameplayTasksComponent when the task is ready to run. */
	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

public:
	virtual void TickTask(float DeltaTime) override;

private:
	bool bStopActiveMontages{false};
	FMontageBlendSettings BlendSettings;
	FGameplayTag InjuryState{};
	UPROPERTY(Transient)
	TObjectPtr<UChooserTable> GetUpTable{};

private:
	/** Predicts where the ragdoll will land and stores time/direction to that impact. */
	void RefreshRagdollImpactDirection();

	/** Advances the auto-roll forces and applies the resulting torque to the spine. */
	void RefreshRagdollRollBehavior(float DeltaTime);

	/** Drives PhysicsControl strengths from the Ragdoll_Strength_* anim curves. */
	void RefreshRagdollPhysicsStrengthsFromCurves() const;

	bool AreComponentsValid() const;

	FRagdollingState RagdollingState;

	UPROPERTY(Transient)
	TWeakObjectPtr<USkeletalMeshComponent> Mesh;

	UPROPERTY(Transient)
	TWeakObjectPtr<UCapsuleComponent> CapsuleComponent;

	UPROPERTY(Transient)
	TWeakObjectPtr<UPhysicsControlComponent> PhysicsControlComponent;

	static const FName NAME_Ragdoll;
	static const FName NAME_All;
	static const FName NAME_CharacterMesh;
	static const FName NAME_spine_05;
	static const FName NAME_RagdollTrace;
	static const FName NAME_ParentSpace;
	static const FName NAME_ParentSpace_Legs;
	static const FName NAME_ParentSpace_Arms;
	static const FName NAME_ParentSpace_Torso;
	static const FName NAME_ParentSpace_Head;
	static const FName NAME_Ragdoll_Strength_Legs;
	static const FName NAME_Ragdoll_Strength_Arms;
	static const FName NAME_Ragdoll_Strength_Torso;
	static const FName NAME_Ragdoll_Strength_Head;
};
