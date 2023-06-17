// Fill out your copyright notice in the Description page of Project Settings.

#include "StableDiffusionMoviePipeline.h"
#include "StableDiffusionToolsSettings.h"
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
#include "MovieScene.h"
#include "RenderingThread.h"
#include "MoviePipelineImageQuantization.h"
#include "GenericPlatform/GenericPlatformProcess.h"
#include "ImageWriteTask.h"
#include "ImageWriteQueue.h"
#include "StableDiffusionBlueprintLibrary.h"
#include "StableDiffusionToolsModule.h"
#include "Runtime/Launch/Resources/Version.h"



UStableDiffusionMoviePipeline::UStableDiffusionMoviePipeline() : UMoviePipelineDeferredPassBase()
{
	PassIdentifier = FMoviePipelinePassIdentifier("StableDiffusion");
}

void UStableDiffusionMoviePipeline::PostInitProperties()
{
	Super::PostInitProperties();

	auto Settings = GetMutableDefault<UStableDiffusionToolsSettings>();
	Settings->ReloadConfig(UStableDiffusionToolsSettings::StaticClass());

	if (!ImageGeneratorOverride) {
		if (Settings->GetGeneratorType() && Settings->GetGeneratorType() != UStableDiffusionBridge::StaticClass()) {
			ImageGeneratorOverride = Settings->GetGeneratorType();
		}
	}
	else {
		// We don't want to include the base bridge class as it has no implementation
		if (ImageGeneratorOverride->StaticClass() == UStableDiffusionBridge::StaticClass()) {
			ImageGeneratorOverride = nullptr;
		}
	}
}

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

	// Reset track containers
	LayerProcessorTracks.Reset();
	PromptTracks.Reset();
	OptionsTrack = nullptr;

	// Make sure model is loaded before we render
	auto SDSubsystem = GEditor->GetEditorSubsystem<UStableDiffusionSubsystem>();

	if (!SDSubsystem->GeneratorBridge || SDSubsystem->GeneratorBridge->StaticClass()->IsChildOf(ImageGeneratorOverride)) {
		SDSubsystem->CreateBridge(ImageGeneratorOverride);
	}

	auto Tracks = InPipeline->GetTargetSequence()->GetMovieScene()->GetMasterTracks();
	for (auto Track : Tracks) {
		if (auto MasterOptionsTrack = Cast<UStableDiffusionOptionsTrack>(Track)) {
			OptionsTrack = MasterOptionsTrack;
		} else if (auto PromptTrack = Cast<UStableDiffusionPromptMovieSceneTrack>(Track)){
			PromptTracks.Add(PromptTrack);
		}
		else if (auto LayerProcessorTrack = Cast<UStableDiffusionLayerProcessorTrack>(Track)) {
			LayerProcessorTracks.Add(LayerProcessorTrack);
		}
	}
}

void UStableDiffusionMoviePipeline::SetupImpl(const MoviePipeline::FMoviePipelineRenderPassInitSettings& InPassInitSettings)
{
	Super::SetupImpl(InPassInitSettings);
}

void UStableDiffusionMoviePipeline::TeardownForPipelineImpl(UMoviePipeline* InPipeline)
{
	PromptTracks.Reset();
}

void UStableDiffusionMoviePipeline::GatherOutputPassesImpl(TArray<FMoviePipelinePassIdentifier>& ExpectedRenderPasses) {
	Super::GatherOutputPassesImpl(ExpectedRenderPasses);
}


