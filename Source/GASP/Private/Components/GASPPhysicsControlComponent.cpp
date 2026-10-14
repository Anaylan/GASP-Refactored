#include "Components/GASPPhysicsControlComponent.h"
#include "PhysicsControlAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "HAL/IConsoleManager.h"
#include "Utils/GASPPhysicsControlLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPPhysicsControlComponent)

const FName UGASPPhysicsControlComponent::RagdollProfileName(TEXT("Ragdoll"));

namespace GASPPhysicsControlVars
{
	int32 ProfileIndex{0};
	FAutoConsoleVariableRef CVarProfileIndex(
		TEXT("gasp.physics.profile"), ProfileIndex,
		TEXT("Index into the PhysicsControlAsset profiles, in the asset's order"), ECVF_Default);

	const FName ConstraintProfile_Free(TEXT("Free"));
}

UGASPPhysicsControlComponent::UGASPPhysicsControlComponent()
{
	// Default limbs for the UE mannequin skeleton. Data, not logic: a character with a different
	// skeleton or a different number of legs overrides this in its Blueprint.
	RagdollSelfCollisionGroups = {
		FGASPBodyGroup{{TEXT("thigh_l"), TEXT("calf_l"), TEXT("foot_l")}},
		FGASPBodyGroup{{TEXT("thigh_r"), TEXT("calf_r"), TEXT("foot_r")}}
	};
}

void UGASPPhysicsControlComponent::BeginPlay()
{
	Super::BeginPlay();

	if (auto* Mesh{GetSkeletalMesh()})
	{
		CreateControlsAndBodyModifiersFromPhysicsControlAsset(Mesh, nullptr, NAME_None);
	}

	// Applied after the controls exist; invoking a profile before that has nothing to act on.
	SelectProfile(GASPPhysicsControlVars::ProfileIndex);

	ProfileIndexChangedHandle = GASPPhysicsControlVars::CVarProfileIndex->OnChangedDelegate().AddWeakLambda(
		this, [this](const IConsoleVariable* CVar)
		{
			SelectProfile(CVar ? CVar->GetInt() : 0);
		});
}

void UGASPPhysicsControlComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GASPPhysicsControlVars::CVarProfileIndex->OnChangedDelegate().Remove(ProfileIndexChangedHandle);
	ProfileIndexChangedHandle.Reset();

	Super::EndPlay(EndPlayReason);
}

TArray<FName> UGASPPhysicsControlComponent::GetProfileNames() const
{
	TArray<FName> Names;

	// Profiles is the compiled runtime map; its insertion order matches the asset, which is what the index refers to.
	if (const auto* Asset{PhysicsControlAsset.LoadSynchronous()})
	{
		Asset->Profiles.GetKeys(Names);
	}

	return Names;
}

FName UGASPPhysicsControlComponent::GetProfileName(const int32 Index) const
{
	const auto Names{GetProfileNames()};
	return Names.IsValidIndex(Index) ? Names[Index] : NAME_None;
}

void UGASPPhysicsControlComponent::ApplyProfile(const FName ProfileName)
{
	auto* Mesh{GetSkeletalMesh()};
	if (!Mesh)
	{
		return;
	}

	AppliedProfileName = ProfileName;
	InvokeControlProfile(ProfileName);

	const bool bRagdollProfile{ProfileName == RagdollProfileName};
	SetRagdollSelfCollision(*Mesh, bRagdollProfile);
	Mesh->SetConstraintProfileForAll(bRagdollProfile ? NAME_None : GASPPhysicsControlVars::ConstraintProfile_Free);
}

void UGASPPhysicsControlComponent::ApplySelectedProfile()
{
	ApplyProfile(SelectedProfileName);
}

USkeletalMeshComponent* UGASPPhysicsControlComponent::GetSkeletalMesh() const
{
	return Cast<USkeletalMeshComponent>(GetAttachParent());
}

void UGASPPhysicsControlComponent::SelectProfile(const int32 Index)
{
	SelectedProfileName = GetProfileName(Index);
	ApplyProfile(SelectedProfileName);
}

void UGASPPhysicsControlComponent::SetRagdollSelfCollision(USkeletalMeshComponent& Mesh, const bool bEnabled) const
{
	// Every pair of distinct limb groups, so the number of legs is a property of the character
	// rather than of this function: two groups for a biped, six for a six-legged one.
	for (int32 First{0}; First < RagdollSelfCollisionGroups.Num(); ++First)
	{
		const auto& FirstBones{RagdollSelfCollisionGroups[First].BoneNames};
		if (FirstBones.IsEmpty())
		{
			continue;
		}

		for (int32 Second{First + 1}; Second < RagdollSelfCollisionGroups.Num(); ++Second)
		{
			const auto& SecondBones{RagdollSelfCollisionGroups[Second].BoneNames};
			if (SecondBones.IsEmpty())
			{
				continue;
			}

			if (bEnabled)
			{
				UGASPPhysicsControlLibrary::EnableCollisionBetweenBodyArrays(&Mesh, FirstBones, &Mesh, SecondBones);
			}
			else
			{
				UGASPPhysicsControlLibrary::DisableCollisionBetweenBodyArrays(&Mesh, FirstBones, &Mesh, SecondBones);
			}
		}
	}
}
