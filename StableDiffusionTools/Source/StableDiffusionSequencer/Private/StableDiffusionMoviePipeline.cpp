// Fill out your copyright notice in the Description page of Project Settings.


#include "StableDiffusionMoviePipeline.h"
#include "Engine/TextureRenderTarget2D.h"
#include "StableDiffusionSubsystem.h"
#include "MoviePipelineQueue.h"
#include "MoviePipeline.h"
#include "Async/Async.h"
#include "ImageUtils.h"
#include "EngineModule.h"
#include "IImageWrapperModule.h"
#include "LevelSequence.h"
#include "Misc/FileHelper.h"
#include "MoviePipelineOutputSetting.h"
#include "Channels/MovieSceneChannelProxy.h"
#include "RenderingThread.h"
#include "MoviePipelineImageQuantization.h"
#include "GenericPlatform/GenericPlatformProcess.h"
#include "ImageWriteTask.h"
#include "ImageWriteQueue.h"

#if WITH_EDITOR
FText UStableDiffusionMoviePipeline::GetFooterText(UMoviePipelineExecutorJob* InJob) const {
	return NSLOCTEXT(
		"MovieRenderPipeline",
		"DeferredBasePassSetting_FooterText_StableDiffusion",
		"Rendered frames are passed to the Stable Diffusion subsystem for processing");
}
#endif

void UStableDiffusionMoviePipeline::SetupForPipelineImpl(UMoviePipeline* InPipeline)
{
	Super::SetupForPipelineImpl(InPipeline);

	// Make sure model is loaded before we render
	auto SDSubsystem = GEditor->GetEditorSubsystem<UStableDiffusionSubsystem>();
	auto Tracks = InPipeline->GetTargetSequence()->GetMovieScene()->GetMasterTracks();
	for (auto Track : Tracks) {
		if (auto MasterOptionsTrack = Cast<UStableDiffusionOptionsTrack>(Track)) {
			OptionsTrack = MasterOptionsTrack;
			if (SDSubsystem) {
				auto Sections = OptionsTrack->GetAllSections();
				if (Sections.Num()) {
					// Use the first found SD options section to pull model info from
					auto OptionsSection = Cast<UStableDiffusionOptionsSection>(Sections[0]);
					if (OptionsSection) {
						if (OptionsSection->ModelAsset) {
							if (SDSubsystem->ModelOptions != OptionsSection->ModelAsset->Options) {
								SDSubsystem->InitModel(OptionsSection->ModelAsset->Options, false);
							}
						}
					}
				}
			}
		} else if (auto PromptTrack = Cast<UStableDiffusionPromptMovieSceneTrack>(Track)){
			PromptTracks.Add(PromptTrack);
		}
	}


}

void UStableDiffusionMoviePipeline::SetupImpl(const MoviePipeline::FMoviePipelineRenderPassInitSettings& InPassInitSettings)
{
	// Create stencil render material
	TSoftObjectPtr<UMaterialInterface> StencilMatRef = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(UStableDiffusionSubsystem::StencilLayerMaterialAsset));
	auto StencilActorLayerMat = StencilMatRef.LoadSynchronous();
	StencilMatInst = StencilMatRef.LoadSynchronous();

	// Create stencil render target
	StencilActorLayerRenderTarget = NewObject<UTextureRenderTarget2D>(GetTransientPackage());
	StencilActorLayerRenderTarget->ClearColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);

	// Initialize to the tile size (not final size) and use a 16 bit back buffer to avoid precision issues when accumulating later
	//StencilRenderTarget->InitCustomFormat(InPassInitSettings.BackbufferResolution.X, InPassInitSettings.BackbufferResolution.Y, EPixelFormat::PF_FloatRGBA, false);
	StencilActorLayerRenderTarget->InitCustomFormat(InPassInitSettings.BackbufferResolution.X, InPassInitSettings.BackbufferResolution.Y, PF_FloatRGBA, false);
	StencilActorLayerRenderTarget->UpdateResourceImmediate(true);

	// OCIO: Since this is a manually created Render target we don't need Gamma to be applied.
	// We use this render target to render to via a display extension that utilizes Display Gamma
	// which has a default value of 2.2 (DefaultDisplayGamma), therefore we need to set Gamma on this render target to 2.2 to cancel out any unwanted effects.
	StencilActorLayerRenderTarget->TargetGamma = FOpenColorIODisplayExtension::DefaultDisplayGamma;

	// Always add one stencil layer view state for the inpainting stencil mask
	StencilLayerViewStates.Add(FSceneViewStateReference());
	TileRenderTargets.Add(StencilActorLayerRenderTarget);

	Super::SetupImpl(InPassInitSettings);
}

