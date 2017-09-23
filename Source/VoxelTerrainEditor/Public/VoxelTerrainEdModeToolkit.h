// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Editor/UnrealEd/Public/Toolkits/BaseToolkit.h"
#include "IDetailsView.h"

class AVoxelTerrain;
class FVoxelTerrainEdModeDetails;
class FVoxelTerrainEdModeToolkit : public FModeToolkit
{
public:
	~FVoxelTerrainEdModeToolkit();

	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;

	/** Initializes the geometry mode toolkit */
	virtual void Init(const TSharedPtr< class IToolkitHost >& InitToolkitHost) override;

	/** IToolkit interface */
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual class FEdMode* GetEditorMode() const override;
	virtual TSharedPtr<class SWidget> GetInlineContent() const override { return ToolkitWidget; }

	static bool IsVoxelTerrainSelected();
	static FReply OnButtonClick(FVector InOffset);
	static void OnFieldUpdate(const FText&, ETextCommit::Type, FText);

	bool IsModeEnabled(FName ModeName) const;
	bool IsModeActive(FName ModeName) const;

	void OnChangeTool(FName ToolName);
	bool IsToolEnabled(FName ToolName) const;
	bool IsToolActive(FName ToolName) const;
	void NotifyToolChanged();

private:
	UVoxelTerrainData* UISettings;
	TSharedPtr<IDetailsView> DetailsPanel;
	TSharedPtr<FVoxelTerrainEdModeDetails> Customization;

	FSlateBrush ToolIcon;

	TSharedPtr<SWidget> ToolkitWidget;
	FButtonStyle ButtonStyle;
	bool GetIsPropertyVisible(const FPropertyAndParent& PropertyAndParent) const;
	bool GetIsPropertyEditingEnabled(const FPropertyAndParent& PropertyAndParent) const;
protected:
	void OnChangeMode(FName ModeName);
};
