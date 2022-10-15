#include "StableDiffusionPromptMovieSceneSection.h"
#include "Channels/MovieSceneChannelProxy.h"

EMovieSceneChannelProxyType UStableDiffusionPromptMovieSceneSection::CacheChannelProxy()
{
	FMovieSceneChannelProxyData Channels;

#if WITH_EDITOR
	Channels.Add(
		PromptWeight,
		FMovieSceneChannelMetaData("PromptWeight", FText::FromString("Prompt Weight")),
		TMovieSceneExternalValue<float>::Make()
	);
#else
	Channels.Add(PromptWeight);
#endif
	ChannelProxy = MakeShared<FMovieSceneChannelProxy>(MoveTemp(Channels));

	return EMovieSceneChannelProxyType::Static;
}
