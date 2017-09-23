// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "VoxelTerrainPrivatePCH.h"
#include "VoxelTerrainActor.h"
#include "VoxelTerrainData.h"

#define LOCTEXT_NAMESPACE "FVoxelTerrainModule"

void FVoxelTerrainModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	//FVoxelTerrainEdModeStyle::Initialize();
	//FEditorModeRegistry::Get().RegisterMode<FVoxelTerrainEdMode>(FVoxelTerrainEdMode::EM_VoxelTerrainEdModeId, LOCTEXT("VoxelTerrainEdModeName", "Voxels"), FSlateIcon(FVoxelTerrainEdModeStyle::Get()->GetStyleSetName(), "Plugins.TabIcon"), true);
}

void FVoxelTerrainModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	//FEditorModeRegistry::Get().UnregisterMode(FVoxelTerrainEdMode::EM_VoxelTerrainEdModeId);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FVoxelTerrainModule, VoxelTerrain)