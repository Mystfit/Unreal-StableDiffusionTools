#include "StableDiffusionPromptMovieSceneTrack.h"
#include "StableDiffusionPromptMovieSceneSection.h"

UStableDiffusionPromptMovieSceneTrack::UStableDiffusionPromptMovieSceneTrack(const FObjectInitializer& ObjectInitializer)
{
	// By default, don't evaluate cvar values in pre and postroll as that would conflict with
// the previous shot's desired values.
	EvalOptions.bEvaluateInPreroll = EvalOptions.bEvaluateInPostroll = false;

//#if WITH_EDITORONLY_DATA
//	TrackTint = FColor(65, 89, 194, 65);
//#endif

	SupportedBlendTypes = FMovieSceneBlendTypeField::All();
}

//FText UStableDiffusionPromptMovieSceneTrack::GetDisplayName() const
//{
//	static FText TrackName = FText::FromString(FString(TEXT("Stable Diffusion Prompt Track")));
//	return TrackName;
//}
//
//
//FName UStableDiffusionPromptMovieSceneTrack::GetTrackName() const
//{
//	return UStableDiffusionPromptMovieSceneTrack::GetDataTrackName();
//}
//
//
//const FName UStableDiffusionPromptMovieSceneTrack::GetDataTrackName()
//{
//	static FName TheName = FName(*FString(TEXT("StableDiffusionPromptTrack")));
//	return TheName;
//}

bool UStableDiffusionPromptMovieSceneTrack::IsEmpty() const
{
	return Sections.Num() == 0;
}

bool UStableDiffusionPromptMovieSceneTrack::SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const
{
	return SectionClass == UStableDiffusionPromptMovieSceneSection::StaticClass();
}

UMovieSceneSection* UStableDiffusionPromptMovieSceneTrack::CreateNewSection()
{
	UMovieSceneSection* Section = NewObject<UStableDiffusionPromptMovieSceneSection>(this, NAME_None, RF_Transactional);
	Sections.Add(Section);
	return Section;
}


void UStableDiffusionPromptMovieSceneTrack::AddSection(UMovieSceneSection& Section)
{
	Sections.Add(&Section);
}


void UStableDiffusionPromptMovieSceneTrack::RemoveSection(UMovieSceneSection& Section)
{
	Sections.Remove(&Section);
}


const TArray<UMovieSceneSection*>& UStableDiffusionPromptMovieSceneTrack::GetAllSections() const
{
	return Sections;
}