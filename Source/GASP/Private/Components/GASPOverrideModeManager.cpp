#include "Components/GASPOverrideModeComponent.h"

#include "Tasks/CharacterTask_Override.h"


// Sets default values for this component's properties
UGASPOverrideModeComponent::UGASPOverrideModeComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
}

void UGASPOverrideModeComponent::AddOverrideLayer(FGameplayTag OverrideTag,
                                                  TSubclassOf<UAnimInstance> OverrideAnimClass)
{
	if (InstancedOverrideTasks.Contains(OverrideTag))
	{
		return;
	}

	auto Task = UCharacterTask_Override::CreateOverrideTask(GetOwner(), OverrideAnimClass);
	Task->ReadyForActivation();

	InstancedOverrideTasks.Add(OverrideTag, Task);
}

void UGASPOverrideModeComponent::RemoveOverrideLayer(FGameplayTag OverrideTag)
{
	if (auto Task{InstancedOverrideTasks.FindRef(OverrideTag)})
	{
		Task->EndTask();
		InstancedOverrideTasks.Remove(OverrideTag);
	}
}


// Called when the game starts
void UGASPOverrideModeComponent::BeginPlay()
{
	Super::BeginPlay();

	InstancedOverrideTasks.Reset();
}
