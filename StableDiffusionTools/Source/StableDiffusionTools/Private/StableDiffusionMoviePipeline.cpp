// Fill out your copyright notice in the Description page of Project Settings.


#include "StableDiffusionMoviePipeline.h"
#include "Engine/TextureRenderTarget2D.h"
#include "StableDiffusionSubsystem.h"
#include "MoviePipeline.h"
#include "ImageUtils.h"
#include "EngineModule.h"
#include "RenderingThread.h"

#if WITH_EDITOR
FText UStableDiffusionMoviePipeline::GetFooterText(UMoviePipelineExecutorJob* InJob) const {
	return NSLOCTEXT(
		"MovieRenderPipeline",
		"DeferredBasePassSetting_FooterText_StableDiffusion",
		"Rendered frames are passed to the Stable Diffusion subsystem for processing");
}
#endif

void UStableDiffusionMoviePipeline::RenderSample_GameThreadImpl(const FMoviePipelineRenderPassMetrics& InSampleState)
{
	UMoviePipelineImagePassBase::RenderSample_GameThreadImpl(InSampleState);

	// Wait for a surface to be available to write to. This will stall the game thread while the RHI/Render Thread catch up.
	{
		SCOPE_CYCLE_COUNTER(STAT_MoviePipeline_WaitForAvailableSurface);
		SurfaceQueue->BlockUntilAnyAvailable();
	}

	// Main Render Pass
	{
		FMoviePipelinePassIdentifier LayerPassIdentifier;
		FMoviePipelineRenderPassMetrics InOutSampleState = InSampleState;

		CurrentLayerIndex = INDEX_NONE;
		TSharedPtr<FSceneViewFamilyContext> ViewFamily = CalculateViewFamily(InOutSampleState);

		// Add post-processing materials if needed
		FSceneView* View = const_cast<FSceneView*>(ViewFamily->Views[0]);
		View->FinalPostProcessSettings.BufferVisualizationOverviewMaterials.Empty();
		View->FinalPostProcessSettings.BufferVisualizationPipes.Empty();

		for (UMaterialInterface* Material : ActivePostProcessMaterials)
		{
			if (Material)
			{
				View->FinalPostProcessSettings.BufferVisualizationOverviewMaterials.Add(Material);
			}
		}

		for (UMaterialInterface* VisMaterial : View->FinalPostProcessSettings.BufferVisualizationOverviewMaterials)
		{
			// If this was just to contribute to the history buffer, no need to go any further.
			if (InOutSampleState.bDiscardResult)
			{
				continue;
			}
			LayerPassIdentifier = FMoviePipelinePassIdentifier(PassIdentifier.Name + VisMaterial->GetName());

			auto BufferPipe = MakeShared<FImagePixelPipe, ESPMode::ThreadSafe>();
			BufferPipe->AddEndpoint(MakeForwardingEndpoint(LayerPassIdentifier, InSampleState));

			View->FinalPostProcessSettings.BufferVisualizationPipes.Add(VisMaterial->GetFName(), BufferPipe);
		}

		LayerPassIdentifier  = FMoviePipelinePassIdentifier(PassIdentifier.Name);
		int32 NumValidMaterials = View->FinalPostProcessSettings.BufferVisualizationPipes.Num();
		View->FinalPostProcessSettings.bBufferVisualizationDumpRequired = NumValidMaterials > 0;

		// Submit to be rendered. Main render pass always uses target 0.
		FRenderTarget* RenderTarget = GetViewRenderTarget()->GameThread_GetRenderTargetResource();
		FCanvas Canvas = FCanvas(RenderTarget, nullptr, GetPipeline()->GetWorld(), ERHIFeatureLevel::SM5, FCanvas::CDM_DeferDrawing, 1.0f);
		GetRendererModule().BeginRenderingViewFamily(&Canvas, ViewFamily.Get());
		
		// Wait for scene to render first before generating SD image
		FlushRenderingCommands();

#if WITH_EDITOR
		auto SDSubsystem = GEditor->GetEditorSubsystem<UStableDiffusionSubsystem>();
		if (SDSubsystem) {
			// Get input image from rendered data
			// TODO: Add float colour support to the generated images
			TArray<FColor> ColorData;
			ColorData.InsertUninitialized(0, RenderTarget->GetSizeXY().X * RenderTarget->GetSizeXY().Y);
			RenderTarget->ReadPixels(ColorData);

			// TODO: Replace hardcoded images with prompt curves
			auto SDResult = SDSubsystem->GeneratorBridge->GenerateImageFromStartImage(
				"the michelin man on an island in the ocean, cumulus clouds, sparkling ocean, 4k", 
				RenderTarget->GetSizeXY().X, 
				RenderTarget->GetSizeXY().Y, 
				RenderTarget->GetSizeXY().X, 
				RenderTarget->GetSizeXY().Y, 
				ColorData, 
				TArray<FColor>(), 
				float(InSampleState.FrameIndex+1 + 10.0)/100.0f,
				150, 
				99999
			);

			TArray<FFloat16Color> ConvertedResultPixels;
			ConvertedResultPixels.InsertUninitialized(0, SDResult.PixelData.Num());
			for (size_t idx = 0; idx < SDResult.PixelData.Num(); ++idx) {
				ConvertedResultPixels[idx] = FFloat16Color(SDResult.PixelData[idx]);
			}

			// Blit generated image to the render target
			ENQUEUE_RENDER_COMMAND(UpdateMoviePipelineRenderTarget)([this, RenderTarget, ConvertedResultPixels](FRHICommandListImmediate& RHICmdList){
				if (!ConvertedResultPixels.IsEmpty()) {
					RHICmdList.UpdateTexture2D(
						RenderTarget->GetRenderTargetTexture(),
						0,
						FUpdateTextureRegion2D(0, 0, 0, 0, RenderTarget->GetSizeXY().X, RenderTarget->GetSizeXY().Y),
						RenderTarget->GetSizeXY().X * sizeof(FFloat16Color),
						(uint8*)ConvertedResultPixels.GetData()
					);
				}
			});
		}
#endif

		// Readback + Accumulate.
		PostRendererSubmission(InSampleState, LayerPassIdentifier, GetOutputFileSortingOrder() + 1, Canvas);
	}
}
