// Pavel Gornostaev <https://github.com/Pavreally>

#pragma once

#include "Modules/ModuleManager.h"
#include "Data/AITCSCoreData.h"
#include "Graph/AITCSTacticalGraphAsset.h"
#include "Interfaces/AITCSTacticalDecisionProvider.h"

/**
 * Core module implementation for the AITacticalCombatSystem plugin.
 * Responsible for module startup and shutdown lifecycle behavior.
 */
class FAITCSCoreModule : public IModuleInterface
{
public:
	/** Initialize core AITCS resources and register module-level dependencies. */
	virtual void StartupModule() override;

	/** Release core resources and unregister module-level dependencies. */
	virtual void ShutdownModule() override;
};
