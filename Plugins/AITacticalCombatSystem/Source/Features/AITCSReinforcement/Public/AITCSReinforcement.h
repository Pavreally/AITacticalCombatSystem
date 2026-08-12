// Pavel Gornostaev <https://github.com/Pavreally>

#pragma once

#include "Modules/ModuleManager.h"

class FAITCSReinforcementModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
