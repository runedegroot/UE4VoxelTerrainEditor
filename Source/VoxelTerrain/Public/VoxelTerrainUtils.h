#pragma once
#include "Object.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VoxelTerrainUtils.generated.h"

class AVoxelTerrain;
UCLASS()
class VOXELTERRAIN_API UVoxelTerrainUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Voxel Terrain Utility")
		static FVector WorldPosToTerrainPos(FVector in, AVoxelTerrain* voxelTerrain);
	UFUNCTION(BlueprintCallable, Category = "Voxel Terrain Utility")
		static FVector TerrainPosToWorldPos(FVector in, AVoxelTerrain* voxelTerrain);
	UFUNCTION(BlueprintCallable, Category = "Voxel Terrain Utility")
		static void HightlightVoxel(FVector terrainPos, AVoxelTerrain* voxelTerrain, float toolSize = 1.f, FColor color = FColor::Blue, float thickness = 3.0f);
	UFUNCTION(BlueprintCallable, Category = "Voxel Terrain Utility")
		static FVector ScreenCenterToTerrainPos(AVoxelTerrain* voxelTerrain);
	UFUNCTION(BlueprintCallable, Category = "Voxel Terrain Utility")
		static FVector MousePosToTerrainPos(AVoxelTerrain* voxelTerrain);

	UFUNCTION(BlueprintCallable, Category = "Voxel Terrain Utility")
		static FIntPoint WorldPosToChunkPos(FVector in, AVoxelTerrain* voxelTerrain);

	UFUNCTION(BlueprintCallable, Category = "Voxel Terrain Utility")
		static FIntPoint TerrainPosToChunkPos(FVector in, AVoxelTerrain* voxelTerrain);

	static AVoxelTerrain* GetTerrainUnderMouse(FViewport*);
	static bool ArrayContainsVector(TArray<FVector> investigatedVoxels, FVector toCheck);
};