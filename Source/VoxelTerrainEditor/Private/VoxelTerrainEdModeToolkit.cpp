// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "VoxelTerrainEditorPrivatePCH.h"
#include "VoxelTerrainActor.h"
#include "VoxelTerrainEdMode.h"
#include "VoxelTerrainEdModeToolkit.h"
#include "VoxelTerrainEdModeStyle.h"
#include "VoxelTerrainEdModeDetails.h"
#include "VoxelTerrainEdModeCommands.h"
#include "VoxelTerrainEdMode.h"
#include "SlateBasics.h"
#include "SlateExtras.h"
#include "PropertyEditorModule.h"
#include "VoxelTerrainUtils.h"
#include "VoxelTerrainData.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "IDetailPropertyRow.h"
#include "DetailWidgetRow.h"
#include "IDetailGroup.h"

#define LOCTEXT_NAMESPACE "FVoxelTerrainEdModeToolkit"

void FVoxelTerrainEdModeToolkit::RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	//
}

void FVoxelTerrainEdModeToolkit::UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager)
{
	//
}

void FVoxelTerrainEdModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost)
{
	// BUTTOM COMMANDS:
	FVoxelTerrainEdModeCommands::Register();
	TSharedRef<FUICommandList> CommandList = GetToolkitCommands();
	auto NameToCommandMap = FVoxelTerrainEdModeCommands::Get().NameToCommandMap;

#define MAP_MODE(ModeName) CommandList->MapAction(NameToCommandMap.FindChecked(ModeName), FUIAction(FExecuteAction::CreateRaw(this, &FVoxelTerrainEdModeToolkit::OnChangeMode, FName(ModeName)), FCanExecuteAction::CreateRaw(this, &FVoxelTerrainEdModeToolkit::IsModeEnabled, FName(ModeName)), FIsActionChecked::CreateRaw(this, &FVoxelTerrainEdModeToolkit::IsModeActive, FName(ModeName))));
	MAP_MODE("ToolMode_Manage");
	MAP_MODE("ToolMode_Sculpt");
	MAP_MODE("ToolMode_Paint");
#undef MAP_MODE

#define MAP_TOOL(ToolName) CommandList->MapAction(NameToCommandMap.FindChecked(ToolName), FUIAction(FExecuteAction::CreateRaw(this, &FVoxelTerrainEdModeToolkit::OnChangeTool, FName(ToolName)), FCanExecuteAction::CreateRaw(this, &FVoxelTerrainEdModeToolkit::IsToolEnabled, FName(ToolName)), FIsActionChecked::CreateRaw(this, &FVoxelTerrainEdModeToolkit::IsToolActive, FName(ToolName))));
	MAP_TOOL("Tool_Select");
	MAP_TOOL("Tool_Sculpt");
	MAP_TOOL("Tool_Smooth");
	MAP_TOOL("Tool_Flatten");
	MAP_TOOL("Tool_Ramp");
	MAP_TOOL("Tool_Retopologize");
	MAP_TOOL("Tool_Paint");
#undef MAP_TOOL

	// MODES:
	FToolBarBuilder ModeSwitchButtons(CommandList, FMultiBoxCustomization::None);
	{
		ModeSwitchButtons.AddToolBarButton(FVoxelTerrainEdModeCommands::Get().ManageMode, NAME_None, LOCTEXT("Plugins.Mode.Manage", "Manage"), LOCTEXT("Plugins.Mode.Manage", "Contains tools to add a new terrain, import/export terrain, add/remove chunks and manage terrain"), FSlateIcon(FVoxelTerrainEdModeStyle::Get()->GetStyleSetName(), "Plugins.Mode.Manage"));
		ModeSwitchButtons.AddToolBarButton(FVoxelTerrainEdModeCommands::Get().SculptMode, NAME_None, LOCTEXT("Plugins.Mode.Sculpt", "Sculpt"), LOCTEXT("Plugins.Mode.Sculpt", "Contains tools that modify the shape of a terrain"), FSlateIcon(FVoxelTerrainEdModeStyle::Get()->GetStyleSetName(), "Plugins.Mode.Sculpt"));
		ModeSwitchButtons.AddToolBarButton(FVoxelTerrainEdModeCommands::Get().PaintMode, NAME_None, LOCTEXT("Plugins.Mode.Paint", "Paint"), LOCTEXT("Plugins.Mode.Paint", "Contains tools that paint terrain materials on to a terrain"), FSlateIcon(FVoxelTerrainEdModeStyle::Get()->GetStyleSetName(), "Plugins.Mode.Paint"));
	}

	//SETTINGS PANEL
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs(false, false, false, FDetailsViewArgs::HideNameArea);

	DetailsPanel = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsPanel->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateRaw(this, &FVoxelTerrainEdModeToolkit::GetIsPropertyVisible));

	FVoxelTerrainEdMode* VoxelTerrainEdMode = (FVoxelTerrainEdMode*)GetEditorMode();
	if (VoxelTerrainEdMode)
	{
		DetailsPanel->SetObject(VoxelTerrainEdMode->EdModeSettings, true);
	}

	PropertyEditorModule.RegisterCustomClassLayout(FName("VoxelTerrainEdModeData"), FOnGetDetailCustomizationInstance::CreateStatic(&FVoxelTerrainEdModeDetails::MakeInstance));

	SAssignNew(ToolkitWidget, SScrollBox)
		+ SScrollBox::Slot()
		.Padding(5)
		.HAlign(HAlign_Center)
		[
			SNew(SBox)
			[
				ModeSwitchButtons.MakeWidget()
			]
		]
	+ SScrollBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
		.Padding(0)
		[
			DetailsPanel.ToSharedRef()
		]
		];

	FModeToolkit::Init(InitToolkitHost);
}

