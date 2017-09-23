// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "VoxelTerrainEditorPrivatePCH.h"
#include "VoxelTerrainEdMode.h"
#include "VoxelTerrainEdModeToolkit.h"
#include "Toolkits/ToolkitManager.h"
#include "VoxelTerrainData.h"
#include "VoxelTerrainEdModeData.h"
#include "VoxelTerrainSelectTool.h"
#include "VoxelTerrainSculptTool.h"
#include "VoxelTerrainSmoothTool.h"
#include "VoxelTerrainFlattenTool.h"
#include "VoxelTerrainRampTool.h"
#include "VoxelTerrainRetopologizeTool.h"
#include "VoxelTerrainPaintTool.h"

const FEditorModeID FVoxelTerrainEdMode::EM_VoxelTerrainEdModeId = TEXT("EM_VoxelTerrainEdMode");

FVoxelTerrainEdMode::FVoxelTerrainEdMode()
{
	CurrentToolMode = nullptr;
	CurrentTool = nullptr;
	CurrentToolIndex = INDEX_NONE;
	InitializeToolModes();
	InitializeTools();

	EdModeSettings = NewObject<UVoxelTerrainEdModeData>(GetTransientPackage(), TEXT("VoxelTerrainEdModeData"), RF_Transactional);
	EdModeSettings->RandomSeed = FMath::FloorToInt(FMath::RandRange(0, 50000));
}

FVoxelTerrainEdMode::~FVoxelTerrainEdMode()
{
	VoxelTerrainToolModes.Empty();

}

/** FGCObject interface */
void FVoxelTerrainEdMode::AddReferencedObjects(FReferenceCollector& Collector)
{
	// Call parent implementation
	FEdMode::AddReferencedObjects(Collector);

	Collector.AddReferencedObject(EdModeSettings);
}

void FVoxelTerrainEdMode::InitializeToolModes()
{
	FVoxelTerrainToolMode* ToolMode_Manage = new(VoxelTerrainToolModes)FVoxelTerrainToolMode(TEXT("ToolMode_Manage"));
	ToolMode_Manage->ValidTools.Add(TEXT("Tool_Select"));
	ToolMode_Manage->CurrentToolName = TEXT("Tool_Select");

	FVoxelTerrainToolMode* ToolMode_Sculpt = new(VoxelTerrainToolModes)FVoxelTerrainToolMode(TEXT("ToolMode_Sculpt"));
	ToolMode_Sculpt->ValidTools.Add(TEXT("Tool_Sculpt"));
	ToolMode_Sculpt->ValidTools.Add(TEXT("Tool_Smooth"));
	ToolMode_Sculpt->ValidTools.Add(TEXT("Tool_Flatten"));
	ToolMode_Sculpt->ValidTools.Add(TEXT("Tool_Ramp"));
	ToolMode_Sculpt->ValidTools.Add(TEXT("Tool_Retopologize"));
	ToolMode_Sculpt->CurrentToolName = TEXT("Tool_Sculpt");

	FVoxelTerrainToolMode* ToolMode_Paint = new(VoxelTerrainToolModes)FVoxelTerrainToolMode(TEXT("ToolMode_Paint"));
	ToolMode_Paint->ValidTools.Add(TEXT("Tool_Paint"));
	ToolMode_Paint->CurrentToolName = TEXT("Tool_Paint");
}

void FVoxelTerrainEdMode::InitializeTools()
{
	FVoxelTerrainSelectTool* Tool_Select = new FVoxelTerrainSelectTool();
	VoxelTerrainTools.Add(MoveTemp(Tool_Select));

	FVoxelTerrainSculptTool* Tool_Sculpt = new FVoxelTerrainSculptTool();
	VoxelTerrainTools.Add(MoveTemp(Tool_Sculpt));

	FVoxelTerrainSmoothTool* Tool_Smooth = new FVoxelTerrainSmoothTool();
	VoxelTerrainTools.Add(MoveTemp(Tool_Smooth));

	FVoxelTerrainFlattenTool* Tool_Flatten = new FVoxelTerrainFlattenTool();
	VoxelTerrainTools.Add(MoveTemp(Tool_Flatten));

	FVoxelTerrainRampTool* Tool_Ramp = new FVoxelTerrainRampTool();
	VoxelTerrainTools.Add(MoveTemp(Tool_Ramp));

	FVoxelTerrainRetopologizeTool* Tool_Retopologize = new FVoxelTerrainRetopologizeTool();
	VoxelTerrainTools.Add(MoveTemp(Tool_Retopologize));

	FVoxelTerrainPaintTool* Tool_Paint = new FVoxelTerrainPaintTool();
	VoxelTerrainTools.Add(MoveTemp(Tool_Paint));
}

