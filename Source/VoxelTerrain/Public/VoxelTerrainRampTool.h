#pragma once
#include "VoxelTerrainTool.h"

class VOXELTERRAIN_API FVoxelTerrainRampTool : public FVoxelTerrainTool
{
public:
	virtual bool Click() override;
	virtual bool Enter() override;
	virtual void Exit() override;
	virtual bool MouseMove() override;
	virtual bool InputKey(FViewport *, FKey, EInputEvent) override;
	virtual void Render(FViewport *, const FSceneView *) override;
	virtual void Tick(FViewport*, const FSceneView *, float) override;
	virtual const TCHAR* GetToolName() override { return TEXT("Tool_Ramp"); }
private:
	AVoxelTerrain* voxelTerrain;
	FHitResult* hitResult;
	FPlane* plane = nullptr;
	bool pressed = false;
	bool shift = false;
	float CalculateIncreaseValue(FVector voxelToInvestigate, FVector voxelBrushCenter, float voxelRadius);
	float CalculateMaxValue(float distance);
};