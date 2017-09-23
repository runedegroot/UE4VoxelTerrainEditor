#include "VoxelTerrainPrivatePCH.h"
#include "VoxelTerrainPaintTool.h"
#include "VoxelTerrainUtils.h"
#include "VoxelTerrainActor.h"
#include "VoxelTerrainData.h"
#include "VoxelTerrainChunkComponent.h"

DECLARE_CYCLE_STAT(TEXT("Paint/Tick"), STAT_PaintTick, STATGROUP_VoxelTerrainPaintTool);

bool FVoxelTerrainPaintTool::Click() {
	return false;
}

bool FVoxelTerrainPaintTool::Enter() {
	voxelTerrain = nullptr;
	return true;
}

void FVoxelTerrainPaintTool::Exit() {
	if (voxelTerrain != nullptr)
	{
		voxelTerrain->UpdateBrushPosition(FVector((float)INT_MAX));
		voxelTerrain = nullptr;
	}
	pressed = false;
	shift = false;
}

bool FVoxelTerrainPaintTool::MouseMove() {
	return false;
}

bool FVoxelTerrainPaintTool::InputKey(FViewport * viewport, FKey Key, EInputEvent Event) {
	if (voxelTerrain == nullptr)
		return false;
	if (Key == EKeys::LeftMouseButton && Event != EInputEvent::IE_Repeat)
		pressed = (Event == EInputEvent::IE_Pressed);
	else if (Key == EKeys::LeftShift && Event != EInputEvent::IE_Repeat)
		shift = (Event == EInputEvent::IE_Pressed);
	return false;
}

void FVoxelTerrainPaintTool::Render(FViewport * viewport, const FSceneView * view) {
	//
}

void FVoxelTerrainPaintTool::Tick(FViewport * viewport, const FSceneView * view, float deltaTime) {
	SCOPE_CYCLE_COUNTER(STAT_PaintTick);

	FVector2D pos = FVector2D(viewport->GetMouseX(), viewport->GetMouseY());
	FVector RayOrigin = FVector();
	FVector RayDirection = FVector();
	FSceneView::DeprojectScreenToWorld(pos, view->UnconstrainedViewRect, view->ViewMatrices.GetInvViewProjectionMatrix(), RayOrigin, RayDirection);
	//TRACE IN DIR :D
	hitResult = new FHitResult(ForceInit);
	FCollisionQueryParams paramsCall = FCollisionQueryParams(FName(TEXT("Trace")), true);
	//Ignore player if available
	APlayerController* playerController = viewport->GetClient()->GetWorld()->GetFirstPlayerController();
	if (playerController != NULL)
		paramsCall.AddIgnoredActor(playerController->GetPawn());
	bool traced = viewport->GetClient()->GetWorld()->LineTraceSingleByChannel(*hitResult, RayOrigin, RayOrigin + RayDirection * WORLD_MAX, ECollisionChannel::ECC_WorldDynamic, paramsCall);
	if (traced && hitResult->GetActor()->IsA<AVoxelTerrain>())
	{
		AVoxelTerrain* currentTerrain = (AVoxelTerrain*)hitResult->GetActor();
		if (currentTerrain != voxelTerrain)
		{
			if (voxelTerrain != nullptr) {
				voxelTerrain->UpdateBrushPosition(FVector((float)INT_MAX));
			}
			voxelTerrain = currentTerrain;
		}
		currentTerrain->UpdateBrushPosition(hitResult->ImpactPoint);
		currentTerrain->UpdateBrushInfo(BrushSize, BrushFalloff);

		if (pressed)
		{
			//Calculate diameter in voxel space

			UVoxelTerrainData* data = voxelTerrain->Settings;
			FVector center = UVoxelTerrainUtils::WorldPosToTerrainPos(hitResult->ImpactPoint, voxelTerrain) + hitResult->Normal;
			float radius = ((BrushSize*0.01f)*data->VoxelsPerMeter + 2)*0.5f;
			for (int x = center.X - radius; x < center.X + radius; x++)
			{
				for (int y = center.Y - radius; y < center.Y + radius; y++)
				{
					for (int z = center.Z - radius; z < center.Z + radius; z++)
					{
						if (z < 0 || z >= data->TerrainHeight)
							continue;
						FVector voxelToCheck = FVector(x, y, z);
						if (FVector::Dist(center, voxelToCheck) > radius) {
							continue;
						}

						FIntPoint chunkPos;
						chunkPos.X = FMath::FloorToInt(x / (float)data->ChunkSize);
						chunkPos.Y = FMath::FloorToInt(y / (float)data->ChunkSize);
						FIntVector localPos;
						localPos.X = x%data->ChunkSize;
						if (localPos.X < 0)
							localPos.X = data->ChunkSize - FMath::Abs(localPos.X);
						localPos.Y = y%data->ChunkSize;
						if (localPos.Y < 0)
							localPos.Y = data->ChunkSize - FMath::Abs(localPos.Y);
						localPos.Z = z;
						UVoxelTerrainChunkComponent* component = voxelTerrain->GetChunkComponentAt(chunkPos);
						if (!component)
							continue;
						UVoxelTerrainChunkData* chunkData = component->chunkData;

						if (!shift && PaintMaterial)
						{
							if (component->GetPaintLayer(PaintMaterial) != -1) {
								FVoxelTerrainVoxel* voxel = component->GetVoxelAt(FIntVector(localPos.X, localPos.Y, localPos.Z), true);
								if (voxel)
									voxel->Material = PaintMaterial;
							}
						}
						else if (shift)
						{
							FVoxelTerrainVoxel* voxel = component->GetVoxelAt(FIntVector(localPos.X, localPos.Y, localPos.Z), true);
							if (voxel)
								voxel->Material = NULL;
						}
					}

				}
			}

			UE_LOG(LogTemp, Warning, TEXT("update brush"));
			FIntPoint chunkPos;
			chunkPos.X = FMath::FloorToInt(center.X / (float)data->ChunkSize);
			chunkPos.Y = FMath::FloorToInt(center.Y / (float)data->ChunkSize);
			int range = (radius + 1) / data->ChunkSize;
			if (range <= 0)
				range = 1;
			voxelTerrain->UpdateChunks(chunkPos, range);
		}
	}
	else
	{
		if (voxelTerrain != nullptr)
		{
			voxelTerrain->UpdateBrushPosition(FVector((float)INT_MAX));
			voxelTerrain = nullptr;
		}
	}
}