#pragma once

#include "CharacterTask.h"
#include "Animation/AnimMontage.h"
#include "Types/StructTypes.h"
#include "RagdollTask.generated.h"

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

/**
 * Drives one ragdoll episode: hands the mesh to Chaos, hands the capsule to
 * UMovementMode_Ragdolling, and each frame refreshes the character's FRagdollingState
 * (impact prediction, auto-roll torque, physics control strengths from anim curves).
 *
 * The task owns no ragdoll state. AGASPCharacter owns FRagdollingState; this task is
 * its sole writer while active, and only on the game thread.
 */
UCLASS(Blueprintable)
class GASP_API URagdollTask : public UCharacterTask
{
	GENERATED_BODY()

public:
	URagdollTask();

	UFUNCTION(BlueprintCallable, Category="Ability|Tasks",
		meta = (AdvancedDisplay = "TaskOwner, Priority", DefaultToSelf = "TaskOwner", BlueprintInternalUseOnly = "TRUE"
		))
	static URagdollTask* CreateRagdollTask(TScriptInterface<IGameplayTaskOwnerInterface> TaskOwner,
	                                       const bool bStopActiveMontages, const FMontageBlendSettings BlendSettings,
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
	void RefreshRagdollImpactDirection() const;
	void RefreshRagdollRollBehavior(float DeltaTime) const;
	void RefreshRagdollPhysicsStrengthsFromCurves() const;

	bool AreComponentsValid() const;

	FRagdollingState* RagdollingState;

	UPROPERTY(Transient)
	TWeakObjectPtr<USkeletalMeshComponent> Mesh;

	UPROPERTY(Transient)
	TWeakObjectPtr<UGASPMoverComponent> MoverComponent;

	UPROPERTY(Transient)
	TWeakObjectPtr<UCapsuleComponent> CapsuleComponent;

	UPROPERTY(Transient)
	TWeakObjectPtr<UPhysicsControlComponent> PhysicsControlComponent;

	static const FName NAME_Ragdoll;
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
