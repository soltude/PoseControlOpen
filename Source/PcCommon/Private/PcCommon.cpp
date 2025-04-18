// Copyright Epic Games, Inc. All Rights Reserved.

#include "PcCommon.h"
#include "CustomLogging.h"

DEFINE_LOG_CATEGORY(LogAnchor);

#define LOCTEXT_NAMESPACE "FPcCommonModule"

void FPcCommonModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FPcCommonModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

}



#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FPcCommonModule, PcCommon)