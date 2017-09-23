#include "VoxelTerrainEditorPrivatePCH.h"
#include "VoxelTerrainEdModeDetails.h"
#include "VoxelTerrainActor.h"
#include "VoxelTerrainEdMode.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "IDetailPropertyRow.h"
#include "DetailWidgetRow.h"
#include "IDetailGroup.h"
#include "PropertyHandle.h"
#include "PropertyCustomizationHelpers.h"
#include "VoxelTerrainSculptTool.h"
#include "VoxelTerrainData.h"
#include "VoxelTerrainEdModeData.h"
#include "VoxelTerrainEdModeCommands.h"
#include "VoxelTerrainEdModeStyle.h"

#define LOCTEXT_NAMESPACE "FVoxelTerrainEdModeDetails"

TSharedRef<IDetailCustomization> FVoxelTerrainEdModeDetails::MakeInstance()
{
	return MakeShareable(new FVoxelTerrainEdModeDetails);
}

void FVoxelTerrainEdModeDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	CommandList = GetEditorMode()->GetUICommandList();
	if (IsToolModeActive("ToolMode_Manage"))
	{
		IDetailCategoryBuilder& TerrainSettingsCategory = DetailBuilder.EditCategory("Terrain Settings");
		TerrainSettingsCategory.AddCustomRow(FText::GetEmpty())
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
			.Padding(15, 12, 0, 12)
			[
				SNew(STextBlock)
				.Font(DetailBuilder.GetDetailFont())
			.Text(LOCTEXT("Material_Tip", "Hint: Assign a material to see landscape layers"))
			]
			];
		TerrainSettingsCategory.AddCustomRow(FText::GetEmpty())
			[
				SNew(SButton)
				.Text(LOCTEXT("Create", "Create"))
			//.AddMetaData<FTutorialMetaData>(FTutorialMetaData(TEXT("CreateButton"), TEXT("LevelEditorToolBox")))
			.OnClicked_Raw(this, &FVoxelTerrainEdModeDetails::OnCreateButtonClicked)
			];
	}
	if (IsToolModeActive("ToolMode_Sculpt"))
	{
		IDetailCategoryBuilder& ToolSelectionCategory = DetailBuilder.EditCategory("Tools", FText::GetEmpty(), ECategoryPriority::Important);

		//Create tool selection widget
		FToolBarBuilder ToolSwitchButtons(CommandList, FMultiBoxCustomization::None);
		{
			ToolSwitchButtons.AddToolBarButton(FVoxelTerrainEdModeCommands::Get().SculptTool, NAME_None, LOCTEXT("Plugins.Tool.Sculpt", "Sculpt"), LOCTEXT("Plugins.Tool.Sculpt", "Smooth terrain sculpting tool"), FSlateIcon(FVoxelTerrainEdModeStyle::Get()->GetStyleSetName(), "Plugins.Tool.Sculpt"));
			ToolSwitchButtons.AddToolBarButton(FVoxelTerrainEdModeCommands::Get().SmoothTool, NAME_None, LOCTEXT("Plugins.Tool.Smooth", "Smooth"), LOCTEXT("Plugins.Tool.Smooth", "Smoothing tool that averages out terrain in brush"), FSlateIcon(FVoxelTerrainEdModeStyle::Get()->GetStyleSetName(), "Plugins.Tool.Smooth"));
			ToolSwitchButtons.AddToolBarButton(FVoxelTerrainEdModeCommands::Get().FlattenTool, NAME_None, LOCTEXT("Plugins.Tool.Flatten", "Flatten"), LOCTEXT("Plugins.Tool.Flatten", "Clip terrain in brush to brush center height"), FSlateIcon(FVoxelTerrainEdModeStyle::Get()->GetStyleSetName(), "Plugins.Tool.Flatten"));
			ToolSwitchButtons.AddToolBarButton(FVoxelTerrainEdModeCommands::Get().RampTool, NAME_None, LOCTEXT("Plugins.Tool.Ramp", "Ramp"), LOCTEXT("Plugins.Tool.Ramp", "Clip terrain in brush to brush center height with slope"), FSlateIcon(FVoxelTerrainEdModeStyle::Get()->GetStyleSetName(), "Plugins.Tool.Ramp"));
			ToolSwitchButtons.AddToolBarButton(FVoxelTerrainEdModeCommands::Get().RetopologizeTool, NAME_None, LOCTEXT("Plugins.Tool.Retopologize", "Retopologize"), LOCTEXT("Plugins.Tool.Retopologize", "Make terrain clamp to voxels to create a blocky area"), FSlateIcon(FVoxelTerrainEdModeStyle::Get()->GetStyleSetName(), "Plugins.Tool.Retopologize"));
		}

		ToolSelectionCategory.AddCustomRow(FText::GetEmpty())
			[
				SNew(SBox)
				.Padding(5)
			.HAlign(HAlign_Center)
			[
				ToolSwitchButtons.MakeWidget()
			]
			];
	}
	if (IsToolModeActive("ToolMode_Sculpt") || IsToolModeActive("ToolMode_Paint"))
	{
		DetailBuilder.GetProperty("BrushSize")->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FVoxelTerrainEdModeDetails::UpdateSculptInfo));
		DetailBuilder.GetProperty("BrushFalloff")->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FVoxelTerrainEdModeDetails::UpdateSculptInfo));
		DetailBuilder.GetProperty("ToolStrength")->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FVoxelTerrainEdModeDetails::UpdateSculptInfo));
	}
	if (IsToolModeActive("ToolMode_Paint"))
	{
		DetailBuilder.GetProperty("PaintMaterial")->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FVoxelTerrainEdModeDetails::UpdatePaintMaterial));
	}
}

