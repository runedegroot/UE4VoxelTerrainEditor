#include "VoxelTerrainPrivatePCH.h"
#include "VoxelTerrainUtils.h"
#include "VoxelTerrainActor.h"
#include "VoxelTerrainChunkComponent.h"

FVector UVoxelTerrainUtils::WorldPosToTerrainPos(FVector in, AVoxelTerrain* voxelTerrain)
{
	float x = (in.X - voxelTerrain->GetActorLocation().X)*voxelTerrain->Settings->VoxelsPerMeter*0.01f;
	float y = (in.Y - voxelTerrain->GetActorLocation().Y)*voxelTerrain->Settings->VoxelsPerMeter*0.01f;
	float z = (in.Z - voxelTerrain->GetActorLocation().Z)*voxelTerrain->Settings->VoxelsPerMeter*0.01f;
	return FVector(x, y, z);
}

FVector UVoxelTerrainUtils::TerrainPosToWorldPos(FVector in, AVoxelTerrain* voxelTerrain)
{
	float x = (in.X *voxelTerrain->UnitsPerVoxel) + voxelTerrain->GetActorLocation().X;
	float y = (in.Y *voxelTerrain->UnitsPerVoxel) + voxelTerrain->GetActorLocation().Y;
	float z = (in.Z *voxelTerrain->UnitsPerVoxel) + voxelTerrain->GetActorLocation().Z;
	return FVector(x, y, z);
}

FIntPoint UVoxelTerrainUtils::WorldPosToChunkPos(FVector in, AVoxelTerrain* voxelTerrain)
{
	//Terrain pos
	float x = (in.X - voxelTerrain->GetActorLocation().X)*voxelTerrain->Settings->VoxelsPerMeter*0.01f;
	float y = (in.Y - voxelTerrain->GetActorLocation().Y)*voxelTerrain->Settings->VoxelsPerMeter*0.01f;

	return TerrainPosToChunkPos(FVector(x, y, 0), voxelTerrain);
}

FIntPoint UVoxelTerrainUtils::TerrainPosToChunkPos(FVector in, AVoxelTerrain* voxelTerrain)
{
	FIntPoint chunkPos;
	chunkPos.X = FMath::FloorToInt(in.X / (float)voxelTerrain->Settings->ChunkSize);
	chunkPos.Y = FMath::FloorToInt(in.Y / (float)voxelTerrain->Settings->ChunkSize);
	return chunkPos;
}

