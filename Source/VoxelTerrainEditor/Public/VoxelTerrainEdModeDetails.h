#pragma once

#include "Editor/UnrealEd/Public/Toolkits/BaseToolkit.h"
#include "IDetailCustomNodeBuilder.h"
#include "IDetailCustomization.h"

class FVoxelTerrainEdModeDetails : public IDetailCustomization //, public TSharedFromThis<FLandscapeEditorDetailCustomization_Base>
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
private:
	FVoxelTerrainEdMode * GetEditorMode();
	TSharedPtr<FUICommandList> CommandList;
	bool IsToolActive(FName ToolName);
	bool IsToolModeActive(FName ToolModeName);
	void UpdateSculptInfo();
	void UpdatePaintMaterial();
	FReply OnCreateButtonClicked();
};