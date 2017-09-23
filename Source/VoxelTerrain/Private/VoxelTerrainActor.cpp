// Fill out your copyright notice in the Description page of Project Settings.

#include "VoxelTerrainPrivatePCH.h"
#include "VoxelTerrainActor.h"
#include "VoxelTerrainNoise.h"
#include "VoxelTerrainUtils.h"
#include "VoxelTerrainSculptTool.h"
#include "VoxelTerrainSmoothTool.h"
#include "VoxelTerrainFlattenTool.h"
#include "VoxelTerrainRampTool.h"
#include "VoxelTerrainRetopologizeTool.h"
#include "VoxelTerrainPaintTool.h"
#include "VoxelTerrainChunkComponent.h"
#include "InputCoreTypes.h"

DECLARE_CYCLE_STAT(TEXT("Actor/LoadChunkAt"), STAT_LoadChunkAt, STATGROUP_VoxelTerrainActor);
DECLARE_CYCLE_STAT(TEXT("Actor/GetChunkDataAt"), STAT_GetChunkDataAt, STATGROUP_VoxelTerrainActor);
DECLARE_CYCLE_STAT(TEXT("Actor/GetVoxelAt"), STAT_GetVoxelAt, STATGROUP_VoxelTerrainActor);
DECLARE_CYCLE_STAT(TEXT("Actor/GetChunkComponentAt"), STAT_GetChunkComponentAt, STATGROUP_VoxelTerrainActor);
DECLARE_CYCLE_STAT(TEXT("Actor/GetPerlinValue"), STAT_GetPerlinValue, STATGROUP_VoxelTerrainActor);
DECLARE_CYCLE_STAT(TEXT("Actor/GenerateChunkData"), STAT_GenerateChunkData, STATGROUP_VoxelTerrainActor);

// Sets default values
AVoxelTerrain::AVoxelTerrain(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	SetActorEnableCollision(true);
	RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("DefaultSceneRoot"));

	static ConstructorHelpers::FObjectFinder<UMaterial> Material(TEXT("Material'/VoxelTerrain/SphereBrush'"));

	tools.Add(new FVoxelTerrainSculptTool());
	tools.Add(new FVoxelTerrainSmoothTool());
	tools.Add(new FVoxelTerrainFlattenTool());
	tools.Add(new FVoxelTerrainRampTool());
	tools.Add(new FVoxelTerrainRetopologizeTool());
	tools.Add(new FVoxelTerrainPaintTool());

	if (Material.Succeeded())
	{
		brushMaterial = Material.Object;
	}
}

// Called when an actor is done spawning into the world
void AVoxelTerrain::OnConstruction(const FTransform & Transform)
{
	Super::OnConstruction(Transform);
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("OnConstruction called"));
	if (Settings)
	{
		Initialize();
	}
}

// Called when the game starts or when spawned
void AVoxelTerrain::BeginPlay()
{
	Super::BeginPlay();
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TEXT("BeginPlay called"));
	if (Settings)
	{
		Initialize();
	}
}

void AVoxelTerrain::Destroyed()
{
	for (auto& ElemX : chunkComponents)
	{
		for (auto& ElemY : ElemX.Value.chunkComponents)
		{
			UVoxelTerrainChunkComponent* chunkComponent = ElemY.Value;
			RemoveChunk(chunkComponent);
		}
	}
}

void AVoxelTerrain::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	//
}

void AVoxelTerrain::CreateVoxelTerrain(UVoxelTerrainData* settings)
{
	//Settings = NewObject<UVoxelTerrainData>(this, UVoxelTerrainData::StaticClass(), NAME_None, RF_NoFlags, settings);
	Settings = NewObject<UVoxelTerrainData>(this, NAME_None, RF_NoFlags, settings);

	RootComponent->SetWorldLocation(FVector(0, 0, -Settings->TerrainHeight * 100));
	RootComponent->SetMobility(Settings->TerrainMode == EVoxelTerrainMode::VTM_Static ? EComponentMobility::Static : EComponentMobility::Movable);

	Initialize();
}

