#include "Input/GASPPlayerController.h"

#include "Actors/GASPCharacter.h"

void AGASPPlayerController::ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult)
{
	float DeltaMS = static_cast<float>(SimTimeMs);
	
	const auto* GCharacter = static_cast<AGASPCharacter*>(GetPawn());

	FCharacterDefaultInputs& CharacterInputs = InputCmdResult.InputCollection.FindOrAddMutableDataByType<FCharacterDefaultInputs>();
	CharacterInputs.ControlRotation = GetControlRotation();

}
