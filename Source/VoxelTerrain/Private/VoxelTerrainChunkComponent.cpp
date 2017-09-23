#include "VoxelTerrainPrivatePCH.h"
#include "VoxelTerrainChunkComponent.h"
#include "VoxelTerrainActor.h"
#include "VoxelTerrainData.h"
#include "VoxelTerrainUtils.h"
#include "Kismet/KismetMathLibrary.h"

DECLARE_CYCLE_STAT(TEXT("Component/UpdateMesh"), STAT_UpdateMesh, STATGROUP_VoxelTerrainChunkComponent);
DECLARE_CYCLE_STAT(TEXT("Component/UpdateComplete"), STAT_UpdateComplete, STATGROUP_VoxelTerrainChunkComponent);
DECLARE_CYCLE_STAT(TEXT("Component/GetVoxelAt"), STAT_CompGetVoxelAt, STATGROUP_VoxelTerrainChunkComponent);
DECLARE_CYCLE_STAT(TEXT("Component/DoWork"), STAT_DoWork, STATGROUP_VoxelTerrainChunkComponent);
DECLARE_CYCLE_STAT(TEXT("Component/DoWork/Empty"), STAT_Empty, STATGROUP_VoxelTerrainChunkComponent);
DECLARE_CYCLE_STAT(TEXT("Component/DoWork/Vertices"), STAT_Vertices, STATGROUP_VoxelTerrainChunkComponent);
DECLARE_CYCLE_STAT(TEXT("Component/DoWork/Triangles"), STAT_Triangles, STATGROUP_VoxelTerrainChunkComponent);
DECLARE_CYCLE_STAT(TEXT("Component/DoWork/Normals"), STAT_Normals, STATGROUP_VoxelTerrainChunkComponent);
DECLARE_CYCLE_STAT(TEXT("Component/DoWork/MergeVertices"), STAT_MergeVertices, STATGROUP_VoxelTerrainChunkComponent);
DECLARE_CYCLE_STAT(TEXT("Component/DoWork/Copy"), STAT_Copy, STATGROUP_VoxelTerrainChunkComponent);

UVoxelTerrainChunkComponent::UVoxelTerrainChunkComponent(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	voxelTerrain = (AVoxelTerrain*)GetOuter();
}

UVoxelTerrainChunkComponent::~UVoxelTerrainChunkComponent()
{
	if (OnUpdateComplete.IsValid() && OnUpdateComplete->IsBound())
		OnUpdateComplete->Unbind();
	if (updateTask)
		updateTask->EnsureCompletion(false);

	if (OnFoliageComplete.IsValid() && OnFoliageComplete->IsBound())
		OnFoliageComplete->Unbind();
	if (foliageTask)
		foliageTask->EnsureCompletion(false);
}

void UVoxelTerrainChunkComponent::Initialize(FIntPoint inPos, UMaterialInterface* material) {
	Position = inPos;
	UE_LOG(LogTemp, Warning, TEXT("INIT COMPONENT %d,%d"), inPos.X, inPos.Y);
	UVoxelTerrainChunkData* data = voxelTerrain->Settings->GetChunkDataAt(inPos);
	if (data) {
		saved = true;
		chunkData = data;
	}
	else
		chunkData = voxelTerrain->GenerateChunkData(inPos);


	UMaterialInstanceDynamic* dynMaterial = UMaterialInstanceDynamic::Create(material, this);
	SetMaterial(0, dynMaterial);
	for (int i = 0; i < chunkData->layers.Num(); i++)
	{
		UpdateMaterialTextures(i);
	}
}

int UVoxelTerrainChunkComponent::GetPaintLayer(UVoxelTerrainMaterial* paintLayer)
{
	if (!paintLayer)
		return -1;
	for (int i = 0; i < chunkData->layers.Num(); i++)
	{
		if (chunkData->layers[i] == paintLayer) {
			return i;
		}
	}
	if (chunkData->layers.Num() < 4) {
		chunkData->layers.Add(paintLayer);
		UpdateMaterialTextures(chunkData->layers.Num() - 1);
		return chunkData->layers.Num() - 1;
	}
	UE_LOG(LogTemp, Warning, TEXT("Chunk already contains the maximum of 4 materials"));
	return -1;
}

void UVoxelTerrainChunkComponent::UpdateMaterialTextures(int layerIndex)
{
	UMaterialInstanceDynamic* material = (UMaterialInstanceDynamic*)GetMaterial(0);
	UVoxelTerrainMaterial* paintLayer = chunkData->layers[layerIndex];
	if (paintLayer->FloorTextureC)
		material->SetTextureParameterValue(FName(*(FString("FloorTextureC") + FString::FromInt(layerIndex + 1))), paintLayer->FloorTextureC);
	if (paintLayer->FloorTextureN)
		material->SetTextureParameterValue(FName(*(FString("FloorTextureN") + FString::FromInt(layerIndex + 1))), paintLayer->FloorTextureN);
	if (paintLayer->FloorTextureD)
		material->SetTextureParameterValue(FName(*(FString("FloorTextureD") + FString::FromInt(layerIndex + 1))), paintLayer->FloorTextureD);
	if (paintLayer->WallTextureC)
		material->SetTextureParameterValue(FName(*(FString("WallTextureC") + FString::FromInt(layerIndex + 1))), paintLayer->WallTextureC);
	if (paintLayer->WallTextureN)
		material->SetTextureParameterValue(FName(*(FString("WallTextureN") + FString::FromInt(layerIndex + 1))), paintLayer->WallTextureN);
	if (paintLayer->WallTextureD)
		material->SetTextureParameterValue(FName(*(FString("WallTextureD") + FString::FromInt(layerIndex + 1))), paintLayer->WallTextureD);
}