void UStableDiffusionMoviePipeline::RenderSample_GameThreadImpl(const FMoviePipelineRenderPassMetrics& InSampleState)
{
	UMoviePipelineImagePassBase::RenderSample_GameThreadImpl(InSampleState);

	// Wait for a surface to be available to write to. This will stall the game thread while the RHI/Render Thread catch up.
	{
		// Wait for a all surfaces to be available to write to. This will stall the game thread while the RHI/Render Thread catch up.
		SCOPE_CYCLE_COUNTER(STAT_MoviePipeline_WaitForAvailableSurface);
		for (TPair<FIntPoint, TSharedPtr<FMoviePipelineSurfaceQueue, ESPMode::ThreadSafe>> SurfaceQueueIt : SurfaceQueues)
		{
			if (SurfaceQueueIt.Value.IsValid())
			{
				SurfaceQueueIt.Value->BlockUntilAnyAvailable();
			}
		}
	}

	// Main Render Pass
	{
		FMoviePipelineRenderPassMetrics InOutSampleState = InSampleState;
		FMoviePipelinePassIdentifier LayerPassIdentifier(PassIdentifier);
		LayerPassIdentifier.Name = PassIdentifier.Name;
		LayerPassIdentifier.CameraName = GetCameraName(0);

		// Setup render targets and drawing surfaces
		FStableDiffusionDeferredPassRenderStatePayload Payload;
		Payload.CameraIndex = 0;
		Payload.TileIndex = InOutSampleState.TileIndexes;
		TWeakObjectPtr<UTextureRenderTarget2D> ViewRenderTarget = GetOrCreateViewRenderTarget(InOutSampleState.BackbufferSize, (IViewCalcPayload*)(&Payload));
		check(ViewRenderTarget.IsValid());
		FRenderTarget* RenderTarget = ViewRenderTarget->GameThread_GetRenderTargetResource();
		FCanvas Canvas = FCanvas(RenderTarget, nullptr, GetPipeline()->GetWorld(), ERHIFeatureLevel::SM5, FCanvas::CDM_ImmediateDrawing, 1.0f);

#if WITH_EDITOR
		auto SDSubsystem = GEditor->GetEditorSubsystem<UStableDiffusionSubsystem>();
		if (SDSubsystem) {
			// Get input image from rendered data
			// TODO: Add float colour support to the generated images
			FStableDiffusionInput Input;
			Input.PreviewIterationRate = -1;
			Input.DebugPythonImages = DebugPythonImages;
			Input.Options.InSizeX = RenderTarget->GetSizeXY().X;
			Input.Options.InSizeY = RenderTarget->GetSizeXY().Y;
			Input.Options.OutSizeX = RenderTarget->GetSizeXY().X;
			Input.Options.OutSizeY = RenderTarget->GetSizeXY().Y;

			// Get frame time for curve evaluation
			auto EffectiveFrame = FFrameNumber(this->GetPipeline()->GetOutputState().EffectiveFrameNumber);
			auto TargetSequencer = this->GetPipeline()->GetTargetSequence();
			auto OriginalSeqFramerateRatio = TargetSequencer->GetMovieScene()->GetDisplayRate().AsDecimal() / this->GetPipeline()->GetPipelineMasterConfig()->GetEffectiveFrameRate(TargetSequencer).AsDecimal();

			// To evaluate curves we need to use the original sequence frame number. 
			// Frame number for curves includes subframes so we also mult by 1000 to get the subframe number
			auto FullFrameTime = EffectiveFrame * OriginalSeqFramerateRatio * 1000.0f;

			// Build layer processor options
			TArray<FLayerData> Layers;
			for (auto Track : LayerProcessorTracks) {
				for (auto Section : Track->Sections) {
					if (auto LayerProcessorSection = Cast<UMovieSceneParameterSection>(Section)) {
						if (LayerProcessorSection->IsActive()) {
							
							// Get frame range of the section
							bool InRange = true;
							InRange &= (LayerProcessorSection->HasStartFrame()) ? LayerProcessorSection->GetInclusiveStartFrame() < FullFrameTime : true;
							InRange &= (LayerProcessorSection->HasEndFrame()) ? LayerProcessorSection->GetExclusiveEndFrame() > FullFrameTime : true;
							if (InRange)
							{
								auto ScalarParams = LayerProcessorSection->GetScalarParameterNamesAndCurves();
								
								// Evaluate options for the layer processor
								if (auto LayerProcessor = Track->LayerProcessor) {
									auto LayerOptions = Track->LayerProcessor->AllocateLayerOptions();

									if (IsValid(LayerOptions)) {
										// Iterate over all properties in the processor options class
										for (TFieldIterator<FProperty> PropertyIt(LayerOptions->GetClass()); PropertyIt; ++PropertyIt)
										{
											if (FProperty* Prop = *PropertyIt) {
												if (const FFloatProperty* FloatProperty = CastField<FFloatProperty>(Prop)) {
													// Match the property against the available parameter curves in this section
													auto Param = ScalarParams.FindByPredicate([&Prop](const FScalarParameterNameAndCurve& ParamNameAndCurve) {
														return ParamNameAndCurve.ParameterName == Prop->GetFName();
														});

													// Evaluate the parameter at the current time and set the value in the layer options object
													if (Param) {
														float ParamValAtTime;
														Param->ParameterCurve.Evaluate(FullFrameTime, ParamValAtTime);
														Prop->SetValue_InContainer(LayerOptions, &ParamValAtTime);
													}
												}
											}
										}
									}

									// Assign the layer processor and options to the input
									FLayerData Layer;
									Layer.LayerType = Track->LayerType;
									Layer.Role = Track->Role;
									Layer.Processor = LayerProcessor;
									Layer.ProcessorOptions = LayerOptions;
									//Layers.Add(MoveTemp(Layer));
									Input.InputLayers.Add(MoveTemp(Layer));
								}
							}
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

			// Start a new capture pass for each layer
			for (auto& Layer : Input.InputLayers) {
				if (Layer.Processor) {			
					//Copy the layer to avoid modifying the original layer array
					FLayerData TargetLayer = Layer;
					
					// Prepare rendering the layer
					TSharedPtr<FSceneViewFamilyContext> ViewFamily;
					FSceneView* View = BeginSDLayerPass(InOutSampleState, ViewFamily);
					TargetLayer.Processor->BeginCaptureLayer(FIntPoint(Input.Options.OutSizeX, Input.Options.OutSizeY), nullptr, TargetLayer.ProcessorOptions);

					// Set up post processing material from layer processor
					View->FinalPostProcessSettings.AddBlendable(TargetLayer.Processor->GetActivePostMaterial(), 1.0f);
					IBlendableInterface* BlendableInterface = Cast<IBlendableInterface>(TargetLayer.Processor->GetActivePostMaterial());
					if (BlendableInterface) {
						ViewFamily->EngineShowFlags.SetPostProcessMaterial(true);
						BlendableInterface->OverrideBlendableSettings(*View, 1.f);
					}
					ViewFamily->EngineShowFlags.SetPostProcessing(true);
					View->FinalPostProcessSettings.bBufferVisualizationDumpRequired = true;

					// Render the layer
					GetRendererModule().BeginRenderingViewFamily(&Canvas, ViewFamily.Get());
					RenderTarget->ReadPixels(TargetLayer.LayerPixels, FReadSurfaceDataFlags());

					// Cleanup before move
					View->FinalPostProcessSettings.RemoveBlendable(TargetLayer.Processor->PostMaterial);
					TargetLayer.Processor->EndCaptureLayer();

					Input.ProcessedLayers.Add(MoveTemp(TargetLayer));
				}
			}

			if (OptionsTrack) {
				for (auto Section : OptionsTrack->Sections) {
					if (Section) {
						auto OptionSection = Cast<UStableDiffusionOptionsSection>(Section);
						if (OptionSection) {
							//Reload model if it doesn't match the current options track
							if (OptionSection->ModelAsset) {
								if (SDSubsystem->ModelOptions != OptionSection->ModelAsset->Options || SDSubsystem->GetModelStatus().ModelStatus != EModelStatus::Loaded) {
									SDSubsystem->InitModel(OptionSection->ModelAsset->Options, OptionSection->PipelineAsset->Options, Input.ProcessedLayers, false, AllowNSFW, EPaddingMode::zeros);
								}
							}
							if (SDSubsystem->GetModelStatus().ModelStatus != EModelStatus::Loaded) {
								UE_LOG(LogTemp, Error, TEXT("No model asset provided in Stable Diffusion Options section and no model loaded in StableDiffusionSubsystem. Please add a model asset to the Options track or initialize the StableDiffusionSubsystem model."))
							}
							// Evaluate curve values
							OptionSection->GetStrengthChannel().Evaluate(FullFrameTime, Input.Options.Strength);
							OptionSection->GetIterationsChannel().Evaluate(FullFrameTime, Input.Options.Iterations);
							OptionSection->GetSeedChannel().Evaluate(FullFrameTime, Input.Options.Seed);
						}
					}
				}
			}
			
			// Generate new SD frame immediately
			UTexture2D* OutTexture = UTexture2D::CreateTransient(Input.Options.OutSizeX, Input.Options.OutSizeY);
			auto SDResult = SDSubsystem->GeneratorBridge->GenerateImageFromStartImage(Input, OutTexture, nullptr);
			
			TUniquePtr<FImagePixelData> SDImageDataBuffer16bit;
			if (!IsValid(SDResult.OutTexture)) {
				UE_LOG(LogTemp, Error, TEXT("Stable diffusion generator failed to return any pixel data on frame %d. Please add a model asset to the Options track or initialize the StableDiffusionSubsystem model."), EffectiveFrame.Value);
						
				// Insert blank frame
				TArray<FColor> EmptyPixels;
				EmptyPixels.InsertUninitialized(0, Input.Options.OutSizeX * Input.Options.OutSizeY);
				TUniquePtr<TImagePixelData<FColor>> SDImageDataBuffer8bit = MakeUnique<TImagePixelData<FColor>>(FIntPoint(Input.Options.OutSizeX, Input.Options.OutSizeY), TArray64<FColor>(MoveTemp(EmptyPixels)));
				SDImageDataBuffer16bit = UE::MoviePipeline::QuantizeImagePixelDataToBitDepth(SDImageDataBuffer8bit.Get(), 16);
			}
			else {
				UStableDiffusionBlueprintLibrary::UpdateTextureSync(OutTexture);
				TArray<FColor> Pixels = UStableDiffusionBlueprintLibrary::ReadPixels(OutTexture);

				// Convert 8bit BGRA FColors returned from SD to 16bit BGRA
				TUniquePtr<TImagePixelData<FColor>> SDImageDataBuffer8bit;
				SDImageDataBuffer8bit = MakeUnique<TImagePixelData<FColor>>(FIntPoint(SDResult.OutWidth, SDResult.OutHeight), TArray64<FColor>(MoveTemp(Pixels)));
				SDImageDataBuffer16bit = UE::MoviePipeline::QuantizeImagePixelDataToBitDepth(SDImageDataBuffer8bit.Get(), 16);
			}

			ENQUEUE_RENDER_COMMAND(UpdateMoviePipelineRenderTarget)([this, Buffer=MoveTemp(SDImageDataBuffer16bit), RenderTarget](FRHICommandListImmediate& RHICmdList) {
				int64 OutSize;
				const void* OutRawData = nullptr;
				Buffer->GetRawData(OutRawData, OutSize);
				RHICmdList.UpdateTexture2D(
					RenderTarget->GetRenderTargetTexture(),
					0,
					FUpdateTextureRegion2D(0, 0, 0, 0, RenderTarget->GetSizeXY().X, RenderTarget->GetSizeXY().Y),
					RenderTarget->GetSizeXY().X * sizeof(FFloat16Color),
					(uint8*)OutRawData
				);
				RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThread);
			});

			// Readback + Accumulate
			PostRendererSubmission(InSampleState, LayerPassIdentifier, GetOutputFileSortingOrder() + 1, Canvas);
		}
#endif
	}
}

FSceneView* UStableDiffusionMoviePipeline::BeginSDLayerPass(FMoviePipelineRenderPassMetrics& InOutSampleState, TSharedPtr<FSceneViewFamilyContext>& ViewFamily)
{
	// Get new view state for our stencil render
	FStableDiffusionDeferredPassRenderStatePayload RenderState;
	RenderState.CameraIndex = 0;
	RenderState.TileIndex = InOutSampleState.TileIndexes;
	RenderState.SceneViewIndex = 0;
	ViewFamily = CalculateViewFamily(InOutSampleState, &RenderState);

	ViewFamily->bIsMultipleViewFamily = true;
	ViewFamily->EngineShowFlags.PostProcessing = 1;
	ViewFamily->EngineShowFlags.SetPostProcessMaterial(true);
	ViewFamily->EngineShowFlags.SetPostProcessing(true);
	
	FSceneView* View = const_cast<FSceneView*>(ViewFamily->Views[0]);
	View->FinalPostProcessSettings.bBufferVisualizationDumpRequired = true;
	return View;
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
				if (!IsValid(Image)) {
					continue;
				}
				UStableDiffusionBlueprintLibrary::UpdateTextureSync(Image);

				// Read half-float pixels from source texture
				FFloat16Color* MipData = reinterpret_cast<FFloat16Color*>(Image->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_ONLY));
				TArrayView64<FFloat16Color> SourceColors(MipData, Image->GetSizeX() * Image->GetSizeY());

				// Convert pixels from FFloat16Color to FColor
				TArray<FColor> QuantitizedPixelData;
				QuantitizedPixelData.InsertUninitialized(0, SourceColors.Num());
				for (int idx = 0; idx < SourceColors.Num(); ++idx) {
					QuantitizedPixelData[idx] = SourceColors[idx].GetFloats().ToFColor(true);
				}

				// Unlock source texture since we've converted the pixel data
				Image->GetPlatformData()->Mips[0].BulkData.Unlock();
				
				// Build our upsample parameters
				FStableDiffusionImageResult UpsampleInput;
				UpsampleInput.OutWidth = Image->GetSizeX();
				UpsampleInput.OutHeight = Image->GetSizeY();
				UpsampleInput.Upsampled = false;
				UpsampleInput.Completed = false;
				UpsampleInput.OutTexture = UStableDiffusionBlueprintLibrary::ColorBufferToTexture(QuantitizedPixelData, FIntPoint(Image->GetSizeX(), Image->GetSizeY()), nullptr, true);
				UStableDiffusionBlueprintLibrary::UpdateTextureSync(UpsampleInput.OutTexture);
				
				// Create a destination texture that is 4x times larger than the input to hold the upsample result
				// TODO: Allow for arbitary resize factors 
				UTexture2D* UpsampledTexture = UTexture2D::CreateTransient(UpsampleInput.OutTexture->GetSizeX() * 4, UpsampleInput.OutTexture->GetSizeY() * 4);
				FStableDiffusionImageResult UpsampleResult = SDSubsystem->GeneratorBridge->UpsampleImage(UpsampleInput, UpsampledTexture);

				if (IsValid(UpsampleResult.OutTexture)) {
					UStableDiffusionBlueprintLibrary::UpdateTextureSync(UpsampleResult.OutTexture);

					// Build an export task that will async write the upsampled image to disk
					TUniquePtr<FImageWriteTask> ExportTask = MakeUnique<FImageWriteTask>();
					ExportTask->Format = EImageFormat::EXR;
					ExportTask->CompressionQuality = (int32)EImageCompressionQuality::Default;
					FString OutputName = FString::Printf(TEXT("%s%s"), *UpscaledFramePrefix, *FPaths::GetBaseFilename(file));
					FString OutputDirectory = OutputSettings->OutputDirectory.Path;
					FString OutputPath = FPaths::Combine(OutputDirectory, OutputName);
					FString OutputPathResolved;

					TMap<FString, FString> FormatOverrides{ {TEXT("ext"), *FPaths::GetExtension(file)} };
					FMoviePipelineFormatArgs OutArgs;
					GetPipeline()->ResolveFilenameFormatArguments(OutputPath, FormatOverrides, OutputPathResolved, OutArgs);
					ExportTask->Filename = OutputPathResolved;

					// Convert RGBA pixels back to FloatRGBA
					TArray<FColor> SrcPixels = UStableDiffusionBlueprintLibrary::ReadPixels(UpsampleResult.OutTexture);
					TArray64<FFloat16Color> ConvertedSrcPixels;
					ConvertedSrcPixels.InsertUninitialized(0, SrcPixels.Num());
					for (size_t idx = 0; idx < SrcPixels.Num(); ++idx) {
						ConvertedSrcPixels[idx] = FFloat16Color(SrcPixels[idx]);
					}
					TUniquePtr<TImagePixelData<FFloat16Color>> UpscaledPixelData = MakeUnique<TImagePixelData<FFloat16Color>>(
						FIntPoint(UpsampleResult.OutWidth, UpsampleResult.OutHeight),
						MoveTemp(ConvertedSrcPixels)
						);
					ExportTask->PixelData = MoveTemp(UpscaledPixelData);
					
					// Enqueue image write
					GetPipeline()->ImageWriteQueue->Enqueue(MoveTemp(ExportTask));
				}
			}

			SDSubsystem->GeneratorBridge->StopUpsample();
		}
	}
}
