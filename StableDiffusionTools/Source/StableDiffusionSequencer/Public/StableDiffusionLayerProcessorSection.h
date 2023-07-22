#pragma once

#include "CoreMinimal.h"
#include "Sections/MovieSceneParameterSection.h"
#include "LayerProcessorBase.h"
#include "StableDiffusionGenerationOptions.h"
#include "StableDiffusionLayerProcessorSection.generated.h"


UCLASS()
class STABLEDIFFUSIONSEQUENCER_API UStableDiffusionLayerProcessorSection : public UMovieSceneParameterSection
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced, Category = "Layers")
		TObjectPtr<ULayerProcessorOptions> LayerProcessorOptionOverride;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (Category = "Layers", Description = "The index of the image pipeline stage this track section will control"))
		int ImagePipelineStageIndex;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(Category = "Layers", Description = "The index of the layer inside the stage this track section will control"))
		int LayerIndex;
};
