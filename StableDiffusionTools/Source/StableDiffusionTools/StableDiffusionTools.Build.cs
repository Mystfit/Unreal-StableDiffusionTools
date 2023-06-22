// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class StableDiffusionTools : ModuleRules
{
	public StableDiffusionTools(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_1;
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"ActorLayerUtilities",
                "VPUtilities",
                "GeometryFramework",
                "GeometryScriptingCore",
                "DynamicMesh"
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Projects",
				"InputCore",
				"EditorFramework",
				"UnrealEd",
				"ToolMenus",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"Blutility",
				"UMG",
				"AssetRegistry",
                "AssetTools",
                "EditorSubsystem",
				"MovieSceneCapture",  
                "RemoteSession",
				"LevelEditor",
				"PythonScriptPlugin",
				"DesktopPlatform",
				"MovieRenderPipelineRenderPasses",
				"MovieRenderPipelineSettings",
				"MovieRenderPipelineCore",
				"ImageWriteQueue",
				"RHI",
				"RenderCore",
				"ImageCore",
                "MaterialEditor",
                "HTTP"
				// ... add private dependencies that you statically link with here ...	
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
