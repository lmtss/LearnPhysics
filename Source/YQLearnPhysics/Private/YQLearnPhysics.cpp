// Copyright Epic Games, Inc. All Rights Reserved.

#include "YQLearnPhysics.h"

#include "Interfaces/IPluginManager.h"

#include "YQPhysicsScene.h"

#define LOCTEXT_NAMESPACE "FYQLearnPhysicsModule"



void FYQLearnPhysicsModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FString RealShaderDirectory = FPaths::Combine(
		IPluginManager::Get().FindPlugin(TEXT("YQLearnPhysics"))->GetBaseDir(),
		TEXT("Shaders")
	);

	FString VirtualShaderDirectory = FString(TEXT("/YQLearnPhysics"));

	AddShaderSourceDirectoryMapping(VirtualShaderDirectory, RealShaderDirectory);

}

void FYQLearnPhysicsModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FYQLearnPhysicsModule, YQLearnPhysics)