void AVoxelTerrain::ActivateTool(EVoxelTerrainTools tool, APlayerController* playerController)
{
	switch (tool)
	{
	case EVoxelTerrainTools::Sculpt:
		activeTool = GetTool("Tool_Sculpt");
		break;
	case EVoxelTerrainTools::Smooth:
		activeTool = GetTool("Tool_Smooth");
		break;
	case EVoxelTerrainTools::Flatten:
		activeTool = GetTool("Tool_Flatten");
		break;
	case EVoxelTerrainTools::Ramp:
		activeTool = GetTool("Tool_Ramp");
		break;
	case EVoxelTerrainTools::Retopologize:
		activeTool = GetTool("Tool_Retopologize");
		break;
	case EVoxelTerrainTools::Paint:
		activeTool = GetTool("Tool_Paint");
		break;
	}
	activeTool->Enter();
	activeController = playerController;
}

FVoxelTerrainTool* AVoxelTerrain::GetTool(FName ToolName)
{
	for (int32 i = 0; i < tools.Num(); ++i)
	{
		FVoxelTerrainTool* tool = tools[i];
		if (ToolName == tool->GetToolName())
		{
			return tool;
		}
	}
	return NULL;
}

void AVoxelTerrain::UpdateTool(float brushFalloff, float brushSize, float toolStrength)
{
	for (int32 i = 0; i < tools.Num(); ++i)
	{
		if (brushFalloff >= 0)tools[i]->BrushFalloff = brushFalloff;
		if (brushSize >= 0)tools[i]->BrushSize = brushSize;
		if (toolStrength >= 0)tools[i]->ToolStrength = toolStrength;
		if (FString(tools[i]->GetToolName()) == FString("Tool_Paint"))
			((FVoxelTerrainPaintTool*)tools[i])->PaintMaterial = PaintMaterial;
	}
}

void AVoxelTerrain::DeactivateTool()
{
	activeTool->Exit();
	if (activeTool)
		activeTool = nullptr;
	if (activeController)
		activeController = nullptr;
}

void AVoxelTerrain::SetFoliageEnabled(bool enabled)
{
	if (renderFoliage != enabled)
	{
		renderFoliage = enabled;
		UpdateAllChunks();
	}
}

void AVoxelTerrain::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (activeTool && activeController)
	{
		ULocalPlayer* localPlayer = activeController->GetLocalPlayer();

		FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
			localPlayer->ViewportClient->Viewport,
			GetWorld()->Scene,
			localPlayer->ViewportClient->EngineShowFlags)
			.SetRealtimeUpdate(true));

		FVector ViewLocation;
		FRotator ViewRotation;
		FSceneView* sceneView = localPlayer->CalcSceneView(&ViewFamily, /*out*/ ViewLocation, /*out*/ ViewRotation, localPlayer->ViewportClient->Viewport);
		FIntPoint size = GetWorld()->GetGameViewport()->Viewport->GetSizeXY();
		size.X = size.X*0.5f;
		size.Y = size.Y*0.5f;
		GetWorld()->GetGameViewport()->Viewport->SetMouse(size.X, size.Y);
		if (activeController->WasInputKeyJustPressed(EKeys::LeftShift))
			activeTool->InputKey(GetWorld()->GetGameViewport()->Viewport, EKeys::LeftShift, EInputEvent::IE_Pressed);
		if (activeController->WasInputKeyJustReleased(EKeys::LeftShift))
			activeTool->InputKey(GetWorld()->GetGameViewport()->Viewport, EKeys::LeftShift, EInputEvent::IE_Released);

		if (activeController->WasInputKeyJustPressed(EKeys::LeftMouseButton))
			activeTool->InputKey(GetWorld()->GetGameViewport()->Viewport, EKeys::LeftMouseButton, EInputEvent::IE_Pressed);
		if (activeController->WasInputKeyJustReleased(EKeys::LeftMouseButton))
			activeTool->InputKey(GetWorld()->GetGameViewport()->Viewport, EKeys::LeftMouseButton, EInputEvent::IE_Released);

		activeTool->Tick(GetWorld()->GetGameViewport()->Viewport, sceneView, DeltaTime);
	}
}

