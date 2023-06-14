// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class StableDiffusionSequencer: ModuleRules
{
	public StableDiffusionSequencer(ReadOnlyTargetRules Target) : base(Target)
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
				//"../StableDiffusionTools/Public"
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
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UMG",
				"AssetRegistry",
				"Sequencer",
				"MovieSceneCapture",
				"MovieRenderPipelineRenderPasses",
				"MovieRenderPipelineSettings",
				"MovieRenderPipelineCore",
				"MovieScene",
				"MovieSceneTracks",
				"ImageWriteQueue",
				"ImageWrapper",
				"RHI",
				"RenderCore",
				"OpenColorIO",
				"ActorLayerUtilities",
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
