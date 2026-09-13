#include "GASP.h"

#if ALLOW_CONSOLE
#include "Engine/Console.h"
#endif

DEFINE_LOG_CATEGORY(LogGASP);

void FGASPModule::StartupModule()
{
#if ALLOW_CONSOLE
	AutoCompleteEntriesHandle = UConsole::RegisterConsoleAutoCompleteEntries.AddLambda(
		[](TArray<FAutoCompleteCommand>& AutoCompleteCommands)
	{
		const auto CommandColor{GetDefault<UConsoleSettings>()->AutoCompleteCommandColor};

		auto* Command{&AutoCompleteCommands.AddDefaulted_GetRef()};
		Command->Command = FString{TEXTVIEW("gasp.Debug.DrawCharacterShapes")};
		Command->Desc = FString{TEXTVIEW("Displays debug shapes.")};
		Command->Color = CommandColor;

		Command = &AutoCompleteCommands.AddDefaulted_GetRef();
		Command->Command = FString{TEXTVIEW("gasp.Debug.DrawCharacterStates")};
		Command->Desc = FString{TEXTVIEW("Displays debug states.")};
		Command->Color = CommandColor;

		Command = &AutoCompleteCommands.AddDefaulted_GetRef();
		Command->Command = FString{TEXTVIEW("gasp.Debug.DrawCharacterGraphs")};
		Command->Desc = FString{TEXTVIEW("Displays debug graphs.")};
		Command->Color = CommandColor;
	});
#endif
}

void FGASPModule::ShutdownModule()
{
#if ALLOW_CONSOLE
	UConsole::RegisterConsoleAutoCompleteEntries.Remove(AutoCompleteEntriesHandle);
	AutoCompleteEntriesHandle.Reset();
#endif
}


IMPLEMENT_MODULE(FGASPModule, GASP)
