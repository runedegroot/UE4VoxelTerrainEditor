#pragma once

class FVoxelTerrainEdModeStyle
{
public:

	static void Initialize();
	static void Shutdown();

	static TSharedPtr< class ISlateStyle > Get();
	static const FName GetStyleSetName();

private:

	static FString InContent(const FString& RelativePath, const ANSICHAR* Extension);
private:

	static TSharedPtr< class FSlateStyleSet > StyleSet;
};