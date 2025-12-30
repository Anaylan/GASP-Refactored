#pragma once

#include "Modules/ModuleManager.h"

struct FAutoCompleteCommand;

class FGASPModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
