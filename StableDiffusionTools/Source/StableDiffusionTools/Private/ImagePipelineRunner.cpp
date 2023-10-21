// Fill out your copyright notice in the Description page of Project Settings.


#include "ImagePipelineRunner.h"
#include "Editor.h"
#include "StableDiffusionSubsystem.h"
#include "Async/Async.h"
#include "AssetRegistry/AssetRegistryModule.h"


UImagePipelineRunner::UImagePipelineRunner(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		AddToRoot();
	}
}

UImagePipelineRunner* UImagePipelineRunner::RunImagePipeline(UStableDiffusionImageResultAsset* OutAsset, TArray<UImagePipelineStageAsset*> Stages, FStableDiffusionInput Input, EInputImageSource ImageSourceType, bool AllowNSFW, int32 SeamlessMode, bool SaveLayers)
{
	UImagePipelineRunner* ImagePipelineRunner = NewObject<UImagePipelineRunner>();
	ImagePipelineRunner->Stages = Stages;
	ImagePipelineRunner->Input = Input;
	ImagePipelineRunner->ImageSourceType = ImageSourceType;
	ImagePipelineRunner->AllowNSFW = AllowNSFW;
	ImagePipelineRunner->SeamlessMode = SeamlessMode;
	ImagePipelineRunner->OutAsset = OutAsset;
	ImagePipelineRunner->SaveLayers = SaveLayers;
	return ImagePipelineRunner;
}

