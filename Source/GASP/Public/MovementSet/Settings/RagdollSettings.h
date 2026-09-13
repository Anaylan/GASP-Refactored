#pragma once

#include "MovementMode.h"
#include "UObject/Object.h"
#include "RagdollSettings.generated.h"

/**
 * Capsule shape the character takes while ragdolling. Registered by UMovementMode_Ragdolling through
 * SharedSettingsClasses and consumed by FMovementModifier_Ragdoll, which owns the resize itself.
 */
UCLASS()
class GASP_API URagdollSettings : public UObject, public IMovementSettingsInterface
{
	GENERATED_BODY()

public:
	// Capsule half height while ragdolling. The radius is left alone.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll", meta = (ClampMin = "0", UIMin = "0", ForceUnits = "cm"))
	float CapsuleHalfHeight = 34.f;

	// Eye height while ragdolling, applied to the owning pawn.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ragdoll", meta = (ClampMin = "0", UIMin = "0", ForceUnits = "cm"))
	float CapsuleEyeHeight = 34.f;

	virtual FString GetDisplayName() const override { return GetName(); }
};
