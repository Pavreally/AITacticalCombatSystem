// Pavel Gornostaev <https://github.com/Pavreally>

#pragma once

#include "Modules/ModuleManager.h"
#include "Actors/AITCSDirectorActor.h"
#include "Components/AITCSTacticalUnitComponent.h"
#include "Director/AITCSDirectorSubsystem.h"
#include "Features/AITCSTacticalEvaluator.h"
#include "Features/AITCSTacticalFeatureTypes.h"

/**
 * Runtime module implementation for the AITacticalCombatSystem plugin.
 * Responsible for runtime subsystem registration and module lifecycle.
 */
class FAITCSRuntimeModule : public IModuleInterface
{
public:
	/** Initialize the runtime module and register runtime components. */
	virtual void StartupModule() override;

	/** Shutdown the runtime module and cleanup runtime resources. */
	virtual void ShutdownModule() override;
};