FVoxelTerrainVoxel* UVoxelTerrainChunkComponent::GetVoxelAt(FIntVector inPos, bool edit)
{
	SCOPE_CYCLE_COUNTER(STAT_CompGetVoxelAt);
	if (edit && !saved) {
		chunkData = voxelTerrain->Settings->SetChunkDataAt(Position, chunkData);
		saved = true;
	}
	if (edit)
		chunkData->MarkPackageDirty();
	return chunkData->GetVoxelAt(inPos);
}

void UVoxelTerrainChunkComponent::UpdateMesh()
{
	SCOPE_CYCLE_COUNTER(STAT_UpdateMesh);
	if (!updateTask || updateTask->IsDone()) {
		OnUpdateComplete = MakeShareable(new FUpdateComplete());
		OnUpdateComplete->BindUObject(this, &UVoxelTerrainChunkComponent::UpdateComplete);
		updateTask = new FAsyncTask<ChunkMeshUpdateAsyncTask>(voxelTerrain, this, OnUpdateComplete);
		UE_LOG(LogTemp, Warning, TEXT("chunk %d, %d started updating"), Position.X, Position.Y, triangles.Num());
		updateTask->StartBackgroundTask();
	}
	else
		queueUpdate = true;
}

void UVoxelTerrainChunkComponent::UpdateComplete()
{
	SCOPE_CYCLE_COUNTER(STAT_UpdateComplete);
	CreateMeshSection(0, positions, triangles, normals, TArray<FVector2D>(), colors, TArray<FProcMeshTangent>(), true);
	if (((AVoxelTerrain*)GetOwner())->DynamicBrushMaterial != NULL)
		CreateMeshSection(1, positions, triangles, normals, TArray<FVector2D>(), colors, TArray<FProcMeshTangent>(), true);

	//Ensures that updates do not queue, execute only once more if there's a queue
	if (queueUpdate)
	{
		queueUpdate = false;
		UpdateMesh();
	}
	else {
		if (voxelTerrain->Settings->FoliageTypes.Num() > 0)
			UpdateFoliage();
	}
}

void UVoxelTerrainChunkComponent::UpdateFoliage()
{
	if (!foliageTask || foliageTask->IsDone()) {
		OnFoliageComplete = MakeShareable(new FFoliageComplete());
		OnFoliageComplete->BindUObject(this, &UVoxelTerrainChunkComponent::FoliageComplete);
		foliageTask = new FAsyncTask<FoliageBuilderAsyncTask>(voxelTerrain, this, OnFoliageComplete);
		UE_LOG(LogTemp, Warning, TEXT("chunk %d, %d started foliage"), Position.X, Position.Y, triangles.Num());
		foliageTask->StartBackgroundTask();
	}
	else
		queueFoliage = true;
}

float SeededRandom(int& seed)
{
	seed = (seed * 196314165) + 907633515;
	union { float f; int32 i; } Result;
	union { float f; int32 i; } Temp;
	const float SRandTemp = 1.0f;
	Temp.f = SRandTemp;
	Result.i = (Temp.i & 0xff800000) | (seed & 0x007fffff);
	return FPlatformMath::Fractional(Result.f);
}