void FVoxelTerrainEdModeDetails::UpdateSculptInfo()
{
	FVoxelTerrainEdMode* VoxelTerrainEdMode = GetEditorMode();
	if (VoxelTerrainEdMode != NULL && VoxelTerrainEdMode->CurrentTool != NULL)
	{
		((FVoxelTerrainTool*)VoxelTerrainEdMode->CurrentTool)->BrushSize = VoxelTerrainEdMode->EdModeSettings->BrushSize;
		((FVoxelTerrainTool*)VoxelTerrainEdMode->CurrentTool)->BrushFalloff = VoxelTerrainEdMode->EdModeSettings->BrushFalloff;
		((FVoxelTerrainTool*)VoxelTerrainEdMode->CurrentTool)->ToolStrength = VoxelTerrainEdMode->EdModeSettings->ToolStrength;
	}
}

void FVoxelTerrainEdModeDetails::UpdatePaintMaterial()
{
	FVoxelTerrainEdMode* VoxelTerrainEdMode = GetEditorMode();
	((FVoxelTerrainPaintTool*)VoxelTerrainEdMode->CurrentTool)->PaintMaterial = VoxelTerrainEdMode->EdModeSettings->PaintMaterial;
}

FVoxelTerrainEdMode * FVoxelTerrainEdModeDetails::GetEditorMode()
{
	return (FVoxelTerrainEdMode*)GLevelEditorModeTools().GetActiveMode(FVoxelTerrainEdMode::EM_VoxelTerrainEdModeId);
}

bool FVoxelTerrainEdModeDetails::IsToolActive(FName ToolName)
{
	FVoxelTerrainEdMode* VoxelTerrainEdMode = GetEditorMode();
	if (VoxelTerrainEdMode != NULL && VoxelTerrainEdMode->CurrentTool != NULL)
	{
		const FName CurrentToolName = VoxelTerrainEdMode->CurrentTool->GetToolName();
		return CurrentToolName == ToolName;
	}
	return false;
}