FVector UVoxelTerrainUtils::MousePosToTerrainPos(AVoxelTerrain* voxelTerrain/*, FViewportClient* ViewportClient*/)
{
	//int32 MouseX = ViewportClient->Viewport->GetMouseX();
	//int32 MouseY = ViewportClient->Viewport->GetMouseY();

	//// Cache a copy of the world pointer	
	//UWorld* World = ViewportClient->GetWorld();

	//UE_LOG(LogTemp, Warning, TEXT("collision at %d"), ViewportClient->Viewport->HasMouseCapture());

	//// Compute a world space ray from the screen space mouse coordinates
	//FSceneViewFamilyContext ViewFamily(FSceneViewFamilyContext::ConstructionValues(ViewportClient->Viewport, ViewportClient->GetScene(), ViewportClient->EngineShowFlags)
	//	.SetRealtimeUpdate(ViewportClient->IsRealtime()));

	//FSceneView* View = ViewportClient->CalcSceneView(&ViewFamily);
	//FViewportCursorLocation MouseViewportRay(View, ViewportClient, MouseX, MouseY);

	//FVector Start = MouseViewportRay.GetOrigin();
	//FVector End = Start + WORLD_MAX * MouseViewportRay.GetDirection();

	//static FName TraceTag = FName(TEXT("VoxelTerrainMouseTrace"));
	//FHitResult HitCall(ForceInit);
	//// Each landscape component has 2 collision shapes, 1 of them is specific to landscape editor
	//// Trace only ECC_Visibility channel, so we do hit only Editor specific shape
	//bool traced = World->LineTraceSingleByChannel(HitCall, Start, End, ECollisionChannel::ECC_WorldDynamic, FCollisionQueryParams(TraceTag, true));

	//if (traced) {
	//	FVector terrainPos = UVoxelTerrainUtils::WorldPosToTerrainPos(HitCall.ImpactPoint, voxelTerrain);
	//	UE_LOG(LogTemp, Warning, TEXT("collision at %f %f %f"), terrainPos.X, terrainPos.Y, terrainPos.Z);//  *HitCall.Actor->GetName());
	//	int xmin = FMath::FloorToInt(terrainPos.X);
	//	int xmax = FMath::CeilToInt(terrainPos.X);
	//	if (xmin == xmax) xmax += HitCall.Normal.X;
	//	int ymin = FMath::FloorToInt(terrainPos.Y);
	//	int ymax = FMath::CeilToInt(terrainPos.Y);
	//	if (ymin == ymax) ymax += HitCall.Normal.Y;
	//	int zmin = FMath::FloorToInt(terrainPos.Z);
	//	int zmax = FMath::CeilToInt(terrainPos.Z);
	//	if (zmin == zmax) zmax += HitCall.Normal.Z;
	//	if (xmin >= 0 && xmax < voxelTerrain->Settings->terrain.Num() - 1 && ymin >= 0 && ymax < voxelTerrain->Settings->terrain[0].Num() - 1 && zmin >= 0 && zmax < voxelTerrain->Settings->terrain[0][0].Num() - 1) {
	//		UE_LOG(LogTemp, Warning, TEXT("deleted voxels at %d %d"), xmin, ymin);
	//		return FVector(xmin < xmax ? xmin : xmax, ymin < ymax ? ymin : ymax, zmin < zmax ? zmin : zmax);
	//	}
	//	else if (voxelTerrain->Settings->terrain.Num() == 0) {
	//		UE_LOG(LogTemp, Warning, TEXT("the terrain array has not been initialized"));
	//		return FVector();
	//	}
	//	else {
	//		UE_LOG(LogTemp, Warning, TEXT("%d %d %d is not a suitable location"), xmin, ymin, zmin);
	//		return FVector();
	//	}
	//}
	return FVector();
}

void UVoxelTerrainUtils::HightlightVoxel(FVector terrainPos, AVoxelTerrain* voxelTerrain, float toolSize, FColor color, float thickness)
{
	float offset = (toolSize - 1) * 100;
	int xmin = FMath::FloorToInt(terrainPos.X);
	int xmax = xmin + 1;
	int ymin = FMath::FloorToInt(terrainPos.Y);
	int ymax = ymin + 1;
	int zmin = FMath::FloorToInt(terrainPos.Z);
	int zmax = zmin + 1;

	//LFF = Left Front Floor
	//RBC = Right Back Ceil
	FVector LFF = TerrainPosToWorldPos(FVector(xmax, ymin, zmin), voxelTerrain);
	LFF -= FVector(-offset, offset, offset);
	FVector RFF = TerrainPosToWorldPos(FVector(xmin, ymin, zmin), voxelTerrain);
	RFF -= FVector(offset, offset, offset);
	FVector LBF = TerrainPosToWorldPos(FVector(xmax, ymax, zmin), voxelTerrain);
	LBF -= FVector(-offset, -offset, offset);
	FVector RBF = TerrainPosToWorldPos(FVector(xmin, ymax, zmin), voxelTerrain);
	RBF -= FVector(offset, -offset, offset);

	FVector LFC = TerrainPosToWorldPos(FVector(xmax, ymin, zmax), voxelTerrain);
	LFC -= FVector(-offset, offset, -offset);
	FVector RFC = TerrainPosToWorldPos(FVector(xmin, ymin, zmax), voxelTerrain);
	RFC -= FVector(offset, offset, -offset);
	FVector LBC = TerrainPosToWorldPos(FVector(xmax, ymax, zmax), voxelTerrain);
	LBC -= FVector(-offset, -offset, -offset);
	FVector RBC = TerrainPosToWorldPos(FVector(xmin, ymax, zmax), voxelTerrain);
	RBC -= FVector(offset, -offset, -offset);

	//Bottom
	DrawDebugLine(voxelTerrain->GetWorld(), LFF, RFF, color, false, -1.0f, 0, thickness);
	DrawDebugLine(voxelTerrain->GetWorld(), RFF, RBF, color, false, -1.0f, 0, thickness);
	DrawDebugLine(voxelTerrain->GetWorld(), RBF, LBF, color, false, -1.0f, 0, thickness);
	DrawDebugLine(voxelTerrain->GetWorld(), LBF, LFF, color, false, -1.0f, 0, thickness);

	//Vertical
	DrawDebugLine(voxelTerrain->GetWorld(), LFF, LFC, color, false, -1.0f, 0, thickness);
	DrawDebugLine(voxelTerrain->GetWorld(), RFF, RFC, color, false, -1.0f, 0, thickness);
	DrawDebugLine(voxelTerrain->GetWorld(), LBF, LBC, color, false, -1.0f, 0, thickness);
	DrawDebugLine(voxelTerrain->GetWorld(), RBF, RBC, color, false, -1.0f, 0, thickness);

	//Top
	DrawDebugLine(voxelTerrain->GetWorld(), LFC, RFC, color, false, -1.0f, 0, thickness);
	DrawDebugLine(voxelTerrain->GetWorld(), RFC, RBC, color, false, -1.0f, 0, thickness);
	DrawDebugLine(voxelTerrain->GetWorld(), RBC, LBC, color, false, -1.0f, 0, thickness);
	DrawDebugLine(voxelTerrain->GetWorld(), LBC, LFC, color, false, -1.0f, 0, thickness);
}

