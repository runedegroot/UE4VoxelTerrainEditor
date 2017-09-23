#pragma once
#include "VoxelTerrainTool.h"

DECLARE_STATS_GROUP(TEXT("VoxelTerrainPaintTool"), STATGROUP_VoxelTerrainPaintTool, STATCAT_Advanced);

class UVoxelTerrainMaterial;
class AVoxelTerrain;
class VOXELTERRAIN_API FVoxelTerrainPaintTool : public FVoxelTerrainTool
{
public:
	virtual bool Click() override;
	virtual bool Enter() override;
	virtual void Exit() override;
	virtual bool MouseMove() override;
	virtual bool InputKey(FViewport *, FKey, EInputEvent) override;
	virtual void Render(FViewport *, const FSceneView *) override;
	virtual void Tick(FViewport*, const FSceneView *, float) override;
	virtual const TCHAR* GetToolName() override { return TEXT("Tool_Paint"); }
	UVoxelTerrainMaterial* PaintMaterial;
private:
	AVoxelTerrain* voxelTerrain;
	FHitResult* hitResult;
	bool pressed = false;
	bool shift = false;
};