bool FVoxelTerrainEdModeDetails::IsToolModeActive(FName ToolModeName)
{
	FVoxelTerrainEdMode* VoxelTerrainEdMode = GetEditorMode();
	if (VoxelTerrainEdMode != NULL && VoxelTerrainEdMode->CurrentToolMode != NULL)
	{
		const FName CurrentToolModeName = VoxelTerrainEdMode->CurrentToolMode->ToolModeName;
		return CurrentToolModeName == ToolModeName;
	}
	return false;
}

FReply FVoxelTerrainEdModeDetails::OnCreateButtonClicked()
{
	FVoxelTerrainEdMode* VoxelTerrainEdMode = GetEditorMode();
	UWorld * world = GEditor->GetEditorWorldContext().World();
	if (!world) {
		UE_LOG(LogTemp, Warning, TEXT("There is no world to create a voxel terrain in"));
		return FReply::Unhandled();
	}
	for (TActorIterator<AVoxelTerrain> ActorItr(world); ActorItr; ++ActorItr)
	{
		// Same as with the Object Iterator, access the subclass instance with the * or -> operators.
		world->DestroyActor(*ActorItr);
	}

	UE_LOG(LogTemp, Warning, TEXT("Spawning a voxel terrain in world %s"), *world->GetName());
	AVoxelTerrain* actor = world->SpawnActor<AVoxelTerrain>(AVoxelTerrain::StaticClass());

	UVoxelTerrainData* terrainData = NewObject<UVoxelTerrainData>(GetTransientPackage(), TEXT("TempVoxelTerrainData"), RF_Transactional);
	terrainData->material = VoxelTerrainEdMode->EdModeSettings->material;
	terrainData->FoliageTypes = TArray<UVoxelTerrainFoliageType*>(VoxelTerrainEdMode->EdModeSettings->FoliageTypes);
	terrainData->TerrainMode = VoxelTerrainEdMode->EdModeSettings->TerrainMode;
	terrainData->MergeVertices = VoxelTerrainEdMode->EdModeSettings->MergeVertices;
	terrainData->VoxelsPerMeter = VoxelTerrainEdMode->EdModeSettings->VoxelsPerMeter;
	terrainData->TerrainSize = VoxelTerrainEdMode->EdModeSettings->TerrainSize;
	terrainData->TerrainHeight = VoxelTerrainEdMode->EdModeSettings->TerrainHeight;
	terrainData->ChunkSize = VoxelTerrainEdMode->EdModeSettings->ChunkSize;
	terrainData->Smooth = VoxelTerrainEdMode->EdModeSettings->Smooth;
	terrainData->UsePerlinNoise = VoxelTerrainEdMode->EdModeSettings->UsePerlinNoise;

	terrainData->Seed = VoxelTerrainEdMode->EdModeSettings->RandomSeed;
	terrainData->Lacunarity = VoxelTerrainEdMode->EdModeSettings->Lacunarity;
	terrainData->Frequency = VoxelTerrainEdMode->EdModeSettings->Frequency;
	terrainData->Amplitude = VoxelTerrainEdMode->EdModeSettings->Amplitude;
	terrainData->Octaves = VoxelTerrainEdMode->EdModeSettings->Octaves;

	terrainData->Caves = VoxelTerrainEdMode->EdModeSettings->Caves;
	terrainData->CaveWidth = VoxelTerrainEdMode->EdModeSettings->CaveWidth;
	terrainData->CaveFrequency = VoxelTerrainEdMode->EdModeSettings->CaveFrequency;
	terrainData->CaveHorizontalMultiplier = VoxelTerrainEdMode->EdModeSettings->CaveHorizontalMultiplier;
	terrainData->CaveVerticalMultiplier = VoxelTerrainEdMode->EdModeSettings->CaveVerticalMultiplier;

	actor->CreateVoxelTerrain(terrainData);
	terrainData->ConditionalBeginDestroy();

	GEditor->SelectActor(actor, true, true, true, true);
	VoxelTerrainEdMode->SetCurrentToolMode("ToolMode_Sculpt");
	return FReply::Handled();
}