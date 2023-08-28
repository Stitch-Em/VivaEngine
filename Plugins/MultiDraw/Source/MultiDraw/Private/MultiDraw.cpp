// Copyright 2023 BlueRose, Inc. All Rights Reserved.

#include "MultiDraw.h"

#define LOCTEXT_NAMESPACE "FMultiDrawModule"

void FMultiDrawModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FMultiDrawModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMultiDrawModule, MultiDraw)