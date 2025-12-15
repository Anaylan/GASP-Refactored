#include "MovementSet/GASPMoverComponent.h"

#include "Components/CapsuleComponent.h"
#include "MovementSet/Modes/MovementMode_Falling.h"
#include "DefaultMovementSet/Modes/WalkingMode.h"
#include "DefaultMovementSet/Modes/FlyingMode.h"
#include "Types/MovementTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPMoverComponent)


// Sets default values for this component's properties
UGASPMoverComponent::UGASPMoverComponent()
{
	MovementModes.Reset();
	MovementModes.Add(DefaultModeNames::Walking, CreateDefaultSubobject<UWalkingMode>(TEXT("Walking")));
	MovementModes.Add(MovementModeNames::Sliding, CreateDefaultSubobject<UWalkingMode>(TEXT("Sliding")));
	MovementModes.Add(DefaultModeNames::Falling, CreateDefaultSubobject<UMovementMode_Falling>(TEXT("Falling")));
	MovementModes.Add(DefaultModeNames::Flying, CreateDefaultSubobject<UFlyingMode>(TEXT("Flying")));
}

FVector UGASPMoverComponent::GetFeetLocation()
{
	if (const auto* Capsule = Cast<UCapsuleComponent>(UpdatedComponent))
	{
		return Capsule->GetComponentLocation() + (-FVector::UpVector * Capsule->GetScaledCapsuleHalfHeight());
	}
	return FVector::ZeroVector;
}
