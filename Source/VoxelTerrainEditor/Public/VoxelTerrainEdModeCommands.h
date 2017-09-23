// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
* Unreal landscape editor actions
*/
class FVoxelTerrainEdModeCommands : public TCommands<FVoxelTerrainEdModeCommands>
{

public:
	FVoxelTerrainEdModeCommands() : TCommands<FVoxelTerrainEdModeCommands>
		(
			"VoxelTerrainEdMode", // Context name for fast lookup
			NSLOCTEXT("Contexts", "VoxelTerrainEdMode", "Voxel Terrain Editor Mode"), // Localized context name for displaying
			NAME_None, //"LevelEditor" // Parent
			FVoxelTerrainEdModeStyle::GetStyleSetName() // Icon Style Set
			)
	{
	}


	/**
	* Initialize commands
	*/
	virtual void RegisterCommands() override;

public:
	// Mode Switch
	TSharedPtr<FUICommandInfo> ManageMode;
	TSharedPtr<FUICommandInfo> SculptMode;
	TSharedPtr<FUICommandInfo> PaintMode;

	//// Tools
	TSharedPtr<FUICommandInfo> SelectTool;
	TSharedPtr<FUICommandInfo> SculptTool;
	TSharedPtr<FUICommandInfo> SmoothTool;
	TSharedPtr<FUICommandInfo> FlattenTool;
	TSharedPtr<FUICommandInfo> RampTool;
	TSharedPtr<FUICommandInfo> RetopologizeTool;
	TSharedPtr<FUICommandInfo> PaintTool;

	// Map
	TMap<FName, TSharedPtr<FUICommandInfo>> NameToCommandMap;
};

///**
// * Implementation of various level editor action callback functions
// */
//class FLevelEditorActionCallbacks
//{
//public:
//};
