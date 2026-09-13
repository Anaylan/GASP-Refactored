#pragma once

#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

/** General purpose log category for the GASP runtime module. */
GASP_API DECLARE_LOG_CATEGORY_EXTERN(LogGASP, Log, All);

class FGASPModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/**
	 * Handle for the console autocomplete binding. A lambda delegate is not bound to any object,
	 * so RemoveAll(this) can never match it and the handle is the only way to unregister.
	 */
	FDelegateHandle AutoCompleteEntriesHandle;
};
