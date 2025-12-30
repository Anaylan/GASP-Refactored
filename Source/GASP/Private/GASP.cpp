#include "GASP.h"

#if ALLOW_CONSOLE
#include "Engine/Console.h"
#endif

#define LOCTEXT_NAMESPACE "GASPModule"

void FGASPModule::StartupModule()
{
#if ALLOW_CONSOLE
	UConsole::RegisterConsoleAutoCompleteEntries.AddLambda([this](TArray<FAutoCompleteCommand>& AutoCompleteCommands)
	{
		const auto CommandColor{GetDefault<UConsoleSettings>()->AutoCompleteCommandColor};

		auto* Command{&AutoCompleteCommands.AddDefaulted_GetRef()};
		Command->Command = FString{TEXTVIEW("Debug.DrawCharacterShapes")};
		Command->Desc = FString{TEXTVIEW("Displays debug shapes.")};
		Command->Color = CommandColor;

		Command = &AutoCompleteCommands.AddDefaulted_GetRef();
		Command->Command = FString{TEXTVIEW("Debug.DrawCharacterStates")};
		Command->Desc = FString{TEXTVIEW("Displays debug states.")};
		Command->Color = CommandColor;

		Command = &AutoCompleteCommands.AddDefaulted_GetRef();
		Command->Command = FString{TEXTVIEW("Debug.DrawCharacterGraphs")};
		Command->Desc = FString{TEXTVIEW("Displays debug graphs.")};
		Command->Color = CommandColor;
	});
#endif
}

void FGASPModule::ShutdownModule()
{
#if ALLOW_CONSOLE
	UConsole::RegisterConsoleAutoCompleteEntries.RemoveAll(this);
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGASPModule, GASP)
