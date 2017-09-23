// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class VoxelTerrain :ModuleRules
{
    public VoxelTerrain( TargetInfo Target )
    {

        PublicIncludePaths.AddRange(
            new string[] {
                "VoxelTerrain/Public"
				// ... add public include paths required here ...
			}
            );


        PrivateIncludePaths.AddRange(
            new string[] {
                "VoxelTerrain/Private",
				// ... add other private include paths required here ...
			}
            );


        PublicDependencyModuleNames.AddRange(
                new string[] {
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "InputCore",
                    "ProceduralMeshComponent",
                    "RHI",
                    "RenderCore",
                    "ShaderCore"
                }
            );

        //PrivateDependencyModuleNames.AddRange(
        //    new string[] {
        //            "EditorStyle",
        //            "Projects",
        //            "PropertyEditor",
        //            "SharedSettingsWidgets",
        //            "DirectoryWatcher",
        //            "GameProjectGeneration",
        //            "MainFrame"
        //    }
        //);

        //PrivateIncludePathModuleNames.AddRange(
        //    new string[] {
        //            "DesktopPlatform",
        //            "GameProjectGeneration",
        //    }
        //);

        //     DynamicallyLoadedModuleNames.AddRange(
        //         new string[]
        //         {
        //	// ... add any modules that your module loads dynamically here ...
        //}
        //         );
    }
}