void FoliageBuilderAsyncTask::DoWork()
{
	//Do raycast from top, get surface normal and place instanced grass
	//UFoliageInstancedStaticMeshComponent
	component->layers.Empty();
	for (int i = 0; i < voxelTerrain->Settings->FoliageTypes.Num(); i++)
	{
		int seed1 = voxelTerrain->Settings->Seed + (component->Position.X*component->Position.Y);
		UVoxelTerrainFoliageType* foliageType = voxelTerrain->Settings->FoliageTypes[i];

		//Density calculation
		float offset = 100 / foliageType->Density;
		float offsetWorldSpace = offset * 100.f;

		TArray<FMatrix> transforms;
		//int instanceCount = 0;

		int chunkX = component->Position.X*voxelTerrain->Settings->ChunkSize;
		int chunkY = component->Position.Y*voxelTerrain->Settings->ChunkSize;
		float startx = offset - (FMath::Fmod(chunkX, offset));
		float starty = offset - (FMath::Fmod(chunkY, offset));

		int currx = 1;
		for (float x = startx; x <= voxelTerrain->Settings->ChunkSize; x += offset, currx++)
		{
			int curry = 1;
			for (float y = starty; y <= voxelTerrain->Settings->ChunkSize; y += offset, curry++)
			{
				int seed2 = seed1 + (currx*currx*curry);
				//RAYCAST FROM WORLDPOS DOWN
				FVector terrainPos;
				terrainPos.X = x + (component->Position.X*voxelTerrain->Settings->ChunkSize);
				terrainPos.Y = y + (component->Position.Y*voxelTerrain->Settings->ChunkSize);
				terrainPos.Z = 0;
				FVector worldPos = UVoxelTerrainUtils::TerrainPosToWorldPos(terrainPos, voxelTerrain);
				worldPos += FVector((SeededRandom(seed2)*offsetWorldSpace * 2 - offsetWorldSpace), (SeededRandom(seed2)*offsetWorldSpace * 2 - offsetWorldSpace), 0);
				FVector startPos = worldPos;
				startPos.Z = 0.f;

				FVector endPos = worldPos;
				endPos.Z = -10000.f;

				FCollisionQueryParams RV_TraceParams = FCollisionQueryParams(FName(TEXT("Trace")), true);
				RV_TraceParams.bTraceComplex = true;
				RV_TraceParams.bTraceAsyncScene = true;
				RV_TraceParams.bReturnPhysicalMaterial = false;

				//Re-initialize hit info
				FHitResult RV_Hit(ForceInit);

				//call GetWorld() from within an actor extending class
				bool traced = voxelTerrain->GetWorld()->LineTraceSingleByChannel(
					RV_Hit,        //result
					startPos,    //start
					endPos, //end
					ECollisionChannel::ECC_WorldDynamic, //collision channel
					RV_TraceParams
					);

				if (traced && RV_Hit.GetActor()->IsA<AVoxelTerrain>() && RV_Hit.Normal.Z >= foliageType->SlopeLimit)
				{
					FRotator rotation = foliageType->AlignToNormal ? UKismetMathLibrary::MakeRotFromZ(RV_Hit.Normal) : FRotator(0, 0, 0);
					if (foliageType->RandomRotation)
					{
						rotation = FRotator(rotation.Quaternion() * FQuat::MakeFromEuler(FVector(0, 0, SeededRandom(seed2)* 360.f)));
					}

					FVector scaling = FVector(0, 0, 0);
					switch (foliageType->Scaling)
					{
					case EVoxelTerrainFoliageScaling::Uniform:
						scaling.X = SeededRandom(seed2)*(foliageType->ScaleX.Max - foliageType->ScaleX.Min) + foliageType->ScaleX.Min;
						scaling.Y = scaling.X;
						scaling.Z = scaling.X;
						break;
					case EVoxelTerrainFoliageScaling::Free:
						scaling.X = SeededRandom(seed2)*(foliageType->ScaleX.Max - foliageType->ScaleX.Min) + foliageType->ScaleX.Min;
						scaling.Y = SeededRandom(seed2)*(foliageType->ScaleY.Max - foliageType->ScaleY.Min) + foliageType->ScaleY.Min;
						scaling.Z = SeededRandom(seed2)*(foliageType->ScaleZ.Max - foliageType->ScaleZ.Min) + foliageType->ScaleZ.Min;
						break;
					case EVoxelTerrainFoliageScaling::LockXY:
						scaling.X = SeededRandom(seed2)*(foliageType->ScaleX.Max - foliageType->ScaleX.Min) + foliageType->ScaleX.Min;
						scaling.Y = scaling.X;
						scaling.Z = SeededRandom(seed2)*(foliageType->ScaleZ.Max - foliageType->ScaleZ.Min) + foliageType->ScaleZ.Min;
						break;
					case EVoxelTerrainFoliageScaling::LockXZ:
						scaling.X = SeededRandom(seed2)*(foliageType->ScaleX.Max - foliageType->ScaleX.Min) + foliageType->ScaleX.Min;
						scaling.Y = SeededRandom(seed2)*(foliageType->ScaleY.Max - foliageType->ScaleY.Min) + foliageType->ScaleY.Min;
						scaling.Z = scaling.X;
						break;
					case EVoxelTerrainFoliageScaling::LockYZ:
						scaling.X = SeededRandom(seed2)*(foliageType->ScaleX.Max - foliageType->ScaleX.Min) + foliageType->ScaleX.Min;
						scaling.Y = SeededRandom(seed2)*(foliageType->ScaleY.Max - foliageType->ScaleY.Min) + foliageType->ScaleY.Min;
						scaling.Z = scaling.Y;
						break;
					}

					FVector position = RV_Hit.ImpactPoint - component->GetComponentLocation();
					position += foliageType->AlignToNormal ? RV_Hit.Normal*foliageType->HeightOffset : FVector(0, 0, foliageType->HeightOffset);

					transforms.Add(FTransform(rotation, position, scaling).ToMatrixWithScale());
					//UE_LOG(LogTemp, Warning, TEXT("Added grass at %f %f %f"), RV_Hit.ImpactPoint.X, RV_Hit.ImpactPoint.Y, RV_Hit.ImpactPoint.Z);
				}
			}
		}

		FoliageLayerInfo* layer = new FoliageLayerInfo();
		layer->Size = transforms.Num();
		if (layer->Size > 0)
		{
			layer->InstanceBuffer.AllocateInstances(layer->Size);

			for (int j = 0; j < transforms.Num(); j++)
			{
				layer->InstanceBuffer.SetInstance(j, transforms[j], 0);
			}

			TArray<int32> SortedInstances;
			TArray<int32> InstanceReorderTable;
			UHierarchicalInstancedStaticMeshComponent::BuildTreeAnyThread(transforms, voxelTerrain->Settings->FoliageTypes[i]->Mesh->GetBounds().GetBox(), layer->ClusterTree, SortedInstances, InstanceReorderTable, layer->OutOcclusionLayerNum, /*DesiredInstancesPerLeaf*/1);

			//SORT
			// in-place sort the instances
			const uint32 InstanceStreamSize = layer->InstanceBuffer.GetStride();
			FInstanceStream32 SwapBuffer;
			check(sizeof(SwapBuffer) >= InstanceStreamSize);

			for (int32 FirstUnfixedIndex = 0; FirstUnfixedIndex < transforms.Num(); FirstUnfixedIndex++)
			{
				int32 LoadFrom = SortedInstances[FirstUnfixedIndex];
				if (LoadFrom != FirstUnfixedIndex)
				{
					check(LoadFrom > FirstUnfixedIndex);
					FMemory::Memcpy(&SwapBuffer, layer->InstanceBuffer.GetInstanceWriteAddress(FirstUnfixedIndex), InstanceStreamSize);
					FMemory::Memcpy(layer->InstanceBuffer.GetInstanceWriteAddress(FirstUnfixedIndex), layer->InstanceBuffer.GetInstanceWriteAddress(LoadFrom), InstanceStreamSize);
					FMemory::Memcpy(layer->InstanceBuffer.GetInstanceWriteAddress(LoadFrom), &SwapBuffer, InstanceStreamSize);

					int32 SwapGoesTo = InstanceReorderTable[FirstUnfixedIndex];
					check(SwapGoesTo > FirstUnfixedIndex);
					check(SortedInstances[SwapGoesTo] == FirstUnfixedIndex);
					SortedInstances[SwapGoesTo] = LoadFrom;
					InstanceReorderTable[LoadFrom] = SwapGoesTo;

					InstanceReorderTable[FirstUnfixedIndex] = FirstUnfixedIndex;
					SortedInstances[FirstUnfixedIndex] = FirstUnfixedIndex;
				}
			}
		}
		component->layers.Add(layer);
	}

	if (onComplete.Pin()->IsBound())
	{
		FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([&]()
		{
			if (onComplete.IsValid() && onComplete.Pin()->IsBound())
				onComplete.Pin()->Execute();
		}, TStatId(), NULL, ENamedThreads::GameThread);
	}
}