void UStableDiffusionMoviePipeline::TeardownForPipelineImpl(UMoviePipeline* InPipeline)
{
	PromptTracks.Reset();
}

void UStableDiffusionMoviePipeline::GatherOutputPassesImpl(TArray<FMoviePipelinePassIdentifier>& ExpectedRenderPasses) {
	Super::GatherOutputPassesImpl(ExpectedRenderPasses);

	ExpectedRenderPasses.Add(StencilPassIdentifier);
}


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
		ViewFamily->bIsFirstViewInMultipleViewFamily = true;

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

		// Setup render targets and drawing surfaces
		FRenderTarget* RenderTarget = GetViewRenderTarget()->GameThread_GetRenderTargetResource();
		FRenderTarget* StencilRT = StencilActorLayerRenderTarget->GameThread_GetRenderTargetResource();
		FCanvas Canvas = FCanvas(RenderTarget, nullptr, GetPipeline()->GetWorld(), ERHIFeatureLevel::SM5, FCanvas::CDM_ImmediateDrawing, 1.0f);
		FCanvas StencilCanvas = FCanvas(StencilRT, nullptr, GetPipeline()->GetWorld(), ERHIFeatureLevel::SM5, FCanvas::CDM_ImmediateDrawing, 1.0f);

#if WITH_EDITOR
		auto SDSubsystem = GEditor->GetEditorSubsystem<UStableDiffusionSubsystem>();
		if (SDSubsystem) {
			// Get input image from rendered data
			// TODO: Add float colour support to the generated images
			FStableDiffusionInput Input;
			Input.Options.InSizeX = RenderTarget->GetSizeXY().X;
			Input.Options.InSizeY = RenderTarget->GetSizeXY().Y;
			Input.Options.OutSizeX = RenderTarget->GetSizeXY().X;
			Input.Options.OutSizeY = RenderTarget->GetSizeXY().Y;
			Input.MaskImagePixels.InsertUninitialized(0, StencilRT->GetSizeXY().X * StencilRT->GetSizeXY().Y);
			Input.InputImagePixels.InsertUninitialized(0, RenderTarget->GetSizeXY().X * RenderTarget->GetSizeXY().Y);

			// Get frame time for curve evaluation
			auto EffectiveFrame = FFrameNumber(this->GetPipeline()->GetOutputState().EffectiveFrameNumber);
			auto TargetSequencer = this->GetPipeline()->GetTargetSequence();
			auto OriginalSeqFramerateRatio = TargetSequencer->GetMovieScene()->GetDisplayRate().AsDecimal() / this->GetPipeline()->GetPipelineMasterConfig()->GetEffectiveFrameRate(TargetSequencer).AsDecimal();

			// To evaluate curves we need to use the original sequence frame number. 
			// Frame number for curves includes subframes so we also mult by 1000 to get the subframe number
			auto FullFrameTime = EffectiveFrame * OriginalSeqFramerateRatio * 1000.0f;

			if (OptionsTrack) {
				for (auto Section : OptionsTrack->Sections) {
					if (Section) {
						auto OptionSection = Cast<UStableDiffusionOptionsSection>(Section);
						if (OptionSection) {
							//Reload model if it doesn't match the current options track
							if (OptionSection->ModelAsset) {
								if (SDSubsystem->ModelOptions != OptionSection->ModelAsset->Options || !SDSubsystem->ModelInitialised) {
									SDSubsystem->InitModel(OptionSection->ModelAsset->Options, false);
								}
							}
							else if (!SDSubsystem->ModelInitialised) {
								UE_LOG(LogTemp, Error, TEXT("No model asset provided in Stable Diffusion Options section and no model loaded in StableDiffusionSubsystem. Please add a model asset to the Options track or initialize the StableDiffusionSubsystem model."))
							}
							// Evaluate curve values
							OptionSection->GetStrengthChannel().Evaluate(FullFrameTime, Input.Options.Strength);
							OptionSection->GetIterationsChannel().Evaluate(FullFrameTime, Input.Options.Iterations);
							OptionSection->GetIterationsChannel().Evaluate(FullFrameTime, Input.Options.Seed);
							
							TArray<FActorLayer> ActorLayers;
							for (auto LayerName : OptionSection->InpaintLayers) {
								ActorLayers.Add(FActorLayer{ LayerName });
							}
							Input.Options.InpaintLayers = ActorLayers;
						}
					}
				}
			}

			// Build combined prompt
			TArray<FString> AccumulatedPrompt;
			for (auto Track : PromptTracks) {
				for (auto Section : Track->Sections) {
					if (auto PromptSection = Cast<UStableDiffusionPromptMovieSceneSection>(Section)) {
						if (PromptSection->IsActive()) {
							FPrompt Prompt = PromptSection->Prompt;
							PromptSection->GetWeightChannel().Evaluate(FullFrameTime, Prompt.Weight);

							// Get frame range of the section
							auto SectionStartFrame = PromptSection->GetInclusiveStartFrame();
							auto SectionEndFrame = PromptSection->GetExclusiveEndFrame();
							if (SectionStartFrame < FullFrameTime && FullFrameTime < SectionEndFrame) {
								Input.Options.AddPrompt(Prompt);
							}
						}
					}
				}
			}

			// Render init image
			// Main render pass always uses target 0.
			GetRendererModule().BeginRenderingViewFamily(&Canvas, ViewFamily.Get());
			RenderTarget->ReadPixels(Input.InputImagePixels, FReadSurfaceDataFlags());

			// Get new view state for our stencil render
			CurrentLayerIndex = 0;
			ViewFamily = CalculateViewFamily(InOutSampleState);
			ViewFamily->bIsRenderedImmediatelyAfterAnotherViewFamily = true;
			ViewFamily->bIsMultipleViewFamily = true;
			ViewFamily->EngineShowFlags.PostProcessing = 1;
			View = const_cast<FSceneView*>(ViewFamily->Views[0]);


			// Set stencil material on view
			View->FinalPostProcessSettings.AddBlendable(StencilMatInst.Get(), 1.0f);
			IBlendableInterface* BlendableInterface = Cast<IBlendableInterface>(StencilMatInst);
			ViewFamily->EngineShowFlags.SetPostProcessMaterial(true);
			ViewFamily->EngineShowFlags.SetPostProcessing(true);
			BlendableInterface->OverrideBlendableSettings(*View, 1.f);
			View->FinalPostProcessSettings.bBufferVisualizationDumpRequired = true;

			{
				TArray<FScopedActorLayerStencil> SavedActorStencilStates;
				for (auto layer : Input.Options.InpaintLayers) {
					SavedActorStencilStates.Emplace(layer);
				}

				auto Accumulate = MakeForwardingEndpoint(FMoviePipelinePassIdentifier(LayerPassIdentifier), InSampleState);
				auto BufferPipe = MakeShared<FImagePixelPipe, ESPMode::ThreadSafe>();
				BufferPipe->AddEndpoint([this, Input = MoveTemp(Input), Canvas, LayerPassIdentifier, Accumulate, RenderTarget, StencilRT, InSampleState, StencilCanvas](TUniquePtr<FImagePixelData>&& InPixelData) mutable
				{
					// Copy frame pixel data from 16 bit to 8 bit
					Input.MaskImagePixels.InsertUninitialized(0, InPixelData->GetSize().X* InPixelData->GetSize().Y);
					TUniquePtr<FImagePixelData> QuantizedPixelData = UE::MoviePipeline::QuantizeImagePixelDataToBitDepth(InPixelData.Get(), 8);
					int64 OutSize;
					const void* OutRawData = nullptr;
					QuantizedPixelData->GetRawData(OutRawData, OutSize);
					memcpy(Input.MaskImagePixels.GetData(), OutRawData, OutSize);

					// Generate new SD frame
					auto SDSubsystem = GEditor->GetEditorSubsystem<UStableDiffusionSubsystem>();
					auto SDResult = SDSubsystem->GeneratorBridge->GenerateImageFromStartImage(Input);

					// Convert 8bit BGRA FColors returned from SD to 16bit BGRA
					TUniquePtr<TImagePixelData<FColor>> SDImageDataBuffer8bit = MakeUnique<TImagePixelData<FColor>>(FIntPoint(SDResult.OutWidth, SDResult.OutHeight), TArray64<FColor>(MoveTemp(SDResult.PixelData)));
					TUniquePtr<FImagePixelData> SDImageDataBuffer16bit = UE::MoviePipeline::QuantizeImagePixelDataToBitDepth(SDImageDataBuffer8bit.Get(), 16);

					TArray<FFloat16Color> ConvertedResultPixels;
					ConvertedResultPixels.InsertUninitialized(0, SDResult.PixelData.Num());
					for (size_t idx = 0; idx < SDResult.PixelData.Num(); ++idx) {
						ConvertedResultPixels[idx] = FFloat16Color(SDResult.PixelData[idx]);
					}
					ENQUEUE_RENDER_COMMAND(UpdateMoviePipelineRenderTarget)([this, &SDImageDataBuffer16bit, RenderTarget, ConvertedResultPixels](FRHICommandListImmediate& RHICmdList) {
						int64 OutSize;
						const void* OutRawData = nullptr;
						SDImageDataBuffer16bit->GetRawData(OutRawData, OutSize);
						RHICmdList.UpdateTexture2D(
							RenderTarget->GetRenderTargetTexture(),
							0,
							FUpdateTextureRegion2D(0, 0, 0, 0, RenderTarget->GetSizeXY().X, RenderTarget->GetSizeXY().Y),
							RenderTarget->GetSizeXY().X * sizeof(FFloat16Color),
							(uint8*)OutRawData
						);
						RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThread);
					});

					Accumulate(MoveTemp(SDImageDataBuffer16bit));
				});

				View->FinalPostProcessSettings.BufferVisualizationPipes.Add(StencilMatInst->GetFName(), BufferPipe);
				View->FinalPostProcessSettings.bBufferVisualizationDumpRequired = true;
				View->FinalPostProcessSettings.BufferVisualizationOverviewMaterials.Add(StencilMatInst);

				// Render stencil view
				GetRendererModule().BeginRenderingViewFamily(&StencilCanvas, ViewFamily.Get());

				PostRendererSubmission(InSampleState, StencilPassIdentifier, GetOutputFileSortingOrder() + 1, Canvas);
			}
		}
