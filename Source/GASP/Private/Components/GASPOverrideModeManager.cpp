#include "Components/GASPOverrideModeManager.h"
#include "GASP.h"
#include "Actors/GASPCharacter.h"
#include "Components/SkeletalMeshComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPOverrideModeManager)

UGASPOverrideModeManager::UGASPOverrideModeManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGASPOverrideModeManager::AddOverrideLayer(FGameplayTag OverrideTag,
                                                TSubclassOf<UAnimInstance> OverrideAnimClass)
{
	if (LinkedOverrideLayers.Contains(OverrideTag))
	{
		UE_LOG(LogGASP, Warning, TEXT("%s: AddOverrideLayer(%s) ignored - already active"),
		       *GetName(), *OverrideTag.ToString());
		return;
	}

	if (auto* Mesh{OwnedCharacter->GetMesh()})
	{
		Mesh->LinkAnimClassLayers(OverrideAnimClass);
	}

	LinkedOverrideLayers.Add(OverrideTag, OverrideAnimClass);
	OnOverrideLayerChanged.Broadcast(OverrideTag, true);
}

void UGASPOverrideModeManager::RemoveOverrideLayer(FGameplayTag OverrideTag)
{
	TSubclassOf<UAnimInstance> OverrideAnimClass;
	if (LinkedOverrideLayers.RemoveAndCopyValue(OverrideTag, OverrideAnimClass))
	{
		if (auto* Mesh{OwnedCharacter->GetMesh()})
		{
			Mesh->UnlinkAnimClassLayers(OverrideAnimClass);
		}
		OnOverrideLayerChanged.Broadcast(OverrideTag, false);
	}
}

void UGASPOverrideModeManager::BeginPlay()
{
	Super::BeginPlay();

	LinkedOverrideLayers.Reset();
	OwnedCharacter = GetOwner<AGASPCharacter>();
}

void UGASPOverrideModeManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (auto* Mesh{OwnedCharacter->GetMesh()})
	{
		for (const auto& [OverrideTag, OverrideAnimClass] : LinkedOverrideLayers)
		{
			Mesh->UnlinkAnimClassLayers(OverrideAnimClass);
		}
	}

	LinkedOverrideLayers.Reset();

	Super::EndPlay(EndPlayReason);
}
