// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Object.h"
#include "VoxelTerrainData.generated.h"

UENUM()
enum EVoxelTerrainMode
{
	VTM_Dynamic 	UMETA(DisplayName = "Dynamic"),
	VTM_Static		UMETA(DisplayName = "Static")
};

UCLASS(BlueprintType)
class UVoxelTerrainMaterial : public UDataAsset
{
	GENERATED_UCLASS_BODY()

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor Textures")
		UTexture* FloorTextureC;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor Textures")
		UTexture* FloorTextureN;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor Textures")
		UTexture* FloorTextureD;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Textures")
		UTexture* WallTextureC;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Textures")
		UTexture* WallTextureN;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Textures")
		UTexture* WallTextureD;
};

UENUM()
enum EVoxelTerrainFoliageScaling
{
	/** Foliage instances will have uniform X,Y and Z scales. */
	Uniform,
	/** Foliage instances will have random X,Y and Z scales. */
	Free,
	/** Locks the X and Y axis scale. */
	LockXY,
	/** Locks the X and Z axis scale. */
	LockXZ,
	/** Locks the Y and Z axis scale. */
	LockYZ
};

UCLASS(BlueprintType)
class UVoxelTerrainFoliageType : public UDataAsset
{
	GENERATED_UCLASS_BODY()

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
		UStaticMesh* Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
		float Density = 200.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
		bool AlignToNormal = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
		float SlopeLimit = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
		bool RandomRotation = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
		float HeightOffset = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scaling")
		TEnumAsByte<EVoxelTerrainFoliageScaling> Scaling = EVoxelTerrainFoliageScaling::Uniform;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scaling")
		FFloatInterval ScaleX;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scaling")
		FFloatInterval ScaleY;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scaling")
		FFloatInterval ScaleZ;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
		bool CastStaticShadow = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
		bool CastDynamicShadow = true;
};

USTRUCT(BlueprintType)
struct FVoxelTerrainVoxel
{
	GENERATED_USTRUCT_BODY()
		UPROPERTY(BlueprintReadWrite)
		float Value = -1;
	UPROPERTY(BlueprintReadWrite)
		UVoxelTerrainMaterial* Material;
};


USTRUCT()
struct FVoxelTerrainRawDataZ
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY()
		TArray<FVoxelTerrainVoxel> Voxels;

	FVoxelTerrainRawDataZ()
	{
	}

	FVoxelTerrainVoxel& operator[](int32 Index)
	{
		//if (Values.Num() > Index)
		return Voxels[Index];
	}

	TArray<FVoxelTerrainVoxel>& operator=(TArray<FVoxelTerrainVoxel>&& Other)
	{
		Voxels = Other;
		return Voxels;
	}

	int Num() {
		return Voxels.Num();
	}
};

USTRUCT()
struct FVoxelTerrainRawDataY
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY()
		TArray<FVoxelTerrainRawDataZ> Values;

	FVoxelTerrainRawDataY()
	{
	}

	FVoxelTerrainRawDataZ& operator[](int32 Index)
	{
		//if(Values.Num() > Index)
		return Values[Index];
	}

	TArray<FVoxelTerrainRawDataZ>& operator=(TArray<FVoxelTerrainRawDataZ>&& Other)
	{
		Values = Other;
		return Values;
	}

	int Num() {
		return Values.Num();
	}
};

USTRUCT()
struct FVoxelTerrainRawDataX
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY()
		TArray<FVoxelTerrainRawDataY> Values;

	FVoxelTerrainRawDataX()
	{
	}

	FVoxelTerrainRawDataY& operator[](int32 Index)
	{
		//if (Values.Num() > Index)
		return Values[Index];
	}

	TArray<FVoxelTerrainRawDataY>& operator=(TArray<FVoxelTerrainRawDataY>&& Other)
	{
		Values = Other;
		return Values;
	}

	int Num() {
		return Values.Num();
	}
};

UCLASS()
class UVoxelTerrainChunkData : public UObject
{
	GENERATED_UCLASS_BODY()
public:

	UPROPERTY()
		FVoxelTerrainRawDataX terrain;

	UPROPERTY()
		TArray<UVoxelTerrainMaterial*> layers;

	FVoxelTerrainRawDataY& operator[](int32 Index)
	{
		//if (Values.Num() > Index)
		return terrain[Index];
	}

	int Num() {
		return terrain.Num();
	}

	FVoxelTerrainVoxel* GetVoxelAt(FIntVector inPos);
};

USTRUCT()
struct FVoxelTerrainChunkDataMap
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY()
		TMap<int, UVoxelTerrainChunkData*> chunks;

	UVoxelTerrainChunkData* operator[](int32 Index)
	{
		//if (Values.Num() > Index)
		return chunks[Index];
	}

};