#endif
	}
}


void UStableDiffusionMoviePipeline::BeginExportImpl(){
	if (!bUpscale) {
		return;
	}

	UMoviePipelineOutputSetting* OutputSettings = GetPipeline()->GetPipelineMasterConfig()->FindSetting<UMoviePipelineOutputSetting>();
	check(OutputSettings);

	auto SDSubsystem = GEditor->GetEditorSubsystem<UStableDiffusionSubsystem>();
	check(SDSubsystem);

	// Free up loaded model so we have enough VRAM to upsample
	SDSubsystem->ReleaseModel();

	auto OutputData = GetPipeline()->GetOutputDataParams();
	for (auto shot : OutputData.ShotData) {
		for (auto renderpass : shot.RenderPassData) {
			// We want to persist the upsampler model so we don't have to keep reloading it every frame
			SDSubsystem->GeneratorBridge->StartUpsample();

			for (auto file : renderpass.Value.FilePaths) {
				// Reload image from disk
				UTexture2D* Image = FImageUtils::ImportFileAsTexture2D(file);
				FFloat16Color* MipData = reinterpret_cast<FFloat16Color*>(Image->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_ONLY));
				TArrayView64<FFloat16Color> SourceColors(MipData, Image->GetSizeX() * Image->GetSizeY());

				// Build our upsample parameters
				FStableDiffusionImageResult UpsampleInput;

				// Convert pixels from FFloat16Color to FColor
				UpsampleInput.PixelData.InsertUninitialized(0, SourceColors.Num());
				for (int idx = 0; idx < SourceColors.Num(); ++idx) {
					UpsampleInput.PixelData[idx] = SourceColors[idx].GetFloats().ToFColor(true);
				}
				Image->GetPlatformData()->Mips[0].BulkData.Unlock();

				// Upsample the image data
				UpsampleInput.OutWidth = Image->GetPlatformData()->SizeX;
				UpsampleInput.OutHeight = Image->GetPlatformData()->SizeY;
				auto UpsampleResult = SDSubsystem->GeneratorBridge->UpsampleImage(UpsampleInput);

				// Build an export task that will async write the upsampled image to disk
				TUniquePtr<FImageWriteTask> ExportTask = MakeUnique<FImageWriteTask>();
				ExportTask->Format = EImageFormat::EXR;
				ExportTask->CompressionQuality = (int32)EImageCompressionQuality::Default;
				FString OutputName = FString::Printf(TEXT("%s%s.%s"), *UpscaledFramePrefix, *FPaths::GetBaseFilename(file), *FPaths::GetExtension(file));
				FString OutputDirectory = OutputSettings->OutputDirectory.Path;
				FString OutputPath = FPaths::Combine(OutputDirectory,OutputName);
				ExportTask->Filename = OutputPath;
				
				// Convert RGBA pixels back to FloatRGBA
				TArray64<FFloat16Color> ConvertedSrcPixels;
				ConvertedSrcPixels.InsertUninitialized(0, UpsampleResult.PixelData.Num());
				for (size_t idx = 0; idx < UpsampleResult.PixelData.Num(); ++idx) {
					ConvertedSrcPixels[idx] = FFloat16Color(UpsampleResult.PixelData[idx]);
				}
				TUniquePtr<TImagePixelData<FFloat16Color>> UpscaledPixelData = MakeUnique<TImagePixelData<FFloat16Color>>(
					FIntPoint(UpsampleResult.OutWidth, UpsampleResult.OutHeight),
					MoveTemp(ConvertedSrcPixels)
				);
				ExportTask->PixelData = MoveTemp(UpscaledPixelData);

				// Enque image write
				GetPipeline()->ImageWriteQueue->Enqueue(MoveTemp(ExportTask));
			}

			SDSubsystem->GeneratorBridge->StopUpsample();
		}
	}
}
