#pragma once
#include "Object.h"

class FVoxelTerrainEdMode;
class VOXELTERRAIN_API FVoxelTerrainTool
{
public:
	virtual bool Click() = 0;
	virtual bool Enter() = 0;
	virtual void Exit() = 0;
	virtual bool MouseMove() = 0;
	virtual bool InputKey(FViewport *, FKey, EInputEvent) = 0;
	virtual void Render(FViewport *, const FSceneView *) = 0;
	virtual void Tick(FViewport*, const FSceneView *, float) = 0;
	virtual const TCHAR* GetToolName() = 0;
	float BrushSize = 500.f;
	float BrushFalloff = 0.5f;
	float ToolStrength = 0.3f;
};