void UVoxelTerrainChunkComponent::FoliageComplete()
{
	for (int i = 0; i < foliageComponents.Num(); i++)
	{
		foliageComponents[i]->DestroyComponent();
	}
	foliageComponents.Empty();
	if (voxelTerrain->renderFoliage)
	{
		for (int i = 0; i < voxelTerrain->Settings->FoliageTypes.Num(); i++)
		{
			if (layers[i]->Size == 0)
				continue;

			UVoxelTerrainFoliageType* foliageType = voxelTerrain->Settings->FoliageTypes[i];

			//Create component
			UHierarchicalInstancedStaticMeshComponent* foliageComponent = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, "", RF_Transient);
			foliageComponent->OnComponentCreated();
			foliageComponent->RegisterComponent();
			if (foliageComponent->bWantsInitializeComponent) foliageComponent->InitializeComponent();
			foliageComponent->AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);

			//Set options
			foliageComponent->SetStaticMesh(foliageType->Mesh);
			foliageComponent->bCastStaticShadow = foliageType->CastStaticShadow;
			foliageComponent->bCastDynamicShadow = foliageType->CastDynamicShadow;

			//Fill buffer
			FMemory::Memswap(&foliageComponent->WriteOncePrebuiltInstanceBuffer, &layers[i]->InstanceBuffer, sizeof(FStaticMeshInstanceData));
			foliageComponent->AcceptPrebuiltTree(layers[i]->ClusterTree, layers[i]->OutOcclusionLayerNum);

			//Add to terrain
			foliageComponent->RecreateRenderState_Concurrent();
			foliageComponents.Add(foliageComponent);
		}
	}

	if (queueFoliage)
	{
		queueFoliage = false;
		UpdateFoliage();
	}
}

