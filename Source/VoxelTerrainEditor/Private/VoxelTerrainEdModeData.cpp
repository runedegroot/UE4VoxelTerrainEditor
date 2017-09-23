#include "VoxelTerrainEditorPrivatePCH.h"
#include "VoxelTerrainEdModeData.h"

// Sets default values
UVoxelTerrainEdModeData::UVoxelTerrainEdModeData(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	//FString path = IPluginManager::Get().FindPlugin(TEXT("VoxelTerrain"))->GetContentDir() + "/TriplanarVoxelTerrain.uasset";
	//material = Cast<UMaterialInterface>(StaticLoadObject(UMaterialInterface::StaticClass(), NULL, *path));
}