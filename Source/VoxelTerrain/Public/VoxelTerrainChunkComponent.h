#pragma once
#include "ProceduralMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "VoxelTerrainChunkComponent.generated.h"

DECLARE_STATS_GROUP(TEXT("VoxelTerrainChunkComponent"), STATGROUP_VoxelTerrainChunkComponent, STATCAT_Advanced);

class AVoxelTerrain;
class UVoxelTerrainChunkData;
class UVoxelTerrainFoliageType;
class UVoxelTerrainMaterial;
class UVoxelTerrainChunkComponent;
struct FVoxelTerrainVoxel;

DECLARE_DELEGATE(FUpdateComplete);
DECLARE_DELEGATE(FFoliageComplete);

class GridCell
{
public:
	GridCell();
	TArray<FVector> p = TArray<FVector>();
	TArray<FVoxelTerrainVoxel*> val = TArray<FVoxelTerrainVoxel*>();
	~GridCell();
};


struct FoliageLayerInfo
{
	FStaticMeshInstanceData InstanceBuffer;
	TArray<FClusterNode> ClusterTree;
	int OutOcclusionLayerNum;
	int Size;
};

class FoliageBuilderAsyncTask : public FNonAbandonableTask
{
private:
	AVoxelTerrain* voxelTerrain;
	UVoxelTerrainChunkComponent* component;
	TWeakPtr<FFoliageComplete, ESPMode::ThreadSafe> onComplete;
public:
	FoliageBuilderAsyncTask(AVoxelTerrain* VoxelTerrain, UVoxelTerrainChunkComponent* Component, TSharedPtr<FFoliageComplete, ESPMode::ThreadSafe> OnComplete = NULL)
	{
		voxelTerrain = VoxelTerrain;
		component = Component;
		onComplete = OnComplete;
	}

	/*This function is needed from the API of the engine.
	My guess is that it provides necessary information
	about the thread that we occupy and the progress of our task*/
	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FoliageBuilderAsyncTask, STATGROUP_ThreadPoolAsyncTasks);
	}

	void DoWork();
};

class ChunkMeshUpdateAsyncTask : public FNonAbandonableTask
{
private:
	AVoxelTerrain* voxelTerrain;
	UVoxelTerrainChunkComponent* component;
	class GridCell gridcell = GridCell();
	static int edgeTable[256];
	static int triTable[256][16];
	int Polygonise(class GridCell grid, double isolevel, TArray<FVector> &triangles, TArray<FColor> &colors);
	FColor DetermineColor(FVoxelTerrainVoxel*);
	FColor DetermineColor(FVoxelTerrainVoxel*, FVoxelTerrainVoxel*);
	static FVector VertexInterp(double isolevel, FVector p1, FVector p2, double valp1, double valp2);
	TWeakPtr<FUpdateComplete, ESPMode::ThreadSafe> onComplete;
public:
	ChunkMeshUpdateAsyncTask(AVoxelTerrain* VoxelTerrain, UVoxelTerrainChunkComponent* Component, TSharedPtr<FUpdateComplete, ESPMode::ThreadSafe> OnComplete)
	{
		voxelTerrain = VoxelTerrain;
		component = Component;
		onComplete = OnComplete;
	}

	/*This function is needed from the API of the engine.
	My guess is that it provides necessary information
	about the thread that we occupy and the progress of our task*/
	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(ChunkMeshUpdateAsyncTask, STATGROUP_ThreadPoolAsyncTasks);
	}

	void DoWork();
};

UCLASS()
class VOXELTERRAIN_API UVoxelTerrainChunkComponent : public UProceduralMeshComponent
{
	GENERATED_UCLASS_BODY()

public:
	~UVoxelTerrainChunkComponent();
	void Initialize(FIntPoint, UMaterialInterface*);

	int GetPaintLayer(UVoxelTerrainMaterial*);
	FVoxelTerrainVoxel* GetVoxelAt(FIntVector voxelIndex, bool edit = false);
	UPROPERTY()
		FIntPoint Position;

	UPROPERTY()
		UVoxelTerrainChunkData* chunkData;

	UFUNCTION(BlueprintCallable, Category = "Voxel Terrain Component")
		void UpdateMesh();
	UFUNCTION(BlueprintCallable, Category = "Voxel Terrain Component")
		void UpdateFoliage();

	TArray<FVector> positions;
	TArray<FColor> colors;
	TArray<int32> triangles;
	TArray<FVector> normals;
	TArray<FoliageLayerInfo*> layers;
	TArray<UHierarchicalInstancedStaticMeshComponent*> foliageComponents;
private:
	FAsyncTask<ChunkMeshUpdateAsyncTask>* updateTask;
	FAsyncTask<FoliageBuilderAsyncTask>* foliageTask;

	TSharedPtr<FUpdateComplete, ESPMode::ThreadSafe> OnUpdateComplete;
	TSharedPtr<FFoliageComplete, ESPMode::ThreadSafe> OnFoliageComplete;

	float SeededRandom(int& seed);
	void UpdateComplete();
	void FoliageComplete();
	void UpdateMaterialTextures(int layerIndex);
	bool queueUpdate = false;
	bool queueFoliage = false;
	UPROPERTY()
		bool saved = false;
	AVoxelTerrain* voxelTerrain;
};