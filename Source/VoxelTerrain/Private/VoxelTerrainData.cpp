#include "VoxelTerrainPrivatePCH.h"
#include "VoxelTerrainData.h"

// Sets default values
UVoxelTerrainData::UVoxelTerrainData(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	//static ConstructorHelpers::FObjectFinder<UMaterial> Material(TEXT("Material'/VoxelTerrain/WorldblendVoxelTerrain'"));

	//if (Material.Succeeded())
	//{
	//	material = Material.Object;
	//}
}

UVoxelTerrainChunkData::UVoxelTerrainChunkData(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	//
}

UVoxelTerrainMaterial::UVoxelTerrainMaterial(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	//
}

UVoxelTerrainFoliageType::UVoxelTerrainFoliageType(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	//
}

UVoxelTerrainChunkData* UVoxelTerrainData::GetChunkDataAt(FIntPoint inPos)
{
	if (chunks.Contains(inPos.X) && chunks[inPos.X].chunks.Contains(inPos.Y))
		return chunks[inPos.X].chunks[inPos.Y];
	return NULL;
}

UVoxelTerrainChunkData* UVoxelTerrainData::SetChunkDataAt(FIntPoint inPos, UVoxelTerrainChunkData* inData)
{
	UVoxelTerrainChunkData* copy = NewObject<UVoxelTerrainChunkData>(this, NAME_None, RF_NoFlags, inData);
	if (!chunks.Contains(inPos.X))
		chunks.Add(inPos.X, FVoxelTerrainChunkDataMap());
	chunks[inPos.X].chunks.Add(inPos.Y, copy);
	return copy;
}

FVoxelTerrainVoxel* UVoxelTerrainChunkData::GetVoxelAt(FIntVector inPos)
{
	return &terrain[inPos.X][inPos.Y][inPos.Z];
}