UCLASS()
class VOXELTERRAIN_API UVoxelTerrainData : public UObject
{
	GENERATED_UCLASS_BODY()
private:
	//Edited chunk data
	UPROPERTY()
		TMap<int, FVoxelTerrainChunkDataMap> chunks;
public:
	UVoxelTerrainChunkData* GetChunkDataAt(FIntPoint);
	UVoxelTerrainChunkData* SetChunkDataAt(FIntPoint, UVoxelTerrainChunkData*);

	UPROPERTY(Category = "Voxel Terrain Settings", EditAnywhere, meta = (/*DisplayName = "Material", */ShowForTools = "Select"), BlueprintReadWrite)
		UMaterialInterface* material;
	UPROPERTY(Category = "Voxel Terrain Settings", EditAnywhere, meta = (ShowForTools = "Select"), BlueprintReadWrite)
		TEnumAsByte<EVoxelTerrainMode> TerrainMode = EVoxelTerrainMode::VTM_Dynamic;
	UPROPERTY(Category = "Voxel Terrain Settings", EditAnywhere, meta = (ShowForTools = "Select"), BlueprintReadWrite)
		bool MergeVertices = true;
	UPROPERTY(Category = "Voxel Terrain Settings", EditAnywhere, meta = (ShowForTools = "Select"), BlueprintReadWrite)
		float VoxelsPerMeter = 1;
	UPROPERTY(Category = "Voxel Terrain Settings", EditAnywhere, meta = (ShowForTools = "Select"), BlueprintReadWrite)
		int TerrainSize = 6;
	UPROPERTY(Category = "Voxel Terrain Settings", EditAnywhere, meta = (ShowForTools = "Select"), BlueprintReadWrite)
		int TerrainHeight = 64;
	UPROPERTY(Category = "Voxel Terrain Settings", EditAnywhere, meta = (ShowForTools = "Select"), BlueprintReadWrite)
		int ChunkSize = 10;
	UPROPERTY(Category = "Voxel Terrain Settings", EditAnywhere, meta = (ShowForTools = "Select", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"), BlueprintReadWrite)
		bool Smooth = true;
	UPROPERTY(Category = "Voxel Terrain Settings", EditAnywhere, meta = (ShowForTools = "Select"), BlueprintReadWrite)
		bool UsePerlinNoise = true;
	UPROPERTY(Category = "Voxel Terrain Settings", EditAnywhere, meta = (ShowForTools = "Select"), BlueprintReadWrite)
		TArray<UVoxelTerrainFoliageType*> FoliageTypes;

	//Noise settings
	UPROPERTY(Category = "Perlin Noise Settings", EditAnywhere, meta = (ShowForTools = "Select"), BlueprintReadWrite)
		int Seed = 1337;
	UPROPERTY(Category = "Perlin Noise Settings", EditAnywhere, meta = (ShowForTools = "Select"), BlueprintReadWrite)
		float Lacunarity = 2.0f;
	UPROPERTY(Category = "Perlin Noise Settings", EditAnywhere, meta = (ShowForTools = "Select"), BlueprintReadWrite)
		float Frequency = 1.0f;
	UPROPERTY(Category = "Perlin Noise Settings", EditAnywhere, meta = (ShowForTools = "Select"), BlueprintReadWrite)
		float Amplitude = 25.0f;
	UPROPERTY(Category = "Perlin Noise Settings", EditAnywhere, meta = (ShowForTools = "Select"), BlueprintReadWrite)
		int Octaves = 4;

	//Cave settings
	UPROPERTY(Category = "Perlin Noise Settings", EditAnywhere, meta = (ShowForTools = "Select"), BlueprintReadWrite)
		bool Caves = true;
	UPROPERTY(Category = "Perlin Noise Settings", EditAnywhere, meta = (ShowForTools = "Select", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"), BlueprintReadWrite)
		float CaveWidth = 0.6f;
	UPROPERTY(Category = "Perlin Noise Settings", EditAnywhere, meta = (ShowForTools = "Select"), BlueprintReadWrite)
		float CaveFrequency = 3.0f;
	UPROPERTY(Category = "Perlin Noise Settings", EditAnywhere, meta = (ShowForTools = "Select"), BlueprintReadWrite)
		float CaveHorizontalMultiplier = 0.4f;
	UPROPERTY(Category = "Perlin Noise Settings", EditAnywhere, meta = (ShowForTools = "Select"), BlueprintReadWrite)
		float CaveVerticalMultiplier = 1.3f;

};