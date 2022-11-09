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
	UPROPERTY(EditAnywhere, Category = "StableDiffusion", meta = (Multiline = true))
	FPrompt Prompt;

	const FMovieSceneFloatChannel& GetWeightChannel() const { return Weight; }

	UPROPERTY(EditAnywhere, Category = "StableDiffusion|Curves")
	FMovieSceneFloatChannel Weight;

private:
	virtual EMovieSceneChannelProxyType CacheChannelProxy() override;
};
