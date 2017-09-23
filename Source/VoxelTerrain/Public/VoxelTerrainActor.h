#pragma once

#include "GameFramework/Actor.h"
#include "VoxelTerrainData.h"
#include "VoxelTerrainActor.generated.h"

//For UE4 Profiler ~ Stat Group
DECLARE_STATS_GROUP(TEXT("VoxelTerrainActor"), STATGROUP_VoxelTerrainActor, STATCAT_Advanced);

class FVoxelTerrainTool;
class UVoxelTerrainNoise;
class FVoxelTerrainSculptTool;

USTRUCT()
struct FVoxelTerrainChunkComponentMap
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY()
		TMap<int, UVoxelTerrainChunkComponent*> chunkComponents;

	UVoxelTerrainChunkComponent* operator[](int32 Index)
	{
		//if (Values.Num() > Index)
		return chunkComponents[Index];
	}
};

UENUM()
enum EVoxelTerrainTools
{
	Sculpt,
	Smooth,
	Flatten,
	Ramp,
	Retopologize,
	Paint
};

UCLASS()
class VOXELTERRAIN_API AVoxelTerrain : public AActor
{
	GENERATED_UCLASS_BODY()
public:
	virtual void OnConstruction(const FTransform & Transform) override;
	virtual void Destroyed() override;
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

	void CreateVoxelTerrain(UVoxelTerrainData * settings);

	UFUNCTION(BlueprintCallable, Category = "Voxel Terrain Actor")
		UVoxelTerrainChunkComponent* GetChunkComponentAt(FIntPoint chunkIndex);

	void UpdateBrushPosition(FVector);
	void UpdateBrushInfo(float size, float falloff);

	UFUNCTION(BlueprintCallable, Category = "Voxel Terrain Actor")
		void ActivateTool(EVoxelTerrainTools tool, APlayerController* playerController);

	UFUNCTION(BlueprintCallable, Category = "Voxel Terrain Actor")
		void UpdateTool(float brushFalloff = -1.f, float brushSize = -1.f, float toolStrength = -1.f);

	UFUNCTION(BlueprintCallable, Category = "Voxel Terrain Actor")
		void DeactivateTool();

	//Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		UVoxelTerrainData* Settings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		UVoxelTerrainMaterial* PaintMaterial;

	float UnitsPerVoxel = -1;

	UFUNCTION(BlueprintCallable, Category = "Voxel Terrain Actor")
		void UpdateChunks(FIntPoint inPos, int radius);

	UFUNCTION(BlueprintCallable, Category = "Voxel Terrain Actor")
		void UpdateAllChunks();

	UFUNCTION(BlueprintCallable, Category = "Voxel Terrain Actor")
		void ViewChunks(FIntPoint inPos, int radius);

	//Get chunk
	UVoxelTerrainChunkData* GenerateChunkData(FIntPoint inPos);

	//Get voxel at world position
	FVoxelTerrainVoxel* GetVoxelAt(FIntVector inPos, bool edit = false);
	FVoxelTerrainVoxel* GetVoxelLocalAt(FIntVector inPos, FIntPoint chunkPos, bool edit = false);

	UFUNCTION(BlueprintCallable, Category = "Voxel Terrain Actor")
		FVoxelTerrainVoxel& GetVoxelAtBP(FIntVector inPos);

	UFUNCTION(BlueprintCallable, Category = "Voxel Terrain Actor")
		void SetVoxelAtBP(FIntVector inPos, UPARAM(ref) FVoxelTerrainVoxel& voxel);

	UPROPERTY(Transient)
		UMaterialInstanceDynamic* DynamicBrushMaterial;
	UFUNCTION(BlueprintCallable, Category = "Voxel Terrain Actor")
		void RemoveChunkAt(FIntPoint chunkIndex);
	UFUNCTION(BlueprintCallable, Category = "Voxel Terrain Actor")
		void RemoveChunk(UVoxelTerrainChunkComponent* chunkComponent);
	UPROPERTY(Transient)
		TMap<int, FVoxelTerrainChunkComponentMap> chunkComponents;

	UFUNCTION(BlueprintCallable, Category = "Voxel Terrain Actor")
		void UpdateChunk(UVoxelTerrainChunkComponent* chunkComponent);
	UFUNCTION(BlueprintCallable, Category = "Voxel Terrain Actor")
		bool UpdateChunkAt(FIntPoint chunkIndex);
	UFUNCTION(BlueprintCallable, Category = "Voxel Terrain Actor")
		UVoxelTerrainChunkComponent* LoadChunkAt(FIntPoint chunkIndex);

	UFUNCTION(BlueprintCallable, Category = "Voxel Terrain Actor")
		void SetFoliageEnabled(bool enabled);
	UPROPERTY(Transient)
		bool renderFoliage = true;
private:
	FCriticalSection mutex;
	UPROPERTY(Transient)
		UMaterial* brushMaterial;
	void Initialize();
	UPROPERTY(Transient)
		UVoxelTerrainNoise* surfaceNoise;
	UPROPERTY(Transient)
		UVoxelTerrainNoise* caveNoise;
	float GetPerlinValue(int x, int y, int z);

	//Tools
	FVoxelTerrainTool * activeTool;
	APlayerController* activeController;
	TArray<FVoxelTerrainTool*> tools;

	FVoxelTerrainTool* GetTool(FName ToolName);

};
