// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.


#include "VoxelTerrainEditorPrivatePCH.h"
#include "VoxelTerrainEdModeCommands.h"
#include "Framework/Commands/Commands.h"
#include "VoxelTerrainEdModeStyle.h"

#define LOCTEXT_NAMESPACE ""

void FVoxelTerrainEdModeCommands::RegisterCommands()
{
	UI_COMMAND(ManageMode, "Mode - Manage", "", EUserInterfaceActionType::RadioButton, FInputChord());
	NameToCommandMap.Add("ToolMode_Manage", ManageMode);
	UI_COMMAND(SculptMode, "Mode - Sculpt", "", EUserInterfaceActionType::RadioButton, FInputChord());
	NameToCommandMap.Add("ToolMode_Sculpt", SculptMode);
	UI_COMMAND(PaintMode, "Mode - Paint", "", EUserInterfaceActionType::RadioButton, FInputChord());
	NameToCommandMap.Add("ToolMode_Paint", PaintMode);

	UI_COMMAND(SelectTool, "Tool - Select", "", EUserInterfaceActionType::RadioButton, FInputChord());
	NameToCommandMap.Add("Tool_Select", SelectTool);

	UI_COMMAND(SculptTool, "Tool - Sculpt", "", EUserInterfaceActionType::RadioButton, FInputChord());
	NameToCommandMap.Add("Tool_Sculpt", SculptTool);
	UI_COMMAND(SmoothTool, "Tool - Smooth", "", EUserInterfaceActionType::RadioButton, FInputChord());
	NameToCommandMap.Add("Tool_Smooth", SmoothTool);
	UI_COMMAND(FlattenTool, "Tool - Flatten", "", EUserInterfaceActionType::RadioButton, FInputChord());
	NameToCommandMap.Add("Tool_Flatten", FlattenTool);
	UI_COMMAND(RampTool, "Tool - Ramp", "", EUserInterfaceActionType::RadioButton, FInputChord());
	NameToCommandMap.Add("Tool_Ramp", RampTool);
	UI_COMMAND(RetopologizeTool, "Tool - Retopologize", "", EUserInterfaceActionType::RadioButton, FInputChord());
	NameToCommandMap.Add("Tool_Retopologize", RetopologizeTool);

	UI_COMMAND(PaintTool, "Tool - Paint", "", EUserInterfaceActionType::RadioButton, FInputChord());
	NameToCommandMap.Add("Tool_Paint", PaintTool);
}

#undef LOCTEXT_NAMESPACE