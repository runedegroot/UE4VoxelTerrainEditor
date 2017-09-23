#include "VoxelTerrainPrivatePCH.h"
#include "VoxelTerrainSculptTool.h"
#include "VoxelTerrainUtils.h"
#include "VoxelTerrainActor.h"
#include "VoxelTerrainData.h"

DECLARE_CYCLE_STAT(TEXT("Sculpt/Tick"), STAT_SculptTick, STATGROUP_VoxelTerrainSculptTool);

bool FVoxelTerrainSculptTool::Click() {
	return false;
}

bool FVoxelTerrainSculptTool::Enter() {
	voxelTerrain = nullptr;
	return true;
}

void FVoxelTerrainSculptTool::Exit() {
	if (voxelTerrain != nullptr)
	{
		voxelTerrain->UpdateBrushPosition(FVector((float)INT_MAX));
		voxelTerrain = nullptr;
	}
	pressed = false;
	shift = false;
}

bool FVoxelTerrainSculptTool::MouseMove() {
	return false;
}

bool FVoxelTerrainSculptTool::InputKey(FViewport * viewport, FKey Key, EInputEvent Event) {
	if (voxelTerrain == nullptr)
		return false;
	if (Key == EKeys::LeftMouseButton && Event != EInputEvent::IE_Repeat)
		pressed = (Event == EInputEvent::IE_Pressed);
	else if (Key == EKeys::LeftShift && Event != EInputEvent::IE_Repeat)
		shift = (Event == EInputEvent::IE_Pressed);
	return false;
}

void FVoxelTerrainSculptTool::Render(FViewport * viewport, const FSceneView * view) {
	//
}

void FVoxelTerrainSculptTool::Tick(FViewport * viewport, const FSceneView * view, float deltaTime) {

	SCOPE_CYCLE_COUNTER(STAT_SculptTick);

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
		//DrawDebugSphere(viewport->GetClient()->GetWorld(), HitCall.ImpactPoint, 100, 32, FColor::Blue);
		//Debugs the normal with the terrain at the mouse position
		//DrawDebugLine(viewport->GetClient()->GetWorld(), HitCall.ImpactPoint, HitCall.ImpactPoint + HitCall.Normal * 200, FColor::Blue);

		//Too change voxels, check the surrounding X voxels where X is the tool radius + 1
		//If a voxel is 100% inside the sphere, change its value to 1.
		//If not, check the sphere radius difference and apply that value
		//MAYBE: Maybe we should only change voxels in the normal direction (use DOT to check if in front)
		//TODO: Do not instantly change the values but smoothly increase them using deltaTime and strength
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
			FVector center = UVoxelTerrainUtils::WorldPosToTerrainPos(hitResult->ImpactPoint, voxelTerrain) + hitResult->Normal;

			//Calculate diameter in voxel space
			float radius = ((BrushSize*0.01f)*data->VoxelsPerMeter + 2)*0.5f;
			for (int x = center.X - radius - 1; x < center.X + radius + 1; x++)
			{
				for (int y = center.Y - radius - 1; y < center.Y + radius + 1; y++)
				{
					for (int z = center.Z - radius - 1; z < center.Z + radius + 1; z++)
					{
						if (z < 0 || z >= data->TerrainHeight)
							continue;
						FVoxelTerrainVoxel* voxel = voxelTerrain->GetVoxelAt(FIntVector(x, y, z), true);
						if (!shift && voxel->Value >= 1 || shift && voxel->Value <= -1)
							continue;
						bool voxelNegative = voxel->Value < 0;
						FVector voxelToCheck = FVector(x, y, z);
						float otherValue = 0.f;
						int otherCount = 0;
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
									if (!shift && currentVoxel->Value > 0 || shift && currentVoxel->Value < 0) {
										found = true;
									}
									if (voxelNegative && currentVoxel->Value > 0)
									{
										otherValue += currentVoxel->Value;
										otherCount++;
										foundOther = true;
									}
									if (!voxelNegative && currentVoxel->Value < 0)
									{
										otherValue += currentVoxel->Value;
										otherCount++;
										foundOther = true;
									}
								}
							}
						}
						otherValue /= otherCount;
						if (!foundOther) {
							if (voxel->Value > 0)
								voxel->Value = 1;
							else
								voxel->Value = -1;
						}
						if (foundOther && found) {
							if (!shift) {
								voxel->Value += CalculateIncreaseValue(voxelToCheck, center, hitResult->Normal, radius)*deltaTime * 10 * FMath::Abs(otherValue);
								//If > 1, make 1
								if (voxel->Value > 1)
									voxel->Value = 1;
							}
							else {
								voxel->Value -= CalculateIncreaseValue(voxelToCheck, center, hitResult->Normal, radius)*deltaTime * 10 * FMath::Abs(otherValue);
								//If <= -1, make -1
								if (voxel->Value < -1)
									voxel->Value = -1;
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
float FVoxelTerrainSculptTool::CalculateIncreaseValue(FVector voxelToInvestigate, FVector voxelBrushCenter, FVector brushNormal, float voxelRadius)
{
	//distance used for brush falloff multiplier
	//difference used to get normal dot value (negative becomes positive multiplier)
	FVector difference = voxelToInvestigate - voxelBrushCenter;
	float distance = difference.Size();
	float normalDot = FVector::DotProduct(difference.GetUnsafeNormal(), brushNormal);

	if (normalDot > 0)
		return 0.f;

	normalDot = FMath::Abs(normalDot);

	//Use distance, dot and toolstrength to calculate increase value
	float innerRadius = voxelRadius*(1 - BrushFalloff);
	if (distance < innerRadius)
	{
		return ToolStrength*normalDot;
	}
	float slerp = FMath::SmoothStep(0, 1, 1 - ((distance - innerRadius) / (voxelRadius - innerRadius)));
	return ToolStrength*slerp*normalDot;
}
