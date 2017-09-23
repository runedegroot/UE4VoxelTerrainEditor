// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Object.h"
#include "VoxelTerrainData.h"
#include "VoxelTerrainEdModeData.generated.h"

UCLASS()
class VOXELTERRAINEDITOR_API UVoxelTerrainEdModeData : public UObject
{
	GENERATED_UCLASS_BODY()
public:

	/////////////////////////
	////GENERATE SETTINGS////
	/////////////////////////

	UPROPERTY(Category = "Terrain Settings", EditAnywhere, meta = (/*DisplayName = "Material", */ShowForTools = "Tool_Select"), BlueprintReadWrite)
		UMaterialInterface* material;
	UPROPERTY(Category = "Terrain Settings", EditAnywhere, meta = (ShowForTools = "Tool_Select"))
		TArray<UVoxelTerrainFoliageType*> FoliageTypes;
	UPROPERTY(Category = "Terrain Settings", EditAnywhere, meta = (ShowForTools = "Tool_Select"))
		TEnumAsByte<EVoxelTerrainMode> TerrainMode = EVoxelTerrainMode::VTM_Dynamic;
	UPROPERTY(Category = "Terrain Settings", EditAnywhere, meta = (ShowForTools = "Tool_Select"))
		bool MergeVertices = true;
	UPROPERTY(Category = "Terrain Settings", EditAnywhere, meta = (ShowForTools = "Tool_Select"))
		float VoxelsPerMeter = 1;
	UPROPERTY(Category = "Terrain Settings", EditAnywhere, meta = (ShowForTools = "Tool_Select"))
		int TerrainSize = 6;
	UPROPERTY(Category = "Terrain Settings", EditAnywhere, meta = (ShowForTools = "Tool_Select"))
		int TerrainHeight = 64;
	UPROPERTY(Category = "Terrain Settings", EditAnywhere, meta = (ShowForTools = "Tool_Select"))
		int ChunkSize = 10;
	UPROPERTY(Category = "Terrain Settings", EditAnywhere, meta = (ShowForTools = "Tool_Select", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
		bool Smooth = true;
	UPROPERTY(Category = "Terrain Settings", EditAnywhere, meta = (ShowForTools = "Tool_Select"))
		bool UsePerlinNoise = true;

	//Noise settings
	UPROPERTY(Category = "Perlin Noise", EditAnywhere, meta = (ShowForTools = "Tool_Select"))
		int RandomSeed = 1337;
	UPROPERTY(Category = "Perlin Noise", EditAnywhere, meta = (ShowForTools = "Tool_Select"))
		float Lacunarity = 2.0f;
	UPROPERTY(Category = "Perlin Noise", EditAnywhere, meta = (ShowForTools = "Tool_Select"))
		float Frequency = 1.0f;
	UPROPERTY(Category = "Perlin Noise", EditAnywhere, meta = (ShowForTools = "Tool_Select"))
		float Amplitude = 25.0f;
	UPROPERTY(Category = "Perlin Noise", EditAnywhere, meta = (ShowForTools = "Tool_Select"))
		int Octaves = 4;

	//Cave settings
	UPROPERTY(Category = "Perlin Noise", EditAnywhere, meta = (ShowForTools = "Tool_Select"))
		bool Caves = true;
	UPROPERTY(Category = "Perlin Noise", EditAnywhere, meta = (ShowForTools = "Tool_Select", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
		float CaveWidth = 0.6f;
	UPROPERTY(Category = "Perlin Noise", EditAnywhere, meta = (ShowForTools = "Tool_Select"))
		float CaveFrequency = 3.0f;
	UPROPERTY(Category = "Perlin Noise", EditAnywhere, meta = (ShowForTools = "Tool_Select"))
		float CaveHorizontalMultiplier = 0.4f;
	UPROPERTY(Category = "Perlin Noise", EditAnywhere, meta = (ShowForTools = "Tool_Select"))
		float CaveVerticalMultiplier = 1.3f;

	//////////////////////
	////PAINT SETTINGS////
	//////////////////////

	UPROPERTY(Category = "Paint Settings", EditAnywhere, meta = (ShowForTools = "Tool_Paint"))
		UVoxelTerrainMaterial* PaintMaterial;

	//////////////////////
	////BRUSH SETTINGS////
	//////////////////////

	UPROPERTY(Category = "Brush Settings", EditAnywhere, meta = (ShowForTools = "Tool_Sculpt,Tool_Smooth,Tool_Flatten,Tool_Ramp,Tool_Retopologize,Tool_Paint", ClampMin = "150.0", ClampMax = "5000.0", UIMin = "150.0", UIMax = "5000.0"))
		float BrushSize = 500.f;
	UPROPERTY(Category = "Brush Settings", EditAnywhere, meta = (ShowForTools = "Tool_Sculpt,Tool_Smooth,Tool_Flatten,Tool_Ramp,Tool_Retopologize,Tool_Paint", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
		float BrushFalloff = 0.5f;
	UPROPERTY(Category = "Tool Settings", EditAnywhere, meta = (ShowForTools = "Tool_Sculpt,Tool_Smooth,Tool_Flatten,Tool_Ramp,Tool_Retopologize", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
		float ToolStrength = 0.3f;

};