void FVoxelTerrainEdMode::Enter()
{
	FEdMode::Enter();

	if (!Toolkit.IsValid() && UsesToolkits())
	{
		Toolkit = MakeShareable(new FVoxelTerrainEdModeToolkit);
		Toolkit->Init(Owner->GetToolkitHost());
	}

	SetCurrentToolMode("ToolMode_Manage");
}

void FVoxelTerrainEdMode::Exit()
{
	if (Toolkit.IsValid())
	{
		FToolkitManager::Get().CloseToolkit(Toolkit.ToSharedRef());
		Toolkit.Reset();
	}

	// Call base Exit method to ensure proper cleanup
	FEdMode::Exit();
}

void FVoxelTerrainEdMode::Tick(FEditorViewportClient * ViewportClient, float DeltaTime)
{
	ViewportClient->Viewport->CaptureMouse(true);
	if (CurrentTool && sceneView)
	{
		CurrentTool->Tick(ViewportClient->Viewport, sceneView, DeltaTime);
	}
}

TSharedRef<FUICommandList> FVoxelTerrainEdMode::GetUICommandList() const
{
	check(Toolkit.IsValid());
	return Toolkit->GetToolkitCommands();
}

bool FVoxelTerrainEdMode::DisallowMouseDeltaTracking() const
{
	// We never want to use the mouse delta tracker while painting
	return bToolActive;
}

bool FVoxelTerrainEdMode::HandleClick(FEditorViewportClient * InViewportClient, HHitProxy * HitProxy, const FViewportClick & Click)
{
	if (CurrentTool && CurrentTool->Click(/*HitProxy, Click*/))
	{
		return false;
	}
	return false;
}

bool FVoxelTerrainEdMode::InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
	if (CurrentTool && CurrentTool->InputKey(Viewport, Key, Event))
	{
		return true;
	}

	if (Key == EKeys::LeftMouseButton && Event == IE_Pressed)
	{
		Viewport->CaptureMouse(true);
		bToolActive = CurrentTool->Enter(/*ViewportClient*/);
		if (!bToolActive)
		{
			Viewport->CaptureMouse(false);
		}
		ViewportClient->Invalidate(false, false);
	}

	if (Event == IE_Released && CurrentTool && bToolActive)
	{
		//Set the cursor position to that of the slate cursor so it wont snap back
		Viewport->SetPreCaptureMousePosFromSlateCursor();
		CurrentTool->Exit(/*ViewportClient*/);
		Viewport->CaptureMouse(false);
		bToolActive = false;
	}

	return false;
}

bool FVoxelTerrainEdMode::InputDelta(FEditorViewportClient * InViewportClient, FViewport * InViewport, FVector & InDrag, FRotator & InRot, FVector & InScale)
{
	if (CurrentTool && CurrentTool->MouseMove(/*InViewportClient, InViewport, InDrag, InRot, InScale*/))
	{
		return true;
	}
	return false;
}

void FVoxelTerrainEdMode::Render(const FSceneView * View, FViewport * Viewport, FPrimitiveDrawInterface * PDI)
{
	if (CurrentTool)
	{
		sceneView = new FSceneView(*View);
		CurrentTool->Render(Viewport, View);
	}
}

bool FVoxelTerrainEdMode::UsesToolkits() const
{
	return true;
}

