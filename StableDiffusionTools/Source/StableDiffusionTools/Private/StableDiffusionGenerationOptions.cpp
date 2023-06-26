#include "StableDiffusionGenerationOptions.h"
#include "StableDiffusionSubsystem.h"

void UStableDiffusionPipelineAsset::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	CachedSchedulers = false;
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

TArray<FString> UStableDiffusionPipelineAsset::GetCompatibleSchedulers()
{
	if (!CachedSchedulers) {
		if (auto subsystem = GEditor->GetEditorSubsystem<UStableDiffusionSubsystem>()) {
			if (auto bridge = subsystem->GeneratorBridge) {
				if (bridge->ModelStatus.ModelStatus == EModelStatus::Loaded) {
					CompatibleSchedulers = bridge->AvailableSchedulers();
					CachedSchedulers = true;
				}
			}
		}
	}
	return CompatibleSchedulers;
}