FVector UVoxelTerrainUtils::ScreenCenterToTerrainPos(AVoxelTerrain* voxelTerrain)
{
	APlayerController* controller = voxelTerrain->GetWorld()->GetFirstPlayerController();
	if (!controller) {
		UE_LOG(LogTemp, Warning, TEXT("You cannot use ScreenCenterToTerrainPos in edit mode!\nPlease use MousePositionToTerrainPos instead."));
		return FVector();
	}
	FHitResult HitCall(ForceInit);
	FCollisionQueryParams paramsCall = FCollisionQueryParams(FName(TEXT("Trace")), true);
	paramsCall.AddIgnoredActor(controller->GetPawn());
	FVector2D size;
	controller->GetWorld()->GetGameViewport()->GetViewportSize(size);
	FIntPoint ViewSize = FIntPoint(size.X, size.Y);
	bool traced = controller->GetHitResultAtScreenPosition(FVector2D(ViewSize.X / 2, ViewSize.Y / 2), ECC_WorldDynamic, paramsCall, HitCall);
	FVector terrainPos = UVoxelTerrainUtils::WorldPosToTerrainPos(HitCall.ImpactPoint, voxelTerrain);
	if (traced) {
		UE_LOG(LogTemp, Warning, TEXT("collision at %f %f %f"), terrainPos.X, terrainPos.Y, terrainPos.Z);//  *HitCall.Actor->GetName());
		int xmin = FMath::FloorToInt(terrainPos.X);
		int xmax = FMath::CeilToInt(terrainPos.X);
		if (xmin == xmax) xmax += HitCall.Normal.X;
		int ymin = FMath::FloorToInt(terrainPos.Y);
		int ymax = FMath::CeilToInt(terrainPos.Y);
		if (ymin == ymax) ymax += HitCall.Normal.Y;
		int zmin = FMath::FloorToInt(terrainPos.Z);
		int zmax = FMath::CeilToInt(terrainPos.Z);
		if (zmin == zmax) zmax += HitCall.Normal.Z;
		return FVector(xmin < xmax ? xmin : xmax, ymin < ymax ? ymin : ymax, zmin < zmax ? zmin : zmax);
	}
	return FVector();
}

