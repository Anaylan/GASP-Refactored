#pragma once

namespace DebugCVars
{
	static FAutoConsoleVariable CVarDrawCharacterShape(
		TEXT("gasp.Debug.DrawCharacterShapes"), 0, TEXT("Displays debug shapes."));
	static FAutoConsoleVariable CVarDrawCharacterStates(
		TEXT("gasp.Debug.DrawCharacterStates"), 0, TEXT("Displays debug states."));
	static FAutoConsoleVariable CVarDrawCharacterGraph(
		TEXT("gasp.Debug.DrawCharacterGraphs"), 0, TEXT("Displays debug graphs."));
}

namespace StateMachineStateNames
{
	const FName IdleLoop{TEXT("Idle Loop")};
	const FName IdleBreak{TEXT("Idle Break")};
	const FName IdleTransition{TEXT("Idle Transition")};
	const FName LocomotionLoop{TEXT("Locomotion Loop")};
	const FName LocomotionTransition{TEXT("Locomotion Transition")};
	const FName InAirLoop{TEXT("In Air Loop")};
	const FName InAirTransition{TEXT("In Air Transition")};
	const FName SlideLoop{TEXT("Slide Loop")};
	const FName SlideTransition{TEXT("Slide Transition")};
}
