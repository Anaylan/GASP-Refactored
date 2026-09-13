#include "Tasks/CharacterTask_Override.h"

#include "Actors/GASPCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "GASP.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CharacterTask_Override)

UCharacterTask_Override::UCharacterTask_Override()
{
	bTickingTask = false;
}

UCharacterTask_Override* UCharacterTask_Override::CreateOverrideTask(
	TScriptInterface<IGameplayTaskOwnerInterface> TaskOwner, const TSubclassOf<UAnimInstance> OverrideAnimClass,
	const uint8 Priority)
{
	auto* MyTask = NewTaskUninitialized<UCharacterTask_Override>();
	if (MyTask && TaskOwner.GetInterface() != nullptr)
	{
		MyTask->InitTask(*TaskOwner, Priority);
		MyTask->OverrideAnimClass = OverrideAnimClass;
	}
	else
	{
		UE_LOG(LogGASP, Error, TEXT("CreateOverrideTask: invalid TaskOwner, task will not activate"));
	}

	return MyTask;
}

void UCharacterTask_Override::Activate()
{
	Super::Activate();

	if (!Character.IsValid() || !OverrideAnimClass)
	{
		EndTask();
		return;
	}

	auto* Mesh = Character->GetMesh();
	if (!IsValid(Mesh))
	{
		EndTask();
		return;
	}

	Mesh->LinkAnimClassLayers(OverrideAnimClass);
	bLayerLinked = true;
	OnApplied.Broadcast();
}

void UCharacterTask_Override::OnDestroy(bool bInOwnerFinished)
{
	if (bLayerLinked && Character.IsValid())
	{
		if (auto* Mesh = Character->GetMesh())
		{
			Mesh->UnlinkAnimClassLayers(OverrideAnimClass);
		}
		bLayerLinked = false;
	}

	OnRemoved.Broadcast();
	Super::OnDestroy(bInOwnerFinished);
}
