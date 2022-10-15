#pragma once

#include "CoreMinimal.h"
#include "PromptAsset.h"
#include "MovieSceneSection.h"
#include "Channels/MovieSceneFloatChannel.h"
#include "StableDiffusionPromptMovieSceneSection.generated.h"


UCLASS()
class STABLEDIFFUSIONSEQUENCER_API UStableDiffusionPromptMovieSceneSection : public UMovieSceneSection
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, meta = (Multiline = true, ShowOnlyInnerProperties))
	FPrompt Prompt;

	/**
	 * Public access to this section's internal data function
	 */
	const FMovieSceneFloatChannel& GetPromptWeightChannel() const { return PromptWeight; }

protected:

	/** Float data */
	UPROPERTY()
	FMovieSceneFloatChannel PromptWeight;

private:

	virtual EMovieSceneChannelProxyType CacheChannelProxy() override;
};