void AVoxelTerrain::Initialize()
{
	UnitsPerVoxel = (1 / Settings->VoxelsPerMeter) * 100;

	if (brushMaterial != NULL)
		DynamicBrushMaterial = UMaterialInstanceDynamic::Create(brushMaterial, this);

	surfaceNoise = NewObject<UVoxelTerrainNoise>(this, NAME_None, RF_Transient);
	surfaceNoise->SetSeed(Settings->Seed);
	surfaceNoise->SetFractalOctaves(Settings->Octaves);
	surfaceNoise->SetFractalLacunarity(Settings->Lacunarity);
	//0.006 is used to adapt the default frequency
	surfaceNoise->SetFrequency(0.006f * Settings->Frequency * (1 / Settings->VoxelsPerMeter));

	caveNoise = NewObject<UVoxelTerrainNoise>(this, NAME_None, RF_Transient);
	caveNoise->SetSeed(Settings->Seed);
	caveNoise->SetFractalOctaves(Settings->Octaves);
	//0.006 is used to adapt the default frequency
	caveNoise->SetFrequency(0.006f * Settings->Frequency * Settings->CaveFrequency * (1 / Settings->VoxelsPerMeter));

	TArray<UVoxelTerrainChunkComponent*> components;
	for (int x = -Settings->TerrainSize; x <= Settings->TerrainSize; x++)
	{
		for (int y = -Settings->TerrainSize; y <= Settings->TerrainSize; y++)
		{
			components.Add(LoadChunkAt(FIntPoint(x, y)));
		}
	}
	for (int i = 0; i < components.Num(); i++)
	{
		UpdateChunk(components[i]);
	}
}

void AVoxelTerrain::UpdateChunks(FIntPoint inPos, int chunkRadius)
{
	for (int x = inPos.X - chunkRadius; x <= inPos.X + chunkRadius; x++)
	{
		for (int y = inPos.Y - chunkRadius; y <= inPos.Y + chunkRadius; y++)
		{
			UpdateChunkAt(FIntPoint(x, y));
		}
	}
}

void AVoxelTerrain::ViewChunks(FIntPoint inPos, int chunkRadius)
{
	for (auto& ElemX : chunkComponents)
	{
		for (auto& ElemY : ElemX.Value.chunkComponents)
		{
			UVoxelTerrainChunkComponent* chunkComponent = ElemY.Value;
			FIntPoint pos = chunkComponent->Position;
			if (pos.X < inPos.X - chunkRadius || pos.X > inPos.X + chunkRadius
				|| pos.Y < inPos.Y - chunkRadius || pos.Y > inPos.Y + chunkRadius)
			{
				//Chunk is out of range, so we delete it
				RemoveChunk(chunkComponent);
			}
		}
	}

	TArray<UVoxelTerrainChunkComponent*> components;
	for (int x = inPos.X - chunkRadius; x <= inPos.X + chunkRadius; x++)
	{
		for (int y = inPos.Y - chunkRadius; y <= inPos.Y + chunkRadius; y++)
		{
			if (GetChunkComponentAt(FIntPoint(x, y)) == NULL)
				components.Add(LoadChunkAt(FIntPoint(x, y)));
		}
	}
	for (int i = 0; i < components.Num(); i++)
	{
		UpdateChunk(components[i]);
	}
}

void AVoxelTerrain::UpdateAllChunks()
{
	for (auto& ElemX : chunkComponents)
	{
		for (auto& ElemY : ElemX.Value.chunkComponents)
		{
			UVoxelTerrainChunkComponent* chunkComponent = ElemY.Value;
			UpdateChunk(chunkComponent);
		}
	}
}