void UImagePipelineRunner::Activate()
{
	FStableDiffusionPipelineImageResult LastAssetStageResult;

	if (!Stages.Num()) {
		UE_LOG(LogTemp, Warning, TEXT("Image pipeline runner was not provided any stages. Exiting early."));
		Complete(LastAssetStageResult, TArray<UImagePipelineStageAsset*>());
	}

	// Gather custom schedulers from pipelines
	TArray<TObjectPtr<UImagePipelineStageAsset>> TempStages;
	for (auto Stage : Stages) {
		if (!IsValid(Stage))
			continue;
		
		// Duplicate the pipeline stage and take ownership of it and save it later
		// Needs to happen before the async thread is run
		auto TempStage = DuplicateObject<UImagePipelineStageAsset>(Stage, OutAsset);
		TempStage->ClearFlags(RF_Transient);
		TempStage->SetFlags(RF_Public | RF_Standalone);
		TempStage->AddToRoot();
		TempStages.Add(TempStage);
	}

	AsyncTask(ENamedThreads::AnyHiPriThreadHiPriTask, [this, LastAssetStageResult, TempStages=MoveTemp(TempStages)]() mutable {
		if (UStableDiffusionSubsystem* Subsystem = GEditor->GetEditorSubsystem<UStableDiffusionSubsystem>()) {
			for (size_t StageIdx = 0; StageIdx < TempStages.Num(); ++StageIdx) {
				if (Subsystem->IsStopping()) {
					break;
				}

				// Save geneartion options in asset
				OutAsset->GenerationInputs = Input;

				// In order to process the pipeline, we need to use both the previous and current stages
				UImagePipelineStageAsset* PrevStage = (StageIdx) ? TempStages[StageIdx - 1] : nullptr;
				UImagePipelineStageAsset* CurrentStage = TempStages[StageIdx];
				auto CurrentImageSource = ImageSourceType;

				if (!IsValid(CurrentStage)) {
					break;
				}

				// Init model at the start of each stage.
				// TODO: Cache last model and only re-init if model options have changed
				Subsystem->InitModel(CurrentStage->Model->Options, CurrentStage->Pipeline, CurrentStage->LORAAsset, CurrentStage->TextualInversionAsset, CurrentStage->Layers, false, AllowNSFW, SeamlessMode);
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
				Input.SaveLayers = SaveLayers;
				Input.Options.GuidanceScale = (CurrentStage->OverrideInputOptions.OverrideGuidanceScale) ? CurrentStage->OverrideInputOptions.GuidanceScale : Input.Options.GuidanceScale;
				Input.Options.Iterations = (CurrentStage->OverrideInputOptions.OverrideIterations) ? CurrentStage->OverrideInputOptions.Iterations : Input.Options.Iterations;
				Input.Options.LoraWeight = (CurrentStage->OverrideInputOptions.OverrideLoraWeight) ? CurrentStage->OverrideInputOptions.LoraWeight : Input.Options.LoraWeight;
				Input.Options.NegativePrompts = (CurrentStage->OverrideInputOptions.OverrideNegativePrompts) ? CurrentStage->OverrideInputOptions.NegativePrompts : Input.Options.NegativePrompts;
				Input.Options.PositivePrompts = (CurrentStage->OverrideInputOptions.OverridePositivePrompts) ? CurrentStage->OverrideInputOptions.PositivePrompts : Input.Options.PositivePrompts;
				Input.Options.OutSizeX = (CurrentStage->OverrideInputOptions.OverrideOutSizeX) ? CurrentStage->OverrideInputOptions.OutSizeX : Input.Options.OutSizeX;
				Input.Options.OutSizeY = (CurrentStage->OverrideInputOptions.OverrideOutSizeY) ? CurrentStage->OverrideInputOptions.OutSizeY : Input.Options.OutSizeY;
				Input.Options.Seed = (CurrentStage->OverrideInputOptions.OverrideSeed) ? CurrentStage->OverrideInputOptions.Seed : Input.Options.Seed;
				Input.Options.Strength = (CurrentStage->OverrideInputOptions.OverrideStrength) ? CurrentStage->OverrideInputOptions.Strength : Input.Options.Strength;
				
				// Always use the seed and output type from the stage
				Input.OutputType = CurrentStage->OutputType;
				Input.Options.Seed = (Input.Options.RandomSeed) ? FMath::Rand() : Input.Options.Seed;

				// Use last image result as input for next stage's layers
				if (LastAssetStageResult.ImageOutput.Completed) {
					for (auto& Layer : CurrentStage->Layers) {
						if (Layer.OutputType == EImageType::Latent) {
							Layer.LatentData = LastAssetStageResult.ImageOutput.OutLatent;
						}
					}

					// Pass along last generated texture
					Input.OverrideTextureInput = LastAssetStageResult.ImageOutput.OutTexture;
					CurrentImageSource = EInputImageSource::Texture;
				}

				// Generate the image
				LastAssetStageResult.ImageOutput = Subsystem->GenerateImageSync(Input, CurrentStage, CurrentImageSource);
				
				// Move ownership of texture and stage to the output asset
				if (IsValid(CurrentStage) && IsValid(LastAssetStageResult.ImageOutput.OutTexture)) {
					TSharedPtr<TPromise<bool>> GameThreadPromise = MakeShared<TPromise<bool>>();
					AsyncTask(ENamedThreads::GameThread, [this, LastAssetStageResult, CurrentStage, GameThreadPromise]() {
						LastAssetStageResult.ImageOutput.OutTexture->Rename(nullptr, OutAsset);
						LastAssetStageResult.ImageOutput.OutTexture->ClearFlags(RF_Transient);
						LastAssetStageResult.ImageOutput.OutTexture->SetFlags(RF_Public | RF_Standalone);
						LastAssetStageResult.ImageOutput.OutTexture->PostEditChange();

						CurrentStage->RemoveFromRoot();
						GameThreadPromise->SetValue(true);
					});
					GameThreadPromise->GetFuture().Wait();
				}

				// Store the result in the output asset
				LastAssetStageResult.PipelineStage = CurrentStage;
				OutAsset->StageResults.Add(LastAssetStageResult);

				// Mark package dirty early
				OutAsset->MarkPackageDirty();
				OutAsset->PostEditChange();

				// Broadcast on game thread
				if (StageIdx < TempStages.Num() - 1) {
					AsyncTask(ENamedThreads::GameThread, [this, LastAssetStageResult]() {
						OnStageCompleted.Broadcast(LastAssetStageResult, OutAsset);
					});
				}

				// We may have cancelled the image generation
				if (Subsystem->IsStopping()) {
					UE_LOG(LogTemp, Error, TEXT("Stopping image pipeline after image generation"));
					break;
				}

				// Handle errors
				if (LastAssetStageResult.ImageOutput.Completed) {
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
		Complete(LastAssetStageResult, TempStages);
	});
}


void UImagePipelineRunner::Complete(FStableDiffusionPipelineImageResult& Result, TArray<UImagePipelineStageAsset*> Pipeline)
{
#if !UE_SERVER
	RemoveFromRoot();
	AsyncTask(ENamedThreads::GameThread, [this, Pipeline=MoveTemp(Pipeline), Result=MoveTemp(Result)]() {

		// Move generated objects into the data asset
		if (IsValid(OutAsset)) {
			// Change pipeline stage outers to the output asset
			for (auto& StageResult : OutAsset->StageResults)
			{
				
			}
		}

		OnAllStagesCompleted.Broadcast(Result, OutAsset);
	});
#endif

}