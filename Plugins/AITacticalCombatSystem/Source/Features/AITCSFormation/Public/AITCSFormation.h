// Pavel Gornostaev <https://github.com/Pavreally>

#pragma once

#include "Modules/ModuleManager.h"
#include "StateTree/AITCSFormationDestinationFunctionST.h"

class FAITCSFormationModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
