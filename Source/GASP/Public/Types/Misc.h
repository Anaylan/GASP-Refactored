#pragma once

/**
 * 
 */
namespace DebugCVars
{
	static FAutoConsoleVariable CVarDrawCharacterShape(
		TEXT("gasp.Debug.DrawCharacterShapes"), 0, TEXT("Displays debug shapes."));
	static FAutoConsoleVariable CVarDrawCharacterStates(
		TEXT("gasp.Debug.DrawCharacterStates"), 0, TEXT("Displays debug states."));
	static FAutoConsoleVariable CVarDrawCharacterGraph(
		TEXT("gasp.Debug.DrawCharacterGraphs"), 0, TEXT("Displays debug graphs."));
}