AVoxelTerrain* UVoxelTerrainUtils::GetTerrainUnderMouse(FViewport* viewport)
{
	FHitResult HitCall(ForceInit);
	FCollisionQueryParams paramsCall = FCollisionQueryParams(FName(TEXT("Trace")), true);
	FIntPoint size = viewport->GetSizeXY();
	FIntPoint ViewSize = FIntPoint(size.X, size.Y);

	bool traced = viewport->GetClient()->GetWorld()->GetFirstPlayerController()->GetHitResultAtScreenPosition(FVector2D(ViewSize.X / 2, ViewSize.Y / 2), ECC_WorldDynamic, paramsCall, HitCall);
	if (traced && HitCall.GetActor()->IsA<AVoxelTerrain>())
		return (AVoxelTerrain*)HitCall.GetActor();
	return NULL;
}

///** Trace under the mouse cursor and return the landscape hit and the hit location (in landscape quad space) */
//bool FVoxelTerrainEdMode::VoxelTerrainMouseTrace(FEditorViewportClient* ViewportClient, float& OutHitX, float& OutHitY)
//{
//	int32 MouseX = ViewportClient->Viewport->GetMouseX();
//	int32 MouseY = ViewportClient->Viewport->GetMouseY();
//
//	return VoxelTerrainMouseTrace(ViewportClient, MouseX, MouseY, OutHitX, OutHitY);
//}
//
//bool FVoxelTerrainEdMode::VoxelTerrainMouseTrace(FEditorViewportClient* ViewportClient, FVector& OutHitLocation)
//{
//	int32 MouseX = ViewportClient->Viewport->GetMouseX();
//	int32 MouseY = ViewportClient->Viewport->GetMouseY();
//
//	return VoxelTerrainMouseTrace(ViewportClient, MouseX, MouseY, OutHitLocation);
//}
//
///** Trace under the specified coordinates and return the landscape hit and the hit location (in landscape quad space) */
//bool FVoxelTerrainEdMode::VoxelTerrainMouseTrace(FEditorViewportClient* ViewportClient, int32 MouseX, int32 MouseY, float& OutHitX, float& OutHitY)
//{
//	FVector HitLocation;
//	bool bResult = VoxelTerrainMouseTrace(ViewportClient, MouseX, MouseY, HitLocation);
//	OutHitX = HitLocation.X;
//	OutHitY = HitLocation.Y;
//	return bResult;
//}
//
//bool FVoxelTerrainEdMode::VoxelTerrainMouseTrace(FEditorViewportClient* ViewportClient, int32 MouseX, int32 MouseY, FVector& OutHitLocation)
//{
//	// Cache a copy of the world pointer	
//	UWorld* World = ViewportClient->GetWorld();
//
//	// Compute a world space ray from the screen space mouse coordinates
//	FSceneViewFamilyContext ViewFamily(FSceneViewFamilyContext::ConstructionValues(ViewportClient->Viewport, ViewportClient->GetScene(), ViewportClient->EngineShowFlags)
//		.SetRealtimeUpdate(ViewportClient->IsRealtime()));
//
//	FSceneView* View = ViewportClient->CalcSceneView(&ViewFamily);
//	FViewportCursorLocation MouseViewportRay(View, ViewportClient, MouseX, MouseY);
//
//	FVector Start = MouseViewportRay.GetOrigin();
//	FVector End = Start + WORLD_MAX * MouseViewportRay.GetDirection();
//
//	static FName TraceTag = FName(TEXT("VoxelTerrainMouseTrace"));
//	FHitResult HitCall(ForceInit);
//	// Each landscape component has 2 collision shapes, 1 of them is specific to landscape editor
//	// Trace only ECC_Visibility channel, so we do hit only Editor specific shape
//	bool traced = World->LineTraceSingleByChannel(HitCall, Start, End, ECollisionChannel::ECC_WorldDynamic, FCollisionQueryParams(TraceTag, true));
//	OutHitLocation = HitCall.ImpactPoint;
//	return traced;
//}

bool UVoxelTerrainUtils::ArrayContainsVector(TArray<FVector> investigatedVoxels, FVector toCheck)
{
	for (int i = 0; i < investigatedVoxels.Num(); i++)
	{
		FVector current = investigatedVoxels[i];
		if ((int)current.X == (int)toCheck.X && (int)current.Y == (int)toCheck.Y && (int)current.Z == (int)toCheck.Z)
			return true;
	}
	return false;
}