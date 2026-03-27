#pragma once

#include "EnumTypes.generated.h"

/* Returns the enumeration index as number. */
template <typename T>
static FORCEINLINE int32 GetIndexByValue(const T InValue)
{
	return StaticEnum<T>()->GetIndexByValue(static_cast<int32>(InValue));
}

/* Returns the enumeration value as string. */
template <typename T>
static FORCEINLINE FString GetNameStringByValue(const T InValue)
{
	return StaticEnum<T>()->GetNameStringByValue(static_cast<int32>(InValue));
}

UENUM(BlueprintType, meta = (ScriptName = "EMovementDirection"))
enum class EMovementDirection : uint8
{
	F UMETA(DisplayName = "Forward"),
	B UMETA(DisplayName = "Backward"),
	LL UMETA(DisplayName = "Left"),
	FL UMETA(DisplayName = "Forward->Left"),
	RR UMETA(DisplayName = "Right"),
	FR UMETA(DisplayName = "Forward->Right"),
};

UENUM(BlueprintType, meta = (ScriptName = "EMovementDirectionBias"))
enum class EMovementDirectionBias : uint8
{
	LeftFootForward UMETA(DisplayName = "LeftFootForward"),
	RightFootForward UMETA(DisplayName = "RightFootForward")
};

UENUM(BlueprintType, meta = (ScriptName = "EStateMachineState"))
enum class EStateMachineState : uint8
{
	IdleLoop UMETA(DisplayName = "Idle Loop"),
	TransitionToIdleLoop UMETA(DisplayName = "Transition to Idle Loop"),
	LocomotionLoop UMETA(DisplayName = "Locomotion Loop"),
	TransitionToLocomotionLoop UMETA(DisplayName = "Transition to Locomotion Loop"),
	InAirLoop UMETA(DisplayName = "In Air Loop"),
	TransitionToInAirLoop UMETA(DisplayName = "Transition to In Air Loop"),
	IdleBreak UMETA(DisplayName = "Idle Break"),
	TransitionToSlide UMETA(DisplayName = "Transition to Slide"),
	SlideLoop UMETA(DisplayName = "Slide Loop"),
};

// Anim notifies
UENUM(BlueprintType, meta = (ScriptName = "EEarlyTransitionCondition"))
enum class EEarlyTransitionCondition : uint8
{
	Always UMETA(DisplayName = "Always"),
	GaitNotEqual UMETA(DisplayName = "GaitNotEqual")
};

UENUM(BlueprintType, meta = (ScriptName = "EEarlyTransitionDestination"))
enum class EEarlyTransitionDestination : uint8
{
	ReTransition UMETA(DisplayName = "Re-Transition"),
	TransitionToLoop UMETA(DisplayName = "Transition To Loop")
};

UENUM(BlueprintType, meta = (ScriptName = "EFoleyEventSide"))
enum class EFoleyEventSide : uint8
{
	None UMETA(DisplayName = "None"),
	Left UMETA(DisplayName = "Left"),
	Right UMETA(DisplayName = "Right")
};

UENUM(BlueprintType, meta = (ScriptName = "ETraversalBlendOutCondition"))
enum class ETraversalBlendOutCondition : uint8
{
	ForceBlendOut UMETA(DisplayName = "Force Blend Out"),
	WithMovementInput UMETA(DisplayName = "With Movement Input"),
	IfFalling UMETA(DisplayName = "If Falling")
};