UEditorEngine * FVoxelTerrainEdMode::GetEditorEngine()
{
	return GEditor;
}

void FVoxelTerrainEdMode::SetCurrentToolMode(FName ToolModeName, bool bRestoreCurrentTool)
{
	if (CurrentToolMode == NULL || ToolModeName != CurrentToolMode->ToolModeName)
	{
		for (int32 i = 0; i < VoxelTerrainToolModes.Num(); ++i)
		{
			if (VoxelTerrainToolModes[i].ToolModeName == ToolModeName)
			{
				CurrentToolMode = &VoxelTerrainToolModes[i];
				if (bRestoreCurrentTool)
				{
					if (CurrentToolMode->CurrentToolName == NAME_None)
					{
						CurrentToolMode->CurrentToolName = CurrentToolMode->ValidTools[0];
					}
					SetCurrentTool(CurrentToolMode->CurrentToolName);
				}
				break;
			}
		}
	}
}

void FVoxelTerrainEdMode::SetCurrentTool(FName ToolName)
{
	// Several tools have identically named versions for sculpting and painting
	// Prefer the one with the same target type as the current mode

	int32 BackupToolIndex = INDEX_NONE;
	int32 ToolIndex = INDEX_NONE;
	for (int32 i = 0; i < VoxelTerrainTools.Num(); ++i)
	{
		FVoxelTerrainTool* Tool = VoxelTerrainTools[i];
		if (ToolName == Tool->GetToolName())
		{
			ToolIndex = i;
			break;
		}
	}

	if (ToolIndex == INDEX_NONE)
	{
		checkf(BackupToolIndex != INDEX_NONE, TEXT("Tool '%s' not found, please check name is correct!"), *ToolName.ToString());
		ToolIndex = BackupToolIndex;
	}
	check(ToolIndex != INDEX_NONE);

	SetCurrentTool(ToolIndex);
}

FVoxelTerrainTool* FVoxelTerrainEdMode::GetTool(FName ToolName)
{
	for (int32 i = 0; i < VoxelTerrainTools.Num(); ++i)
	{
		FVoxelTerrainTool* tool = VoxelTerrainTools[i];
		if (ToolName == tool->GetToolName())
		{
			return tool;
		}
	}
	return NULL;
}

void FVoxelTerrainEdMode::SetCurrentTool(int32 ToolIndex)
{
	if (CurrentTool)
	{
		CurrentTool->Exit();
	}
	CurrentToolIndex = VoxelTerrainTools.IsValidIndex(ToolIndex) ? ToolIndex : 0;
	CurrentTool = VoxelTerrainTools[CurrentToolIndex];
	if (!CurrentToolMode->ValidTools.Contains(CurrentTool->GetToolName()))
	{
		// if tool isn't valid for this mode then automatically switch modes
		// this mostly happens with shortcut keys
		bool bFoundValidMode = false;
		for (int32 i = 0; i < VoxelTerrainToolModes.Num(); ++i)
		{
			if (VoxelTerrainToolModes[i].ValidTools.Contains(CurrentTool->GetToolName()))
			{
				SetCurrentToolMode(VoxelTerrainToolModes[i].ToolModeName, false);
				bFoundValidMode = true;
				break;
			}
		}
		check(bFoundValidMode);
	}

	CurrentTool->Enter();

	CurrentToolMode->CurrentToolName = CurrentTool->GetToolName();

	if (Toolkit.IsValid())
	{
		StaticCastSharedPtr<FVoxelTerrainEdModeToolkit>(Toolkit)->NotifyToolChanged();
	}
}

bool FVoxelTerrainEdMode::MouseMove(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 MouseX, int32 MouseY)
{
	if (CurrentTool)
	{
		CurrentTool->MouseMove(/*ViewportClient, Viewport, MouseX, MouseY*/);
		ViewportClient->Invalidate(false, false);
		return true;
	}
	return false;
}

bool FVoxelTerrainEdMode::CapturedMouseMove(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 MouseX, int32 MouseY)
{
	return MouseMove(ViewportClient, Viewport, MouseX, MouseY);
}






