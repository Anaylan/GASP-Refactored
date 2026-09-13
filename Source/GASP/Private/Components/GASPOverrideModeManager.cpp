#include "Components/GASPOverrideModeManager.h"
#include "GASP.h"
#include "Tasks/CharacterTask_Override.h"


UGASPOverrideModeManager::UGASPOverrideModeManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGASPOverrideModeManager::AddOverrideLayer(FGameplayTag OverrideTag,
                                                TSubclassOf<UAnimInstance> OverrideAnimClass)
{
	if (InstancedOverrideTasks.Contains(OverrideTag))
	{
		UE_LOG(LogGASP, Warning, TEXT("%s: AddOverrideLayer(%s) ignored - already active"),
		       *GetName(), *OverrideTag.ToString());
		return;
	}

	auto Task = UCharacterTask_Override::CreateOverrideTask(GetOwner(), OverrideAnimClass);
	Task->ReadyForActivation();

	InstancedOverrideTasks.Add(OverrideTag, Task);
	OnOverrideLayerChanged.Broadcast(OverrideTag, true);
}

void UGASPOverrideModeManager::RemoveOverrideLayer(FGameplayTag OverrideTag)
{
	if (const auto Task{InstancedOverrideTasks.FindRef(OverrideTag)})
	{
		Task->EndTask();
		InstancedOverrideTasks.Remove(OverrideTag);
		OnOverrideLayerChanged.Broadcast(OverrideTag, false);
	}
}

void UGASPOverrideModeManager::BeginPlay()
{
	Super::BeginPlay();

	InstancedOverrideTasks.Reset();
}

void UGASPOverrideModeManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (const auto& Pair : InstancedOverrideTasks)
	{
		if (Pair.Value)
		{
			Pair.Value->EndTask();
		}
	}
	InstancedOverrideTasks.Reset();

	Super::EndPlay(EndPlayReason);
}
