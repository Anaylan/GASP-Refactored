#include "Tasks/CharacterTask.h"
#include "Actors/GASPCharacter.h"
#include "GameFramework/Controller.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CharacterTask)

UCharacterTask::UCharacterTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UCharacterTask::Activate()
{
	Super::Activate();

	Character = Cast<AGASPCharacter>(GetOwnerActor());
}

void UCharacterTask::OnDestroy(bool bInOwnerFinished)
{
	Character = nullptr;
	Super::OnDestroy(bInOwnerFinished);
}

FString UCharacterTask::GetDebugString() const
{
	return FString::Printf(TEXT("%s (Owner: %s, State: %s)"),
	                       *GetName(),
	                       Character.IsValid() ? *Character->GetName() : TEXT("None"),
	                       *GetTaskStateName());
}
