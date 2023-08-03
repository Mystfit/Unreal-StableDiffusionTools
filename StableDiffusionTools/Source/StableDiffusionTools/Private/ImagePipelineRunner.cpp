// Fill out your copyright notice in the Description page of Project Settings.


#include "ImagePipelineRunner.h"
#include "Editor.h"
#include "StableDiffusionSubsystem.h"


UImagePipelineRunner::UImagePipelineRunner(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		AddToRoot();
	}
}

UImagePipelineRunner* UImagePipelineRunner::RunImagePipeline(TArray<UImagePipelineStageAsset*> Stages, FStableDiffusionInput Input, EInputImageSource ImageSourceType, bool AllowNSFW, EPaddingMode PaddingMode)
{
	UImagePipelineRunner* ImagePipelineRunner = NewObject<UImagePipelineRunner>();
	ImagePipelineRunner->Stages = Stages;
	ImagePipelineRunner->Input = Input;
	ImagePipelineRunner->ImageSourceType = ImageSourceType;
	ImagePipelineRunner->AllowNSFW = AllowNSFW;
	ImagePipelineRunner->PaddingMode = PaddingMode;
	return ImagePipelineRunner;
}

void UImagePipelineRunner::Activate()
{
	FStableDiffusionImageResult LastStageResult;

	if (!Stages.Num()) {
		UE_LOG(LogTemp, Warning, TEXT("Image pipeline runner was not provided any stages. Exiting early."));
		Complete(LastStageResult);
	}

	// Gather custom schedulers from pipelines
	TArray<TObjectPtr<UStableDiffusionPipelineAsset>> TempPipelines;
	for (auto Stage : Stages) {
		TObjectPtr<UStableDiffusionPipelineAsset> Pipeline = Stage->Pipeline;
		
		// Duplicate the pipelineasset and take ownership of it so we can modify it with our scheduler override
		// Needs to happen before the async thread is run
		if (!Stage->Scheduler.IsEmpty()) {
			Pipeline = DuplicateObject<UStableDiffusionPipelineAsset>(Stage->Pipeline, this);
			Pipeline->Options.Scheduler = Stage->Scheduler;
		}

		TempPipelines.Add(Pipeline);
	}

	AsyncTask(ENamedThreads::AnyHiPriThreadHiPriTask, [this, LastStageResult, TempPipelines]() mutable {
		if (UStableDiffusionSubsystem* Subsystem = GEditor->GetEditorSubsystem<UStableDiffusionSubsystem>()) {
			for (size_t StageIdx = 0; StageIdx < Stages.Num(); ++StageIdx) {
				if (Subsystem->IsStopping()) {
					break;
				}

				// In order to process the pipeline, we need to use both the previous and current stages
				UImagePipelineStageAsset* PrevStage = (StageIdx) ? Stages[StageIdx - 1] : nullptr;
				UImagePipelineStageAsset* CurrentStage = Stages[StageIdx];
				TObjectPtr<UStableDiffusionPipelineAsset> TempPipelineAsset = TempPipelines[StageIdx];

				// Init model at the start of each stage.
				// TODO: Cache last model and only re-init if model options have changed
				Subsystem->InitModel(CurrentStage->Model->Options, TempPipelineAsset, CurrentStage->LORAAsset, CurrentStage->TextualInversionAsset, CurrentStage->Layers, false, AllowNSFW, PaddingMode);
				if (Subsystem->GetModelStatus().ModelStatus != EModelStatus::Loaded) {
					UE_LOG(LogTemp, Error, TEXT("Failed to load model. Check the output log for more information"));
					Subsystem->StopGeneratingImage();
					if(Subsystem->GeneratorBridge){
						Subsystem->GeneratorBridge->ModelStatus.ErrorMsg = "Failed to load the specified model";
						Subsystem->GeneratorBridge->ModelStatus.ModelStatus = EModelStatus::Error;
					}
					
					break;
				}

				// We may have cancelled the model load
				if (Subsystem->IsStopping()) {
					UE_LOG(LogTemp, Error, TEXT("Stopping image pipeline after modoel init"));
					break;
				}

				// Optionally override global generation options with per-stage options
				Input.Options.GuidanceScale = (CurrentStage->OverrideInputOptions.OverrideGuidanceScale) ? CurrentStage->OverrideInputOptions.GuidanceScale : Input.Options.GuidanceScale;
				Input.Options.Iterations = (CurrentStage->OverrideInputOptions.OverrideIterations) ? CurrentStage->OverrideInputOptions.Iterations : Input.Options.Iterations;
				Input.Options.LoraWeight = (CurrentStage->OverrideInputOptions.OverrideLoraWeight) ? CurrentStage->OverrideInputOptions.LoraWeight : Input.Options.LoraWeight;
				Input.Options.NegativePrompts = (CurrentStage->OverrideInputOptions.OverrideNegativePrompts) ? CurrentStage->OverrideInputOptions.NegativePrompts : Input.Options.NegativePrompts;
				Input.Options.PositivePrompts = (CurrentStage->OverrideInputOptions.OverridePositivePrompts) ? CurrentStage->OverrideInputOptions.PositivePrompts : Input.Options.PositivePrompts;
				Input.Options.OutSizeX = (CurrentStage->OverrideInputOptions.OverrideOutSizeX) ? CurrentStage->OverrideInputOptions.OutSizeX : Input.Options.OutSizeX;
				Input.Options.OutSizeY = (CurrentStage->OverrideInputOptions.OverrideOutSizeY) ? CurrentStage->OverrideInputOptions.OutSizeY : Input.Options.OutSizeY;
				Input.Options.Seed = (CurrentStage->OverrideInputOptions.OverrideSeed) ? CurrentStage->OverrideInputOptions.Seed : Input.Options.Seed;
				Input.Options.Strength = (CurrentStage->OverrideInputOptions.OverrideStrength) ? CurrentStage->OverrideInputOptions.Strength : Input.Options.Strength;

				// Copy layers and output type from the stage
				Input.InputLayers = CurrentStage->Layers;
				Input.OutputType = CurrentStage->OutputType;
				Input.Options.Seed = (Input.Options.RandomSeed) ? FMath::Rand() : Input.Options.Seed;

				// Use last image result as input for next stage's layers
				if (LastStageResult.Completed) {
					for (auto& Layer : Input.InputLayers) {
						if (Layer.OutputType == EImageType::Latent) {
							Layer.LatentData = LastStageResult.OutLatent;
						}
					}
				}

				// Generate the image
				LastStageResult = Subsystem->GenerateImageSync(Input, ImageSourceType);

				// Broadcast on game thread
				if (StageIdx < Stages.Num() - 1) {
					AsyncTask(ENamedThreads::GameThread, [this, LastStageResult]() {
						OnStageCompleted.Broadcast(LastStageResult);
					});
				}

				// We may have cancelled the image generation
				if (Subsystem->IsStopping()) {
					UE_LOG(LogTemp, Error, TEXT("Stopping image pipeline after image generation"));
					break;
				}

				// Handle errors
				if (LastStageResult.Completed) {
					UE_LOG(LogTemp, Log, TEXT("Completed pipeline stage %d"), StageIdx);
				}
				else {
					UE_LOG(LogTemp, Error, TEXT("Failed to generate image for pipeline stage. Check the output log for more information"));
					Subsystem->StopGeneratingImage();
					break;
				}
			}

			// Reset stopped flag if we aborted early
			Subsystem->ClearIsStopping();
		}

		// Broadcast last pipeline result
		Complete(LastStageResult);
	});
}


void UImagePipelineRunner::Complete(FStableDiffusionImageResult& Result)
{
#if !UE_SERVER
	RemoveFromRoot();
	AsyncTask(ENamedThreads::GameThread, [this, Result=MoveTemp(Result)]() {
		OnAllStagesCompleted.Broadcast(Result);
	});
#endif

}