void ChunkMeshUpdateAsyncTask::DoWork()
{
	SCOPE_CYCLE_COUNTER(STAT_DoWork);
	{
		SCOPE_CYCLE_COUNTER(STAT_Empty);
		component->positions.Empty();
		component->colors.Empty();
		component->normals.Empty();
		component->triangles.Empty();
	}

	//VERTICES
	{
		SCOPE_CYCLE_COUNTER(STAT_Vertices);
		for (int y = 0; y < voxelTerrain->Settings->ChunkSize; y++)
		{
			for (int x = 0; x < voxelTerrain->Settings->ChunkSize; x++)
			{
				for (int z = 0; z < voxelTerrain->Settings->TerrainHeight - 2; z++)
				{
					//UE_LOG(LogTemp, Warning, TEXT("%d %d %d"), x, y, z);
					gridcell.p[0] = FVector(x, y, z + 1);
					gridcell.p[1] = FVector(x + 1, y, z + 1);
					gridcell.p[2] = FVector(x + 1, y, z);
					gridcell.p[3] = FVector(x, y, z);
					gridcell.p[4] = FVector(x, y + 1, z + 1);
					gridcell.p[5] = FVector(x + 1, y + 1, z + 1);
					gridcell.p[6] = FVector(x + 1, y + 1, z);
					gridcell.p[7] = FVector(x, y + 1, z);
					if (x == voxelTerrain->Settings->ChunkSize - 1 || y == voxelTerrain->Settings->ChunkSize - 1) {
						gridcell.val[0] = voxelTerrain->GetVoxelLocalAt(FIntVector(x, y, z + 1), component->Position);
						gridcell.val[1] = voxelTerrain->GetVoxelLocalAt(FIntVector(x + 1, y, z + 1), component->Position);
						gridcell.val[2] = voxelTerrain->GetVoxelLocalAt(FIntVector(x + 1, y, z), component->Position);
						gridcell.val[3] = voxelTerrain->GetVoxelLocalAt(FIntVector(x, y, z), component->Position);
						gridcell.val[4] = voxelTerrain->GetVoxelLocalAt(FIntVector(x, y + 1, z + 1), component->Position);
						gridcell.val[5] = voxelTerrain->GetVoxelLocalAt(FIntVector(x + 1, y + 1, z + 1), component->Position);
						gridcell.val[6] = voxelTerrain->GetVoxelLocalAt(FIntVector(x + 1, y + 1, z), component->Position);
						gridcell.val[7] = voxelTerrain->GetVoxelLocalAt(FIntVector(x, y + 1, z), component->Position);
					}
					else {
						gridcell.val[0] = component->GetVoxelAt(FIntVector(x, y, z + 1));
						gridcell.val[1] = component->GetVoxelAt(FIntVector(x + 1, y, z + 1));
						gridcell.val[2] = component->GetVoxelAt(FIntVector(x + 1, y, z));
						gridcell.val[3] = component->GetVoxelAt(FIntVector(x, y, z));
						gridcell.val[4] = component->GetVoxelAt(FIntVector(x, y + 1, z + 1));
						gridcell.val[5] = component->GetVoxelAt(FIntVector(x + 1, y + 1, z + 1));
						gridcell.val[6] = component->GetVoxelAt(FIntVector(x + 1, y + 1, z));
						gridcell.val[7] = component->GetVoxelAt(FIntVector(x, y + 1, z));
					}
					Polygonise(gridcell, 0, component->positions, component->colors);
				}
			}
		}
		if (component->positions.Num() == 0) {
			UE_LOG(LogTemp, Warning, TEXT("cannot create mesh data for terrain at chunk %d, %d"), component->Position.X, component->Position.Y);
			return;
		}
	}
	{
		SCOPE_CYCLE_COUNTER(STAT_Triangles);
		//TRIANGLES
		for (int i = 0; i < component->positions.Num(); i++)
		{
			component->triangles.Add(i);
		}
	}
	{
		SCOPE_CYCLE_COUNTER(STAT_Normals);
		//NORMALS
		for (int i = 0; i < component->positions.Num(); i += 3)
		{
			FVector v1 = (component->positions)[i + 1] - (component->positions)[i + 2];
			FVector v2 = (component->positions)[i] - (component->positions)[i + 2];
			FVector normal = FVector::CrossProduct(v1, v2);
			normal.Normalize();
			component->normals.Add(normal);
			component->normals.Add(normal);
			component->normals.Add(normal);
		}
	}

	{
		SCOPE_CYCLE_COUNTER(STAT_MergeVertices);
		//merge vertices
		if (voxelTerrain->Settings->MergeVertices) {
			TArray<FVector> finalPositions;
			TArray<FColor> finalColors;
			TArray<int32> finalTriangles;
			TArray<FVector> finalNormals;
			TArray<FVector> extraPositions;
			TArray<FColor> extraColors;
			TArray<FVector> extraNormals;
			for (int y = -1; y <= voxelTerrain->Settings->ChunkSize; y++)
			{
				for (int x = -1; x <= voxelTerrain->Settings->ChunkSize; x++)
				{
					for (int z = 0; z < voxelTerrain->Settings->TerrainHeight - 1; z++)
					{
						if (x == -1 || x == voxelTerrain->Settings->ChunkSize
							|| y == -1 || y == voxelTerrain->Settings->ChunkSize)
						{
							//UE_LOG(LogTemp, Warning, TEXT("%d %d %d"), x, y, z);
							gridcell.p[0] = FVector(x, y, z + 1);
							gridcell.p[1] = FVector(x + 1, y, z + 1);
							gridcell.p[2] = FVector(x + 1, y, z);
							gridcell.p[3] = FVector(x, y, z);
							gridcell.p[4] = FVector(x, y + 1, z + 1);
							gridcell.p[5] = FVector(x + 1, y + 1, z + 1);
							gridcell.p[6] = FVector(x + 1, y + 1, z);
							gridcell.p[7] = FVector(x, y + 1, z);
							gridcell.val[0] = voxelTerrain->GetVoxelLocalAt(FIntVector(x, y, z + 1), component->Position);
							gridcell.val[1] = voxelTerrain->GetVoxelLocalAt(FIntVector(x + 1, y, z + 1), component->Position);
							gridcell.val[2] = voxelTerrain->GetVoxelLocalAt(FIntVector(x + 1, y, z), component->Position);
							gridcell.val[3] = voxelTerrain->GetVoxelLocalAt(FIntVector(x, y, z), component->Position);
							gridcell.val[4] = voxelTerrain->GetVoxelLocalAt(FIntVector(x, y + 1, z + 1), component->Position);
							gridcell.val[5] = voxelTerrain->GetVoxelLocalAt(FIntVector(x + 1, y + 1, z + 1), component->Position);
							gridcell.val[6] = voxelTerrain->GetVoxelLocalAt(FIntVector(x + 1, y + 1, z), component->Position);
							gridcell.val[7] = voxelTerrain->GetVoxelLocalAt(FIntVector(x, y + 1, z), component->Position);
							Polygonise(gridcell, 0, extraPositions, extraColors);
						}
					}
				}
			}
			for (int i = 0; i < extraPositions.Num(); i += 3)
			{
				FVector v1 = extraPositions[i + 1] - extraPositions[i + 2];
				FVector v2 = extraPositions[i] - extraPositions[i + 2];
				FVector normal = FVector::CrossProduct(v1, v2);
				normal.Normalize();
				extraNormals.Add(normal);
				extraNormals.Add(normal);
				extraNormals.Add(normal);
			}
			for (int i = 0; i < component->positions.Num(); i++)
			{
				bool alreadyExists = false;
				int j = 0;
				for (; j < finalPositions.Num(); j++)
				{
					if ((component->positions)[i] == finalPositions[j]) {
						alreadyExists = true;
						break;
					}
				}
				if (alreadyExists) {
					finalTriangles.Add(j);
					finalNormals[j] = finalNormals[j] + component->normals[i];
				}
				else {
					finalPositions.Add(component->positions[i]);
					finalColors.Add(component->colors[i]);
					finalNormals.Add(component->normals[i]);
					finalTriangles.Add(j);
				}
			}
			for (int i = 0; i < extraPositions.Num(); i++)
			{
				int j = 0;
				for (; j < finalPositions.Num(); j++)
				{
					if (extraPositions[i] == finalPositions[j]) {
						finalNormals[j] = finalNormals[j] + extraNormals[i];
						break;
					}
				}
			}
			//normalize all added normals again so they average
			for (int i = 0; i < finalNormals.Num(); i++)
			{
				finalNormals[i].Normalize();
			}
			{
				SCOPE_CYCLE_COUNTER(STAT_Copy);
				component->positions.Empty();
				component->positions.Insert(finalPositions, 0);
				component->colors.Empty();
				component->colors.Insert(finalColors, 0);
				component->normals.Empty();
				component->normals.Insert(finalNormals, 0);
				component->triangles.Empty();
				component->triangles.Insert(finalTriangles, 0);
			}

		}
	}

	//FPlatformProcess::Sleep(1);
	if (onComplete.Pin()->IsBound())
	{
		FGraphEventRef Task = FFunctionGraphTask::CreateAndDispatchWhenReady([&]()
		{
			if (onComplete.IsValid() && onComplete.Pin()->IsBound())
				onComplete.Pin()->Execute();
		}, TStatId(), NULL, ENamedThreads::GameThread);
		//FTaskGraphInterface::Get().WaitUntilTaskCompletes(Task);
	}
}

