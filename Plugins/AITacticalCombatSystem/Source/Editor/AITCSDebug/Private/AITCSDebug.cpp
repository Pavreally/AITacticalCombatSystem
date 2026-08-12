// Pavel Gornostaev <https://github.com/Pavreally>

// Copyright Epic Games, Inc. All Rights Reserved.

#include "AITCSDebug.h"

#define LOCTEXT_NAMESPACE "FAITCSDebugModule"

void FAITCSDebugModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FAITCSDebugModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAITCSDebugModule, AITCSDebug)