#include "StableDiffusionOptionsSection.h"
#include "StableDiffusionGenerationOptions.h"
#include "Channels/MovieSceneChannelProxy.h"

UStableDiffusionOptionsSection::UStableDiffusionOptionsSection(const FObjectInitializer& ObjectInitializer)
{
	FStableDiffusionGenerationOptions DefaultOptions;
	StrengthCurve.SetDefault(DefaultOptions.Strength);
	IterationsCurve.SetDefault(DefaultOptions.Iterations);
	SeedCurve.SetDefault(0);	// Makes more sense for animations to have a consistent seed

	FMovieSceneChannelProxyData Channels;

	#if WITH_EDITOR

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

#if WITH_EDITOR
void UStableDiffusionOptionsSection::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if ((PropertyName == GET_MEMBER_NAME_CHECKED(UStableDiffusionOptionsSection, PipelineAsset)))
	{
		if (PipelineAsset) {
			PipelineStages.Reset();

			// Update pipeline stages
			for (auto Stage : PipelineAsset->Stages) {
				UObject* DuplicatedStage = StaticDuplicateObject(Stage, this->GetOuter());
				PipelineStages.Add(CastChecked<UImagePipelineStageAsset>(DuplicatedStage));
			}
		}
	}
}
#endif