#pragma once

#include "CoreMinimal.h"
#include "Sections/MovieSceneParameterSection.h"
#include "LayerProcessorBase.h"
#include "StableDiffusionLayerProcessorSection.generated.h"


UCLASS()
class STABLEDIFFUSIONSEQUENCER_API UStableDiffusionLayerProcessorSection : public UMovieSceneParameterSection
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Instanced, Category = "Layers")
	TObjectPtr<ULayerProcessorOptions> LayerProcessorOptionOverride;
};
