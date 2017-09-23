#include "VoxelTerrainEditorPrivatePCH.h"
#include "VoxelTerrainEdModeStyle.h"
#include "SlateStyle.h"
#include "IPluginManager.h"
#include "UnrealEd.h"

#define LOCTEXT_NAMESPACE "FVoxelTerrainEdModeStyle"

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( FVoxelTerrainEdModeStyle::InContent( RelativePath, ".png" ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( FVoxelTerrainEdModeStyle::InContent( RelativePath, ".png" ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( FVoxelTerrainEdModeStyle::InContent( RelativePath, ".png" ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( FVoxelTerrainEdModeStyle::InContent( RelativePath, ".ttf" ), __VA_ARGS__ )
#define TTF_CORE_FONT(RelativePath, ...) FSlateFontInfo( StyleSet->RootToCoreContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( FVoxelTerrainEdModeStyle::InContent( RelativePath, ".otf" ), __VA_ARGS__ )
#define OTF_CORE_FONT(RelativePath, ...) FSlateFontInfo( StyleSet->RootToCoreContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )

FString FVoxelTerrainEdModeStyle::InContent(const FString & RelativePath, const ANSICHAR * Extension)
{
	static FString ContentDir = IPluginManager::Get().FindPlugin(TEXT("VoxelTerrain"))->GetContentDir();
	return (ContentDir / RelativePath) + Extension;
}

TSharedPtr< FSlateStyleSet > FVoxelTerrainEdModeStyle::StyleSet = NULL;
TSharedPtr< class ISlateStyle > FVoxelTerrainEdModeStyle::Get() { return StyleSet; }

void FVoxelTerrainEdModeStyle::Initialize()
{
	// Const icon sizes
	const FVector2D Icon8x8(8.0f, 8.0f);
	const FVector2D Icon9x19(9.0f, 19.0f);
	const FVector2D Icon10x10(10.0f, 10.0f);
	const FVector2D Icon12x12(12.0f, 12.0f);
	const FVector2D Icon16x16(16.0f, 16.0f);
	const FVector2D Icon20x20(20.0f, 20.0f);
	const FVector2D Icon22x22(22.0f, 22.0f);
	const FVector2D Icon24x24(24.0f, 24.0f);
	const FVector2D Icon27x31(27.0f, 31.0f);
	const FVector2D Icon26x26(26.0f, 26.0f);
	const FVector2D Icon32x32(32.0f, 32.0f);
	const FVector2D Icon40x40(40.0f, 40.0f);
	const FVector2D Icon75x82(75.0f, 82.0f);
	const FVector2D Icon360x32(360.0f, 32.0f);
	const FVector2D Icon171x39(171.0f, 39.0f);
	const FVector2D Icon170x50(170.0f, 50.0f);
	const FVector2D Icon128x128(128.0f, 128.0f);
	const FVector2D Icon267x140(170.0f, 50.0f);

	// Only register once
	if (StyleSet.IsValid())
	{
		return;
	}

	StyleSet = MakeShareable(new FSlateStyleSet("VoxelTerrainEdMode"));
	StyleSet->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

	//Icons
	const FTextBlockStyle NormalText = FEditorStyle::GetWidgetStyle<FTextBlockStyle>("NormalText");

	StyleSet->Set("Plugins.Tab", new IMAGE_BRUSH("mode_40", Icon40x40));
	StyleSet->Set("Plugins.Mode.Manage", new IMAGE_BRUSH("toolmode_manage_40", Icon40x40));
	StyleSet->Set("Plugins.Mode.Sculpt", new IMAGE_BRUSH("toolmode_sculpt_40", Icon40x40));
	StyleSet->Set("Plugins.Mode.Paint", new IMAGE_BRUSH("toolmode_paint_40", Icon40x40));

	StyleSet->Set("Plugins.Tool.Sculpt", new IMAGE_BRUSH("tool_sculpt_40", Icon40x40));
	StyleSet->Set("Plugins.Tool.Smooth", new IMAGE_BRUSH("tool_smooth_40", Icon40x40));
	StyleSet->Set("Plugins.Tool.Flatten", new IMAGE_BRUSH("tool_flatten_40", Icon40x40));
	StyleSet->Set("Plugins.Tool.Ramp", new IMAGE_BRUSH("tool_ramp_40", Icon40x40));
	StyleSet->Set("Plugins.Tool.Erosion", new IMAGE_BRUSH("tool_erosion_40", Icon40x40));
	StyleSet->Set("Plugins.Tool.Noise", new IMAGE_BRUSH("tool_noise_40", Icon40x40));
	StyleSet->Set("Plugins.Tool.Retopologize", new IMAGE_BRUSH("tool_retopologize_40", Icon40x40));

	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
}

const FName FVoxelTerrainEdModeStyle::GetStyleSetName()
{
	return StyleSet->GetStyleSetName();
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef TTF_CORE_FONT
#undef OTF_FONT
#undef OTF_CORE_FONT

void FVoxelTerrainEdModeStyle::Shutdown()
{
	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
		ensure(StyleSet.IsUnique());
		StyleSet.Reset();
	}
}
