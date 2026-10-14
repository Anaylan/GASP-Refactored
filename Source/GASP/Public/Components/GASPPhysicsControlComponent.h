#pragma once

#include "PhysicsControlComponent.h"
#include "Types/StructTypes.h"
#include "GASPPhysicsControlComponent.generated.h"

/**
 * Physics control that owns its named profiles.
 *
 * On BeginPlay it creates controls from its PhysicsControlAsset on the skeletal mesh it is attached
 * to and applies the profile chosen through gasp.physics.profile. Profile names come straight from
 * the asset, so there is no separate list to keep in sync.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GASP_API UGASPPhysicsControlComponent : public UPhysicsControlComponent
{
	GENERATED_BODY()

public:
	UGASPPhysicsControlComponent();

	/** Profile names from the PhysicsControlAsset, in the order the asset compiled them. */
	UFUNCTION(BlueprintPure, Category = "GASP|Physics")
	TArray<FName> GetProfileNames() const;

	UFUNCTION(BlueprintPure, Category = "GASP|Physics")
	FName GetProfileName(int32 Index) const;

	/** Invokes the profile and switches ragdoll self-collision and constraint profiles to match it. */
	UFUNCTION(BlueprintCallable, Category = "GASP|Physics")
	void ApplyProfile(FName ProfileName);

	/** Re-applies the profile chosen through gasp.physics.profile, e.g. once a ragdoll ends. */
	UFUNCTION(BlueprintCallable, Category = "GASP|Physics")
	void ApplySelectedProfile();

	UFUNCTION(BlueprintPure, Category = "GASP|Physics")
	FName GetAppliedProfileName() const { return AppliedProfileName; }

	/**
	 * Limbs that must not interpenetrate while the Ragdoll physics profile is active.
	 *
	 * Collision is enabled between every pair of distinct groups when that profile is applied and
	 * disabled again for any other, so a biped lists two groups and a six-legged character lists
	 * six. Bones inside a single group are left to whatever the physics asset says.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ragdolling")
	TArray<FGASPBodyGroup> RagdollSelfCollisionGroups;

	static const FName RagdollProfileName;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	USkeletalMeshComponent* GetSkeletalMesh() const;
	void SelectProfile(int32 Index);
	void SetRagdollSelfCollision(USkeletalMeshComponent& Mesh, bool bEnabled) const;

	FName SelectedProfileName{NAME_None};
	FName AppliedProfileName{NAME_None};
	FDelegateHandle ProfileIndexChangedHandle;
};