int ChunkMeshUpdateAsyncTask::edgeTable[256] = {
	0x0  , 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
	0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
	0x190, 0x99 , 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
	0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
	0x230, 0x339, 0x33 , 0x13a, 0x636, 0x73f, 0x435, 0x53c,
	0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
	0x3a0, 0x2a9, 0x1a3, 0xaa , 0x7a6, 0x6af, 0x5a5, 0x4ac,
	0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
	0x460, 0x569, 0x663, 0x76a, 0x66 , 0x16f, 0x265, 0x36c,
	0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
	0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff , 0x3f5, 0x2fc,
	0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
	0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55 , 0x15c,
	0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
	0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc ,
	0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
	0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
	0xcc , 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
	0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
	0x15c, 0x55 , 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
	0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
	0x2fc, 0x3f5, 0xff , 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
	0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
	0x36c, 0x265, 0x16f, 0x66 , 0x76a, 0x663, 0x569, 0x460,
	0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
	0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa , 0x1a3, 0x2a9, 0x3a0,
	0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
	0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33 , 0x339, 0x230,
	0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
	0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99 , 0x190,
	0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
	0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0
};
int ChunkMeshUpdateAsyncTask::triTable[256][16] = { { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1 },
{ 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1 },
{ 3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1 },
{ 3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1 },
{ 9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1 },
{ 1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1 },
{ 9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1 },
{ 2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1 },
{ 8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1 },
{ 9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1 },
{ 4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1 },
{ 3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1 },
{ 1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1 },
{ 4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1 },
{ 4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1 },
{ 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1 },
{ 1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1 },
{ 5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1 },
{ 2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1 },
{ 9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1 },
{ 0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1 },
{ 2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1 },
{ 10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1 },
{ 4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1 },
{ 5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1 },
{ 5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1 },
{ 9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1 },
{ 0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1 },
{ 1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1 },
{ 10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1 },
{ 8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1 },
{ 2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1 },
{ 7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1 },
{ 9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1 },
{ 2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1 },
{ 11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1 },
{ 9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1 },
{ 5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1 },
{ 11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1 },
{ 11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1 },
{ 1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1 },
{ 9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1 },
{ 5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1 },
{ 2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1 },
{ 0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1 },
{ 5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1 },
{ 6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1 },
{ 0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1 },
{ 3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1 },
{ 6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1 },
{ 5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1 },
{ 1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1 },
{ 10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1 },
{ 6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1 },
{ 1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1 },
{ 8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1 },
{ 7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1 },
{ 3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1 },
{ 5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1 },
{ 0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1 },
{ 9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1 },
{ 8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1 },
{ 5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1 },
{ 0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1 },
{ 6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1 },
{ 10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1 },
{ 10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1 },
{ 8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1 },
{ 1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1 },
{ 3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1 },
{ 0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1 },
{ 10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1 },
{ 0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1 },
{ 3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1 },
{ 6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1 },
{ 9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1 },
{ 8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1 },
{ 3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1 },
{ 6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1 },
{ 0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1 },
{ 10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1 },
{ 10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1 },
{ 1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1 },
{ 2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1 },
{ 7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1 },
{ 7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1 },
{ 2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1 },
{ 1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1 },
{ 11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1 },
{ 8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1 },
{ 0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1 },
{ 7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1 },
{ 10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1 },
{ 2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1 },
{ 6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1 },
{ 7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1 },
{ 2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1 },
{ 1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1 },
{ 10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1 },
{ 10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1 },
{ 0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1 },
{ 7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1 },
{ 6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1 },
{ 8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1 },
{ 9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1 },
{ 6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1 },
{ 1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1 },
{ 4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1 },
{ 10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1 },
{ 8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1 },
{ 0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1 },
{ 1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1 },
{ 8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1 },
{ 10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1 },
{ 4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1 },
{ 10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1 },
{ 5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1 },
{ 11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1 },
{ 9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1 },
{ 6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1 },
{ 7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1 },
{ 3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1 },
{ 7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1 },
{ 9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1 },
{ 3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1 },
{ 6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1 },
{ 9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1 },
{ 1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1 },
{ 4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1 },
{ 7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1 },
{ 6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1 },
{ 3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1 },
{ 0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1 },
{ 6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1 },
{ 1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1 },
{ 0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1 },
{ 11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1 },
{ 6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1 },
{ 5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1 },
{ 9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1 },
{ 1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1 },
{ 1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1 },
{ 10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1 },
{ 0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1 },
{ 5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1 },
{ 10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1 },
{ 11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1 },
{ 0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1 },
{ 9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1 },
{ 7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1 },
{ 2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1 },
{ 8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1 },
{ 9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1 },
{ 9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1 },
{ 1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1 },
{ 9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1 },
{ 9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1 },
{ 5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1 },
{ 0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1 },
{ 10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1 },
{ 2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1 },
{ 0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1 },
{ 0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1 },
{ 9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1 },
{ 5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1 },
{ 3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1 },
{ 5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1 },
{ 8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1 },
{ 0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1 },
{ 9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1 },
{ 0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1 },
{ 1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1 },
{ 3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1 },
{ 4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1 },
{ 9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1 },
{ 11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1 },
{ 11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1 },
{ 2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1 },
{ 9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1 },
{ 3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1 },
{ 1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1 },
{ 4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1 },
{ 4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1 },
{ 0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1 },
{ 3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1 },
{ 3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1 },
{ 0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1 },
{ 9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1 },
{ 1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ 0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
{ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }
};


int ChunkMeshUpdateAsyncTask::Polygonise(class GridCell grid, double isolevel, TArray<FVector> &vertices, TArray<FColor> &colors)
{
	FVector vertlist[12];
	FColor color;

	/*
	Determine the index into the edge table which
	tells us which vertices are inside of the surface
	*/
	int cubeindex = 0;
	if (grid.val[0]->Value < isolevel)
		cubeindex |= 1;
	if (grid.val[1]->Value < isolevel)
		cubeindex |= 2;
	if (grid.val[2]->Value < isolevel)
		cubeindex |= 4;
	if (grid.val[3]->Value < isolevel)
		cubeindex |= 8;
	if (grid.val[4]->Value < isolevel)
		cubeindex |= 16;
	if (grid.val[5]->Value < isolevel)
		cubeindex |= 32;
	if (grid.val[6]->Value < isolevel)
		cubeindex |= 64;
	if (grid.val[7]->Value < isolevel)
		cubeindex |= 128;

	/* Cube is entirely in/out of the surface */
	if (edgeTable[cubeindex] == 0)
		return 0;

	/* Find the vertices where the surface intersects the cube */
	if ((edgeTable[cubeindex] & 1) != 0)
		vertlist[0] =
		VertexInterp(isolevel, grid.p[0], grid.p[1], grid.val[0]->Value, grid.val[1]->Value);
	if ((edgeTable[cubeindex] & 2) != 0)
		vertlist[1] =
		VertexInterp(isolevel, grid.p[1], grid.p[2], grid.val[1]->Value, grid.val[2]->Value);
	if ((edgeTable[cubeindex] & 4) != 0)
		vertlist[2] =
		VertexInterp(isolevel, grid.p[2], grid.p[3], grid.val[2]->Value, grid.val[3]->Value);
	if ((edgeTable[cubeindex] & 8) != 0)
		vertlist[3] =
		VertexInterp(isolevel, grid.p[3], grid.p[0], grid.val[3]->Value, grid.val[0]->Value);
	if ((edgeTable[cubeindex] & 16) != 0)
		vertlist[4] =
		VertexInterp(isolevel, grid.p[4], grid.p[5], grid.val[4]->Value, grid.val[5]->Value);
	if ((edgeTable[cubeindex] & 32) != 0)
		vertlist[5] =
		VertexInterp(isolevel, grid.p[5], grid.p[6], grid.val[5]->Value, grid.val[6]->Value);
	if ((edgeTable[cubeindex] & 64) != 0)
		vertlist[6] =
		VertexInterp(isolevel, grid.p[6], grid.p[7], grid.val[6]->Value, grid.val[7]->Value);
	if ((edgeTable[cubeindex] & 128) != 0)
		vertlist[7] =
		VertexInterp(isolevel, grid.p[7], grid.p[4], grid.val[7]->Value, grid.val[4]->Value);
	if ((edgeTable[cubeindex] & 256) != 0)
		vertlist[8] =
		VertexInterp(isolevel, grid.p[0], grid.p[4], grid.val[0]->Value, grid.val[4]->Value);
	if ((edgeTable[cubeindex] & 512) != 0)
		vertlist[9] =
		VertexInterp(isolevel, grid.p[1], grid.p[5], grid.val[1]->Value, grid.val[5]->Value);
	if ((edgeTable[cubeindex] & 1024) != 0)
		vertlist[10] =
		VertexInterp(isolevel, grid.p[2], grid.p[6], grid.val[2]->Value, grid.val[6]->Value);
	if ((edgeTable[cubeindex] & 2048) != 0)
		vertlist[11] =
		VertexInterp(isolevel, grid.p[3], grid.p[7], grid.val[3]->Value, grid.val[7]->Value);

	//color = DetermineColor(grid.val[0]);
	//TArray<FColor> newColors;
	//newColors.Add(color);

	//for (int i = 1; i < 8; i++)
	//{
	//	FColor newColor = DetermineColor(grid.val[i]);
	//	bool found = false;
	//	for (int j = 0; j < newColors.Num(); j++)
	//	{
	//		if (newColor == newColors[j]) {
	//			found = true;
	//			break;
	//		}
	//	}
	//	if (!found) {
	//		newColors.Add(newColor);
	//		color.R = (color.R + newColor.R) *0.5f;
	//		color.G = (color.G + newColor.G) *0.5f;
	//		color.B = (color.B + newColor.B) *0.5f;
	//		color.A = (color.A + newColor.A) *0.5f;
	//	}
	//}

	TArray<FColor> newColors;
	for (int i = 1; i < 12; i++)
	{
		newColors.Add(DetermineColor(grid.val[0], grid.val[1]));
		newColors.Add(DetermineColor(grid.val[1], grid.val[2]));
		newColors.Add(DetermineColor(grid.val[2], grid.val[3]));
		newColors.Add(DetermineColor(grid.val[3], grid.val[0]));

		newColors.Add(DetermineColor(grid.val[4], grid.val[5]));
		newColors.Add(DetermineColor(grid.val[5], grid.val[6]));
		newColors.Add(DetermineColor(grid.val[6], grid.val[7]));
		newColors.Add(DetermineColor(grid.val[7], grid.val[4]));

		newColors.Add(DetermineColor(grid.val[0], grid.val[4]));
		newColors.Add(DetermineColor(grid.val[1], grid.val[5]));
		newColors.Add(DetermineColor(grid.val[2], grid.val[6]));
		newColors.Add(DetermineColor(grid.val[3], grid.val[7]));
	}

	int ntriang = 0;
	for (int i = 0; triTable[cubeindex][i] != -1; i += 3)
	{
		vertices.Add(vertlist[triTable[cubeindex][i + 2]] * voxelTerrain->UnitsPerVoxel);
		vertices.Add(vertlist[triTable[cubeindex][i + 1]] * voxelTerrain->UnitsPerVoxel);
		vertices.Add(vertlist[triTable[cubeindex][i]] * voxelTerrain->UnitsPerVoxel);

		colors.Add(newColors[triTable[cubeindex][i + 2]]);
		colors.Add(newColors[triTable[cubeindex][i + 1]]);
		colors.Add(newColors[triTable[cubeindex][i]]);

		ntriang++;
	}

	return ntriang;
}

FColor ChunkMeshUpdateAsyncTask::DetermineColor(FVoxelTerrainVoxel* voxel)
{
	FColor color = FColor();
	int targetLayer = component->GetPaintLayer(voxel->Material);
	color.R = targetLayer == 0 ? 255 : 0;
	color.G = targetLayer == 1 ? 255 : 0;
	color.B = targetLayer == 2 ? 255 : 0;
	color.A = targetLayer == 3 ? 255 : 0;
	return color;
}

FColor ChunkMeshUpdateAsyncTask::DetermineColor(FVoxelTerrainVoxel* voxel1, FVoxelTerrainVoxel* voxel2)
{
	FColor color1 = DetermineColor(voxel1);
	FColor color2 = DetermineColor(voxel2);
	return FColor((color1.R + color2.R)*0.5f, (color1.G + color2.G)*0.5f, (color1.B + color2.B)*0.5f, (color1.A + color2.A)*0.5f);
}

FVector ChunkMeshUpdateAsyncTask::VertexInterp(double isolevel, FVector p1, FVector p2, double valp1, double valp2)
{
	double mu;
	FVector p = FVector();
	if (FMath::Abs(isolevel - valp1) < 0.00001)
		return (p1);
	if (FMath::Abs(isolevel - valp2) < 0.00001)
		return (p2);
	if (FMath::Abs(valp1 - valp2) < 0.00001)
		return (p1);
	mu = (isolevel - valp1) / (valp2 - valp1);
	p.X = (float)(p1.X + mu * (p2.X - p1.X));
	p.Y = (float)(p1.Y + mu * (p2.Y - p1.Y));
	p.Z = (float)(p1.Z + mu * (p2.Z - p1.Z));
	return p;
}

GridCell::GridCell()
{
	p.SetNumUninitialized(8);
	val.SetNumUninitialized(8);
}

GridCell::~GridCell()
{
}
