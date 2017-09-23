// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class VoxelTerrainEditor :ModuleRules
{
    public VoxelTerrainEditor( TargetInfo Target )
    {

        PublicIncludePaths.AddRange(
            new string[] {
                "VoxelTerrainEditor/Public"
				// ... add public include paths required here ...
			}
            );


        PrivateIncludePaths.AddRange(
            new string[] {
                "VoxelTerrainEditor/Private",
				// ... add other private include paths required here ...
			}
            );


        PublicDependencyModuleNames.AddRange(
                new string[] {
                    "Core",
                    "CoreUObject",		// @todo Mac: for some reason CoreUObject and Engine are needed to link in debug on Mac
                    "Engine",
                    "InputCore",
                    "Slate",
                    "SlateCore",
                    "ProceduralMeshComponent",
                    "VoxelTerrain"
                }
            );

        PrivateDependencyModuleNames.AddRange(
            new string[] {
                    "EditorStyle",
                    "Projects",
                    "UnrealEd",
                    "PropertyEditor",
                    "SharedSettingsWidgets",
                    "DirectoryWatcher",
                    "GameProjectGeneration",
                    "MainFrame"
            }
        );

        PrivateIncludePathModuleNames.AddRange(
            new string[] {
                    "DesktopPlatform",
                    "GameProjectGeneration"
            }
        );

        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
				// ... add any modules that your module loads dynamically here ...
			}
            );
    }
}