UVoxelTerrainChunkData* AVoxelTerrain::GenerateChunkData(FIntPoint inPos)
{
	SCOPE_CYCLE_COUNTER(STAT_GenerateChunkData);
	UVoxelTerrainChunkData* chunkData = NewObject<UVoxelTerrainChunkData>(this, NAME_None, RF_Transient);
	for (int x = 0; x < Settings->ChunkSize + 1; x++)
	{
		(*chunkData).terrain.Values.Add(FVoxelTerrainRawDataY());
		for (int y = 0; y < Settings->ChunkSize + 1; y++)
		{
			(*chunkData)[x].Values.Add(FVoxelTerrainRawDataZ());
			for (int z = 0; z < Settings->TerrainHeight; z++)
			{
				(*chunkData)[x][y].Voxels.Add(FVoxelTerrainVoxel());
			}
		}
	}

	for (int z = 0; z < Settings->TerrainHeight; z++)
	{
		for (int y = 0; y < Settings->ChunkSize + 1; y++)
		{
			for (int x = 0; x < Settings->ChunkSize + 1; x++)
			{
				if (Settings->UsePerlinNoise) {
					int noiseX = (x + (inPos.X * Settings->ChunkSize));
					int noiseY = (y + (inPos.Y * Settings->ChunkSize));
					chunkData->terrain[x][y][z].Value = GetPerlinValue(noiseX, noiseY, z);
				}
				else {
					//Flat Terrain
					if (z < Settings->TerrainHeight - 10)
						(*chunkData)[x][y][z].Value = 1;
				}
			}
		}
	}
	return chunkData;
}

FVoxelTerrainVoxel* AVoxelTerrain::GetVoxelLocalAt(FIntVector inPos, FIntPoint chunkPos, bool edit)
{
	FIntVector worldPos;
	worldPos.X = inPos.X + (chunkPos.X*Settings->ChunkSize);
	worldPos.Y = inPos.Y + (chunkPos.Y*Settings->ChunkSize);
	worldPos.Z = inPos.Z;
	return GetVoxelAt(worldPos, edit);
}

FVoxelTerrainVoxel* AVoxelTerrain::GetVoxelAt(FIntVector inPos, bool edit)
{
	SCOPE_CYCLE_COUNTER(STAT_GetVoxelAt);
	FIntPoint chunkPos;
	chunkPos.X = FMath::FloorToInt(inPos.X / (float)Settings->ChunkSize);
	chunkPos.Y = FMath::FloorToInt(inPos.Y / (float)Settings->ChunkSize);
	FIntVector localPos;
	localPos.X = inPos.X%Settings->ChunkSize;
	if (localPos.X < 0)
		localPos.X = Settings->ChunkSize - FMath::Abs(localPos.X);
	localPos.Y = inPos.Y%Settings->ChunkSize;
	if (localPos.Y < 0)
		localPos.Y = Settings->ChunkSize - FMath::Abs(localPos.Y);
	localPos.Z = FMath::Clamp(inPos.Z, 0, Settings->TerrainHeight - 1);
	UVoxelTerrainChunkComponent* component = GetChunkComponentAt(chunkPos);
	if (component)
		return component->GetVoxelAt(FIntVector(localPos.X, localPos.Y, localPos.Z), edit);

	//If there's no chunk at this position, return a temporary voxel
	//This could be the case at the edge of a map
	FVoxelTerrainVoxel* tmpVoxel = new FVoxelTerrainVoxel();
	tmpVoxel->Value = GetPerlinValue(inPos.X, inPos.Y, localPos.Z);
	return tmpVoxel;
}

FVoxelTerrainVoxel& AVoxelTerrain::GetVoxelAtBP(FIntVector inPos)
{
	return *GetVoxelAt(inPos, false);
}

void AVoxelTerrain::SetVoxelAtBP(FIntVector inPos, FVoxelTerrainVoxel& voxel)
{
	FVoxelTerrainVoxel* container = GetVoxelAt(inPos, true);
	*container = voxel;
}

UVoxelTerrainChunkComponent* AVoxelTerrain::GetChunkComponentAt(FIntPoint inPos)
{
	SCOPE_CYCLE_COUNTER(STAT_GetChunkComponentAt);
	FScopeLock Lock(&mutex);
	if (chunkComponents.Contains(inPos.X) && chunkComponents[inPos.X].chunkComponents.Contains(inPos.Y))
		return chunkComponents[inPos.X][inPos.Y];
	return NULL;
}