FVoxelTerrainEdModeToolkit::~FVoxelTerrainEdModeToolkit()
{
	//CUSTOM DETAILS
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyEditorModule.UnregisterCustomClassLayout(FName("VoxelTerrainCustomLayout"));
}

bool FVoxelTerrainEdModeToolkit::IsVoxelTerrainSelected()
{
	USelection* selection = GEditor->GetSelectedActors();
	if (selection->Num() == 0)
		return false;
	for (int i = 0; i < selection->Num(); i++) {
		if (selection->GetSelectedObject(i)->IsA<AVoxelTerrain>())
			return true;
	}
	return false;
}

FReply FVoxelTerrainEdModeToolkit::OnButtonClick(FVector InOffset)
{
	USelection* SelectedActors = GEditor->GetSelectedActors();

	// Let editor know that we're about to do something that we want to undo/redo
	GEditor->BeginTransaction(LOCTEXT("MoveActorsTransactionName", "MoveActors"));

	// For each selected actor
	for (FSelectionIterator Iter(*SelectedActors); Iter; ++Iter)
	{
		if (AActor* LevelActor = Cast<AActor>(*Iter))
		{
			// Register actor in opened transaction (undo/redo)
			LevelActor->Modify();
			// Move actor to given location
			LevelActor->TeleportTo(LevelActor->GetActorLocation() + InOffset, FRotator(0, 0, 0));
		}
	}

	// We're done moving actors so close transaction
	GEditor->EndTransaction();

	return FReply::Handled();
}

void FVoxelTerrainEdModeToolkit::OnFieldUpdate(const FText& NewText, ETextCommit::Type TextType, FText Text)
{
	//
}

FName FVoxelTerrainEdModeToolkit::GetToolkitFName() const
{
	return FName("VoxelTerrainEdMode");
}

FText FVoxelTerrainEdModeToolkit::GetBaseToolkitName() const
{
	return NSLOCTEXT("VoxelTerrainEdModeToolkit", "DisplayName", "VoxelTerrainEdMode Tool");
}

class FEdMode* FVoxelTerrainEdModeToolkit::GetEditorMode() const
{
	return GLevelEditorModeTools().GetActiveMode(FVoxelTerrainEdMode::EM_VoxelTerrainEdModeId);
}

void FVoxelTerrainEdModeToolkit::OnChangeMode(FName ModeName)
{
	FVoxelTerrainEdMode* VoxelTerrainEdMode = (FVoxelTerrainEdMode*)GetEditorMode();
	if (VoxelTerrainEdMode)
	{
		VoxelTerrainEdMode->SetCurrentToolMode(ModeName);
	}
}

bool FVoxelTerrainEdModeToolkit::IsModeEnabled(FName ModeName) const
{
	FVoxelTerrainEdMode* VoxelTerrainEdMode = (FVoxelTerrainEdMode*)GetEditorMode();
	if (VoxelTerrainEdMode)
	{
		// Manage is the only mode enabled if we have no landscape
		//if (ModeName == "ToolMode_Manage" || VoxelTerrainEdMode->GetLandscapeList().Num() > 0)
		//{
		return true;
		//}
	}

	return false;
}

