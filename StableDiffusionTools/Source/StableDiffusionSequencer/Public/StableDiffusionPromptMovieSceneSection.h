#pragma once

#include "CoreMinimal.h"
#include "PromptAsset.h"
#include "MovieSceneSection.h"
#include "Channels/MovieSceneFloatChannel.h"
#include "Channels/MovieSceneIntegerChannel.h"
#include "StableDiffusionPromptMovieSceneSection.generated.h"


UCLASS()
class STABLEDIFFUSIONSEQUENCER_API UStableDiffusionPromptMovieSceneSection : public UMovieSceneSection
{
	GENERATED_BODY()
	UStableDiffusionPromptMovieSceneSection(const FObjectInitializer& ObjectInitializer);

public:

	UPROPERTY(EditAnywhere, meta = (Multiline = true, ShowOnlyInnerProperties))
	FPrompt Prompt;

	/**
	 * Public access to this section's internal data function
	 */
	const FMovieSceneFloatChannel& GetWeightChannel() const { return Weight; }
	const FMovieSceneIntegerChannel& GetRepeatsChannel() const { return Repeats; }

protected:

	UPROPERTY()
	FMovieSceneFloatChannel Weight;

	UPROPERTY()
	FMovieSceneIntegerChannel Repeats;

private:

	virtual EMovieSceneChannelProxyType CacheChannelProxy() override;
};
