#include "VoxelTerrainPrivatePCH.h"
#include "VoxelTerrainSelectTool.h"

bool FVoxelTerrainSelectTool::Click() {
	return false;
}

bool FVoxelTerrainSelectTool::Enter() {
	return true;
}

void FVoxelTerrainSelectTool::Exit() {
	//
}

bool FVoxelTerrainSelectTool::MouseMove() {
	return false;
}

bool FVoxelTerrainSelectTool::InputKey(FViewport * viewport, FKey Key, EInputEvent Event) {
	return false;
}

void FVoxelTerrainSelectTool::Render(FViewport * viewport, const FSceneView * view) {
	//
}

void FVoxelTerrainSelectTool::Tick(FViewport * viewport, const FSceneView * view, float deltaTime) {
	//
}