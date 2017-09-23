#include "VoxelTerrainPrivatePCH.h"
#include "VoxelTerrainRampTool.h"
#include "VoxelTerrainUtils.h"
#include "VoxelTerrainActor.h"
#include "VoxelTerrainData.h"

bool FVoxelTerrainRampTool::Click() {
	return false;
}

bool FVoxelTerrainRampTool::Enter() {
	voxelTerrain = nullptr;
	return true;
}

void FVoxelTerrainRampTool::Exit() {
	if (voxelTerrain != nullptr)
	{
		voxelTerrain->UpdateBrushPosition(FVector((float)INT_MAX));
		voxelTerrain = nullptr;
	}
	pressed = false;
	shift = false;
	if (plane)
		delete plane;
	plane = NULL;
}

bool FVoxelTerrainRampTool::MouseMove() {
	return false;
}

bool FVoxelTerrainRampTool::InputKey(FViewport * viewport, FKey Key, EInputEvent Event) {
	if (voxelTerrain == nullptr)
		return false;
	if (Key == EKeys::LeftMouseButton && Event != EInputEvent::IE_Repeat)
	{
		pressed = (Event == EInputEvent::IE_Pressed);
		if (!pressed && plane)
		{
			delete plane;
			plane = NULL;
		}
	}
	else if (Key == EKeys::LeftShift && Event != EInputEvent::IE_Repeat)
		shift = (Event == EInputEvent::IE_Pressed);
	return false;
}

void FVoxelTerrainRampTool::Render(FViewport * viewport, const FSceneView * view) {
	//
}

void FVoxelTerrainRampTool::Tick(FViewport * viewport, const FSceneView * view, float deltaTime) {
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
			UVoxelTerrainData* data = voxelTerrain->Settings;
			FVector center = UVoxelTerrainUtils::WorldPosToTerrainPos(hitResult->ImpactPoint, voxelTerrain);

			if (!plane)
				plane = new FPlane(center, hitResult->Normal);

			//Calculate diameter in voxel space
			float radius = ((BrushSize*0.01f)*data->VoxelsPerMeter + 2)*0.5f;
			for (int x = center.X - radius; x < center.X + radius; x++)
			{
				for (int y = center.Y - radius; y < center.Y + radius; y++)
				{
					for (int z = center.Z - radius; z < center.Z + radius; z++)
					{
						if (z < 0 || z >= data->TerrainHeight)
							continue;
						FVoxelTerrainVoxel* voxel = voxelTerrain->GetVoxelAt(FIntVector(x, y, z), true);
						bool voxelNegative = voxel->Value < 0;
						FVector voxelToCheck = FVector(x, y, z);
						bool found = false;
						bool foundOther = false;
						for (int a = voxelToCheck.X - 1; a <= voxelToCheck.X + 1; a++)
						{
							for (int b = voxelToCheck.Y - 1; b <= voxelToCheck.Y + 1; b++)
							{
								for (int c = voxelToCheck.Z - 1; c <= voxelToCheck.Z + 1; c++)
								{
									if (c < 0 || c >= data->TerrainHeight)
										continue;
									FVoxelTerrainVoxel* currentVoxel = voxelTerrain->GetVoxelAt(FIntVector(a, b, c), true);
									if (currentVoxel == voxel)
										continue;
									if (currentVoxel->Value == 1 || currentVoxel->Value == -1) {
										found = true;
									}
									if (voxelNegative && currentVoxel->Value > 0)
										foundOther = true;
									if (!voxelNegative && currentVoxel->Value < 0)
										foundOther = true;
								}
							}
						}
						if (!foundOther) {
							if (voxel->Value > 0)
								voxel->Value = 1;
							else
								voxel->Value = -1;
						}
						if (foundOther && found) {
							voxel->Value += CalculateIncreaseValue(voxelToCheck, center, radius)*deltaTime * 3;
							float normalDot = plane->PlaneDot(voxelToCheck);
							if (normalDot < 0)
							{
								voxel->Value = FMath::Clamp<float>(voxel->Value, -1.f, CalculateMaxValue(normalDot));
							}
							else
							{
								voxel->Value = FMath::Clamp<float>(voxel->Value, -CalculateMaxValue(normalDot), 1.0f);
							}
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

//This function uses the dot together with the falloff to determine how much the voxel needs to be increased
float FVoxelTerrainRampTool::CalculateIncreaseValue(FVector voxelToInvestigate, FVector voxelBrushCenter, float voxelRadius)
{
	//distance used for brush falloff multiplier
	float distance = FVector::Dist(voxelToInvestigate, voxelBrushCenter);
	//difference used to get normal dot value (negative becomes positive multiplier)
	FVector difference = voxelToInvestigate - voxelBrushCenter;
	float normalDot = plane->PlaneDot(voxelToInvestigate);

	//Use distance, dot and toolstrength to calculate increase value
	float innerRadius = voxelRadius*(1 - BrushFalloff);
	if (distance < innerRadius)
	{
		if (normalDot < 0)
			return ToolStrength*FMath::Abs(normalDot);
		else if (normalDot > 0)
			return -(ToolStrength*FMath::Abs(normalDot));
		return 0.f;
	}
	float slerp = FMath::SmoothStep(0, 1, 1 - ((distance - innerRadius) / (voxelRadius - innerRadius)));
	float result = ToolStrength*slerp*FMath::Abs(normalDot);
	if (normalDot < 0)
		return result;
	else if (normalDot > 0)
		return -result;
	return 0.f;
}

float FVoxelTerrainRampTool::CalculateMaxValue(float distance)
{
	float interLength = 1 / voxelTerrain->Settings->VoxelsPerMeter;
	if (distance > interLength)
		return 1;
	//1.33
	return FMath::Abs(distance / interLength);
}