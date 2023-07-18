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

	AsyncTask(ENamedThreads::AnyHiPriThreadHiPriTask, [this, LastStageResult]() mutable {
		if (UStableDiffusionSubsystem* Subsystem = GEditor->GetEditorSubsystem<UStableDiffusionSubsystem>()) {
			for (size_t StageIdx = 0; StageIdx < Stages.Num(); ++StageIdx) {
				UImagePipelineStageAsset* PrevStage = (StageIdx) ? Stages[StageIdx - 1] : nullptr;
				UImagePipelineStageAsset* CurrentStage = Stages[StageIdx];

				Subsystem->InitModel(CurrentStage->Model->Options, CurrentStage->Pipeline, CurrentStage->LORAAsset, CurrentStage->TextualInversionAsset, CurrentStage->Layers, false, AllowNSFW, PaddingMode);
				if (Subsystem->GetModelStatus().ModelStatus != EModelStatus::Loaded) {
					UE_LOG(LogTemp, Error, TEXT("Failed to load model. Check the output log for more information"));
					Complete(LastStageResult);
					return;
				}

				if (Input.Options.AllowOverrides) {
					Input.Options.GuidanceScale = (CurrentStage->OverrideInputOptions.OverrideGuidanceScale) ? CurrentStage->OverrideInputOptions.GuidanceScale : Input.Options.GuidanceScale;
					Input.Options.Iterations = (CurrentStage->OverrideInputOptions.OverrideIterations) ? CurrentStage->OverrideInputOptions.Iterations : Input.Options.Iterations;
					Input.Options.LoraWeight = (CurrentStage->OverrideInputOptions.OverrideLoraWeight) ? CurrentStage->OverrideInputOptions.LoraWeight : Input.Options.LoraWeight;
					Input.Options.NegativePrompts = (CurrentStage->OverrideInputOptions.OverrideNegativePrompts) ? CurrentStage->OverrideInputOptions.NegativePrompts : Input.Options.NegativePrompts;
					Input.Options.PositivePrompts = (CurrentStage->OverrideInputOptions.OverridePositivePrompts) ? CurrentStage->OverrideInputOptions.PositivePrompts : Input.Options.PositivePrompts;
					Input.Options.OutSizeX = (CurrentStage->OverrideInputOptions.OverrideOutSizeX) ? CurrentStage->OverrideInputOptions.OutSizeX : Input.Options.OutSizeX;
					Input.Options.OutSizeY = (CurrentStage->OverrideInputOptions.OverrideOutSizeY) ? CurrentStage->OverrideInputOptions.OutSizeY : Input.Options.OutSizeY;
					Input.Options.Seed = (CurrentStage->OverrideInputOptions.OverrideSeed) ? CurrentStage->OverrideInputOptions.Seed : Input.Options.Seed;
					Input.Options.Strength = (CurrentStage->OverrideInputOptions.OverrideStrength) ? CurrentStage->OverrideInputOptions.Strength : Input.Options.Strength;
				}

				// Set input settings from current stage
				Input.InputLayers = CurrentStage->Layers;
				Input.OutputType = CurrentStage->OutputType;
				Input.Options.Seed = (Input.Options.Seed < 0) ? FMath::Rand() : Input.Options.Seed;

				if (LastStageResult.Completed) {
					// Use last image result as input for next stage's layers
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
				
			}
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