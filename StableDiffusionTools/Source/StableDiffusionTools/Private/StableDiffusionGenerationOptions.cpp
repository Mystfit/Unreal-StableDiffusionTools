#include "StableDiffusionGenerationOptions.h"
#include "StableDiffusionSubsystem.h"

void UImagePipelineStageAsset::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

TArray<FString> UImagePipelineStageAsset::GetCompatibleSchedulers()
{
	if (IsValid(Model)) {
		if (auto subsystem = GEditor->GetEditorSubsystem<UStableDiffusionSubsystem>()) {
			if (auto bridge = subsystem->GeneratorBridge) {
				if (bridge->ModelStatus.ModelStatus != EModelStatus::Loading &&
					bridge->ModelStatus.ModelStatus != EModelStatus::Downloading && 
					Model->Options.Model != LastQueriedModel
				) {
					LastQueriedModel = Model->Options.Model;
					LoadModel();
				}
					
				if (bridge->ModelStatus.ModelStatus == EModelStatus::Loaded) {
					CompatibleSchedulers = bridge->AvailableSchedulers();
				}
				else {
					CompatibleSchedulers.Reset();
					CompatibleSchedulers.Add("Model loading...");
				}
			}
		}
	}
	
	return CompatibleSchedulers;
}

UImagePipelineStageAsset::UImagePipelineStageAsset()
{
}

void UImagePipelineStageAsset::LoadModel()
{
	if (IsValid(Model)) {
		if (auto subsystem = GEditor->GetEditorSubsystem<UStableDiffusionSubsystem>()) {
			subsystem->InitModel(Model->Options, Pipeline, nullptr, nullptr, TArray<FLayerProcessorContext>(), true, true, EPaddingMode::zeros);
		}
	}
}
