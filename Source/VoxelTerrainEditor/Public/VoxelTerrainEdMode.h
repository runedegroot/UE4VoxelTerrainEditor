// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UnrealEd.h" 
#include "Editor.h"
#include "VoxelTerrainTool.h"

struct FVoxelTerrainToolMode
{
	const FName				ToolModeName;

	TArray<FName>			ValidTools;
	FName					CurrentToolName;

	FVoxelTerrainToolMode(FName InToolModeName)
		: ToolModeName(InToolModeName)
	{
	}
};

class UVoxelTerrainData;
class UVoxelTerrainEdModeData;
class FVoxelTerrainEdMode : public FEdMode
{
private:
	TArray<FVoxelTerrainToolMode> VoxelTerrainToolModes;
	TArray<FVoxelTerrainTool*> VoxelTerrainTools;
	void InitializeToolModes();
	void InitializeTools();

public:
	const static FEditorModeID EM_VoxelTerrainEdModeId;
	FVoxelTerrainEdMode();
	~FVoxelTerrainEdMode();

	TSharedRef<FUICommandList> GetUICommandList() const;

	//VIRTUAL FUNCTIONS

	/** FGCObject interface */
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	/** FEdMode: Called when the mode is entered */
	virtual void Enter() override;

	/** FEdMode: Called when the mode is exited */
	virtual void Exit() override;

	/** FEdMode: Called when the mouse is moved over the viewport */
	virtual bool MouseMove(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 x, int32 y) override;

	/**
	* FEdMode: Called when the mouse is moved while a window input capture is in effect
	*
	* @param	InViewportClient	Level editor viewport client that captured the mouse input
	* @param	InViewport			Viewport that captured the mouse input
	* @param	InMouseX			New mouse cursor X coordinate
	* @param	InMouseY			New mouse cursor Y coordinate
	*
	* @return	true if input was handled
	*/
	virtual bool CapturedMouseMove(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY) override;

	/** FEdMode: Allow us to disable mouse delta tracking during painting */
	virtual bool DisallowMouseDeltaTracking() const override;

	/** FEdMode: Called once per frame */
	virtual void Tick(FEditorViewportClient* ViewportClient, float DeltaTime) override;

	/** FEdMode: Called when clicking on a hit proxy */
	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click) override;

	/** FEdMode: Called when a key is pressed */
	virtual bool InputKey(FEditorViewportClient* InViewportClient, FViewport* InViewport, FKey InKey, EInputEvent InEvent) override;

	/** FEdMode: Called when mouse drag input is applied */
	virtual bool InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale) override;

	/** FEdMode: Render elements for the landscape tool */
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) override;

	//DONE

	bool UsesToolkits() const override;
	// End of FEdMode interface

	UVoxelTerrainEdModeData* EdModeSettings;
	FVoxelTerrainToolMode* CurrentToolMode;
	FVoxelTerrainTool* CurrentTool;

	UEditorEngine* GetEditorEngine();

	// UI setting for additional UI Tools
	int32 CurrentToolIndex;

	void SetCurrentToolMode(FName, bool = true);
	void SetCurrentTool(FName);
	void SetCurrentTool(int32);
	FVoxelTerrainTool* GetTool(FName ToolName);

	bool bToolActive = false;

	const FSceneView * sceneView = nullptr;
};
