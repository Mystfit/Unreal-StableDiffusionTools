#include "StableDiffusionPromptMovieSceneSection.h"
#include "Channels/MovieSceneChannelProxy.h"

EMovieSceneChannelProxyType UStableDiffusionPromptMovieSceneSection::CacheChannelProxy()
{
	FMovieSceneChannelProxyData Channels;

#if WITH_EDITOR
	Channels.Add(
		Weight,
		FMovieSceneChannelMetaData("Weight", FText::FromString("Weight")),
		TMovieSceneExternalValue<float>::Make()
	);

	Channels.Add(
		Repeats,
		FMovieSceneChannelMetaData("Repeats", FText::FromString("Repeats")),
		TMovieSceneExternalValue<int>::Make()
	);
#else
	Channels.Add(Weight);
	Channels.Add(Repeats);
#endif
	ChannelProxy = MakeShared<FMovieSceneChannelProxy>(MoveTemp(Channels));

	return EMovieSceneChannelProxyType::Static;
}
