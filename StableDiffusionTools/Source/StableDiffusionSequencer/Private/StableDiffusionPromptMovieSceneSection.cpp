#include "StableDiffusionPromptMovieSceneSection.h"
#include "Channels/MovieSceneChannelProxy.h"

UStableDiffusionPromptMovieSceneSection::UStableDiffusionPromptMovieSceneSection(const FObjectInitializer& ObjectInitializer)
{
	Weight.SetDefault(1.0f);
}

EMovieSceneChannelProxyType UStableDiffusionPromptMovieSceneSection::CacheChannelProxy()
{
	FMovieSceneChannelProxyData Channels;

#if WITH_EDITOR
	Channels.Add(
		Weight,
		FMovieSceneChannelMetaData("Weight", FText::FromString("Weight")),
		TMovieSceneExternalValue<float>::Make()
	);
#else
	Channels.Add(Weight);
#endif
	ChannelProxy = MakeShared<FMovieSceneChannelProxy>(MoveTemp(Channels));

	return EMovieSceneChannelProxyType::Static;
}
