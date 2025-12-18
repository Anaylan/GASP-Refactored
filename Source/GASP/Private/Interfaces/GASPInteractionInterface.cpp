#include "Interfaces/GASPInteractionInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPInteractionInterface)

bool UInteractionTransformBlueprintFunctionLibrary::SetInteractionTransform(UObject* InteractionTransformObject,
                                                                            const FTransform& InteractionTransform)
{
	if (IsValid(InteractionTransformObject) &&
		InteractionTransformObject->Implements<UGASPInteractionInterface>())
	{
		IGASPInteractionInterface::Execute_SetInteractionTransform(InteractionTransformObject,
		                                                                    InteractionTransform);
		return true;
	}
	return false;
}

bool UInteractionTransformBlueprintFunctionLibrary::GetInteractionTransform(UObject* InteractionTransformObject,
                                                                            FTransform& OutInteractionTransform)
{
	if (IsValid(InteractionTransformObject) &&
		InteractionTransformObject->Implements<UGASPInteractionInterface>())
	{
		IGASPInteractionInterface::Execute_GetInteractionTransform(InteractionTransformObject,
		                                                                    OutInteractionTransform);
		return true;
	}
	return false;
}