bool FVoxelTerrainEdModeToolkit::IsModeActive(FName ModeName) const
{
	FVoxelTerrainEdMode* VoxelTerrainEdMode = (FVoxelTerrainEdMode*)GetEditorMode();
	if (VoxelTerrainEdMode && VoxelTerrainEdMode->CurrentTool)
	{
		return VoxelTerrainEdMode->CurrentToolMode->ToolModeName == ModeName;
	}

	return false;
}

void FVoxelTerrainEdModeToolkit::OnChangeTool(FName ToolName)
{
	FVoxelTerrainEdMode* VoxelTerrainEdMode = (FVoxelTerrainEdMode*)GetEditorMode();
	if (VoxelTerrainEdMode != nullptr)
	{
		VoxelTerrainEdMode->SetCurrentTool(ToolName);
	}
}

bool FVoxelTerrainEdModeToolkit::IsToolEnabled(FName ToolName) const
{
	FVoxelTerrainEdMode* VoxelTerrainEdMode = (FVoxelTerrainEdMode*)GetEditorMode();
	if (VoxelTerrainEdMode != nullptr)
	{
		//if (ToolName == "NewLandscape" || VoxelTerrainEdMode->GetLandscapeList().Num() > 0)
		//{
		return true;
		//}
	}

	return false;
}

bool FVoxelTerrainEdModeToolkit::IsToolActive(FName ToolName) const
{
	FVoxelTerrainEdMode* VoxelTerrainEdMode = (FVoxelTerrainEdMode*)GetEditorMode();
	if (VoxelTerrainEdMode != nullptr)
	{
		const FName CurrentToolName = VoxelTerrainEdMode->CurrentTool->GetToolName();
		return CurrentToolName == ToolName;
	}

	return false;
}

bool FVoxelTerrainEdModeToolkit::GetIsPropertyVisible(const FPropertyAndParent& PropertyAndParent) const
{
	const UProperty& Property = PropertyAndParent.Property;
	FVoxelTerrainEdMode* VoxelTerrainEdMode = (FVoxelTerrainEdMode*)GetEditorMode();
	if (VoxelTerrainEdMode != nullptr && VoxelTerrainEdMode->CurrentTool != nullptr)
	{
		if (Property.HasMetaData("ShowForTools"))
		{
			const FName CurrentToolName = VoxelTerrainEdMode->CurrentTool->GetToolName();

			TArray<FString> ShowForTools;
			Property.GetMetaData("ShowForTools").ParseIntoArray(ShowForTools, TEXT(","), true);
			if (!ShowForTools.Contains(CurrentToolName.ToString()))
			{
				return false;
			}
		}
		//if (Property.HasMetaData("ShowForBrushes"))
		//{
		//	const FName CurrentBrushSetName = VoxelTerrainEdMode->LandscapeBrushSets[LandscapeEdMode->CurrentBrushSetIndex].BrushSetName;
		//	// const FName CurrentBrushName = LandscapeEdMode->CurrentBrush->GetBrushName();

		//	TArray<FString> ShowForBrushes;
		//	Property.GetMetaData("ShowForBrushes").ParseIntoArray(ShowForBrushes, TEXT(","), true);
		//	if (!ShowForBrushes.Contains(CurrentBrushSetName.ToString()))
		//		//&& !ShowForBrushes.Contains(CurrentBrushName.ToString())
		//	{
		//		return false;
		//	}
		//}
		//if (Property.HasMetaData("ShowForTargetTypes"))
		//{
		//	static const TCHAR* TargetTypeNames[] = { TEXT("Heightmap"), TEXT("Weightmap"), TEXT("Visibility") };

		//	TArray<FString> ShowForTargetTypes;
		//	Property.GetMetaData("ShowForTargetTypes").ParseIntoArray(ShowForTargetTypes, TEXT(","), true);

		//	const ELandscapeToolTargetType::Type CurrentTargetType = LandscapeEdMode->CurrentToolTarget.TargetType;
		//	if (CurrentTargetType == ELandscapeToolTargetType::Invalid ||
		//		ShowForTargetTypes.FindByKey(TargetTypeNames[CurrentTargetType]) == nullptr)
		//	{
		//		return false;
		//	}
		//}

		return true;
	}

	return false;
}

void FVoxelTerrainEdModeToolkit::NotifyToolChanged()
{
	FVoxelTerrainEdMode* VoxelTerrainEdMode = (FVoxelTerrainEdMode*)GetEditorMode();
	if (VoxelTerrainEdMode)
	{
		// Refresh details panel
		DetailsPanel->SetObject(VoxelTerrainEdMode->EdModeSettings, true);
	}
}

#undef LOCTEXT_NAMESPACE
