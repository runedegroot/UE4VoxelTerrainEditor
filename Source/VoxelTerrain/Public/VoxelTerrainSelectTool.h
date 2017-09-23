#pragma once
#include "VoxelTerrainTool.h"

class VOXELTERRAIN_API FVoxelTerrainSelectTool : public FVoxelTerrainTool
{
public:
	virtual bool Click() override;
	virtual bool Enter() override;
	virtual void Exit() override;
	virtual bool MouseMove() override;
	virtual bool InputKey(FViewport *, FKey, EInputEvent) override;
	virtual void Render(FViewport *, const FSceneView *) override;
	virtual void Tick(FViewport*, const FSceneView *, float) override;
	virtual const TCHAR* GetToolName() override { return TEXT("Tool_Select"); }
};