#include "MovementSet/GASPMoverComponent.h"

#include "Components/CapsuleComponent.h"
#include "MovementSet/Modes/MovementMode_Falling.h"
#include "DefaultMovementSet/Modes/FlyingMode.h"
#include "MovementSet/Modes/MovementMode_Sliding.h"
#include "MovementSet/Modes/MovementMode_Walking.h"
#include "Types/MovementTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPMoverComponent)


// Sets default values for this component's properties
UGASPMoverComponent::UGASPMoverComponent()
{
	MovementModes.Reset();
	MovementModes.Add(DefaultModeNames::Walking, CreateDefaultSubobject<UMovementMode_Walking>(DefaultModeNames::Walking));
	MovementModes.Add(MovementModeNames::Sliding, CreateDefaultSubobject<UMovementMode_Sliding>(MovementModeNames::Sliding));
	MovementModes.Add(DefaultModeNames::Falling, CreateDefaultSubobject<UMovementMode_Falling>(DefaultModeNames::Falling));
	MovementModes.Add(DefaultModeNames::Flying, CreateDefaultSubobject<UFlyingMode>(DefaultModeNames::Flying));

	bSyncInputsForSimProxy = true;
}

FVector UGASPMoverComponent::GetFeetLocation()
{
	if (const auto* Capsule = Cast<UCapsuleComponent>(UpdatedComponent))
	{
		return Capsule->GetComponentLocation() + (-FVector::UpVector * Capsule->GetScaledCapsuleHalfHeight());
	}
	return FVector::ZeroVector;
}
