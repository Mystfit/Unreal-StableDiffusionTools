#pragma once

#include "CoreMinimal.h"
#include "StableDiffusionGenerationOptions.h"
#include "MovieSceneSection.h"
#include "StableDiffusionGenerationOptions.h"
#include "Channels/MovieSceneFloatChannel.h"
#include "Channels/MovieSceneIntegerChannel.h"
#include "StableDiffusionOptionsSection.generated.h"


UCLASS()
class STABLEDIFFUSIONSEQUENCER_API UStableDiffusionOptionsSection : public UMovieSceneSection
{
	GENERATED_BODY()
	UStableDiffusionOptionsSection(const FObjectInitializer& ObjectInitializer);
public:
	UPROPERTY(EditAnywhere, Category = "StableDiffusion|Model", meta = (Multiline = true))
	UStableDiffusionModelAsset* ModelAsset;

	UPROPERTY(EditAnywhere, Category = "StableDiffusion|Pipeline", meta = (Multiline = true))
	UStableDiffusionPipelineAsset* PipelineAsset;

	UPROPERTY(EditAnywhere, Category = "StableDiffusion|Pipeline", meta = (Multiline = true))
	FString SchedulerOverride;

	UPROPERTY(EditAnywhere, Category = "StableDiffusion|LORA", meta = (Multiline = true))
	UStableDiffusionLORAAsset* LORAAsset;

	/**
	 * Public access to this section's internal data function
	 */
	const FMovieSceneFloatChannel& GetStrengthChannel() const { return StrengthCurve; }
	const FMovieSceneIntegerChannel& GetIterationsChannel() const { return IterationsCurve; }
	const FMovieSceneIntegerChannel& GetSeedChannel() const { return SeedCurve; }


	UPROPERTY(EditAnywhere, Category = "StableDiffusion|Curves")
	FMovieSceneFloatChannel StrengthCurve;

	UPROPERTY(EditAnywhere, Category = "StableDiffusion|Curves")
	FMovieSceneIntegerChannel IterationsCurve;

	UPROPERTY(EditAnywhere, Category = "StableDiffusion|Curves")
	FMovieSceneIntegerChannel SeedCurve;
};
