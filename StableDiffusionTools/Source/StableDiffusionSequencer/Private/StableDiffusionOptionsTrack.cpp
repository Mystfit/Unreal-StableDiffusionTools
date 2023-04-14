#include "StableDiffusionOptionsTrack.h"
#include "Sections/MovieSceneParameterSection.h"
#include "StableDiffusionOptionsSection.h"

UStableDiffusionOptionsTrack::UStableDiffusionOptionsTrack(const FObjectInitializer& ObjectInitializer)
{
	// By default, don't evaluate cvar values in pre and postroll as that would conflict with
// the previous shot's desired values.
	EvalOptions.bEvaluateInPreroll = EvalOptions.bEvaluateInPostroll = false;
	//SupportedBlendTypes.Add(EMovieSceneBlendType::Absolute);
}

FText UStableDiffusionOptionsTrack::GetDisplayName() const
{
	static FText TrackName = FText::FromString(FString(TEXT("Stable Diffusion Options Track")));
	return TrackName;
}


FName UStableDiffusionOptionsTrack::GetTrackName() const
{
	return UStableDiffusionOptionsTrack::GetDataTrackName();
}


const FName UStableDiffusionOptionsTrack::GetDataTrackName()
{
	static FName TheName = FName(*FString(TEXT("StableDiffusionOptionsTrack")));
	return TheName;
}


bool UStableDiffusionOptionsTrack::IsEmpty() const
{
	return Sections.Num() == 0;
}

bool UStableDiffusionOptionsTrack::SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const
{
	return SectionClass == UStableDiffusionOptionsSection::StaticClass() || SectionClass == UMovieSceneParameterSection::StaticClass();
}

UMovieSceneSection* UStableDiffusionOptionsTrack::CreateNewSection()
{
	UMovieSceneSection* Section = NewObject<UStableDiffusionOptionsSection>(this, NAME_None, RF_Transactional);
	Sections.Add(Section);
	return Section;
}


void UStableDiffusionOptionsTrack::AddSection(UMovieSceneSection& Section)
{
	Sections.Add(&Section);
}


void UStableDiffusionOptionsTrack::RemoveSection(UMovieSceneSection& Section)
{
	Sections.Remove(&Section);
}


const TArray<UMovieSceneSection*>& UStableDiffusionOptionsTrack::GetAllSections() const
{
	return Sections;
}