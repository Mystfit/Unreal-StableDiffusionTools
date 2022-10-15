#include "StableDiffusionOptionsSection.h"
#include "Channels/MovieSceneChannelProxy.h"


UStableDiffusionOptionsSection::UStableDiffusionOptionsSection(const FObjectInitializer& ObjectInitializer)
{
	FMovieSceneChannelProxyData Channels;

	#if WITH_EDITOR

	//static FColorSectionEditorData EditorData;
	Channels.Add(
		StrengthCurve, 
		FMovieSceneChannelMetaData("Strength", FText::FromString("Strength")), 
		TMovieSceneExternalValue<float>::Make()
	);
	Channels.Add(
		IterationsCurve, 
		FMovieSceneChannelMetaData("Iterations", FText::FromString("Iterations")), 
		TMovieSceneExternalValue<int32>::Make()
	);
	Channels.Add(
		SeedCurve, 
		FMovieSceneChannelMetaData("Seed", FText::FromString("Seed")),
		TMovieSceneExternalValue<int32>::Make()
	);

	#else
		Channels.Add(StrengthCurve);
		Channels.Add(IterationsCurve);
		Channels.Add(SeedCurve);
	#endif

	ChannelProxy = MakeShared<FMovieSceneChannelProxy>(MoveTemp(Channels));
}


//EMovieSceneChannelProxyType UStableDiffusionOptionsSection::CacheChannelProxy()
//{
//#if WITH_EDITOR
//
//	ChannelProxy = MakeShared<FMovieSceneChannelProxy>(FloatCurve, FMovieSceneChannelMetaData(), TMovieSceneExternalValue<float>::Make());
//
//#else
//
//	ChannelProxy = MakeShared<FMovieSceneChannelProxy>(Strength);
//
//#endif
//
//	return EMovieSceneChannelProxyType::Static;
//}
