// Fill out your copyright notice in the Description page of Project Settings.


#include "MovementSet/GASPMoverComponent.h"

#include "Components/CapsuleComponent.h"
#include "MovementSet/Modes/MovementMode_Falling.h"
#include "MovementSet/Modes/MovementMode_Slide.h"
#include "MovementSet/Modes/MovementMode_Walking.h"
#include "DefaultMovementSet/Modes/FlyingMode.h"
#include "Types/MovementTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPMoverComponent)


// Sets default values for this component's properties
UGASPMoverComponent::UGASPMoverComponent()
{
	MovementModes.Reset();
	MovementModes.Add(DefaultModeNames::Walking, CreateDefaultSubobject<UMovementMode_Walking>(TEXT("Walking")));
	MovementModes.Add(DefaultModeNames::Falling, CreateDefaultSubobject<UMovementMode_Falling>(TEXT("Falling")));
	MovementModes.Add(DefaultModeNames::Flying, CreateDefaultSubobject<UFlyingMode>(TEXT("Flying")));
	MovementModes.Add(MovementModeNames::Sliding, CreateDefaultSubobject<UMovementMode_Slide>(TEXT("Sliding")));
}

FVector UGASPMoverComponent::GetFeetLocation()
{
	if(auto* Capsule = Cast<UCapsuleComponent>(UpdatedComponent))
	{
		return Capsule->GetComponentLocation() + (-FVector::UpVector * Capsule->GetScaledCapsuleHalfHeight());
	}
	return FVector::ZeroVector;
}