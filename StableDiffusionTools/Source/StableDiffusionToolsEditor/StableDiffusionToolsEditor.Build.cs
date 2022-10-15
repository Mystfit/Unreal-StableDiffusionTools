// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class StableDiffusionToolsEditor : ModuleRules
{
	public StableDiffusionToolsEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
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
				"StableDiffusionTools"
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