float AVoxelTerrain::GetPerlinValue(int x, int y, int z)
{
	SCOPE_CYCLE_COUNTER(STAT_GetPerlinValue);

	//Calc surface field
	float center = Settings->TerrainHeight - Settings->Amplitude;
	float current = z - center;
	float amplitude = Settings->Amplitude;
	float noiseLocal = current / amplitude;
	float noise = surfaceNoise->GetSimplexFractal(x, y);
	float value = noiseLocal - noise;
	value = FMath::Clamp<float>(value, -1, 1);
	value = -value;
	if (!Settings->Smooth) {
		if (value >= 0)
			value = 1.f;
		else
			value = -1.f;
	}

	//Calc surface height
	float surfaceHeight = center + noise*amplitude;

	//Calc cave field
	float cave = Settings->Caves ? caveNoise->GetSimplexFractal(x*Settings->CaveHorizontalMultiplier, y*Settings->CaveHorizontalMultiplier, z *Settings->CaveVerticalMultiplier) + (1 - Settings->CaveWidth) : 1;
	if (!Settings->Smooth) {
		if (cave >= 0)
			cave = 1.f;
		else
			cave = -1.f;
	}

	if (z <= 1)
		return 1;
	if (z >= FMath::FloorToInt(surfaceHeight) && cave >= 0)
		return value;
	return cave;
}

void AVoxelTerrain::RemoveChunkAt(FIntPoint inPos)
{
	UVoxelTerrainChunkComponent* component = GetChunkComponentAt(inPos);
	if (!component)
		return;
	RemoveChunk(component);
}

void AVoxelTerrain::RemoveChunk(UVoxelTerrainChunkComponent* chunkComponent)
{
	{
		FScopeLock Lock(&mutex);
		chunkComponents[chunkComponent->Position.X].chunkComponents.Remove(chunkComponent->Position.Y);
	}
	for (int i = 0; i < chunkComponent->foliageComponents.Num(); i++)
	{
		chunkComponent->foliageComponents[i]->DestroyComponent();
	}
	chunkComponent->DestroyComponent();
}

UVoxelTerrainChunkComponent* AVoxelTerrain::LoadChunkAt(FIntPoint inPos)
{
	SCOPE_CYCLE_COUNTER(STAT_LoadChunkAt);
	UVoxelTerrainChunkComponent* component = NewObject<UVoxelTerrainChunkComponent>(this, *inPos.ToString(), RF_Transient);
	component->Initialize(inPos, Settings->material);
	component->OnComponentCreated();
	component->RegisterComponent();
	if (component->bWantsInitializeComponent) component->InitializeComponent();
	{
		FScopeLock Lock(&mutex);
		if (!chunkComponents.Contains(inPos.X))
			chunkComponents.Add(inPos.X, FVoxelTerrainChunkComponentMap());
		chunkComponents[inPos.X].chunkComponents.Add(inPos.Y, component);
	}
	if (DynamicBrushMaterial != NULL)
		component->SetMaterial(1, DynamicBrushMaterial);
	component->SetWorldLocation(FVector(inPos.X * Settings->ChunkSize * UnitsPerVoxel, inPos.Y * Settings->ChunkSize * UnitsPerVoxel, 0));
	component->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	component->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	component->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	return component;
}

bool AVoxelTerrain::UpdateChunkAt(FIntPoint inPos)
{
	UVoxelTerrainChunkComponent* component = GetChunkComponentAt(inPos);
	if (component) {
		component->UpdateMesh();
		return true;
	}
	return false;
}

void AVoxelTerrain::UpdateChunk(UVoxelTerrainChunkComponent* component)
{
	component->UpdateMesh();
}

void AVoxelTerrain::UpdateBrushPosition(FVector brushPosition)
{
	if (DynamicBrushMaterial && chunkComponents.Num() > 0)
	{
		DynamicBrushMaterial->SetVectorParameterValue(FName(TEXT("WorldPosition")), FLinearColor(brushPosition.X, brushPosition.Y, brushPosition.Z, 0));
	}
}

void AVoxelTerrain::UpdateBrushInfo(float brushSize, float brushFalloff)
{
	if (DynamicBrushMaterial && chunkComponents.Num() > 0)
	{
		float inside = brushSize*(1 - brushFalloff);
		float outside = brushSize - inside;
		DynamicBrushMaterial->SetScalarParameterValue(FName(TEXT("LocalRadius")), inside*0.5f);
		DynamicBrushMaterial->SetScalarParameterValue(FName(TEXT("LocalFalloff")), outside*0.5f);
	}
}

